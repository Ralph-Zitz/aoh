/*   Example Room 10  (Mateese, 06-Oct--92)
**
** This is an indoors room, fully lighted, but through the windows the
** changing of the day time can be seen.
** The sunlight itself no effect on the internal light.
**
** This room is similar to xroom8, but works in a different way: it directly
** asks the nightday-server.
*/

#include <properties.h>
#include <nightday.h>
#include <weather.h>

inherit "/std/room";

object nightday;  /* The night_day-server */

string fwindows(); // forward

/*-------------------------------------------------------------------------
** Make sure that we know the night_day-server, and that he knows us.
*/

static void _chk_server() 
{
  if (nightday) return;
  if (!(nightday = find_object(NIGHTDAY))) {
    catch(NIGHTDAY->reset());
    nightday = find_object(NIGHTDAY);
  }
  if (nightday) nightday->AddClient(this_object());
}

/*-------------------------------------------------------------------------*/
void create() 
{

  ::create();

  Set(P_INT_SHORT, "The other 'indoors' example room with windows");

  Set(P_INT_LONG,
"You are in another 'indoors' exm. room (/d/archwiz/common/lpc/exm/xroom10.c).\n"
"This one is fully lighted, but through the windows the changing of the "
"daylight is percepted.\n"
"You are in a small hut with some big windows.\n"
"The hut extends to the south.\n"
      );

  AddExit("south", "/d/archwiz/common/lpc/exm/xroom8");

  AddDetail(({"window", "windows"}), #'fwindows);

    /* This room is 'indoors', so tell the night_day-server manually
    ** to notify us.
    */
  _chk_server();
}

/*-------------------------------------------------------------------------
** In case the server got updated inbetween, re-register with it.
*/

void init () 
{
  ::init();
  _chk_server();
}

/*-------------------------------------------------------------------------
** When the player looks at a window, he shall see the sky outside.
** The room itself doesn't know about the daytime, so we have to ask the
** server.
*/

string fwindows() 
{
  _chk_server();
  return "Through the window you see the land outside.\n"
         +(nightday ? (string) nightday->QueryStateDesc()
                    : ""
          );
}

/* But it can't see the sky directly, since this is an 'indoors' room. */

/*-------------------------------------------------------------------------
** This function is called by the night_and_day-Server.
** We just announce the changing...
*/

void SetStateData (mixed * data) 
{
  tell_room (this_object(), "The light outside changes.\n");
}

/***************************************************************************/

