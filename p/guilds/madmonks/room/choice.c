#include <properties.h>
#include <moving.h>
#include <rooms.h>
#include <wizlevels.h>
#include <stdrooms.h>

inherit "/std/room";

#define TP  this_player()
#define TO  this_object()

private static mapping players = ([]);

_query_player() { return players; }

void create() {
  ::create();
  SetIntShort("Choice Room");
  SetIntLong(
"");
  /* SetIntLight(1); */
  SetProp(P_TPORT, TPORT_NO);
  SetMagicProtection(([ST_ALL:"You can't cast spells yet.\n"]));
  SetIndoors(1);
}

void init() {
  object death;

  ::init();
  death = present("moot", TO);
  if (TP != death) {
    if (IS_IMMORTAL(TP) && !TP->Query(P_NOWIZ)) {
      write(
"WHAT ARE YOU DOING HERE? LIKE TO GO THE WAY OF HONOUR AND POWER?\n"
           );
      TP->move(TP->QueryHome(), M_TELEPORT);
    } else if (TP->QueryGhost()) {
      add_action("filter", "", 1);
      players[TP] = 0;
      set_heart_beat(1);
    } else {
      write(
"WHAT ARE YOU DOING HERE? You arnt wise enough for Seppuko. BEGONE!\n");
      TP->move(TP->QueryHome(), M_TELEPORT);
    }
  }
}
int filter(string str) {
  object player;
  player=previous_object();  switch (query_verb()) {
  default:
    write("JUST TYPE LIFE OR HONOUR!!!!!!!\n\n");
    return 1;
  case "life":
    write("Suddenly both of your teacher are in the same room as you.\n"
          "They look at you shaking their heads and are whispering a short\n"
          "time with each other. Then they are looking at you dangerous!\n");
    write("Magictcs and Balgarion suddenly begin to speak to you:\n\n"
          "WELL YOU MAKE YOUR WAY. BECAUSE OF OUR HIGH HONOUR IT IS \n"
          "FORBIDDEN FOR US TO LIFE AFTER WE LOST A FIGHT. BUT YOU TAKE\n"
          "THIS WAY. SO WE HAVE TO\n"
          "..........................BANISH............................\n"
          "                                 YOU NOW. YOU HAVE TO LEAVE \n"
          "THIS GUILD AND WILL NEVER BE A MEMBER AGAIN. YOU LOOS ALL\n"
          "YOUR STATS, GUILDLEVELS AND POWERS WITHOUT GETTING THEM BACK.\n"
          "..................NOW LEAVE US............................\n");
    write("They make a gesture and find youself nacked and without the \n"
          "tattoo in front of the guild\n");
    call_out("banish",10,player);
   return 1;
  case "honour":
    write("Balgarion says to you with a smile on his lips:\n"
          "THIS WAS THE RIGHT JOICE!\n");
    call_out("sepuko",5,player);
    return 1;
  }
}
sepuko(object player)
{
tell_object(player,"He spoke this words and suddenly you are in the secred room\
s of the SENSEI.\n\n");
tell_room(environment(player),"Balgarion takes his student with him.\n");

player->move("/players/balgarion/guild/room/death_room",M_TELEPORT,
  "to his Senseis");
}

banish(object player) {
  object ob;
  ob=present("mm_tattoo",TP);
  if (ob) ob->remove();
  player->SetGhost(0);
  player->SetGuild("Banished by  MADMONKS");
  player->SetDeadOb(0);
  player->move(CHURCH,M_TELEPORT,
    "out of the guild");
}

