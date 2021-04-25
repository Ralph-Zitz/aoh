#include "path.h"
#include <moving.h>
#include <config.h>

inherit "/std/room";

private static int counter,open,small,large;

private int vanish() {
  tell_room(this_object(),
  "Suddenly the pedestals begin to move down. They vanish into the ground. "
"You hear a sharp klick.\n");
  small=large=2;
  return open=1;
}

public int open(string str) {
  object ob,ob2;

  if (!str)
    return (int)notify_fail("Open what?\n",NOTIFY_NOT_OBJ)&&0;
  if (str!="coffin")
    return (int)notify_fail("Do you see that here?\n",NOTIFY_NOT_OBJ)&&0;
  if (!open) return (int) notify_fail(
    "You're not able to lift the lid. It seems much too heavy.\n",
    NOTIFY_NOT_VALID)&&0;
  if (open==2) return (int) notify_fail(
    "The coffin already is open and empty.\n",
    NOTIFY_NOT_VALID) && 0;
  if (!this_player()) return 0;
  write(
"You manage to open it... Look... There's a skeleton in it... It raises...\n"
"A pirate skeleton attacks you! It has a staff in its hands.\n");
  show(this_player()->QueryName()+
      " opens the coffin. A pirate skeleton appears.\n",this_player());
  tell_room(this_object(),"You have the feeling that this is your last foe. "
"A small cache at the base of the coffin opens. You see a small chest in it.\n");
  open=2;
  ob=clone_object("/std/npc");
  ob->SetName("skeleton");
  ob->SetLevel(14);
  ob->SetHP(200);
  ob->SetAlign(-300);
  ob->SetIds(({"pirate","skeleton"}));
  ob->SetAds(({"pirate","a"}));
  ob->SetRace("undead");
  ob->SetShort("a pirate skeleton");
  ob->SetLong("You see an evil looking undead made of bones.\n");
  ob->SetHands(({({"its left hand",0,14}),({"its staff",0,18})}));
  ob->SetAC(7);
  ob->SetAggressive(1);
  ob->move(this_object(),M_SILENT);
  ob2=clone_object(PIROBJ("staff1"));
  ob2->move(ob,M_SILENT);
  ob->wield(ob2);
  ob=clone_object(PIROBJ("smchest"));
  ob->move(this_object(),M_SILENT);
  ob2=clone_object(PIROBJ("paper"));
  ob2->move(ob,M_SILENT);
  ob2=clone_object(PIROBJ("chal"));
  ob2->move(ob,M_SILENT);
  return 1;
}

public int search(string str) {
  if (!str)
    return (int)notify_fail("Search what?\n",NOTIFY_NOT_OBJ)&&0;
  if (!this_object()) return 0;
  if (present("skeleton",this_object())) {
    write("The skeleton hinders you.\n");
    return 1;
  }
  if (str!="coffin") return (int) notify_fail(
    "You search and search... but you see nothing of any interest.\n",
    NOTIFY_NOT_VALID) && 0;
  if (open!=2) return (int) notify_fail(
    "A coffin.\n",NOTIFY_NOT_VALID) && 0;
  write(
"As you search the coffin, you discover a small ankh-shaped opening in it.\n");
  return 1;
}

public int insert(string str) {
  string what;
  object ob;

  if (!this_player()) return 0;
  if (!str) return (int)notify_fail("Insert what?\n",NOTIFY_NOT_OBJ)&&0;
  if ((sscanf(str,"%s into opening",what)!=1) &&
    (sscanf(str,"%s in opening",what)!=1) &&
    (sscanf(str,"%s into the opening",what)!=1)) return (int)
    notify_fail("Insert what into the opening?\n",
                NOTIFY_NOT_VALID) && 0;
  if ((what!="rusty ankh") && (what!="ankh")) return (int)
    notify_fail("That won't fit.\n",NOTIFY_NOT_VALID) && 0;
  if (present("skeleton",this_object())) return (int)
    notify_fail("This bloody undead pirate won't like that.\n",
                NOTIFY_NOT_VALID) && 0;
  if (open!=2) return (int) notify_fail(
    "You don't see an opening.\n",
    NOTIFY_NOT_VALID) && 0;
  if (!ob=present("ankh",this_player())) return (int)
    notify_fail("You don't have an ankh.\n",
                NOTIFY_NOT_VALID) && 0;
  if (lower_case(ob->QueryShort())!="a rusty ankh") return (int)
    notify_fail("That ankh doesn't fit.\n",
                NOTIFY_NOT_VALID) && 0;
  write("You insert the ankh.\nCLICK!\nA staff appears.\nA ring appears.\n");
  show(this_player()->QueryName()+" inserts an ankh into an opening. "
"Some objects appear.\n",this_player());
  ob->remove();
  if (ob) destruct(ob);
  ob=clone_object(PIROBJ("pirstaff2"));
  ob->move(this_object(),M_DROP);
  ob=clone_object(PIROBJ("sring"));
  ob->move(this_object(),M_DROP);
  return 1;
}

public int enter(string str) {
  if (!str) return (int) notify_fail("Enter what?\n",NOTIFY_NOT_OBJ)&&0;
  if (!this_player()) return 0;
  if (str=="coffin") return (int) notify_fail(
    "You really don't want to enter this place. Forget about that.\n",
    NOTIFY_NOT_VALID) && 0;
  if ((str!="field")&&(str!="magic field")) return (int)
    notify_fail("You see no sense in that.\n",
    NOTIFY_NOT_VALID) && 0;
  write("You enter the field.\n");
  this_player()->move(CITY("church"),M_GO,"into the shimmering field");
  return 1;
}

