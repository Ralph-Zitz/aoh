/* Nightfall city tailor shop
   Cadra 11-04-1994
   please do not copy the idea... we want to have something
   special for Nightfall City that is nowhere else
*/

#include "path.h"
#include <moving.h>
#include <combat.h>

inherit "/std/shop";

private static int line;
private static string *message;
object ob;
 
create() {
  ::create();
  SetIntShort("The famous Nightfall Tailor Shop");
  SetIndoors(1);
  SetBright(50);
  SetIntLong(
"You see some tailors busy at work. They sew the most beautiful cloaks you "
"ever have seen. You may give them order to sew a special cloak for you, too, "
"just order a cloak and see what is to happen. But be warned: it is expensive.\n");
  AddDetail(({"tailor shop","shop","nightfall tailor shop"}),#'IntLong);
  SetSignDesc(
"The sign reads:\n"
"You may ORDER a special cloak here. After you ordered the tailor will "
"ask you for a oneline Shortdescription of the cloak, then for a Longdescription "
"that can be as long as you want, but the longer it is the more "
"expensive the cloak will be. The cloak won't be autoloading, but you may "
"sell it in a shop.\n");
  AddExit ("northwest","./vill_road1");
  SetSell(0);
  SetAppraise(0);
  SetList(0);
  SetIdentify(0);
  RemoveDustbin();
  SetMinCash(1);
  AddRoomCmd("order","do_buy");
}


break_input() {
  write("You canceled the order.\n");
}


show_help() {
  write(
"~q :	cancel order\n"
"~h :	show this help\n"
"~d :	delete last line\n"
"~r :	redisplays what you've written so far\n\n"
"Use ** or . to end the input.\n");
}


del_line() {
  if (!line) {return;}
  line--;
  message = message[0..line-1];
}


private display_text() {
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
 length = strlen(implode(explode(long,"\n"),"")) * 10 +2000;
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
"The cloak is too heavy for you. The tailor drops it on his counter.\n");
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
      case 'r': display_text(); go_on = 0; break;
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
  write("The tailor wants to have a long description of your cloak.\n");
  write("Now you might give a detailed description of your wanted cloak. "
"End it with '.' or '**' or help with '~h':\n");
     input();
}


do_buy(string str) {
  if (!this_player()) {return;}
  if (!str) {return;}
  if ((str == "cloak") || (str == "clothes")) {
    ob = clone_object("/std/armour");
    ob->SetAC(1);
    ob->SetArmourType(AT_CLOAK);
    ob->SetShort("a special cloak");
    ob->SetLong(
"This cloak is made by the most expensive tailor of Nightfall.\n");
    ob->SetWeight(1000);
    ob->SetValue(2000);
    ob->SetIds( ({"cloak","armour","clothes"}) );
    ob->SetAds( ({"a","very","special","expensive","velvet","satin","silky"}) );
    write(
"The tailor needs a SHORT description of the cloak you wish to have.\n");
    write("Short: ");
    input_to("cloakshort");
    return 1;
    }
  return notify_fail("We do not sew such strange things.\n");
}
