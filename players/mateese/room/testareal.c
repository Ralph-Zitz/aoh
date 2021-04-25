/* 3x3-Testareal
 *
 * Combined rooms and master for the testareal.
 */

#include <config.h>
#include <properties.h>
#include <rooms.h>
#include <moving.h>

inherit "/std/room";

#define THIS this_object()
#define TP   this_player()

#define WORKROOM     (WPATH "mateese/workroom")
#define AREALMASTER  (WPATH "mateese/room/testareal")

int leave ();  // forward
int do_exit(); // forward

/*--------------------------------------------------------------------------*/

/* --- Variables of the Blueprint (master) --- */

static object * rooms;  // Existing rooms.

/* --- Variables of a Clone (room( --- */

static int x, y, nr;  // Coordinate [0..2, 0..2], lfd. Nr

/*--------------------------------------------------------------------------*/

int QueryNr () { return nr; }

/*--------------------------------------------------------------------------*/
void create() 

/* Setup. 
 *   For the master, set up the rooms array and collect existing
 *   old rooms. Old rooms which are overlaid by other old rooms are removed.
 *   For the clones, do the basic initialisation.
 */

{
  ::create();
  if (blueprint(THIS))
  {
    object * old;
    int      i, ix;

    rooms = allocate(9);
    /* Collect existing rooms */
    old = find_clones(THIS);
    for (i = sizeof(old); i--; )
    {
      ix = old[i]->QueryNr();
      if (!rooms[ix] || program_time(old[i]) > program_time(rooms[ix]))
      {
        if (rooms[ix])
          rooms[ix]->remove();
        rooms[ix] = old[i];
      }
      else
        old[i]->remove();
    }
    AddExit("workroom", WORKROOM); // just in case.
    return;
  }

  /* Clone setup */

  SetIntShort ("Mateese's Testareal");
    /* The exits are added when the coordinate is set */
  AddDetail("void", "What do you expect to see?\n");
  AddExit("leave",#'leave);
  Set(P_INDOORS, 1);
  Set(P_TPORT, TPORT_OUT);
}

/*--------------------------------------------------------------------------*/
object mkroom (int x, int y) 

/* Master: Return the room at coordinates <x>,<y>. If necessary, make it.
 */

{
  object r;

  if (r = rooms[3*y+x]) 
    return r;
  r = clone_object(AREALMASTER);
  rooms[3*y+x] = r;
  if (r) 
    r->set_xy(x, y);
  return r;
}

/*--------------------------------------------------------------------------*/
void set_xy (int xc, int yc) 

/* Clone: Initialize the room at coordinates <xc>, <yc>.
 */

{ 
  x = xc; y = yc; nr = 3 * y + x;
  if (y < 2) AddExit("south", #'do_exit);
  if (x < 2) AddExit("east",  #'do_exit);
  if (x > 0) AddExit("west",  #'do_exit);
  if (y > 0) AddExit("north", #'do_exit);
  if (x == 1 && y == 1) {
    AddExit("up",    #'do_exit);
    AddExit("down",  #'do_exit);
  }
  SetIntLong(
"This is Mateese's Testareal, a large open plain floating in the void.\n"
+({
    "It continues south and east.\n"
  , "To the north is the void.\n"
  , "It continues west and south.\n"
  , "To the west is the void.\n"
  , "It stretches out in every direction. Of course you can leave it.\n"
  , "To the east is the void.\n"
  , "It continues north and east.\n"
  , "To the south is the void.\n"
  , "It continues north and west.\n"
  })[nr]
            );
}

/*--------------------------------------------------------------------------*/
int do_exit()

/* Clone: Handle a movement.
 */

{
  object new;
  object master;

  call_other(AREALMASTER, "???");
  master = find_object(AREALMASTER);
  if (!master) 
    return 0;

  switch (query_verb()) {
  case "north": new = master->mkroom(x,y-1);
                break;
  case "south": new = master->mkroom(x,y+1);
                break;
  case "east" : new = master->mkroom(x+1,y);
                break;
  case "west" : new = master->mkroom(x-1,y);
                break;
  case "up"   : new = master->mkroom(x+1,y);
                break;
  case "down" : new = master->mkroom(x-1,y);
                break;
  default: return 0;
  }
  if (!new) return 0;
  TP->move(new, M_GO, query_verb());
  return 1;
}

/*--------------------------------------------------------------------------*/
int leave() 

/* Clone: Leave the testareal.
 */

{
  write ("The plane folds up around you...\n");
  if (ME_OK != TP->move(WORKROOM, M_TELEPORT))
    write("...then unfolds again.\n");
  return 1;
}

/*--------------------------------------------------------------------------*/
