/* Death Room
**
** Make death such boring that players learn to avoid it.
** The player now returns to the place where it died.
**
**   26-Aug-92 [m] Adapted for native.
**   25-Nov-93 [p] combine_strings
**   18-May-94 [h] rewrite of functions
**   28-May-94 [l] Small bugfix. Also changed text to give "SHE" for females.
**                 Reformatted some lines to max 80 chars.
**   07-Seb-94 [m] Wizards are expelled from the room.
**   20-Jan-95 [p] Use preprocessor string concatenation instead of line
**		   continuations. Make player mapping private. Use closure.
**   09-Dec-95 [m] Wizards with P_NOWIZ set may die normally.
**   01-Jan-96 [m] Some details added.
**
**  [m] Mateese
**  [p] Pepel
**  [h] Hyp
**  [l] Loco
*/

#include <properties.h>
#include <moving.h>
#include <rooms.h>
#include <wizlevels.h>
#include <stdrooms.h>

inherit "/std/room";

#define TP           this_player()
#define TO           this_object()
#define WRITE(x)     tell_object(who, "\n" x)
#define SPEAK(x)     tell_object(who, "\nBalgarion says: " x)
#define G(maletxt,femaletxt) ((who->QueryGender()==GENDER_MALE) ? maletxt : femaletxt)

private static mapping players = ([]);

_query_player() { return players; }

void create() {
  ::create();
  SetIntShort("Seppuko Room");
  SetIntLong(
"This part of the Dojo is only meet by the Senseis. The only colour present\n"
"here is white. The south side is open, looks like there you can go to the \n"
"veranda. At the northern side yuo recognice two sets of swords you cant \n"
"believe that such a fine work exist.\n"
"Outside you can side a small table on which a sharp looking dagger is lying.\n\n");
  /* SetIntLight(1); */
  SetProp(P_TPORT, TPORT_NO);
  SetMagicProtection(([ST_ALL:"You can't cast spells yet.\n"]));
  SetIndoors(1);
  AddItem("/players/balgarion/guild/npc/balg.c", REFRESH_REMOVE);

  AddDetail("table", 
"The table is made of white wood with red ornaments on it.\n"
           );
  AddDetail("set of swords", 
"Try set1 or set2.\n"
           );
  AddDetail("set1",
"A wonderfull set of 2 swords included a katana and a wakizashi. They are \n"
"of very fine work and you think you know the owner of them. YES......\n"
"it is.............................MAGICTCS.....your teacher of fighting.\n"
           );
  AddDetail("set2",
"After a deeper look you see no weapons in the sheathes. But you know where\n"
"you have seen these ornaments present on the sheathes. The Katana is in \n"
"Balgarion`s hand and the wakizashi lies on the table outside."
           );
  AddDetail("dagger",
"The wakizashi of set2 owned by Balgarion and use for SEPPUKO.\n"
           );
}

