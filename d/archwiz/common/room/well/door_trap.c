/*
* Black room after well
* Created by Joern
* Last Changes by Thragor, 04mar94
* Changes by Cadra, 30sep94
*/

#include "path.h"
#include <moving.h>
inherit "/std/room";

private static int west_door_open;

int open(string str)
{
 notify_fail("Open what?\n");
 if (member(({"door","west door","east door"}),str)==-1)
  return 0;
 write("There is no handle, and you can't push it up.\n");
 return 1;
}

int close(string str)
{
 notify_fail("Close what?\n");
 if (member(({"door","west door","east door"}),str)==-1)
  return 0;
 write("There is no handle, and you can't push it closed.\n");
 return 1;
}

int toggle_door()
{
 write("You move the lever.\n");
 if (!this_player()) return 0;
 show(this_player()->QueryName() + " pulled the lever.\n",this_player());
 if (west_door_open)
 {
  tell_room(this_object(), "The west door closed.\nThe east door opened.\n");
  tell_room(environment(this_player()), "The west door opened.\n");
  tell_room(WELL("after_trap"), "The door closes.\n");
  tell_room(WELL("well"), "The door opens.\n");
  west_door_open = 0;
 }
 else
 {
  tell_room(this_object(), "The west door opens.\nThe east door closed.\n");
  tell_room(environment(this_player()), "The west door closed.\n");
  tell_room(WELL("after_trap"), "The door opens.\n");
  tell_room(WELL("well"), "The door closes.\n");
  west_door_open = 1;
 }
}

int query_west_door()
{
 return west_door_open;
}

west() {
 if (!west_door_open)
 {
  write("The door is closed.\n");
  return ME_NO_ENTER;
 }
 if (!this_player()) return 0;
 return this_player()->move(WELL("after_trap"),M_GO,"west");
}

int east()
{
 if (west_door_open)
 {
  write("The door is closed.\n");
  return ME_NO_ENTER;
 }
 if (!this_player()) return 0;
 return this_player()->move(WELL("well"),M_GO,"east");
}

look_west() {
  if (!west_door_open)
    return "The west door is closed.\n";
  return "The west door is open.\n";
}

look_east() {
  if (!west_door_open)
    return "The east door is open.\n";
  return "The east door is closed.\n";
}

create()
{
 ::create();
 SetIntShort("Room with black walls");
 SetIntLong(
"A room with black walls. There is a door to the east,\n\
and a door to the west.\n");
 SetIndoors(1);
 SetIntLight(0);
 AddDetail(({"room","black room"}),
QueryIntLong());
 AddDetail(({"walls","wall","ceiling","ground","floor",
 "black walls","black wall","black ceiling","black floor",
 "black ground"}),
"Everything is black in this room.\n");
 AddDetail("door","Which one? The east or the west door?\n");
 AddDetail("doors","Doors. You see nothing special about them.\n\
One is open, the other closed.\n");
  AddDetail(({"west door","door to the west"}),
#'look_west);
 AddDetail(({"east door","door to the east"}),
#'look_east);
 AddRoomCmd("open",#'open);
 AddExit("east",#'east);
 AddRoomCmd("close",#'close);
 AddExit("west",#'west);
 HideExit("east");
 HideExit("west");
 reset();
}

reset()
{
 ::reset();
 west_door_open = 1;
}
