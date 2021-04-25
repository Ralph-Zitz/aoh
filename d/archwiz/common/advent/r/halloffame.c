/* Advent - The Hall Of Fame -- Mateese, 29-Jan-97
 *
 * In this room east of the 'After Grate' players can look at who solved
 * the cave before. As this requires a more-operation, we have to redefine
 * the 'read' command.
 * They will also find the hint to the 'advent' command here.
 */

#include <properties.h>
#include <rooms.h>
#include <moving.h>
#include "advent.h"

inherit ADVENT("i/stdroom");
inherit "/obj/lib/string";

static int    lastsize;  /* Last size of the l/SPELUNKERS file */
static string wall;      /* Current data from the l/SPELUNKERS file */

/*-----------------------------------------------------------------------*/
void create()
{
  string msg;
    
  ::create();

  // Main description
  Set(P_INT_SHORT, "Hall Of Fame");
  Set(P_INT_LONG,
"A chamber hewn out of the rock quite recently. One wall is surprisingly "
"flat and covered with small scratches. A note has been glued to one of "
"the other walls.\n"
     );
  Set(P_BRIGHT, 0);
  Set(P_NODWARF, 1);
  Set(P_NOORIG, 1);

  lastsize = -2;
  wall = "";
    
  // Exits
  AddExit("west", ADVENT("r/aftergrate"));

  // Details
  AddDetail(({ "scratches", "wall", "flat wall", "wall of fame" }),
"The scratches appear to be a list of names and dates. "
"A big headline says 'Wall of Fame'.\n"
	   );
  AddDetail("note", 
"The note is firmly glued to the wall. It reads:\n"
"  Here underground you can use a new command: 'adventure'.\n"
	   );
  AddReadMsg("note", 
"  Here underground you can use a new command: 'adventure'.\n"
	   );
    
  // Items
  AddItem("/std/board", REFRESH_HOME
	 , ([ P_SHORT:"The Advent Cave Board"
            , P_LONG: 
"The message board serves as information exchange for the spelunkers "
"roaming these caves and tunnels.\n"
            , "Groups":({ "a.advent" })
           ])
	 );

}

public void init()
{
  ::init();
  add_action("fread", "read");
}

/*-----------------------------------------------------------------------*/
public int fread (string arg)

/* The player can read the wall of fame, using more, from the end on.
 */

{
  int size;
    
  if (!arg)
    return 0;
  if (-1 == member(({"wall", "fame", "wall of fame", "scratches", "list"
                    ,"names", "dates" }), norm_id(arg)))
    return 0;
  
  /* If necessary, reread the file */
  size = file_size(ADVENT("l/SPELUNKERS"));
  if (size != lastsize)
  {
    lastsize = size;
    wall = "     ----- Wall Of Fame -----\n\n";
    if (size > 0)
      wall += read_file(ADVENT("l/SPELUNKERS"));
    else
      wall += "Apparently nobody got famous enough yet to be listed here.\n";
  }
  smore(wall);
  return 1;
}

/*************************************************************************/