string QueryIntLong(mixed arg) {
  int i;
  string rc;

  rc = ::QueryIntLong(arg);
  if (!arg && (players[TP] >= 40 && players[TP] < 62))
    return rc + "Magictcs the teacher of the forces.\n";
  return rc;
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
  switch (query_verb()) {
  default:
    write("You are not in the mood to do this.\n");
    return 1;
  case "exa":
  case "l":
  case "look":
  case "take":
  break;
  }
}
void death_messages(object who, int waiting) {
  object dmark;
  mixed place;
  int notplace, rc;
  
  switch (++waiting) {
  case 5:
    SPEAK("YOU ARE GOING THE RIGHT WAY FOR A MADMONK.\n");
    WRITE(
"Balgarion looks deep in your eyes and it seems that his lips are going to a\n"
"smile. Now he looks like the friendly teacher you know. He comes to you and\n"
"take your hand. Than he helps you to stand up and gives you a giant bearhug.\n"
"Then he becomes serious again.\n");
    break;
  case 10:
    SPEAK("IAM SORRY TO LOOSE A GOOD STUDENT LIKE YOU.\n");
    WRITE(
"You feel shame. You really know now that you hurt this wonderfull man.\n"
"Didnt he teach you so much and Magictcs too?? SHAME over you!!\n");
    break;
  case 15:
    SPEAK("I WILL DO YOU A FAVOUR. I WILL HELP YOU DOING SEPPUKO. SO YOU CAN DIE IN HONOUR!\n");
    WRITE(
"Now you know why you see Balgarion the first time with his katana.\n"
"Befor you can cry of terror he will cut your head. He is a real friend.\n");
    break;
  case 20:
    SPEAK("BUT FIRST I HAVE TO CHECK YOUR WAY OF LIVING.\n");
    WRITE(
"Balgarion gets an scroll out of his robe and opens it. Than you starts to\n"
"read what is writen on it. You feel a little confused. What did he mean\n"
"with WAY OF LIVING? Suddenly you remember all the little things you did\n"
"wrong. Didnt you leave a fellower in the middle of a fight cause you was\n"
"a little hurt? You remember so many hours when you casts curses over your\n"
"teachers and you wished them to hell. And again you are looking for a mouse\n"
"hole to crawl in it. Then you think it cant be getting bad.\n");
    break;
  case 30:
    switch (who->QueryProp(P_ALIGN)) {
    case -10000..-1:
      SPEAK("HOW NICE. ARE YOU SURE THAT YOU LIKE TO FOLLOW THE RULES OF\n"
	    "THE GODS OF NIGHTFALL OR DO YOU LIKE YOUR SENSEIS SO MUCH TO\n"
	    "FOLLOW OUR RULES. BUT I SEE YOU WAS A VERY GOOD STUDENT OF MAGICTCS\n"
	    "AND MYSELF. BE SURE THAT WE WILL KEEP YOU IN OUR MINDS.\n");
    case 0:
      SPEAK(
"WHAT A FENCE-CLIMBER WE HAVE HERE! NEVER MADE UP YOUR MIND IN\n"
"ALL YOUR LIFE, DID YOU? WELL, DON'T WORRY. YOU WON'T HAVE TO NOW \n"
"EITHER! HAHAHA!\n");
      break;
    case 1..10000:
      SPEAK(
"UH ARE YOU SURE YOU ARE A MEMBER OF THE RIGHT GUILD? MAYBE THE PRIESTS\n"
"WILL BE A BETTER ONE FOR YOU. OR ARE YOU SEARCHING FOR THE REAL SAMURAI?\n"
"IF YOU WILL GET ONE LET ME KNOW THAT!!!\n");
      break;
    }
    break;
  case 35:
    SPEAK(
"OK ITS TIME FOR YOU KNOW. IT LOOKS LIKE YOU PREPARED YOURSELF VERY WELL FOR\n"
"THE DEATH. LETS GO NOW.\n");
    WRITE(
"Balgarion now leads you to the veranda in front of the table. He points to\n"
"the floor so you have to knee down. You open your robe and lay it packed\n"
"down on the floor. Then you take the wakizashi laying on the table and \n"
"check the sharpnes. OUTCH that hurts a little. Your blood ruins the white\n"
"of the floor and table. You starts to concentrate to your life and your\n"
"hands starts to work of their own. Your right hand gets the dagger with\n"
"a hard hold and it points to your stomach! The left one hold the right one \n"
"to get a better feeling when opening your body.\n");
    break;
  case 40:
    WRITE("Suddenly a part of the walls move to the side and Magictcs comes in.\n");
    break;
  case 45:
    WRITE("He looks at the scene in front of him and sends you a thinking look.\n");
    break;
  case 47:
    WRITE("Then it seems that he remembers you and he walk over to Balgarion.\n");
    break;
  case 54:
    SPEAK("HEY MAGICTCS WHATS UP, HE LOST A FIGHT AND HE WANT TO GO THIS WAY.\n");
    break;
  case 56:
    WRITE("Magictcs says: COME ON BALGARION HE IS A VERY GOOD FIGHTER.\n");
    break;
  case 58:
    WRITE("Magictcs voice raise up a little: DIDNT YOU NEVER LOOSE A FIGHT BALGARION?\n");
    break;
  case 60:
    SPEAK(
"YES I CAN REMEMBER ONE OR TWO, BUT AFTER THEM I TRIED TO LEARN MORE,\n"
"DO YOU THINK" +G("HE","SHE")+" WILL DO THIS TOOO?\n");
    break;
  case 62:
    WRITE("Magictcs smiles at you and you know you will never hurt this one.\n");
    break;
  case 64:
    WRITE("You think to yourself: I will learn as much as i can!\n");
    break;
  case 66:
    WRITE(
"Balgarion takes Magictcs and both are going to the sheath where Balgarion\n"
"puts his katana in one. Between this both are talking and than they are \n"
"laughing\n");
    break;
  case 70..1000:
    SPEAK(
"OK STUDENT YOU GET ANOTHER CHANCE TAKE IT AND BEGOME BETTER.\n");
    WRITE(
"Magictcs comes to you and and take the dagger out of your hands. He helps you\n"
"standing up and you are dressing yourself correctly. Than he leads you to the\n"
"open wall and suddenly you are standing in the trainingroom.\n");
    who->move("/players/balgarion/workroom.c", M_SILENT);
    while (dmark = present("death_mark", who)) {
      place = dmark->query_place();
      destruct(dmark);
    }
    notplace = 0;
    if (!place || !(place->QueryProp(P_TPORT) & TPORT_IN))
    {
      notplace = 1;
      place = who->QueryHome() || CHURCH;
    }
    SetProp(P_TPORT, TPORT_OUT);
    if (catch(rc = who->move(place, M_SILENT)) || rc != ME_OK)
    {
      notplace = 1;
      who->move(CHURCH, M_SILENT);
    }
    if (!notplace)
      WRITE("The shape on the ground seems familiar to you ... very familiar...\n\n");
    else
      WRITE("There is NO familiar looking shape on the ground...\n\n");
    SetProp(P_TPORT, TPORT_NO);
    m_delete(players, who);
  }
}  
void heart_beat() {
  int s;

  m_delete(players, 0);
  walk_mapping(players, #'death_messages);
  m_delete(players, 0);
  if (!sizeof(players))
    set_heart_beat(0);
}













