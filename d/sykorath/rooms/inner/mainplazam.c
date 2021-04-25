/* part of the mainplaza of the town
 * made by magictcs - 22 may 97
 *
 * last changes:
 * [m] 08 jan 97 - remade, added first version for pillory
 */

#include <properties.h>
#include <moving.h>
#include <regions.h>
#include <coordinates.h>
#include <ansi.h>
#include <sykorath.h>

inherit BASEROOM;

static int pillory;

//******************
// property handling
//******************
int SetPillory(int p) { return pillory=p; }
int QueryPillory() { return pillory; }

//****************
// special details
//****************
string look_pillory() {
  if (pillory) return
    "A pillory - made out of hard (oak) wood.\n"
    "It has three holes, a bigger one in the middle and two smaller ones "
    "at the end of rod (?)\n";
  return
  "Actually there is no pillory here.\n";
}

string QueryIntLong() {
  if (pillory) return
    "This place is part of the main plaza of the town.\n"
    "There is a pillory build in the middle of the plaza.\n";
  return
    "This place is part of the main plaza of the town.\n"
    "Sometime a pillory can be seen here.\n";
}

//*******
// create
//*******
create() {
  ::create();
  pillory=0;
  SetIntShort("The main plaza");

  SetIndoors(0);
  SetCoordinates( ({ ({600,600,0}), C_SYKORATH }));
  SetRegion(REGION_CITY);

  SetMap("inner");

  /* details */
  AddVItem( ([
    P_IDS:  ({"plaza","place"}),
    P_ADS:  ({"this","main"}),
    P_LONG: #'IntLong ]) );
  AddDetail("town","You are standing inmids the town.\n");
  AddDetail("pillory",#'look_pillory);

  /* Exits */
  AddExit("north",    "./mainplazan");
  AddExit("west",     "./mainplazaw");
  AddExit("south",    "./mainplazas");
  AddExit("east",     "./mainplazae");
  AddExit("northwest","./mainplazanw");
  AddExit("northeast","./mainplazane");
  AddExit("southwest","./mainplazasw");
  AddExit("southeast","./mainplazase");

  Set(P_INT_WIZMSG,
    "%^RED%^add a buildable pillory, torture place,"
    "check the coords, add details!%^RESET%^\n"
    "Wizards can set the pillory to active, by calling SetPillory(1)\n"
  );
}

