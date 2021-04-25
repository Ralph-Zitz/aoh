/* An entrance room for the makler office to sell */

#include <moving.h>
#include <rooms.h>
#include <properties.h>
#include <doors.h>
#include <secure/wizlevels.h>
#include "/p/houses/common/sys/prices.h"
#include "/p/houses/common/sys/houses.h"
inherit "/p/houses/common/room/room";
inherit "/p/houses/common/room/sell";

nosave string enter_verb;

void config();
string QueryEnter();

varargs void create()
{
  mixed dummy;
  object door;
  mixed room;

  (::create());
  if(object_name() == "/p/houses/common/room/home") return;
  Set(P_INDOORS,1);
  Set(P_TPORT,TPORT_NO);
  if(file_size(QueryHomedir() + "/config.o") != -1)
    restore_object(QueryHomedir() + "/config");
  else
    if(file_size(QueryHomedir() + "/allowed") != -1) {
      allowed = explode(read_file((QueryHomedir() + "/allowed")) || "","\n")
	-({""});
      if(!allowed) allowed = ({});
    }
    else allowed = ({});
  config();
  save_object(QueryHomedir() + "/config");
  if(dummy = DoorInfo(opposite(QueryEnter()))) {
    door = dummy[DOOR_OBJECT];
    room = load_object(({string})door->QueryOtherRoom());
    room->AddDoor( QueryEnter(), object_name(), ({string})door->QueryDoorCode()
                 , "This door leads into the house of "+capitalize(QueryOwner())+".\n"
                 , ({ ({mixed})door->QueryKeyCode() })+(({string *})door->QueryKeyIds()||({}))
                 , ({int})door->QueryLockState()
                 );
  }
}

void config() { return; } /* Just a dummy... */

string SetEnter(string v)
{
  return enter_verb = v;
}

string QueryEnter() { return enter_verb; }

void init()
{
  int credit;
  int player_money;

  room::init();
  sell::init();
  if(QueryIsOwner(getuid(this_player()))) {
    add_action("allow","allow");
    add_action("forbid","forbid");
    add_action("list","list");
    add_action("add_owner","add");
    add_action("remove_owner","remove");
    if(query_verb() == QueryEnter()) {
      write("Welcome at home, " + capitalize(getuid(this_player())) + "!\n");
      if(file_size(homedir + "/dept") != -1) {
        credit = to_int(read_file(homedir + "/dept")); /*+++ was /credit +++*/
        rm(homedir + "/dept");
        write("You've to pay " + ({string})"/obj/money_demon"->QueryValueString(credit) +
              " of your credit.\n");
        if(!({int})"/lib/money"->PayValue(this_player(),credit)) {
          write("But you haven't enough money.\n");
          if((player_money =
             ({int})"/lib/money"->QuerySomeonesValue(this_player()) - 1000) < 0) {
            write_file(homedir + "/dept",to_string(credit));
            return;                 /*+++ was credit +++*/
          }
          "/lib/money"->PayValue(this_player(), player_money);
          write("You only can pay " +
                ({string})"/obj/money_demon"->QueryValueString(player_money) + ".\n");
          write_file(homedir + "/dept",to_string(credit - player_money));
          return;               /*+++ was credit +++*/
        }
        write("But now you paid the whole credit back.\n");
        return;
      }
    }
  }
}

int allow_enter(int method, mixed extra)
{
  string player;
  object the_owner;

  if(member(allowed||({}), "all") != -1) return 1;
  if(living(previous_object()) && query_verb() == QueryEnter()) {
    if((player = getuid(previous_object())) != QueryOwner() &&
       !QueryIsOwner(player) &&
       member(allowed||({}), player) == -1 && !IS_IMMORTAL(previous_object())) {
      if(the_owner = find_player(QueryOwner()))
        tell_object(the_owner,(interactive(previous_object())?
                               capitalize(player):
                               ({string})previous_object()->QueryShort()) +
                    " wants to enter your home.\n");
      tell_object(previous_object(),"You can't enter the house. Ask " +
                  capitalize(QueryOwner()) + " for a permission!\n");
      return ME_NO_ENTER;
    }
  }
  return ME_OK;
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
#include <rooms.h>\ninherit \"/p/houses/common/room/home\";\n\nconfig() {\n\
  SetIntShort(\"" + ({string})"/lib/string"->lit_string(QueryIntShort()) +
  "\");\n  SetIntLong(\"" +  ({string})"/lib/string"->lit_string(QueryIntLong()) +
    "\");\n";
  map = QueryExitsDest();
  dummy = m_indices(map);
  for(i = sizeof(dummy); i--; ) {
    out += "  AddExit(\"" + dummy[i] + "\",\"" + map[dummy[i]] + "\");\n";
    if (QueryExits()[dummy[i],3] == 1)   // hidden exit --> save it :)
          out += "  HideExit(\"" + dummy[i] + "\",1);\n";
  }
  if(dummy = QueryItems())
    for(i = sizeof(dummy); i--; )
      out += "  AddItem(\"" + dummy[i][RITEM_FILE] + "\",REFRESH_NONE);\n";
  if(map = QueryDetails()) {
    dummy=m_indices(map);
    for(i = sizeof(dummy); i--; )
      out += "  AddDetail(\"" + dummy[i] + "\",\""+
        ({string})"/lib/string"->lit_string(map[dummy[i]])+"\");\n";
  }
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
              ",LOCK_" +
              (({string})door->QueryDirection()==opposite(QueryEnter())?"LOCKED":"CLOSED") + ");\n");
    }
  }
  out += "  SetEnter(\"" + QueryEnter() + "\");\n";
  write_file(QueryHomedir() + "/" + location[0] + location[1] +
             location[2] + ".c",out + "}\n");
}

