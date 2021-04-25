/*  This is an example of a comment.
**  Comments are ignored by the game driver.
*/

#include <rooms.h>

inherit "/std/room";

varargs void create()
{
  ::create();
 set("internal_short", "An example room");
 SetIntLong(
  "This is an example room to learn how to use the editor. It has shiny\n"+
  "aluminium walls, one window with plastic panes and a single yellow\n"+
  "light bulb hanging from the center of the ceiling.\n");
 AddExit("north","/d/cove/common/center/house.c");
 AddDetail("bulb", "It is an old 30 Watt bulb. Better not break it.\n");
}
oop, this is a mistake!
