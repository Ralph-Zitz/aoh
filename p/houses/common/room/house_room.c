/* A room for the makler office to sell */

#include <rooms.h>
#include <properties.h>
#include <moving.h>
#include <secure/wizlevels.h>
#include "/p/houses/common/sys/prices.h"

inherit "/std/room";

string owner, *coowners, *allowed, *privat;

static string  homedir;
static int    *location;
static string *dirs, *edittext;

create()
{
  string *dummy;
  string file;
  string *exits;
  string *items;
  int i;

  if(member((file = object_name()), '#') != -1) return 1;
  if(file == "/p/houses/common/room/house_room") return 1;
  ::create();
  AddRoomCmd(({"dig","build"}),"dig");
  AddRoomCmd("name","name");
  AddRoomCmd("describe","describe_me");
  AddRoomCmd("set","set_privacy");
  AddRoomCmd("private","is_private");
  AddRoomCmd("close","close_exit");
  AddRoomCmd("move","move_furniture");
  dummy = explode(file,"/");
  owner = dummy[<2];
  homedir = "/" + implode(dummy[0..<2],"/");
  if(file_size(homedir + "/config.o") != -1) {
    restore_object(homedir + "/config");
/*
    rm(homedir + "/allowed");
    rm(homedir + "/coowners");
 */
  }
  else if(file_size(homedir + "/coowners") != -1) {
    coowners = explode(read_file((homedir + "/coowners")) || "","\n")-({""});
    if(!coowners) coowners = ({});
  }
  else coowners = ({});
  save_object(homedir + "/config");
  location = ({dummy[<1][0]-48,dummy[<1][1]-48,dummy[<1][2]-48});
  dirs = ({"north","south","west","east","up","down","northwest","northeast",
             "southwest","southeast"});
}

SetLocation(int *loc)
{
  if(!loc || !sizeof(loc)) return location;
  return location = (loc + ({0,0,0}))[0..2];
}

QueryLocation() { return location; }

SetOwner(string new_owner) { return owner = new_owner; }

QueryOwner() { return owner; }

QueryIsOwner(string str)
{
  return str == QueryOwner() || member(coowners, str) != -1;
}

SetHomedir(string dir) { return homedir = dir; }

QueryHomedir() { return homedir; }

varargs mixed *AddItem(mixed file, int refr, mixed xarg1, mixed xarg2,
		       mixed xarg3)
{
  object clone;

  if(refr != REFRESH_NONE) return ::AddItem(file,refr,xarg1,xarg2,xarg3);
  SetItems((QueryItems()||({})) +
	   ({({file,REFRESH_NONE,clone = clone_object(file)})}));
  clone->move(this_object(),M_TELEPORT);
  return QueryItems()[<1];
}

QueryIntLight()
{
  return ::QueryIntLight() - ((object_name()[<1] == '0')?100:0);
}

security()
{
  if(this_player() != this_interactive()) {
    notify_fail(capitalize(this_interactive()->QueryName()) +
      "tries to build something in your house.\n");
    return 0;
  }
  restore_object(homedir + "/config");
  if(!QueryIsOwner(getuid(this_player()))) {
    notify_fail("You aren't allowed to build anything in this house.\n\
You are not the owner!\n");
    return 0;
  }
  return 1;
}

full_direction_string(string str)
{
  switch(str) {
  case "n": return "north";
  case "s": return "south";
  case "w": return "west";
  case "e": return "east";
  case "u": return "up";
  case "d": return "down";
  case "nw": return "northwest";
  case "ne": return "northeast";
  case "sw": return "southwest";
  case "se": return "southeast";
  default: return str;
  }
}

opposite(string dir)
{
  switch(dir) {
  case "north": return "south";
  case "south": return "north";
  case "west": return "east";
  case "east": return "west";
  case "up": return "down";
  case "down": return "up";
  case "northwest": return "southeast";
  case "northeast": return "southwest";
  case "southwest": return "northeast";
  case "southeast": return "northwest";
  }
}

