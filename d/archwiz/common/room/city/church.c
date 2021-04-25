/*
* The good village church in the Capital of Nightfall
* Changes by Thragor, 07apr94
* Last Changes by Cadra 02.10.1994
*
* /d/archwiz/common/room/pirate/downch1 is below, everything said there will
* also be telled into this.
*/

#include "path.h"
#include <secure/wizlevels.h>
#include <properties.h>
#include <rooms.h>
#include <moving.h>

inherit "std/room";

private static int lamp_is_lit, reboot_time, time_from_reset, last_reset_cycle;
private static int list_length;
private static int scroll;

int say(string str) {
  tell_room(PIRATE("downch1"),
"You hear voices from over your head. Exactly you hear someone saying: "+
str+"\n");
}

int enter(string str) {
  if (!str) return 0;
  if ("pit"!=lower_case(str)) return (int) notify_fail(
    "What do you want to enter?\n") && 0;
  write(
"You enter the bottomless pit. Didn't we tell you that it is BOTTOMless?\n");
  if (!this_player()) return 0;
  this_player()->SetExplored();
  this_player()->move(CITY("pit"),M_GO,"down");
  return 1;
}

look_sill() {
    if (scroll) {
    return "The windowsill looks a bit dusty, but there seems to be a "
"small part where it is more clean. This part looks as if a scroll would "
"fit in.\n";
    }
    scroll=clone_object(SCROLLS("greatrevelation"));
    scroll->move(this_object(),M_SILENT);
    return "You found a scroll on the windowsill. Just get it.\n";
}

reset() {
  ::reset();
  last_reset_cycle = time() - time_from_reset;
  scroll=0;
}

get_time() {
  int i, j;
  string str;
  if (!this_player()) return 0;
  str =
"This is a strange clock indeed which shows a different time than the other "
"clocks you have seen in Nightfall. It surely must have ceased functioning "
"long ago. The hands show ";
  i = time() - reboot_time;
  j = i / 60 / 60 / 24;
  if (j == 1) str += "1 day ";
  else if (j > 0) str += j + " days ";
  i -= j * 60 * 60 * 24;
  j = i / 60 / 60;
  if (j == 1) str += "1 hour ";
  else if (j > 0) str += j + " hours ";
  i -= j * 60 * 60;
  j = i / 60;
  if (j == 1) str += "1 minute ";
  else if (j > 0) str += j + " minutes ";
  i -= j * 60;
  if (i == 1) str += "1 second";
  else if (i > 0) str += i + " seconds";
  str += ".\n";
  if (!IS_WIZARD(this_player())) return str;
  str += "Time since reset is " + (time() - time_from_reset) + " seconds.\n";
  if (last_reset_cycle) str += "Reset cycle: " + last_reset_cycle + "\n";
  return str;
}

get_door() {
  string str;
  str = "You are stunned looking at the door! It is made of shining steel and shows "
"no signs of rust. At one side of the door you see a small button.\n";
  if (((CITY("elevator"))->QueryDoorStatus()==LOCK_OPEN) &&
      (CITY("elevator"))->QueryLevel())
    return str+"The door is open.\n";
  else return str+"The door is closed.\n";
}

my_getfun(string arg)
{
  object obj;
  if (!arg) return 0;
  if (arg=="torch") {
    if (!this_player()) return 0;
    if (!present(arg,this_player()) && !present(arg)) {
      obj=clone_object("/obj/torch");
      obj->Set(P_NOBUY,1);
      if (obj->move(this_player(),M_GET)!=ME_OK) {
	write("You cannot carry it anymore!.\n");
	see(this_player()->QueryName()+
	  " tries to take a torch, but is too weak to carry it.\n");
	obj->remove();
	if (obj) destruct(obj);
	}
      write("You take a torch.\n");
      see(this_player()->QueryName()+" takes a torch.\n");
      return 1;
    }
     else {
      notify_fail("If you need more torches, go to the shop and buy them.\n");
      return 0;
    }
  }
  return 0;
}

west() {
  if ((CITY("elevator"))->QueryDoorStatus() != LOCK_OPEN ||
      (CITY("elevator"))->QueryLevel() != 2) {
    write("The door is closed.\n");
    return 1;
  }
  if (!this_player()) return 0;
if  (this_player()->QueryIsNPC()) return 0;
  this_player()->move(CITY("elevator"),M_GO,"west");
  return 1;
}

