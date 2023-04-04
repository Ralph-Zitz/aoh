// The way to the northern quay and the tower.
// created:          Sonic   [26-May-1997]
// last changed:     Taurec  [11-Jun-1997]
//                   Sonic   [05-Jul-1997] added region/coordinates
//                   Tune    [13-Oct-1997] desc & nightday
#include <silvere.h>
#include <properties.h>
#include <coordinates.h>
#include <regions.h>
#include <moving.h>
#include <macros.h>
#include <rooms.h>
#include <nightday.h>

inherit HARBOUR_BASEROOM;
inherit "/obj/lib/port.c";

// Current ships visiting this harbour:
#define HARBOURFERRY  OBJ("transport/harbourferry")

public varargs void create(int repl)
{
   string dawnmsg;
   HARBOUR_BASEROOM::create(repl);
   port::create();
   Set(P_INDOORS,0);

   Set(P_INT_SHORT,"The docks");
   Set(P_INT_WIZMSG,
    "This is the docking part of the quay.\n"
    "east: City-Gates, Harbourmaster, Inn, quay #2\n"
    "West: The northern defense tower\n");

   Set(P_INT_LONG,
    "Fish nets are piled up alongside pails of old fish. @@TimeMsg@@ "
    "This is the northern docking area of the harbour. A timetable "
    "stands here.\n");

   dawnmsg = "The water near the docks appears slightly greyish-green "
       "in the misty light of dawn.";

   Set(P_TIME_MSG,
    ([
     DAY_DEFAULT_MSG:
       "Gulls caw and call to each other as they wheel through the "
       "sky, looking for handouts or easy prey.",
     NIGHT_DEFAULT_MSG:
       "The black water of the harbour reflects the stars.",

     // early morning messages
     ND_PREDAWN: dawnmsg,
     ND_DAWN: dawnmsg,
     ND_SUNRISE: dawnmsg,

     // evening messages
     ND_SUNSET:
       "The setting sun throws a silvery sheen over the sea.",
     ND_TWILIGHT:
       "The water in the bay appears silvery blue-green in the "
       "misty light of evening.",
     ND_EVENING:
       "The water in the bay appears silvery blue-green in the "
       "misty light of evening.",
    ]));

   AddDetail(({"seagulls","gulls"}),
    "These scavenging white birds can be seen all around the harbour during the day.\n");

   AddDetail(({ "nets", "fish nets"}),
    "Tattered and torn, it is evident that these nets have been discarded.\n");

   AddDetail(({"table","timetable","time table","time-table"}),
     SF(detail_table));
   AddReadMsg(({"table","timetable","time table","time-table"}),
     SF(detail_table));

   AddVItem(
    ([ P_IDS :  ({ "pails" }),
       P_ADS :  ({ "rusty", "tin" }),
       P_LONG :  "Rusty tin pails line the land side of the quay.\n",
       P_SMELL : "The disgusting offal of the last haul spreads its stench "
	         "everywhere.\n"
    ]) );

// ******* ship stuff, ships may be added here *******
/*  *** Tune 04 Apr 98  -- Commented out ship stuff
                           created errors, and not
                           very necessary right now */

   AddDetail(({"table","timetable","time table","time-table"}),
     SF(detail_table));
   AddReadMsg(({"table","timetable","time table","time-table"}),
     SF(detail_table));
   SetAutocall(1);
   AddRoomCmd("course",SF(cmd_course));
   AddShip(HARBOURFERRY);

// ***************************************

   Set(P_INT_MAP,"harbour");
   Set(P_REGION, REGION_CITY|REGION_HARBOUR);
   Set(P_COORDINATES, ({ ({ -250,0,0 }), C_SILVERE }) );
   Set(P_NOWAY_MSG,
    "You'll fall in the water if you don't watch where you're going.\n");

   AddExit("west", SIL_HARBOUR("quay1_3"));
   AddExit("east", SIL_HARBOUR("quay1_1"));

}

void notify_enter(object oldenv,int method,mixed extra) {
  HARBOUR_BASEROOM::notify_enter(oldenv,method,extra);
  port::notify_enter(oldenv,method,extra);
}