dig(string str)
{
  if(!security()) return 0;
  if(!str) {
    notify_fail("You can only dig in a direction.\nExample: dig north\n");
    return 0;
  }
  if(file_size(homedir + "/dept") != -1) {
    notify_fail("You should first pay back your dept.\n");
    return 0;
  }
  str = full_direction_string(str);
  if(member(dirs, str) == -1) {
    notify_fail("Please use one of the directions: north, south, west, east,\n\
up, down, northwest, northeast, southwest, southeast!\n");
    return 0;
  }
  if(QueryExitsDest()[str]) {
    notify_fail("This room already has an exit " + str + ".\n");
    return 0;
  }
  if(!create_exit(str)) {
    notify_fail("You can't build further " + str + ".\n");
    return 0;
  }
  return 1;
}

get_new_filename(string direction)
{
  int i, new_location;

  new_location = ({location[0],location[1],location[2]});
  switch(direction) {
  case "north": --new_location[0]; break;
  case "south": ++new_location[0]; break;
  case "west": --new_location[1]; break;
  case "east": ++new_location[1]; break;
  case "up": ++new_location[2]; break;
  case "down": --new_location[2]; break;
  case "northwest": --new_location[0]; --new_location[1]; break;
  case "northeast": --new_location[0]; ++new_location[1]; break;
  case "southwest": ++new_location[0]; --new_location[1]; break;
  case "southeast": ++new_location[0]; ++new_location[1]; break;
  default: return 0;
  }
  for(i = 3; i--; )
    if(new_location[i] > 2 || new_location[i] < 0) return 0;
  return homedir + "/" + new_location[0] + new_location[1] +
    new_location[2];
}

create_exit(string dir)
{
  string  filename;
  int     i, money;
  object *inv;
  object  ob;

  if(!(filename = get_new_filename(dir))) return 0;
  if(file_size(filename + ".c") == -1) {
    write("You have to pay " + ROOM_PRICE + " for the new room.\n");
    money = "/lib/money"->QuerySomeonesValue(this_player());
    if(money > ROOM_PRICE) {
      "/lib/money"->PayValue(this_player(),ROOM_PRICE);
      write("You pay " + "/obj/money_demon"->QueryValueString(ROOM_PRICE) +
            ".\n");
    } else {
      if(money < 1001) {
        write_file(homedir + "/dept",to_string(ROOM_PRICE));
        write("You get a dept of " + ROOM_PRICE + " copper coins.\n");
      } else {
        write_file(homedir + "/dept",to_string(ROOM_PRICE-money+1000));
        write("You pay " +(money-1000) + " copper coins and get a dept of " +
              (ROOM_PRICE-money+1000) + " copper coins.\n");
        "/lib/money"->PayValue(this_player(),money - 1000);
      }
    }
    write_file(filename + ".c","/* House of " + capitalize(owner) + " */\n\n\
inherit \"/p/houses/common/room/house_room\";\n");
    filename->SetIntShort("a room in the house of " + owner);
    filename->SetIntLong("You are standing in an empty room.\n");
    if(filename[<1] == '0') filename->SetBright(0);
  }
  filename->AddExit(opposite(dir), "./" + location[0] + location[1] +
                    location[2]);
  filename->update_file();
  ob = find_object(filename);
  inv = all_inventory(ob);
  for(i = sizeof(inv); i--; )
    if(living(inv[i])) inv[i]->move(ob);
  AddExit(dir,filename);
  update_file();
  inv = all_inventory(this_object());
  for(i = sizeof(inv); i--; )
    if(living(inv[i])) inv[i]->move(this_object());
  return 1;
}