int allow(string player)
{
  if(!player) {
    notify_fail("Whom you'll allow to enter your home?\n");
    return 0;
  }
  player = lower_case(player);
  if(file_size("/save/" + player[0..0] + "/" + player + ".o") == -1 &&
     player != "all") {
    notify_fail("No such player is playing in " MUDNAME ".\n");
    return 0;
  }
  restore_object(homedir + "/config");
  if(!allowed) allowed = ({});
  if(member(allowed, player) != -1) {
    notify_fail(capitalize(player) + " is already allowed to enter your home!\n");
    return 0;
  }
  write("You allow " + (player == "all"?"all players":capitalize(player)) +
        " to enter your home.\n");
  allowed -= ({player});
  allowed += ({player});
  save_object(homedir + "/config");
  return 1;
}

int add_owner(string player)
{
  if(!player) {
    notify_fail("Whom you'll add to the owners of the house?\n");
    return 0;
  }
  if (strstr(player,"detail")==0) {
    write(
    "If you want to add a detail - just use 'adddetail <detail>' instead of\n"
    "'add detail <detail>'\n");
    return 1;
  }
  player = lower_case(player);
  if(player[0..5] == "owner ") player = player[6..];
  if(file_size("/save/" + player[0..0] + "/" + player + ".o") == -1) {
    notify_fail("No such player is playing in " MUDNAME ".\n");
    return 0;
  }
  restore_object(homedir + "/config");
  if(!coowners) coowners = ({});
  if(member(coowners, player) != -1) {
    notify_fail(capitalize(player) + " is already an owner of this house.\n");
    return 0;
  }
  write("You added " + (player == "all"?"all players":capitalize(player)) +
        " to the owners of this house.\n");
  coowners -= ({player});
  coowners += ({player});
  save_object(homedir + "/config");
  return 1;
}

int forbid(string player)
{
  if(!player) {
    notify_fail("You want to forbid whom to enter your home?\n");
    return 0;
  }
  player = lower_case(player);
  restore_object(homedir + "/config");
  if(!allowed) allowed = ({});
  if(player == "all") {
    allowed = ({});
    save_object(homedir + "/config");
    write("You forbid all players to enter your home.\n");
    return 1;
    }
  if(member(allowed, player) == -1) {
    notify_fail(capitalize(player) + " wasn't allowed to enter your home.\n");
    return 0;
  }
  allowed -= ({player});
  allowed -= ({""});
  save_object(homedir + "/config");
  write("You forbid " + capitalize(player) + " to enter your home.\n");
  return 1;
}

int remove_owner(string player)
{
  if(!player) {
    notify_fail("Who should be removed from the owners of this houses?\n");
    return 0;
  }
  player = lower_case(player);
  if(player[0..5] == "onwer ") player = player[6..];
  restore_object(homedir + "/config");
  if(!coowners) coowners = ({});
  if(player == "all") {
    coowners = ({});
    save_object(homedir + "/config");
    write("You removed all other owners of this house, except " +
          capitalize(QueryOwner()) + ".\n");
    return 1;
  }
  if(player == QueryOwner()) {
    notify_fail("You can't remove " + capitalize(player) +
                " from the owners of this house.\n");
    return 0;
}
  if(member(coowners, player) == -1) {
    notify_fail(capitalize(player) + " was no owner of this house.\n");
    return 0;
  }
  coowners -= ({player});
  coowners -= ({""});
  save_object(homedir + "/config");
  write("You removed " + capitalize(player) +
        " from the owners of this house.\n");
  return 1;
}

int list(string str)
{
  string *cap_allowed;

  if(str && str != "allowed" && str != "owners") {
    notify_fail(
      "'list allowed' lists the people who are allowed to enter this house.\n\
'list owners' lists the people who own this house\n");
    return 0;
  }
  restore_object(homedir + "/config");
  if(!str || str == "allowed") {
    if(member(allowed||({}), "all") != -1)
      write("Everyone is allowed to enter this house.\n");
    else
      if(!sizeof(allowed))
        write("Nobody is allowed to enter this house.\n");
      else
        write("These persons are allowed to enter this house:\n" +
          sprintf("%-79=s\n",implode(map(
            allowed||({}),
            lambda(({'a}),({#'sprintf,"%-16s",({#'capitalize,'a})}))),"")));
  }
  if(!str || str == "owners") {
    if(!sizeof(coowners||({})))
        write(capitalize(QueryOwner()) + " is the owner of this house.\n");
      else
        write("These persons are the owners of this house:\n" +
          sprintf("%-79=s\n",implode(map(
             ({QueryOwner()}) + (coowners||({})),
            lambda(({'a}),({#'sprintf,"%-16s",({#'capitalize,'a})}))),"")));
  }
  return 1;
}

