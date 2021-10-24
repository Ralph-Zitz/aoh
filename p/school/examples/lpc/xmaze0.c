/*   Basic Dynamic Maze Room  (Mateese, 20-Jul-96)
**
** This room is the entrance to the preferred variant of a dynamic maze.
**
** This variant is handled as if the files for the dynamic rooms
** (/d/archwiz/common/lpc/exm/xmaze{0-3}.c) really exist. Behind the
** scenes, using the P_VMASTER property, just xmaze0 needs to exist.
** This one clones and configures /std/room (it could be any other room
** object) for the other maze rooms and the gamedriver will then treat them
** as if they were real. 
** Note that we can everytime replace single dynamic rooms by 'real' ones
** simply by creating the corresponding file. The only requirement to be
** fulfilled would be that these single rooms set their P_VMASTER to
** this room.
** It is not necessary that the VMASTER object is part of the maze, it
** just came in handy here.
**
** -- About dynamic rooms --
**   Dynamic rooms are a simple but effective way to reduce the amount of
** diskspace used to implement a certain uniform area, like a maze.
**   A typical maze consists of many rooms which have only few differences
** in description and content. Implementing the maze in a one-file-per-
** room-system is straightforward, simple and ineffective then.
** Its better to concentrate the variant data in one maze-master, which
** then creates the rooms from one basic file on demand. This basic file
** contains the fixed data (and some utility funs), and gets the variant
** data from the maze master upon creation.
**   This way one has just a few room _files_, but is able to create a maze
** consisting of many room _objects_.
**   Moving the player through the maze takes a certain care, though.
** Since the room objects don't know about their neighboured rooms, the
** move-command of one player needs to be executed by the maze master.
**   The maze master itself may be used as a normal room, but needn't.
**
**  If you`re interested in a big 'real' dynamic maze, take a look at
** Chalamah's quest.
*/

#include <properties.h>
#include <rooms.h>

inherit "/std/room";  /* since we derive from this one */

#define TP this_player()
#define THIS this_object()

/*------------------------------------------------------------------------*/
varargs void create() 
{
  seteuid(getuid());
  ::create();  /* Initialize the /std/room */

  Set(P_INT_SHORT, "The Northeast Room");
  Set(P_INT_LONG,
"You are in a square maze of dynamic rooms, all equal. "
"In detail, this is /d/archwiz/common/lpc/exm/xmaze0.c, the only real "
"room of the maze. This room will create the others on demand. "
"To the east you can leave the maze.\n"
     );

    /* Add the 'normal' exit east */
  AddExit("east", "/p/school/examples/rooms/xroom4");

    /* Add the exits to the dynamic rooms.
    ** Note that these aren't different from 'real' AddExit()s.
    */
  AddExit("west", "/p/school/examples/lpc/xmaze3");
  AddExit("south", "/p/school/examples/lpc/xmaze1");

    /* This is not really necessary, as the gamedriver will
    ** automatically call compile_object() in us.
    ** But it makes the intention clear.
    */
  Set(P_VMASTER, object_name(THIS));
}

/*------------------------------------------------------------------------*/

object compile_object (string filename)

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
  if (1 != sscanf(filename, "/p/school/examples/lpc/xmaze%d.c", number)
   || number < 1 || number > 3
     )
    return 0;

   /* Ok, clone and configure the room according to it's number. */
  room = clone_object("/std/room");
  if (!room)
    return 0;

  switch(number) {
    case 1:
      room->Set(P_INT_SHORT, "The Southeast Room");
      room->AddExit("north", "/p/school/examples/lpc/xmaze0");
      room->AddExit("west", "/p/school/examples/lpc/xmaze2");
      break;
    case 2:
      room->Set(P_INT_SHORT, "The Southwest Room");
      room->AddExit("north", "/p/school/examples/lpc/xmaze3");
      room->AddExit("east", "/p/school/examples/lpc/xmaze1");
      break;
    case 3:
      room->Set(P_INT_SHORT, "The Northwest Room");
      room->AddExit("south", "/p/school/examples/lpc/xmaze2");
      room->AddExit("east", "/p/school/examples/lpc/xmaze0");
      break;
  }
  room->Set(P_INT_LONG,
"You are in a square maze of dynamic rooms, all equal. "
"In detail, this room was created on the fly "
"by /p/school/examples/lpc/xmaze0.c, the only real room in the maze.\n"
           );
  room->Set(P_VMASTER, "/p/school/examples/lpc/xmaze0"); /* !!! */

   /* That's it. */
  return room;
}