close_exit(string str)
{
  string filename;
  object other_room, *inv;
  int    i;

  if(!security()) return 0;
  if(!str) {
    notify_fail("You can close a door in a specified direction.\n\
Example: close north\n");
    return 0;
  }
  str = full_direction_string(str);
  if(member(dirs, str) == -1) {
    notify_fail("Please use one of the directions: " +
                implode(m_indices(QueryExitsDest()),", ")[0..<3] + ".\n");
    return 0;
  }
  if(!QueryExitsDest()[str]) {
    notify_fail("This room has no exit to " + str + ".\n");
    return 0;
  }
  if(!(filename = get_new_filename(str))) {
    notify_fail("There is no room in the " + str + ".\n");
    return 0;
  }
  if(!(other_room = load_object(filename))) {
    notify_fail("ERROR: Can't load room to the " + str + ".\n");
    return 0;
  }
  if(sizeof(other_room->QueryExits()) <= 1) {
    notify_fail("You shouldn't close this exit, it's the only one to the other\
 room.\n");
    return 0;
  }
  other_room->RemoveExit(opposite(str));
  other_room->update_file();
  RemoveExit(str);
  update_file();
  inv = all_inventory(other_room);
  for(i = sizeof(inv); i--; )
    if(living(inv[i])) inv[i]->move(other_room);
  inv = all_inventory(this_object());
  for(i = sizeof(inv); i--; )
    if(living(inv[i])) inv[i]->move(this_object());
  write("You close the exit to the " + str + ".\n");
  say(capitalize(this_player()->QueryShort()?
                 this_player()->QueryName():"Someone") +
      " closed the exit to the " + str + ".\n");
  return 1;
}

help()
{
  return
"You can give the following commands:\n\
~h ...... show this text              ~d ...... delete the last line\n\
. / ** .. save text and end           ~r ...... show the actual text\n";
}

describe_me(string str)
{
  if(!security()) return 0;
  if(str && (str != "room" || str != "here")) {
    notify_fail("What do you want to describe?\n");
    return 0;
  }
  edittext = explode(QueryIntLong()[0..<2],"\n");
  write("----- Actual description: -------------------------------------------\
----------\n" +
        QueryIntLong() +
        "---------------------------------------------------------------------\
----------\n\
Please edit the description of the room (help with \"~h\")!\n]");
  input_to("gettext");
  return 1;
}

name(string str)
{
  if(!security()) return 0;
  if(str && (str != "room" || str != "here")) {
    notify_fail("What do you want to describe?\n");
    return 0;
  }
  write("----- Actual name: --------------------------------------------------\
----------\n" +
        QueryIntShort() +
        "\n-------------------------------------------------------------------\
------------\n\
Please edit the name of the room (help with \"~h\")!\n]");
  input_to("getstring");
  return 1;
}

store_long(string *text)
{
  string new_long;
  string file;

  if(!sizeof(text)) {
    write("No changes are done.\n");
    return;
  }
  write("You changed the description of the room.\n");
  say(capitalize(QueryOwner()) + " changed the description of the room.\n");
  SetIntLong(new_long = implode(text,"\n") + "\n");
  update_file();
}

getstring(string str)
{
  string file;

  switch (str) {
  case "": case ".": case "**":
    write("You didn't change the name of the room.\n");
    return;
  case "~h":
    write(help() + "]");
    input_to("getstring");
    return;
  case "~r":
    write(QueryIntShort() + "\n]");
    input_to("getstring");
    return;
  case "~d":
    write("Name is deleted. Please enter a new one.\n]");
    input_to("getstring");
    return;
  }
  say(capitalize(QueryOwner()) + " gave the room a new name.\n");
  write("You gave a new name to the room.\n");
  SetIntShort(str);
  update_file();
}

gettext(string str)
{
  string file;

  switch (str) {
  case "~r":
    write(implode(edittext,"\n") + "\n]");
    input_to("gettext");
    return;
  case "~d":
    if (!sizeof(edittext)) {
      write("No lines in the buffer.\n]");
      input_to("gettext");
      return;
    }
    edittext = edittext[0..<2];
    write("]");
    input_to("gettext");
    return;
  case ".":
  case "**":
    return store_long(edittext);
  case "~h":
    write(help() + "]");
    input_to("gettext");
    return;
  }
  if (strlen(str) > 79) {
    write("This line ist too long. Please enter again!\n]");
    input_to("gettext");
    return;
  }
  if (!str)
    edittext += ({""});
  else
    edittext += ({str});
  write("]");
  input_to("gettext");
}

