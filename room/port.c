/*
** The Port
** Works with /obj/ship
** File:   /room/newport.c
** Author: Thragor@Nightfall, 11aug95
** Update:
*/

#define SF(x) #'x //'
#define OCEAN "/room/ocean"
#define ME this_object()

#include <config.h>
#include <properties.h>

inherit "/std/room";
inherit "/lib/string";

private nosave string *Pships;
private nosave int Pautocall;

public string *SetShips(string *s) { return Pships = s; }
public string *QueryShips() { return (Pships||({}))+({}); }
public string *AddShip(string shipfile)
// Add a ship that will be called when a player enters
// the port.
{
  shipfile = resolve_file(shipfile);
  return Pships = (Pships||({}))-({shipfile})+({shipfile});
}

public int SetAutocall(int mode)
// If !=0 the ship will be automatically called when a player
// enters the port. Else the ship won't be called (might be
// called by hand.
{
  return Pautocall = mode;
}
public int QueryAutocall() { return Pautocall; }

protected object valid_ship(string ship)
{
  object res;
  string error;
  if (file_size(ship+".c")<0)
    {
      MASTER->runtime_error("Ship not existant!\n",
                            object_name(),ship,-1);
      return 0;
    }
  if (error = catch(res = load_object(ship)))
    {
      MASTER->runtime_error("Ship error: "+error,
                            object_name(),ship,-1);
      return 0;
    }
  return res;
}

protected int get_ship_number(string ship)
{
  object ob;
  mapping ports;
  int *idx,pos;
  string *list;
  if (ob = valid_ship(ship))
    {
      ports = ({mapping})ship->QueryPorts();
      list = m_values(ports);
      idx = m_indices(ports);
      if ((pos = member(list,object_name()))==-1)
	{
	  MASTER->runtime_error("Ship doesn't know this port.\n",
                                object_name(),ship,-1);
          return -1;
	}
      return idx[pos];
    }
  return -1;
}

public mapping QueryTable()
// Returns all ships added to this port and the position of
// the port in the Port-List of the ship.
// If for some reason the ship won't visit this port, a
// -1 is given as number.
// The numbers start at 0.
{
  int i,*nrs;
  string *ships;
  ships = QueryShips();
  nrs = map(ships,SF(get_ship_number));
  return mkmapping(ships,nrs);
}

protected string evaluate_course(int c)
{
  mapping table;
  string *ships;
  int i,nr;
  table = QueryTable();
  i = sizeof(ships = m_indices(table));
  nr = 1;
  while(i--)
    {
      mixed idx;
      object ship;
      mapping ports;
      int j;

      if (table[ships[i]]==-1) continue;
      if (nr<c)
	{
	  nr++;
          continue;
	}
      if (nr>c)
	return "There's no ship no. "+to_string(c)+" on the table.\n";
      ports = ({mapping})ships[i]->QueryPorts();
      j = sizeof(idx = sort_array(m_indices(ports),SF(<)));
      while(j--)
	idx[j] = ports[j,1];
      return wrap(capitalize(({string})ships[i]->Query(P_NAME)||
                        strip_article(({string})ships[i]->QueryShort()))+
             " visits the following ports:\n"+
             implode(idx," - ")+".\n");
    }
  return "No ship found.\n";
}

public int cmd_course(string str)
{
  int nr;

  if (!str)
    return notify_fail("The course of which ship do you want to know?\n",
		       NOTIFY_NOT_OBJ);
  str = lower_case(str);
  if (   sscanf(str,"%d",nr)!=1
      && sscanf(str,"ship %d",nr)!=1
      && sscanf(str,"of ship %d",nr)!=1)
    return notify_fail("Please give the number of the ship you want to know\n"
                       "the course of.\n",NOTIFY_NOT_VALID);
  write(evaluate_course(nr));
  return 1;
}

public string detail_table(string what)
{
  mapping table;
  string *ships,*list;
  int i,nr;
  table = QueryTable();
  i = sizeof(ships = m_indices(table));
  list = ({});
  nr = 1;
  while(i--)
    {
      string line;
      object ship;
      if (table[ships]==-1)
	continue;
      line = radjust(to_string(nr),2)+". "+
             ladjust(capitalize(({string})ships[i]->Query(P_NAME)||
                                strip_article(({string})ships[i]->QueryShort())
                               )+" ",35,".");
      if (!(ship = ({object})ships[i]->QueryShip())||!environment(ship) )
        line+=" sunken";
      else
        if (environment(ship)==ME)
          line+=" in harbour";
        else
	  {
	    if (object_name(environment(ship))==OCEAN)
              line+=" to ";
	    else
              line+=" at ";
	    line+=({string})ship->Harbour();
	  }
      list+=({line});
      nr++;
    }
  if (nr==1)
    return "There are no ships which visit this harbour.\n";
  return
    "Ships, which visit this harbour:\n"
    "--------------------------------\n\n"+implode(list,"\n")+"\n\n"
    "You may get informed about the course of each ship by typing:\n"
    "  course <nr>\n"
    "To enter a ship, just type 'enter <shipname>'.\n";
}

public void CallShip(string file)
// Tells the blueprint of the given ship, that the blueprint shall
// tell the ship, that it soon shall visit this port.
{
  object ob;
  if (ob = valid_ship(file))
    file->CallShip();
}

public void CallShips()
{
  map(QueryShips(),SF(CallShip));
}

public varargs void create()
{
  (::create());
  SetIntShort("A Standard Port");
  SetIntLong(
    "This is a port which automatically calls for the ships.\n"
    "A timetable stands here.\n");
  SetIndoors(0);
  SetAutocall(1);
  AddDetail(({"table","timetable","time table","time-table"}),
    SF(detail_table));
  AddReadMsg(({"table","timetable","time table","time-table"}),
    SF(detail_table));
  AddRoomCmd("course",SF(cmd_course));
}

public void notify_enter(mixed from,int method,mixed extra)
{
  if (query_once_interactive(previous_object())&&QueryAutocall())
    CallShips();
  return (::notify_enter(from,method,extra));
}
