/*   Example Room 5  (Mateese, 29-Sep-92)
**
** This is a simple indoors room.
*/

#include <properties.h>
#include "/p/school/sys/school.h"

inherit "/std/room";

varargs void create() 
{
  ::create();

  Set(P_INT_SHORT, "The 'indoors' example room");

  Set(P_INT_LONG, 
"You are in the 'indoors' example room (/p/school/examples/rooms/xroom5.c). "
"You see nothing but exits to the east and west. "
"The western exit leads back into the example dungeon, the eastern leads "
"outside.\n"
     );

  AddExit ("west", "/p/school/examples/rooms/xroom1");
  AddExit ("east", "/p/school/examples/rooms/xroom6");

  /* No need to set the light - there is an automatic lighting of 75% sunlight.
  ** A Set(P_INT_LIGHT) or Set(P_BRIGHT) would switch the automatic lighting 
  ** off.
  */

}