update_file()
{
  int    i;
  mapping map;
  mixed  dummy;
  string out;
  object door;

  rm(QueryHomedir() + "/" + location[0] + location[1] + location[2] + ".c");
  out = "/* A room in the house of " + QueryOwner() + " */\n\n\
#include <rooms.h>\ninherit \"/p/houses/common/room/house_room\";\n\ncreate() {\n\
  ::create();\n  SetIntShort(\"" +
    "/lib/string"->lit_string(QueryIntShort()) + "\");\n  SetIntLong(\"" +
    "/lib/string"->lit_string(QueryIntLong()) +"\");\n";
  map = QueryExitsDest();
  dummy = m_indices(map);
  for(i = sizeof(dummy); i--; )
    out += "  AddExit(\"" + dummy[i] + "\",\"" + map[dummy[i]] + "\");\n";
  if(dummy = QueryItems())
    for(i = sizeof(dummy); i--; )
      out += "  AddItem(\"" + dummy[i][RITEM_FILE] + "\",REFRESH_NONE);\n";
  if(map = QueryDoors()) {
    dummy = m_indices(map);
    for(i = sizeof(dummy); i--; ) {
      mixed kids;
      string room;

      door = dummy[i];
      room = object_name(door->QueryDoorRoom());
      kids = door->QueryKeyIds();
      if (kids)
        kids = kids[0];
      out += ("  AddDoor(\"" + door->QueryDirection() + "\",\"" +
              (room[0..1]=="//"?room[1..]:room[0..]) +
              "\",\"" + "/lib/string"->lit_string(door->QueryDoorCode()) + "\",\"" +
              "/lib/string"->lit_string(door->QueryLong()) +
              "\"," + (kids?"\"" + kids + "\"":0) +
              ",LOCK_CLOSED);\n");
    }
  }
  write_file(QueryHomedir() + "/" + location[0] + location[1] +
             location[2] + ".c",out + "}\n");
}

set_privacy(string str)
{
  if(!security()) {
    notify_fail("Don't try to disturb the privacy!\n");
    return 0;
  }
  if(!str) {
    notify_fail("Set this room to 'private': set private\n\
Set this room to 'public':  set public.\n");
    return 0;
  }
  str = implode(explode(implode(explode(str,"room "),""),"here "),"");
  restore_object(homedir + "/config");
  if(str == "private") {
    if(!privat) privat = ({object_name()[<3..]});
    else {
      privat -= ({object_name()[<3..]});
      privat += ({object_name()[<3..]});
    }
    save_object(homedir + "/config");
    write("You set this room to 'private'.\n");
    return 1;
  }
  if(str == "public") {
    if(privat) privat -= ({object_name()[<3..]});
    save_object(homedir + "/config");
    write("You set this room to 'public'.\n");
    return 1;
  }
    notify_fail("Set this room to 'private': set privat\n\
Set this room to 'public':  set public.\n");
  return 0;
}

QueryPrivate()
{
  return privat && member(privat, object_name()[<3..]) != -1;
}

is_private(string str)
{
  if(str && str != "here" && str != "room") {
    notify_fail("Check the privacy mode using 'private room'.\n");
    return 0;
  }
  if(QueryPrivate())
    write("This is a private room.\n");
  else
    write("This room is set to public.\n");
  return 1;
}

allow_enter(int method, mixed extra)
{
  if(method != M_GO && method != M_TELEPORT)
    return ::allow_enter(method, extra);
  restore_object(homedir + "/config");
  if(!QueryPrivate())
    return ::allow_enter(method, extra);
  if(QueryIsOwner(getuid(this_player()||previous_object())) ||
     IS_WIZARD(this_player()||previous_object()))
    return ::allow_enter(method, extra);
  tell_object(this_player()||previous_object(),
              "This room is private. You can't enter it!\n");
  return 0;
}
/*
move_furniture(string str)
{
  object victim;
  string direction, obj;

  if(!str) {
    notify_fail("What do you want to move and whereto?\n");
    return 0;
  }
  
  if(!(victim = Locate(obj,1))) {
    notify_fail("There is no " + str + " here.\n");
    return 0;
  }
*/
