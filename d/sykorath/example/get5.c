/*
 * example 5 for getable vitems
 * Magictcs - 15 march 98
 */

/*
  How to use it:
  more enhanced version:
  now we want to use the P_NODROP property too!

  AddVItem( ([
    P_IDS:  ({"id1","id2",...}),
    P_ADS:  ({"ad1","ad2",...}),
    P_LONG: "long description of vitem...",
    P_NOGET: "msg for interactive player"
  ]) );

  This example shows how to use the P_NODROP property in its enhanced form.
  The message is sent only to the interactive player!!
  (The msg can be a closure too:
     P_NOGET: string|closure
  )
*/

#include <properties.h>
#include <moving.h>
#include <regions.h>
#include <coordinates.h>
#include <ansi.h>
#include <sykorath.h>

inherit BASEROOM;

//*******
// create
//*******
varargs void create() {
  string v_table;
  ::create();
  SetIntShort("Example5");
  SetIntLong("Example room5\n"
    "Try to look at the table and the knife on it.\n"
    "Then get the table and/or the knife.\n"
    "The knife and the table are not getable anymore. But they will react "
    "the 'get/take' commands!\n"
    "A table is here.\n");

  SetIndoors(0);
  SetCoordinates( ({ ({600,600,0}), C_SYKORATH }));
  SetRegion(REGION_CITY);

  /* details */
  v_table=AddVItem( ([
    P_IDS:  ({"table"}),
    P_ADS:  ({"old"}),
    P_PREPS: ({"on","from"}),
    P_NOGET: "It's nailed to the ground.\n",       // item is not getable
    P_LONG: "An old table with a knife on it.\n" ]) );
  AddVItem( ([
    P_IDS:  ({"knife"}),
    P_NOGET: "It's fixed to the table.\n",         // item is not getable!
    P_LONG: "A simple knife.\n" ]),v_table);

  /* Exits */
  AddExit("east","./get6");
  AddExit("west","./get4");
}

