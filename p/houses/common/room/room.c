/* A room for the makler office to sell */

#include <rooms.h>
#include <properties.h>
#include <moving.h>
#include <secure/wizlevels.h>
#include "/p/houses/common/sys/prices.h"

#define PL this_player()

inherit "/std/room";

int security();
void update_file();
int create_exit(string dir);

string owner, *coowners, *allowed, *privat;

nosave string  homedir;
nosave int    *location;
nosave string *dirs, *edittext;

string detail;

int check_room () {
  mapping map;
  mixed  dummy;
  string out;
  int i;
  if(!security()) return 0;
  // first the exits:
  out="";
  map = QueryExitsDest();
  dummy = m_indices(map);
  for(i = sizeof(dummy); i--; ) {
    if (QueryExits()[dummy[i],3] == 1)  // hidden
      out += " " + dummy[i];
    }
  if (sizeof(out) != 0)
    write("Hidden exits:"+out+".\n");
  else
    write("No hidden exits.\n");
  // next the details:
  out="";
  map = QueryDetails();
  if (sizeof(map)==0) {
    write("No details.\n");
  }
  else {
    dummy = m_indices(map);
    for(i = sizeof(dummy); i--; ) {
      out += "\n     " + dummy[i];
    }
    write("Details:"+out+".\n");
  }
  return 1;
}

