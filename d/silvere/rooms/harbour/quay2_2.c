// The way to the southern quay and the tower.
// created:          Sonic [26-May-1997]
// last changed:     Tune  [01-Jun-1997]
//                   Sonic [05-Jul-1997] added regions/coordinates
//                   Taurec [10-Oct-1997] improved roomdesc
//                   Taurec [07-Jan-1997] Added all details :)
//                Tune [04-Apr-98]  Removed ship for now

#include <silvere.h>
#include <properties.h>
#include <coordinates.h>
#include <nightday.h>
#include <regions.h>
#include <moving.h>
#include <macros.h>
#include <rooms.h>

inherit HARBOUR_BASEROOM;
inherit "/obj/lib/port.c";

// Current ships visiting this port:
#define HARBOURFERRY  OBJ("transport/harbourferry")

string look_people();
mixed look_beacons();

varargs void create()
{
   HARBOUR_BASEROOM::create();
   port::create();
   Set(P_INDOORS,0);
   Set(P_INT_SHORT,"A street");
   Set(P_INT_LONG,
     "This is just a common landing place for all sorts of ships. But "
     "somehow it is a very special place though. From here, many brave "
     "adventurers started their travel into unknown countries - some "
     "came back as heroes, but others never returned. @@TimeMsg@@ "
     "A timetable stands here.\n");
   Set(P_TIME_MSG,([
     DAY_DEFAULT_MSG:
       "There are many people around here, some are members of races you "
       "never saw before. Waiting for their embarkment, they are all "
       "suspiciously guarding their bagage and goods, glancing around "
       "attentively.",
     NIGHT_DEFAULT_MSG:
       "Even though it is nighttime, there are still some people around "
       "here, lingering in the shadows and waiting for embarkment. The "
       "beacons on the southern defense tower throw everything into a "
       "trembling twilight."
    ]));

   Set(P_INT_WIZMSG,
      "east: City-Gates, Harbourmaster, Inn, quay #1\n"
      "West: The southern defense tower\n");


// ************** SHIP STUFF *****************
// Ships may be added here :)

   AddDetail(({"table","timetable","time table","time-table"}),
     SF(detail_table));
   AddReadMsg(({"table","timetable","time table","time-table"}),
     SF(detail_table));
   SetAutocall(1);
   AddRoomCmd("course",SF(cmd_course));
   AddShip(HARBOURFERRY);

// **************** VITEMS *******************

   AddVItem(([
     P_IDS: ({"people","members"}),
     P_ADS: ({"suspiciously","waiting"}),
     P_LONG: #'look_people
   ]));

   AddVItem(([
     P_IDS: ({"bagage","goods"}),
     P_LONG:
      "Bags, backpacks, wooden boxes, suit cases... guarded suspiciously "
      "by their owners.\n"
   ]));

   AddVItem(([
     P_IDS: ({"beacon","beacons"}),
     P_ADS: ({"shiny"}),
     P_LONG: #'look_beacons
   ]));

// *******************************************

   Set(P_INT_MAP,"harbour");
   Set(P_REGION, REGION_CITY|REGION_HARBOUR);
   Set(P_COORDINATES, ({ ({ -250,20,0 }), C_SILVERE }) );
   Set(P_NOWAY_MSG,"You run silly around and nearly fall in the water.\n");

   AddExit("west", SIL_HARBOUR("quay2_3"));
   AddExit("east", SIL_HARBOUR("quay2_1"));
}

string look_people()
{
  if (({int})NIGHTDAY->IsDay()) return
   "Those people are a mixture of adventurers, warriors, "
   "scientists who want to explore unknown territories, merchants, "
   "and some even look like pirates or other scum. Some are of races "
   "you only heard of in bards tales.\n";
  else return
   "The shadows of the night cover the people around you "
   "into anonymity. Some glowing eyes blink here and there out "
   "from the shadows, and you wonder what kind of race the eyes "
   "owner belongs to.\n";
}

mixed look_beacons()
{
  if (({int})NIGHTDAY->IsDay())
    return "The beacons are only lit at night.\n";
  else return
   "Those beacons serve the ships to find the harbour entrance "
   "even at night. They are blazing fires on top of the two towers "
   "guarding the harbour entrance. Their light creates and eerie flickering "
   "illumination here, a dancing change between shadows and light.\n";
}


void notify_enter(object oldenv,int method,mixed extra) {
  HARBOUR_BASEROOM::notify_enter(oldenv,method,extra);
  port::notify_enter(oldenv,method,extra);
}
