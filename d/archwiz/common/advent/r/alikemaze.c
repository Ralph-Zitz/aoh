/* Advent - Alike Maze -- Mateese, 16-Aug-96
 *
 * This is the master object of the 'Maze of little passages, all alike'.
 * Main entrance is from the West End of Hall of Mists, second (random)
 * entrance is from 'Top of Stalactite'. An exit leads to the Bird Chamber.
 * 
 * The maze itself consists of virtual rooms which are created by this
 * object. Each virtual room has filename of the type 'amaze<nr>', with
 * <nr> being a number from 001 to 014 for the maze rooms and 101..109 
 * for the dead ends.
 * Reserved are all numbers in the range 000..199.
 */

#include <properties.h>
#include <rooms.h>
#include "advent.h"

/*------------------------------------------------------------------------*/
public void create() 
{
  seteuid(getuid(this_object())); /* So we can clone other things */
}

/*------------------------------------------------------------------------*/
public object make_maze (int number)

/* Create and return the maze room #<number>.
 */

{
  object room;
  string * exits;  /* Array of exits, given as 'direction' 'filename' pairs */
  int i;

  switch (number)
  {
  case 1:
    exits = ({ "north", "westhallmists"
	     , "east",  "amaze002"
	     , "south", "amaze004"
	     , "west",  "amaze011"
            });
    break;
  case 2:
    exits = ({ "east",  "amaze004"
	     , "south", "amaze003"
	     , "west",  "amaze001"
            });
    break;
  case 3:
    exits = ({ "north", "piratesend"
	     , "east",  "amaze002"
	     , "south", "amaze006"
	     , "down",  "amaze104"
            });
    break;
  case 4:
    exits = ({ "north", "amaze002"
	     , "east",  "amaze102"
	     , "south", "amaze103"
	     , "west",  "amaze001"
	     , "up",    "amaze014"
	     , "down",  "amaze014"
            });
    break;
  case 5:
    exits = ({ "east",  "amaze006"
	     , "west",  "amaze007"
            });
    break;
  case 6:
    exits = ({ "east",  "amaze003"
	     , "south", "amaze008"
	     , "west",  "amaze005"
	     , "down",  "amaze007"
            });
    break;
  case 7:
    exits = ({ "east",  "amaze008"
	     , "south", "amaze009"
	     , "west",  "amaze005"
	     , "up"  ,  "amaze006"
            });
    break;
  case 8:
    exits = ({ "north", "amaze010"
	     , "east",  "amaze007"
	     , "south", "amaze008"
	     , "west",  "amaze006"
	     , "up"  ,  "amaze009"
	     , "down",  "amaze106"
            });
    break;
  case 9:
    exits = ({ "north", "amaze008"
	     , "south", "amaze105"
	     , "west",  "amaze007"
            });
    break;
  case 10:
    exits = ({ "north", "amaze010"
	     , "east",  "atbrinkpit"
	     , "down",  "amaze108"
            });
    break;
  case 11:
    exits = ({ "north", "amaze001"
	     , "east",  "amaze101"
	     , "south", "amaze011"
	     , "west",  "amaze011"
            });
    break;
  case 12:
    exits = ({ "east",  "amaze013"
	     , "south", "amaze109"
	     , "west",  "amaze107"
            });
    break;
  case 13:
    exits = ({ "north", "atbrinkpit"
	     , "west",  "amaze012"
	     , "northwest", "piratesend"
            });
    break;
  case 14:
    exits = ({ "up",    "amaze004"
	     , "down",  "amaze004"
            });
    break;
  default:
    return 0;
  }

  room = clone_object(ADVENT("i/stdroom"));
  if (!room)
    return 0;
 
  room->Set(P_INT_SHORT, "Maze");
  room->Set(P_INT_LONG,
"You are in a maze of twisty little passages, all alike.\n"
           );
  room->Set(P_BRIGHT, 0);
  for (i = 0; i < sizeof(exits); i += 2)
    room->AddExit(exits[i], ADVENT("r/"+exits[i+1]));
  room->AddNowayMsg("out", "Easier said than done.\n");
  
  return room;
}

/*------------------------------------------------------------------------*/
public object make_deadend (int number)

/* Create and return the Deadend room #<number>.
 */

{
  object room;
  string dir;    /* Direction to leave the dead end */
  string rname;  /* Name of the adjacent room */
      
  switch (number)
  {
  case 1:
    dir = "west"; rname = "amaze011";
    break;
  case 2:
    dir = "west"; rname = "amaze004";
    break;
  case 3:
    dir = "west"; rname = "amaze004";
    break;
  case 4:
    dir = "south"; rname = "amaze003";
    break;
  case 5:
    dir = "west"; rname = "amaze009";
    break;
  case 6:
    dir = "up"; rname = "amaze008";
    break;
  case 7:
    dir = "east"; rname = "amaze012";
    break;
  case 8:
    dir = "up"; rname = "amaze010";
    break;
  case 9:
    dir = "east"; rname = "atbrinkpit";
    break;
  default:
    return 0;
  }
    
  room = clone_object(ADVENT("i/stdroom"));
  if (!room)
    return 0;

  room->Set(P_INT_SHORT, "Dead End");
  room->Set(P_INT_LONG, "You have reached a dead end.\n");
  room->Set(P_BRIGHT, 0);
  room->AddExit( ({ dir, "out" }) , ADVENT("r/"+rname));
  room->HideExit("out", HIDE_ALWAYS);
  room->Set(P_NOWAY_MSG, "You'll have to go back the way you came.\n");

  return room;
}

/*------------------------------------------------------------------------*/
public object compile_object (string filename)

// Compile an virtual object.
//
// Arguments:
//   filename : The requested filename for the virtual object.
//
// Result:
//   The object to serve as the requested virtual object, or 0.
//
// This function is called if the compiler can't find the filename for an
// object to compile. This function has now the opportunity to return an other
// object which will then serve as if it was compiled from <filename>.
// If the function returns 0, the usual 'Could not load'-error will occur.

{
  int number;
  object room;

   /* Check if the filename is of the appropriate pattern. 
   ** We need the number for proper configuration of the room later.
   */
  if (1 != sscanf(filename, ADVENT("r/amaze%d.c"), number)
   || number < 0 || number > 199
     )
    return 0;

  if (number >= 100)
    room = make_deadend(number-100);
  else
    room = make_maze(number);
  
  if (room)
    room->Set(P_VMASTER, object_name(this_object()));

  return room;
}

/*************************************************************************/
