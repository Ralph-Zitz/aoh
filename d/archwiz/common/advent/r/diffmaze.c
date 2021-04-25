/* Advent - Different Maze -- Mateese, 18-Aug-96
 *
 * This is the master object of the 'Maze of little passages, all different'.
 * Note that every room has a unique long description.
 * The maze itself consists of virtual rooms which are created by this
 * object. Each virtual room has filename of the type 'dmaze<nr>', with
 * <nr> being a number from 01 to 11 for the maze rooms (built using
 * i/stdroom). Reserved are all numbers in the range 00..99.
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
public object make_room (int number)

/* Create and return the maze room #<number>.
 */

{
  object room;
  string   desc;
  string * exits;  /* Array of exits, given in order of <dirs> */
  string * dirs;  /* Constant array of directions */
  int i;

  dirs = ({ "north", "east", "south", "west"
          , "northeast", "southeast", "southwest", "northwest"
          , "up", "down" 
         });

  switch (number)
  {
  case 1:
    desc = "maze of twisty little passages";
    exits = ({ /* n  */ "dmaze11", /* e  */ "dmaze09"
	     , /* s  */ "dmaze03", /* w  */ "dmaze10"
	     , /* ne */ "dmaze05", /* se */ "dmaze06"
	     , /* sw */ "dmaze04", /* nw */ "dmaze08"
	     , /* u  */ "dmaze07", /* d  */ "westlonghall"
            });
    break;
  case 2:
    desc = "twisty maze of little passages";
    exits = ({ /* n  */ "dmaze04", /* e  */ "dmaze05"
	     , /* s  */ "vendmachine", /* w  */ "dmaze09"
	     , /* ne */ "dmaze08", /* se */ "dmaze07"
	     , /* sw */ "dmaze03", /* nw */ "dmaze06"
	     , /* u  */ "dmaze11", /* d  */ "dmaze10"
            });
    break;
  case 3:
    desc = "maze of little twisty passages";
    exits = ({ /* n  */ "dmaze10", /* e  */ "dmaze02"
	     , /* s  */ "dmaze11", /* w  */ "dmaze01"
	     , /* ne */ "dmaze07", /* se */ "dmaze04"
	     , /* sw */ "dmaze06", /* nw */ "dmaze05"
	     , /* u  */ "dmaze08", /* d  */ "dmaze09"
            });
    break;
  case 4:
    desc = "maze of twisty passages";
    exits = ({ /* n  */ "dmaze05", /* e  */ "dmaze10"
	     , /* s  */ "dmaze06", /* w  */ "dmaze07"
	     , /* ne */ "dmaze09", /* se */ "dmaze02"
	     , /* sw */ "dmaze08", /* nw */ "dmaze01"
	     , /* u  */ "dmaze03", /* d  */ "dmaze11"
            });
    break;
  case 5:
    desc = "twisty maze of passages";
    exits = ({ /* n  */ "dmaze09", /* e  */ "dmaze08"
	     , /* s  */ "dmaze02", /* w  */ "dmaze04"
	     , /* ne */ "dmaze06", /* se */ "dmaze11"
	     , /* sw */ "dmaze07", /* nw */ "dmaze10"
	     , /* u  */ "dmaze01", /* d  */ "dmaze03"
            });
    break;
  case 6:
    desc = "maze of twisting little passages";
    exits = ({ /* n  */ "dmaze03", /* e  */ "dmaze07"
	     , /* s  */ "dmaze09", /* w  */ "dmaze11"
	     , /* ne */ "dmaze01", /* se */ "dmaze05"
	     , /* sw */ "dmaze02", /* nw */ "dmaze04"
	     , /* u  */ "dmaze10", /* d  */ "dmaze08"
            });
    break;
  case 7:
    desc = "maze of little twisting passages";
    exits = ({ /* n  */ "dmaze01", /* e  */ "dmaze06"
	     , /* s  */ "dmaze05", /* w  */ "dmaze08"
	     , /* ne */ "dmaze10", /* se */ "dmaze03"
	     , /* sw */ "dmaze09", /* nw */ "dmaze11"
	     , /* u  */ "dmaze02", /* d  */ "dmaze04"
            });
    break;
  case 8:
    desc = "twisting maze of little passages";
    exits = ({ /* n  */ "dmaze02", /* e  */ "dmaze01"
	     , /* s  */ "dmaze07", /* w  */ "dmaze03"
	     , /* ne */ "dmaze11", /* se */ "dmaze10"
	     , /* sw */ "dmaze05", /* nw */ "dmaze09"
	     , /* u  */ "dmaze04", /* d  */ "dmaze06"
            });
    break;
  case 9:
    desc = "maze of twisting passages";
    exits = ({ /* n  */ "dmaze08", /* e  */ "dmaze11"
	     , /* s  */ "dmaze04", /* w  */ "dmaze02"
	     , /* ne */ "dmaze03", /* se */ "dmaze01"
	     , /* sw */ "dmaze10", /* nw */ "dmaze07"
	     , /* u  */ "dmaze06", /* d  */ "dmaze05"
            });
    break;
  case 10:
    desc = "twisted maze of little passages";
    exits = ({ /* n  */ "dmaze07", /* e  */ "dmaze03"
	     , /* s  */ "dmaze08", /* w  */ "dmaze06"
	     , /* ne */ "dmaze04", /* se */ "dmaze09"
	     , /* sw */ "dmaze11", /* nw */ "dmaze02"
	     , /* u  */ "dmaze05", /* d  */ "dmaze01"
            });
    break;
  case 11:
    desc = "twisting maze of passages";
    exits = ({ /* n  */ "dmaze06", /* e  */ "dmaze04"
	     , /* s  */ "dmaze10", /* w  */ "dmaze05"
	     , /* ne */ "dmaze02", /* se */ "dmaze08"
	     , /* sw */ "dmaze01", /* nw */ "dmaze03"
	     , /* u  */ "dmaze09", /* d  */ "dmaze07"
            });
    break;
  default:
    return 0;
  }

  room = clone_object(ADVENT("i/stdroom"));
  if (!room)
    return 0;
 
  room->Set(P_INT_SHORT, "Maze");
  room->Set(P_INT_LONG, "You are in a "+desc+", all different.\n");
  room->Set(P_BRIGHT, 0);
  for (i = 0; i < sizeof(dirs); i++)
    if (exits[i])
      room->AddExit(dirs[i], ADVENT("r/"+exits[i]));
  room->AddNowayMsg("out", "Easier said than done.\n");
  
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
  if (1 != sscanf(filename, ADVENT("r/dmaze%d.c"), number)
   || number < 0 || number > 199
     )
    return 0;

  room = make_room(number);
  
  if (room)
    room->Set(P_VMASTER, object_name(this_object()));

  return room;
}

/*************************************************************************/
