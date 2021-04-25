/*
** This is an elevator.
** floor 2: church
*/

#define STOP	0   // the direction to which the elevator moves to
#define DOWN	-1
#define UP	1
#define CLOSE_DOOR_DELAY 20
#define MOVE_COUNT 5
#define MOVE_DELAY 2

#define TP this_player()
#define ME this_object()

#include <moving.h>
#include <lock.h>
#include <config.h>
#include "path.h"

inherit "/std/room";
                    
private static mapping buttons;
// ([button_label:level])
private static mapping destinations;
// ([level:destination])
private static int *call_stack;

private static int level, // intern level showing the position of the label
                          // in the m_indices-array
  door_status,            // LOCK_CLOSED, LOCK_OPEN
  direction,
  move_counter;

private void StartMove();

public int SetDirection(int dir)
{
  return direction = dir;
}

public int QueryDirection()
{
  return direction;
}

public mapping SetDestinations(mapping d)
{
  return destinations = d;
}

public mapping QueryDestinations()
{
  return destinations;
}

public void AddDestination(int level,string destination,mixed label)
{
  int i;
  
  if (!destinations) destinations = m_allocate(0,1);
  if (!buttons) buttons = m_allocate(0,1);
  if (stringp(label)) label = ({label});
  i = sizeof(label);
  while(i--)
    {
      buttons[label[i]] = level;
      AddDetail(({label[i],"button "+label[i]}),
                "There's a '"+to_string(level)+"' on it.\n");
    }
  AddDetail(({"number "+to_string(level),to_string(level)}),
            "It's a very nice number.\n");
  destinations[level] = destination;
}

public string QueryDestination()
{
  if (!destinations) return 0;
  return destinations[level];
}

public int QueryLevel()
{
  return level;
}

// Compat.
public int query_level()
{
  return QueryLevel();
}

public int LeaveElevator()
{
  if (door_status==LOCK_CLOSED)
    return (int)notify_fail("The door is closed.\n")&&0;
  return TP->move(QueryDestination(),M_GO,query_verb());
}

public int *SetCallStack(int *stack)
{
  return call_stack = stack;
}

public int NextLevel()
{
  if (!call_stack||!sizeof(call_stack)) return 0;
  return call_stack[0];
}

public int *QueryCallStack()
{
  return call_stack||({});
}

public int AddCallStack(int newlevel)
{
  if (!call_stack) call_stack = ({});
  if (member(call_stack,newlevel)!=-1) return 0;
  call_stack+=({newlevel});
  return 1;
}

public int RemoveCallStack()
{
  if (!call_stack||!sizeof(call_stack)) return 0;
  call_stack = call_stack[1..];
  return 1;
}

public int SetDoorStatus(int s)
{
  return door_status = s;
}

public int QueryDoorStatus()
{
  return door_status;
}

public int CloseDoor()
{
  if (door_status == LOCK_CLOSED) return 0;
  door_status = LOCK_CLOSED;
  return 1;
}

public int OpenDoor()
{
  if (door_status == LOCK_OPEN) return 0;
  door_status = LOCK_OPEN;
  return 1;
}

public string QueryIntLong()
{
  return (::QueryIntLong())+
         "A small number glowing in a blue light is above the\n"
         "door to the east: "+to_string(QueryLevel())+".\n"+
         ((door_status==LOCK_OPEN)?"There is an open door to the east.\n"
	                          :"There is a closed door to the east.\n");
}

public void auto_close_door()
{
  if (CloseDoor())
    {
      tell_room(ME,"The door swings shut.\n");
      tell_room(QueryDestination(),"The door swings shut.\n");
    }
  StartMove();
}

