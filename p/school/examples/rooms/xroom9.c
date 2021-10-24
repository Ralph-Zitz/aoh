/*   Example Room 9  (Mateese, 29-Sep-92)
**
** This is an outdoors room with dimmed daylight.
*/

#include <config.h>
#include <properties.h>

inherit "/std/room";

varargs void create() 
{
  ::create();

  Set(P_INT_SHORT, "The 'outdoors' example room with dimmed daylight");

  Set(P_INT_LONG,
"You are in another 'outdoors' exm. room (/p/school/examples/rooms/xroom9.c). "
"This one features a dimmed daylight. "
"You are in a thick forest. You can hardly see the sky. "
"A path leads west from here.\n"
     );

  AddExit("west", "/p/school/examples/rooms/xroom6");

    /* Mark this room as being outdoors.
    ** This also takes care of setting the light.
    */
  Set(P_INDOORS, 0);

    /* This is thick forest, so let the sun shine with half power.
    ** This must be done after the call to SetIndoors(0) since that
    ** also does an SetSunBright().
    */
  Set(P_SUNBRIGHT, MAX_SUNBRIGHT / 2);
}