varargs void create()
{
  string *dummy;
  string file;

  if(member((file = object_name()), '#') != -1) return;
  if(file == "/p/houses/common/room/room") return;
  ::create();
  AddRoomCmd(({"dig","build"}),"dig");
  AddRoomCmd("name","name");
  AddRoomCmd("describe","describe_me");
  AddRoomCmd("set","set_privacy");
  AddRoomCmd("private","is_private");
  AddRoomCmd("close","close_exit");
//  AddRoomCmd("move","move_furniture");

  // new by Magictcs
  AddRoomCmd("hide","hide_exit");             // hide exit
  AddRoomCmd("unhide","unhide_exit");         // unhide exit
  AddRoomCmd("check","check_room");           // prints hiddenexit & details
  AddRoomCmd("detail","detail");              // creates details
  AddRoomCmd("smash","destruct");        // remove items
  AddRoomCmd("removedetail","remove_detail"); // remove details
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

int *SetLocation(int *loc)
{
  if(!loc || !sizeof(loc)) return location;
  return location = (loc + ({0,0,0}))[0..2];
}

int *QueryLocation() { return location; }

string SetOwner(string new_owner) { return owner = new_owner; }

string QueryOwner() { return owner; }

int QueryIsOwner(string str)
{
  return str == QueryOwner() || member(coowners,str) != -1;
}

string SetHomedir(string dir) { return homedir = dir; }

string QueryHomedir() { return homedir; }

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

varargs int QueryIntLight()
{
  return ::QueryIntLight() - ((object_name()[<1] == '0')?100:0);
}

int security()
{
  if(this_player() != this_interactive()) {
    notify_fail(capitalize(({string})this_interactive()->QueryName()) +
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

string full_direction_string(string str)
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
  return str;
}

string opposite(string dir)
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
  return 0;
}

// removes all (!) items matching to str
int destruct (string str) {
  mixed dummy;
  int i;
  if(!security()) return 0;
  if(!str) {
    notify_fail("You can only smash an item.\nExample: smash chair\n");
    return 0;
  }
  if(dummy = QueryItems())
    for(i = sizeof(dummy); i--; )
      if (({int})dummy[i][0]->id(str)) {
        write("You smash "+({string})dummy[i][0]->QueryShort()+" to pieces.\n");
        say(capitalize(({string})dummy[i][0]->QueryShort())+" is smashed to pieces.\n");
        dummy[i][2]->remove();
        RemoveItem(i);
        update_file();
      return 1;
      }
  notify_fail("You can only smash an item.\nExample: smash chair\n");
  return 0;
}

int dig(string str)
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

string get_new_filename(string direction)
{
  int i, *new_location;

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

  default: return 0;
  }
  for(i = 3; i--; )
    if(new_location[i] > 2 || new_location[i] < 0) return 0;
  return homedir + "/" + new_location[0] + new_location[1] +
    new_location[2];
}

int create_exit(string dir)
{
  string  filename;
  int     i, money;
  object *inv;
  object  ob;

  if(!(filename = get_new_filename(dir))) return 0;
  if(file_size(filename + ".c") == -1) {
    write("You have to pay " + ROOM_PRICE + " for the new room.\n");
    money = ({int})"/lib/money"->QuerySomeonesValue(this_player());
    if(money > ROOM_PRICE) {
      "/lib/money"->PayValue(this_player(),ROOM_PRICE);
      write("You pay " + ({string})"/obj/money_demon"->QueryValueString(ROOM_PRICE) +
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
inherit \"/p/houses/common/room/room\";\n");
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
  write("You opened an exit to the "+dir+".\n");
  return 1;
}

int close_exit(string str)
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
  if(sizeof(({int})other_room->QueryExits()) <= 1) {
    notify_fail("You shouldn't close this exit, it's the only one to the other\
 room.\n");
    return 0;
  }
  other_room->RemoveExit(({opposite(str)}));
  other_room->update_file();
  RemoveExit(({str}));
  update_file();
  inv = all_inventory(other_room);
  for(i = sizeof(inv); i--; )
    if(living(inv[i])) inv[i]->move(other_room);
  inv = all_inventory(this_object());
  for(i = sizeof(inv); i--; )
    if(living(inv[i])) inv[i]->move(this_object());
  write("You close the exit to the " + str + ".\n");
  say(capitalize(({string})this_player()->QueryShort()?
                 ({string})this_player()->QueryName():"Someone") +
      " closed the exit to the " + str + ".\n");
  return 1;
}

int hide_exit(string str)
{
  if(!security()) return 0;
  if(!str) {
    notify_fail("You can only hide a direction.\nExample: hide north\n");
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
  if(!QueryExitsDest()[str]) {
    notify_fail("This room has no exit to the " + str + ".\n");
    return 0;
  }
  HideExit(str,1);
  update_file();
  write("You hide the exit to the "+str+"\n");
  say(capitalize(({string})this_player()->QueryShort()?
                 ({string})this_player()->QueryName():"Someone") +
      " hides the exit to the " + str + ".\n");
  return 1;
}

int unhide_exit(string str)
{
  if(!security()) return 0;
  if(!str) {
    notify_fail("You can unhide a special direction.\n\Example: unhide north\n");
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
  if(!get_new_filename(str)) {
    notify_fail("There is no room in the " + str + ".\n");
    return 0;
  }
  HideExit(str,0);
  update_file();
  write("You unhide the exit to the " + str + ".\n");
  say(capitalize(({string})this_player()->QueryShort()?
                 ({string})this_player()->QueryName():"Someone") +
      " unhides the exit to the " + str + ".\n");
  return 1;
}

string help()
{
  return
"You can give the following commands:\n\
~h ...... show this text              ~d ...... delete the last line\n\
. / ** .. save text and end           ~r ...... show the actual text\n";
}

int detail(string unnormed)
{
  if(!security()) return 0;
  if(!unnormed) {
    notify_fail("Which detail do you want to describe?\n");
    return 0;
  }
  detail=norm_id(unnormed);                           // global set
  if (sizeof(QueryDetails())==0 ||
    member(m_indices(QueryDetails()),detail) < 0) {   // no detail
    edittext=0;
  write("--- Actual description for detail: "+detail+" ---\n" +
        "---------------------------------------------------------------------\
----------\n\
Please edit the description of detail(help with \"~h\")!\n]");
  }
  else {
    edittext = explode(QueryDetails()[detail][0..<2],"\n");
  write("--- Actual description for detail: "+detail+" ---\n" +
        QueryDetails()[detail]+
        "---------------------------------------------------------------------\
----------\n\
Please edit the description of detail(help with \"~h\")!\n]");
  }
  input_to("getdetailtext");
  return 1;
}


int remove_detail(string str) {
  mapping map;

  if(!security()) return 0;
  if(!str) {
    notify_fail("Which detail do you want to remove?\n");
    return 0;
  }
  map = QueryDetails();
  if (sizeof(map)==0) {
    write("No details are described in this room.\n");
    return 1;
  }
  else {
    if (map[str]) {
      RemoveDetail(str);
      write("You removed the detail "+str+
        " from the description of this room.\n");
      update_file();
    }
    else
      write("There is no detail "+str+".\n"
        "Please use 'check' to see your described details.\n");
  }
  return 1;
}

int describe_me(string str)
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

int name(string str)
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

void store_long(string *text)
{
  if(!sizeof(text)) {
    write("No changes are done.\n");
    return;
  }
  write("You changed the description of the room.\n");
  say(capitalize(QueryOwner()) + " changed the description of the room.\n");
  SetIntLong(implode(text,"\n") + "\n");
  update_file();
}

void getstring(string str)
{
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

void gettext(string str)
{
  switch (str) {
  case "~r":
    if (!sizeof(edittext))
      write("No lines in the buffer.\n]");
    else
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
  if (sizeof(str) > 79) {
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

void getdetailtext(string str)
{
  switch (str) {
  case "~r":
    if (!sizeof(edittext))
      write("No lines in the buffer.\n]");
    else
     write(implode(edittext,"\n") + "\n]");
    input_to("getdetailtext");
    return;
  case "~d":
    if (!sizeof(edittext)) {
      write("No lines in the buffer.\n]");
      input_to("getdetailtext");
      return;
    }
    edittext = edittext[0..<2];
    write("]");
    input_to("getdetailtext");
    return;
  case ".":
  case "**":
    if(!sizeof(edittext)) {
      write("No changes are done.\n");
      return;
    }
    write("You changed a detail of this room.\n");
    AddDetail(detail,implode(edittext,"\n") + "\n");
    update_file();
    return;
  case "~h":
    write(help() + "]");
    input_to("getdetailtext");
    return;
  }
  if (sizeof(str) > 79) {
    write("This line ist too long. Please enter again!\n]");
    input_to("getdetailtext");
    return;
  }
  if (!str)
    edittext += ({""});
  else
    if (edittext==0)
      edittext = ({str});
    else
      edittext += ({str});
  write("]");
  input_to("getdetailtext");
}

void update_file()
{
  int    i;
  mapping map;
  mixed  dummy;
  string out;
  object door;

  rm(QueryHomedir() + "/" + location[0] + location[1] + location[2] + ".c");
  out = "/* A room in the house of " + QueryOwner() + " */\n\n\
#include <rooms.h>\ninherit \"/p/houses/common/room/room\";\n\ncreate() {\n\
  ::create();\n  SetIntShort(\"" +
    ({string})"/lib/string"->lit_string(QueryIntShort()) + "\");\n  SetIntLong(\"" +
    ({string})"/lib/string"->lit_string(QueryIntLong()) +"\");\n";
  map = QueryExitsDest();
  dummy = m_indices(map);
  for(i = sizeof(dummy); i--; ) {
    out += "  AddExit(\"" + dummy[i] + "\",\"" + map[dummy[i]] + "\");\n";
    if (QueryExits()[dummy[i],3] == 1)   // hidden exit --> save it :)
      out += "  HideExit(\"" + dummy[i] + "\",1);\n";
    }
  // saving items
  if(dummy = QueryItems())
    for(i = sizeof(dummy); i--; )
      out += "  AddItem(\"" + dummy[i][RITEM_FILE] + "\",REFRESH_NONE);\n";
  if(map = QueryDetails()) {
    dummy=m_indices(map);
    for(i = sizeof(dummy); i--; )
      out += "  AddDetail(\"" + dummy[i] + "\",\""+
        ({string})"/lib/string"->lit_string(map[dummy[i]])+"\");\n";
  }
  // saving details
  if(map = QueryDoors()) {
    dummy = m_indices(map);
    for(i = sizeof(dummy); i--; ) {
      mixed kids;
      string room;

      door = dummy[i];
      room = object_name(({object})door->QueryDoorRoom());
      kids = ({string *})door->QueryKeyIds();
      if (pointerp(kids) && sizeof(kids))
        kids = kids[0];
      out += ("  AddDoor(\"" + ({string})door->QueryDirection() + "\",\"" +
              (room[0..1]=="//"?room[1..]:room[0..]) +
              "\",\"" + ({string})"/lib/string"->lit_string(({string})door->QueryDoorCode()) + "\",\"" +
              ({string})"/lib/string"->lit_string(({string})door->QueryLong()) +
              "\"," + (kids?"\"" + kids + "\"":0) +
              ",LOCK_CLOSED);\n");
    }
  }
  write_file(QueryHomedir() + "/" + location[0] + location[1] +
             location[2] + ".c",out + "}\n");
}

int set_privacy(string str)
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

int QueryPrivate()
{
  return privat && member(privat, object_name()[<3..]) != -1;
}

int is_private(string str)
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

int allow_enter(int method, mixed extra)
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
