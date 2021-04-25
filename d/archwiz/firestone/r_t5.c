#include <rooms.h>
#include <moving.h>
#include <properties.h>
#include "path.h"

inherit "/std/room";
 
private static object scroll;

look_floor() {
    if (scroll) {
    return "You find nothing.\n";
    }
    scroll=clone_object(SCROLLS("victim"));
    scroll->move(this_object(),M_SILENT);
    return "You found a scroll on the floor. Just get it.\n";
}

create() {
  ::create();
  SetIntShort("In a kennel");
  SetIntLong("You are in a kennel.\n");
  SetIndoors(1);
  AddDetail(({"floor","ground"}),#'look_floor);
  AddDetail("kennel",#'IntLong);
  AddExit("south","./r_t3");
  AddItem("/std/npc",REFRESH_HOME,
    ([P_NAME: "doberman",
      P_SHORT: "a dangerous doberman",
      P_LONG: "A mean and hungry Klingon doberman barks at you...\n",
      P_RACE: "dog",
      P_AGGRESSIVE: 1,
      P_LEVEL: 2,
      P_HANDS: ({ ({"teeth",0,6}),({"teeth",0,6}) }),
      P_AC: 1,
      P_GENDER: 1,
      P_ALIGN: -30]) );
  AddItem("/std/npc",REFRESH_HOME,
    ([P_NAME: "rottweiler",
      P_SHORT: "a huge rottweiler",
      P_LONG:
"A mean hungry rottweiler is snarling at you!\n",
      P_RACE: "dog",
      P_AGGRESSIVE: 1,
      P_LEVEL: 2,
      P_HANDS: ({ ({"sharp teeth",0,6}),({"sharp teeth",0,6}) }),
      P_AC: 1,
      P_GENDER: 1,
      P_ALIGN: -30]) );
  AddItem("/std/npc",REFRESH_HOME,
    ([P_NAME: "watchdog",
      P_SHORT: "a huge watchdog",
      P_LONG:
"You see a black nose, black eyes, black fur, white teeth.\n",
      P_RACE: "dog",
      P_AGGRESSIVE: 1,
      P_LEVEL: 2,
      P_HANDS: ({ ({"teeth",0,6}),({"teeth",0,6}) }),
      P_AC: 1,
      P_GENDER: 1,
      P_ALIGN: -30]) );
  reset();
}

reset() {
  ::reset();
  scroll=0;
}