open(string str) {
  if (str != "door") return 0;
  if ((CITY("elevator"))->QueryLevel() != 2) {
    write("Strange, you can't open it. Something is blocking the door.\n");
    return 1;
  }
  (CITY("elevator"))->open_door("door");
  return 1;
}

close(string str) {
  if (str != "door") return 0;
  (CITY("elevator"))->close_door("door");
  return 1;
}

push(string str) {
  if (!str) return 0;
  if (str=="buttons") return notify_fail(
    "There is only one button.\n");
  if (str!="button") return 0;
  if ((CITY("elevator"))->call_elevator(2))
    lamp_is_lit = 1;
  return 1;
}

elevator_arrives() {
  say("The lamp above the door goes out.\n");
  lamp_is_lit = 0;
}

pray() {
  if (!this_player()) return 0;
  if (this_player()->QueryRace() == "visitor") {
    write("You need to get a real body from Dr. Frankenstein.\n");
    return 1;
  }
  if (this_player()->QueryGhost()) {
    write("You are sucked away by a mighty force and reappear in solid form.\n");
    this_player()->SetGhost(0);
    say(this_player()->QueryName()+" appears in solid form.\n");
  } else {
    write("You kneel down and praise the local gods.\n");
    say(this_player()->QueryName()
        +" kneels down and wholeheartedly praises the local gods.\n");
  }
  return 1;
}

QueryIntLong() { return
"You are in the ancient Church of Nightfall City. There is only dim light "
"here, as the windows are made of coloured glass. The floor is built of "
"huge stone plates. Some of these show large letters on them. Perhaps they "
"are gravestones? There is a massive monolith serving as altar at one "
"end of the church. The size of this cathedral takes your breath away. "
"You wonder how many people could come here to pray without even make "
"you think of a full church. Oddly enough there is a circular pit in the "
"center of the cathedral. You wonder for what reason. At the western wall "
"you detect an alien looking door. It somehow doesn't fit into this church, "
"but it isn't the only strange thing on the western wall. There is also a "
"giant clock with man-sized hands.\n"
"This cathedral is the main place to revive ghosts. If you are dead you may "
"pray here. Be sure that your prayer will be heard.\n"
"You can leave the cathedral through the huge double doors to the "
"south.\n"+show_lamp()+
"There are three notes titled RULES, DISCLAIMER and WARNING pinned to the wall. "
"READ them. "
"A torch lies on the ground near the exit.\n";
}

