/* The Nightfall City jewelry
   by Cadra 11-04-1994
   please do not copy the idea... we need some special things for
   our capital.
*/

#include "path.h"
#include <moving.h>
#include <combat.h>

inherit "/std/shop";

private static int line;
private static string *message;
object ob;

break_input() {
  write("You canceled the order.\n");
}


show_help() {
  write(
"~q :	cancel order\n"
"~h :	show this help\n"
"~d :	delete last line\n"
"~r :	redisplay what you've written so far\n\n"
"Use ** or . to end the input.\n");
}


del_line() {
  if (!line) {return;}
  line--;
  message = message[0..line-1];
}


private displaytext() {
  int i;
  for (i=0;i<sizeof(message);i++) {
    write(to_string(i+1)+". "+message[i]+"\n");
    }
}


end_input() {
 string long, *paying;
 int length;
 write("You ended your message, it will be stored.\n");
 long = implode(message-({0}),"\n") + "\n";
 ob->SetLong(long);
 length = strlen(implode(explode(long,"\n"),"")) * 10 +4000;
 ob->SetValue(length);
 if (QuerySomeonesValue(this_player()) < length) {
  write("Sorry you haven't enough money!\n");
  ob->remove();
  return 1;
  }
 paying = PayValue2 (this_player(), length);
 if (length)
  write("You pay "+paying[0] +
        (paying[1] ? " and receive "+paying[1] : "")+".\n");
 else write("You pay nothing.\n");
 if ((ob->move(this_player(),M_GET)) != ME_OK) {
   write(
"The amulet is too heavy for you. The smith drops it on his counter.\n");
   ob->move(this_object(),M_DROP);
   }
}


input_loop(string str) {
  int go_on;
  go_on = 1;
  if (str[0]=='~') {
    switch(str[1]) {
      case 'q': return break_input(); break;
      case 'h': show_help(); go_on = 0; break;
      case 'd': del_line(); go_on = 0; break;
      case 'r': displaytext(); go_on = 0; break;
      }
    }
  if (str=="."||str=="**") {return end_input();}
  if (go_on) {
    message+=({str});
    line+=1;
    }
  write(to_string(line+1)+"]");
  input_to("input_loop"); 
}


input() {
  line = 0;
  message = ({});
  write(to_string(line+1)+"]");
  input_to("input_loop");
}


cloakshort(string str) {
  if ((str) && (str!="")) {
    ob->SetShort(str); 
    }
  write("The smith wants to have a long description of your amulet.\n");
  write("Now you might give a detailed description of your wanted amulet. "
"End it with '.' or '**' or help with '~h':\n");
     input();
}


do_buy(string str) {
  if (!this_player()) {return;}
  if (!str) {return;}
  if ((str == "amulet") || (str == "amulets")) {
    ob = clone_object("/std/armour");
    ob->SetAC(0);
    ob->SetArmourType(AT_AMULET);
    ob->SetShort("a golden amulet");
    ob->SetLong(
"This amulet is made by the most expensive goldsmith of Nightfall.\n");
    ob->SetWeight(200);
    ob->SetValue(4000);
    ob->SetIds( ({"chain","armour","amulet"}) );
    ob->SetAds( ({"a","very","special","expensive","golden","silver"}) );
    write(
"The smith needs a SHORT description of the amulet you wish to have.\n");
    write("Short: ");
    input_to("cloakshort");
    return 1;
    }
  return notify_fail("We do not make such strange things.\n");
}
 
create() {
  ::create();
  SetIntShort("A jewelry");
  SetIndoors(1);
  SetBright(50);
  SetIntLong(
"You see the master himself working in one corner of the shop surrounded by "
"his apprentices. Everyone is busy working on beautiful and powerful amulets. "
"These amulets will be sold to brave adventurers who gave them in work. "
"You may create your own amulet here, just order an amulet.\n");
   SetSignDesc(
"You may ORDER a very specific amulet here. After you ordered the goldsmith will "
"ask you for a 1line Shortdescription of your own amulet. After that he "
"wants to know a Longdescription of your amulet. The longer the longdescription "
"will be the more expensive the amulet will be. The amulet won't be autoloading, "
"but you may sell it in the general shops all over Nightfall.\n");
  AddExit ("south","./sunalley1");
  SetSell(0);
  SetAppraise(0);
  SetList(0);
  SetIdentify(0);
  RemoveDustbin();
  SetMinCash(1);
  AddRoomCmd("order",#'do_buy);
}

