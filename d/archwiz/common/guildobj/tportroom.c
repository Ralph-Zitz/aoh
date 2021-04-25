/* a travelcenter for the adventurers */
/* Cadra@nightfall 14-06-94 */

#include <moving.h>
#include <secure/wizlevels.h>
#include <config.h>
#include <rooms.h>
#include <magic.h>
#include "path.h"

inherit "std/room";

private static mapping advexits;


AddGuildExit(string place, string path) {
  advexits+=([place:path]);
  return ::AddExit(place,path);
}

UseExit(string verb,string arg,int method) {
  if (!this_player()) return 0;
  if (!query_once_interactive(this_player())) return 0;
  if (((this_player()->QueryGuild())!="adventurer") &&
    !(IS_IMMORTAL(this_player()))) {
    tell_object(this_player(),
"This room is beyond your understanding. Luckily a helpful hand catches you "
"and moves you into a large church.\n");
    return this_player()->move(CITY("church"),M_GO);
    }
  if (member(advexits,verb) >= 0 &&
    (!(QUESTMASTER->QueryQuestSolved(verb,this_player())))) {
    tell_object(this_player(),
"As you wouldn't know where you would arrive this room protects you from "
"visiting unknown area. You have to chose another way. Maybe it would be "
"a good idea to visit that guild on your feet?\n");
    return 1;
    }
  return ::UseExit(verb,arg,method);
}

create() {
  ::create();
  advexits=([]);
  SetIntShort("a very special airport");
  SetIntLong(
"The room is filled with a humming noise. Many busy adventurer running around "
"looking for quests at several places all over Nightfall. There are many portals "
"all around you. Every portal is marked with the name of the adventurerguild it "
"leads to. There is a sign in the middle of the room.\n");
  AddReadMsg(({"sign"}),
"The inscription reads:\n"
"Seeker of adventures you will find yourself able to move through any portal "
"if you know the place that lies behind. If you never found that place on "
"common ways, there is no way for you to pass that portal now.\n");
  AddDetail(({"sign"}),"The inscription reads:\n"
"Seeker of adventures you will find yourself able to move through any portal "
"if you know the place that lies behind. If you never found that place on "
"common ways, there is no way for you to pass that portal now.\n");
  AddDetail(({"portal","portals"}),
"The portals lead into grey mist. You are not sure if you really should try "
"them.\n");
  SetIntNoise("It sounds like a really busy place.\n");
  SetIntSmell("It is wet in here.\n");
  SetTPort(TPORT_NO);
  AddGuildExit("nf-city",CITY("adv_guild.1"));       // Nf-City
  AddGuildExit("camelot","/d/avalon/common/village/houses/advguild"); // Camelot
  AddGuildExit("hobbiton","/d/halfling/common/hobbiton/guild");      // Hobbiton
  AddGuildExit("orccaves","/d/shadows/common/room/orccave/adv_guild"); // Orcs
  AddGuildExit("dark-isle","/d/shadows/common/room/darkcity/adv_guild"); // Dark Isle (Trolls)
  AddGuildExit("hell","/d/hell/common/room/bazaar/hellguild");        // Hell
  AddGuildExit("wooden-city","/d/woodland/common/city/guild");  // Wooden City
  AddGuildExit("mandukam","/d/woodland/common/mandukam/adv_guild"); // Mandukam
  AddGuildExit("giants","/d/orient/common/room/giant/adv_guild"); // Giants
  AddGuildExit("vecna","/d/vecna/common/city/advguild");  //Vecna
  AddGuildExit("fort","/d/woodland/common/fort/adv_guild");  //guildfortress
  AddGuildExit("port-chien","/d/orient/common/room/portchien/adv_guild"); // port chien
AddGuildExit("zzerenzzen","/d/atlantis/common/room/darkelf/zzerenzzenguild");
                                                                // Zzerenzzen
AddGuildExit("atlinten","/d/atlantis/common/room/city/adv_guild48,49");
                                                                // Port Atlinten
  AddGuildExit("dreconia","/d/wasteland/common/room/dreconia/adv_guild");
								// Dreconia
  AddGuildExit("thull","/d/atlantis/common/room/thull/adv_guild");
								// Thull Zelik
  AddGuildExit("kiev","/d/orient/common/room/kiev/adv_guild");  //kiev
  AddGuildExit("border","/d/woodland/common/border/adv_guild");
								// Miner's town
  AddGuildExit("mines","/d/halfling/common/room/dwarf/guild");
							// Dwarven mines
}