create() {
    ::create();
    SetIntShort("Cathedral");
    reboot_time = time();
    AddDetail(({"doors","double doors","huge double doors","huge doors"}),
"There are huge double doors leading south out of the cathedral.\n");
    AddDetail(({"wall","walls"}),
"There are structures painted on the walls.\n");
    AddDetail(({"structure","structures","ornament","ornaments"}),
"They are just ornaments.\n");
    AddDetail(({"west wall","western wall"}),
"There is not only a gigantic clock at the wall but also a very strange "
"looking door.\n");
  AddDetail(({"stones","stone","gravestones","gravestone"}),
"You read some famous names such as: Jameson, Ende, ...\n");
    AddDetail(({"windows","window"}),
"High Gothic windows with many scenes out of life built out of coloured "
"glass. There are small windowsills below them.\n");
    AddDetail(({"glass","coloured glass","colored glass"}),
"You wonder if it would be possible to find anyone nowadays who could "
"build such windows. They look great and made with lot of knowledge. The "
"scenes it shows are made lifely and you guess that life used to look like "
"they show it when the cathedral was built.\n");
    AddDetail(({"ceiling","frescos","fresco"}),
"The whole ceiling is covered with a huge fresco that shows a whole "
"pantheon. You see gods you never knew they would exist on it. The old ones "
"really have had strange gods. But nevertheless the fresco is shining in "
"a soft glow. Perhaps those gods aren't that dead as people think.\n");
    AddDetail(({"god","gods","old gods","old god",
		"goddess","old goddess","goddesses","old goddesses"}),
"You never before saw these gods anywhere else. This church really must "
"be OLD!\n");
    AddDetail(({"glow","soft glow","blue glow","soft blue glow"}),
"The fresco at the ceilign seems to emerge it. It is all around it, but it "
"doesn't lit the ground of the church. Perhaps some of the old gods still "
"care for this old monument in which people prayed to them for thousands "
"of years though only few still know that they are there.\n");
    AddDetail(({"benches","bench"}),
"The whole official part of the church shows benches everywhere. They "
"face the huge monolith to the north. There is a way through them directly "
"from north to south and a way through them from east to west so that you "
"could reach the pit quite easily.\n");
    AddDetail(({"way","ways"}),
"All ways are clean and tidy. Someone must have a look at them from time "
"to time.\n");
    AddDetail(({"altar", "monolith","huge monolith"}),
"The altar is a massive black monolith, which is a marvellous sight. "
"Sculptures depicting unknown gods and goddesses have been deeply carved "
"into it. Those little sculptures look as if they could jump out of the altar "
"and come to look out for the world again. Perhaps this is the place where "
"old gods go to when noone anymore knows about them.\n");
    AddDetail(({"sculpture","sculptures","little sculpture","little sculptures",
		"god statues","statue","statues"}),
"The sculptures look lively. Wasn't this a blink that one had in its eyes? "
"Didn't you hear silent breathing? But all faces look friendly.\n");
    AddDetail(({"face","faces"}),
"The gods faces look very freindly.\n");
    AddDetail(({"pit","circular pit"}),
"As you look down the circular pit you can see no bottom. The sides of the "
"pit are heavily charred. As you throw something into the pit, you cannot "
"hear the sound of an impact. You wonder what this pit is used for.\n");
    AddDetail(({"bottom","bottom of the pit"}),
"There is no bottom to be seen.\n");
    AddDetail(({"sides","sides of the pit","side of the pit",
		"side","side of pit","sides of pit"}),
"The sides of the pit are completely even. You wonder what sort of "
"miracle could have built it this way.\n");
    AddDetail(({"center","center of church","center of the church"}),
"There is a circular pit in the center of the church.\n");
    AddDetail(({"steel","metal","material","strange material","iron","strong iron"}),
"It seems to be a sort of very strong iron.\n");
    AddDetail(({"side of door","side of the door"}),"There is a button.\n");
    AddDetail(({"letters","letter"}),
"One can't barely make them out. No chance to read them.\n");
    AddDetail(({"plates","plate","huge plates","huge plates",
"huge stone plates","huge stone plate"}),
 "The huge plates are building the floor of the church.\n");
    AddDetail("torch","This is a normal torch. Maybe you can take it.\n");
    AddDetail("button","You might push it.\n");
    AddDetail(({"hands","man-sized hands"}),
"They will show you the time if you would look at the clock.\n");
    AddDetail(({"room","church","cathedral"}),QueryIntLong());
    AddReadMsg("rules",lambda(0,({#'read_file,"/etc/RULES"})));
    AddReadMsg("disclaimer",lambda(0,({#'read_file,"/etc/DISCLAIMER"})));
    AddReadMsg("warning",lambda(0,({#'read_file,"/etc/WARNING"})));
    AddExit("south","./vill_green");
    AddExit("west",#'west);
    HideExit("west",1);
    AddRoomCmd("enter",#'enter);
    AddRoomCmd(({"saz","say"}),#'say);
    AddRoomCmd("push",#'push);
    AddRoomCmd("press",#'push);
    AddRoomCmd("close",#'close);
    AddRoomCmd("open",#'open);
    AddRoomCmd("pray",#'pray);
    AddRoomCmd("regenerate",#'pray);
    AddRoomCmd("get",#'my_getfun);
    AddRoomCmd("take",#'my_getfun);
    AddDetail(({"gigantic clock","giant clock","strange clock","clock"}),
	#'get_time);
    AddDetail(({"door","alien door"}),#'get_door);
    AddDetail("rules", "You should really 'read rules'.\n");
    AddDetail("disclaimer", "You should really 'read disclaimer'.\n");
    AddDetail("warning", "You should really 'read warning'.\n");
    AddDetail(({"windowsill","sill","windowsills","sills"}),#'look_sill);
#if 0
    AddItem(OBJ("bloomsign"), REFRESH_REMOVE);
#endif
//     AddItem("/obj/church_teleporter",REFRESH_HOME);

    time_from_reset = time();
    reset();
}

init() {
  ::init();
  add_action("say","'",1);
}

show_lamp() { return lamp_is_lit ? "The lamp above the door is lit.\n" : ""; }


#if 0
/*
 * The next call of combine_free_list() is dependant of how long the free list
 * is. However, resorting the free list goes fast if the list is already
 * sorted.
 */
call_combine() {
  int tmp;
  tmp = time();
  list_length = combine_free_list();
  if (time() - tmp > 20)
    shout("Game driver shouts: Sorry for the delay. I had some garbage collection to do.\n");
  if (list_length < 1000) call_out("call_combine", 10);
  else if (list_length < 5000) call_out("call_combine", 30);
  else call_out("call_combine", 60 * 2);
}
#endif