public int get_me(string str) {
  string what,where;

  if (!str) return 0;
  if (sscanf(str,"%s from %s pedestal",what,where)!=2) return (int)
    notify_fail("Get what from what?\n",NOTIFY_NOT_VALID) && 0;
  return (int) notify_fail(
    "You can't do that due to a strong magic force which fasten it to the\n"+where+
" pedestal.\n",NOTIFY_NOT_VALID)&&0;
}

public int put_me(string str) {
  object ob;
  string what,where;
  if (!this_player()) return 0;
  if (!str) return 0;
  if ((str=="ankh in opening") || (str=="ankh into opening"))
    insert("ankh into opening");
  if ((sscanf(str,"%s onto %s",what,where)!=2)
    && (sscanf(str,"%s on %s",what,where)!=2)) return (int)
      notify_fail("Put what onto what?\n",
                  NOTIFY_NOT_OBJ) && 0;
  if ((what!="bowl") && (what!="figurine")) return (int)
    notify_fail("You see no sense in this.\n",
                NOTIFY_NOT_VALID) && 0;
  if (member(({"pedestal","small pedestal","large pedestal"}),where)==-1)
    return (int) notify_fail("You overthink this idea.\n",
                             NOTIFY_NOT_VALID) && 0;
  if (where=="pedestal") return (int) notify_fail(
    "Onto which pedestal? There are two of them. A large one and a small one.\n",
    NOTIFY_NOT_VALID) && 0;
  if (what=="bowl") {
    if (!ob=present("bowl",this_player())) return (int) notify_fail(
      "You don't have that.\n",
      NOTIFY_NOT_VALID) && 0;
    if (where=="small pedestal") return (int) notify_fail(
      "The bowl would fall to the ground and break.\n",
      NOTIFY_NOT_VALID) && 0;
    if (where=="large pedestal") {
      write("You place the bowl onto the large pedestal.\n");
      show(this_player()->QueryName()+
	" places a bowl onto the large pedestal.\n",this_player());
      ob->remove();
      if (ob) destruct(ob);
      large=1;
      if ((large==1) && (small==1)) vanish();
      }
    }
  if (what=="figurine") {
    if (!ob=(present("pirfigur",this_player()))) return (int)
      notify_fail("You don't have a fitting figurine.\n",
                  NOTIFY_NOT_VALID) && 0;
    if (where=="large pedestal") return (int) notify_fail(
      "A strong force prevents you from doing so.\n") && 0;
    write("You place the figurine onto the small pedestal.\n");
    show(this_player()->QueryName()+
      " puts a figurine of Thor onto the small pedestal.\n",this_player());
    ob->remove();
    if (ob) destruct(ob);
    small=1;
    if ((large==1) && (small==1)) vanish();
    }
  return 1;
}

public string coffin() {
  if (open!=2) return "The coffin is closed.\n";
  return "You take a short look at the coffin. You see nothing special. "
"Perhaps you should take a closer look?\n";
}

public string lpedestal() {
  if (!large)
    return "It's a large empty pedestal with many unreadable symbols on it.\n";
  if (large=1) return "A large pedestal with a bowl on it.\n";
  return "Both pedestals are gone.\n";
}

public string spedestal() {
  if (!small)
    return "It's a small, empty pedestal with many unreadable symbols on it.\n";
  if (small=1) return "A small pedestal with a figurine on it.\n";
  return "Both pedestals are gone.\n";
}

helpme(string str) {
  if (!str || str!="coffin") return 0;
  return search(str);
}

public void create() {
  (::create());
  SetIntShort("a chamber");
  SetIntLong(
"You're in a large chamber. The walls are covered with pictures of the old "
"ancient gods and precious bowls and cups. A large coffin stands in the middle "
"of the room. A small and a large pedestal are flanking the sides of the "
"coffin. In a corner of the room wavers a magic field.\n");
  AddDetail("large pedestal",#'lpedestal);
  AddDetail(({"field","magic field"}),
    "Seems to be one of these standard teleporting devices.\n");
  AddDetail("coffin",#'coffin);
  AddDetail(({"wall","walls"}),"Just normal walls.\n");
  AddDetail("small pedestal",#'spedestal);
  AddDetail("pedestal","Which one? The large or the small one?\n");
  AddDetail(({"god","gods"}),
"You see some gods carrying a bowl and a figurine.\n");
  AddDetail(({"bowl","bowls"}),
"One bowl is carried to a large pedestal and is placed on it.\n");
  AddDetail(({"figurine","figurines"}),
"You see some gods placing a figurine onto a small pedestal.\n");
  AddDetail(({"picture","pictures"}),
"You see some gods on them which seem to do something.\n");
  AddDetail(({"cup","cups"}),
"You can see absolutely nothing remarkable.\n");
  SetIndoors(1);
  SetBright(0);
  AddRoomCmd("insert",#'insert);
  AddRoomCmd("enter",#'enter);
  AddRoomCmd(({"get","take"}),#'get_me);
  AddRoomCmd(({"put","place"}),#'put_me);
  AddRoomCmd("open",#'open);
  AddRoomCmd("search",#'search);
  AddRoomCmd(({"exa","examine"}),#'helpme);
  reset();
}

public void reset() {
  if (!counter) {
  open=large=small=0;
  if (!this_object()) return 0;
  tell_room(this_object(),
    "The room wavers and two pedestals move out of the floor.\n");
  counter=counter+1;
  ::reset();
  }
  else {
    counter=0;
  (::reset());
  }
}