public void do_moving()
{
  int newlevel,dif;
  
  if (!newlevel = NextLevel())
    {
      SetDirection(STOP);
      return;
    }
  if (QueryDirection()==STOP)
    {
      dif = QueryLevel()-NextLevel();
      if (!dif)
        {
	  RemoveCallStack();
          return;
        }
      SetDirection(dif>0?DOWN:UP);
      tell_room(ME,"The elevator "+
		(dif>0?"starts moving down":"jerks up")+"ward.\n");
      move_counter = MOVE_COUNT;
    }
  else
    {
      if (!--move_counter)
        {
	  if (QueryDirection()==UP) level++;
            else level--;
	  if (QueryLevel()!=newlevel)
            {
	      tell_room(ME,
                "On the small sign above the door appears a new "
	        "in a blue light glowing number: "+
                to_string(QueryLevel())+".\n");
              move_counter = MOVE_COUNT;
	    }
	  else
	    {
	      SetDirection(STOP);
	      tell_room(ME,
                "On the small sign above the door appears a new "
	        "number glowing in a blue light: "+
                to_string(QueryLevel())+".\n");
	      tell_room(ME,"The elevator slows down and stops.\n");
              QueryDestination()->elevator_arrives();
	      RemoveCallStack();
	      if (NextLevel()) call_out(#'auto_close_door,CLOSE_DOOR_DELAY);
            }
        }
      else
        tell_room(ME,"The elevator continues ...\n");
    }
  if (move_counter) 
    call_out(#'do_moving,MOVE_DELAY);
}

private void StartMove()
{
  if (find_call_out(#'auto_close_door)!=-1)
    remove_call_out(#'auto_close_door);
  if (door_status==LOCK_OPEN)
    {
      call_out(#'auto_close_door,CLOSE_DOOR_DELAY/2);
      return;
    }
  if (find_call_out(#'do_moving)==-1)
    call_out(#'do_moving,1);
}
  
public int cmd_press(string str)
{
  string b;
  int dest;
  
  if (!TP) return 0;
  if (!str)
    return (int)notify_fail("What do you want to press?\n",
                            NOTIFY_ILL_ARG)&&0;
  str = lower_case(str);
  if (sscanf(str,"button %s",b))
    str = b;
  if (!dest = buttons[str])
    return (int)notify_fail("You don't see that button here.\n",
                            NOTIFY_NOT_VALID)&&0;
  if ((dest==QueryLevel())
      &&QueryDirection()==STOP)
    return (int)notify_fail("You are already at level "+str+".\n",
                            NOTIFY_NOT_VALID)&&0;
  if (!AddCallStack(dest))
    return (int)notify_fail("The button is already glowing.\n",
                            NOTIFY_NOT_VALID)&&0;
  tell_room(ME,TP->QueryName()+" presses the button for level "+str+".\n",
            ({TP}));
  if (door_status==LOCK_OPEN)
    return (int)notify_fail("Nothing happens.\n",
                            NOTIFY_NOT_VALID)&&0;
  write("Ok.\n");
  StartMove();
  return 1;
}

public int open_door(string str)
{
  if (!TP) return 0;
  if (member(({"door","door east","elevator door"}),lower_case(str||""))==-1)
    return (int)notify_fail("Open what?\n",NOTIFY_NOT_OBJ)&&0;
  if (QueryDirection()!=STOP)
    return (int)notify_fail("The door is stuck.\n",
                            NOTIFY_NOT_VALID)&&0;
  if (!OpenDoor())
    return (int)notify_fail("The door is already open.\n",
                            NOTIFY_NOT_VALID)&&0;
  write("Ok.\n");
  tell_room(environment(TP),
    TP->QueryName()+" opens the elevator door.\n",({TP}));
  if (environment(TP)==ME)
    tell_room(QueryDestination(),"The elevator door opens.\n");
  else
    tell_room(ME,"The elevator door opens.\n");
  return 1;
}

public int close_door(string str)
{
  if (!TP) return 0;
  if (member(({"door","door east","elevator door"}),lower_case(str||""))==-1)
    return (int)notify_fail("Close what?\n",NOTIFY_NOT_OBJ)&&0;
  if (!CloseDoor())
    return (int)notify_fail("The door is already closed.\n",
                            NOTIFY_NOT_VALID)&&0;
  write("Ok.\n");
  tell_room(environment(TP),
    TP->QueryName()+" closes the elevator door.\n",({TP}));
  if (environment(TP)==ME)
    tell_room(QueryDestination(),"The elevator door swings shut.\n");
  else
    tell_room(ME,"The elevator door swings shut.\n");
  if (NextLevel()) StartMove();
  
  return 1;
}

/*
 * This routine is called from various rooms that the elevator connects to.
 */
public int call_elevator(int l)
{
  if (!TP) return 0;
  
  if (!AddCallStack(l)) return 0;
  StartMove();
  tell_room(environment(TP),
    "A little white lamp beside the button lights up.\n");
  return 1;
}

/*
 * Called by others to see if the elevator is moving
 */

public status is_moving()
{
  return QueryDirection();
}      

private string descr_door(string what)
{
  if (door_status == LOCK_CLOSED)
    if (what=="open door")
      return "There is no open door here. Only a closed one.\n";
    else
      return "The door is closed.\n";
  else
    if (what=="close door")
      return "There is no close door here. Only an open one.\n";
    else
      return "The door is open.\n";
}

public string descr_number()
{
  return 
    "A small number glowing in a blue light is above the\n"
    "door to the east: "+to_string(QueryLevel())+".\n";
}

public void create()
{
  (::create());
  SetIntShort("elevator");
  SetIntLong("You are in the elevator.\n"
             "On the wall are three buttons numbered 1 to 3.\n");
  AddRoomCmd("press",#'cmd_press);
  AddRoomCmd("push",#'cmd_press);
  AddRoomCmd("open",#'open_door);
  AddRoomCmd("close",#'close_door);
  AddDestination(1,CITY("wiz_hall"),({"1","one"}));
  AddDestination(2,CITY("church"),({"2","two"}));
  AddDestination(3,CITY("attic"),({"3","three"}));

  AddExit("east",#'LeaveElevator);
  HideExit("east",1);
  level = 2;
  door_status = LOCK_CLOSED;
  AddDetail(({"buttons","three buttons"}),
    "YOU wonder what someone might do with them ...\n"
    "Your FINGERS just want to press one of them.\n");
  AddDetail(({"door","open door","close door","elevator door","door east"}),
            #'descr_door);
  AddDetail(({"number","small number","blue number","glowing number",
              "number above door"}),
            #'descr_number);
  AddDetail("wall",
    "There's a door in the wall.\n");
  AddDetail("numbers","You see three numbers.\n");
  AddDetail("three numbers","1, 2 and 3.\n");
  AddDetail(({"blue light","magic light","light"}),
    "It looks magic somehow.\n");
  AddDetail(({"sign","small sign"}),
    "There's a number on it.\n");
}

public void reset()
{
  (::reset());
  CloseDoor();
}
