/* The place to build a home */

#include <moving.h>
#include "/p/houses/common/sys/prices.h"

inherit "/std/room";
inherit "/p/houses/common/room/sell";

create()
{
  ::create();
  SetIntShort("an empty field");
  SetIntLong("You're standing at an empty field.\n\
Maybe it will be built something here in the near future.\n");
  AddRoomCmd("build","build");  
}

build(string str)
{
  int    money;
  string exit;
  mixed  tmp;

  if(getuid(this_player()) != QueryOwner()) {
    notify_fail("You aren't allowed to build here. It isn't your property.\n");
    return 0;
  }
  if(str &&str != "here" && str != "house") {
    notify_fail("You can't build a " + str + " here.\n");
    return 0;
  }
  if(this_interactive() != this_player()) {
    tell_object(this_interactive(),
		"Hey don't try to force anybody to build something!\n");
    notify_fail("Attention! " + capitalize(getuid(this_interactive())) +
		" tried to abuse your rights.\n");
    return 0;
  }
  if((money = "/lib/money"->QuerySomeonesValue(this_player())) < HOUSE_PRICE) {
    if(file_size(object_name()[0..<4] + "dept") != -1) {
      notify_fail("You can't build here. Please pay back your dept first!\n");
      return 0;
    }
    if(money < 1001) {
      write_file(object_name()[0..<4] + "dept",to_string(HOUSE_PRICE));
      write("You get a dept of " +
	    "/obj/money_demon"->QueryValueString(HOUSE_PRICE) +
	    " for to build your house.\n");
    } else {
      money -= 1000;
      write_file(object_name()[0..<4] + "dept",to_string(HOUSE_PRICE - money));
      write("You get a dept of " +
	    "/obj/money_demon"->QueryValueString(HOUSE_PRICE - money) +
	    " for to build your house\nand pay " +
	    "/obj/money_demon"->QueryValueString(money) + " immediately.\n");
      "/lib/money"->PayValue(this_player(),money);
    }
  } else {
    write("You pay " + "/obj/money_demon"->QueryValueString(HOUSE_PRICE) +
	  " for your house.\n");
    "/lib/money"->PayValue(this_player(),HOUSE_PRICE);
  }
  say(capitalize(this_player()->QueryName()) + " builds a house.\n");
  rm(object_name() + ".c");
  write_file(object_name() + ".c",
	     "/* The house of " + capitalize(getuid(this_player())) + " */\n\n\
inherit \"/p/houses/common/room/home\";\n\n\config() {\n\
  SetIntShort(\"the house of " + capitalize(getuid(this_player())) + "\");\n\
  SetIntLong(\"You are standing in an empty room in the home of " +
	     capitalize(getuid(this_player())) + ".\\n\");\n\
  AddExit(\"" + (exit = m_indices(tmp = QueryExitsDest())[0]) + "\",\"" +
	     tmp[exit] + "\");\n\
  SetEnter(\"" + opposite(exit) + "\");\n}\n");
  "/p/houses/common/room/update"->update_me(this_object());
  return 1;
}
    
opposite(string dest)
{
  switch(dest) {
  case "north": return "south";
  case "south": return "north";
  case "west": return "east";
  case "east": return "west";
  case "northwest": return "southeast";
  case "southwest": return "northeast";
  case "northeast": return "southwest";
  case "southeast": return "northwest";
  default: return 0;
  }
}

init()
{
  room::init();
  sell::init();
}

QueryOwner() { return explode(object_name()||"/","/")[<2]; }
