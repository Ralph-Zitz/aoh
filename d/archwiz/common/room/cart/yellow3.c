/* C.A.R.T. Yellow Line -- Mateese, 10-Dec-95
 *
 * In the C.A.R.T. tunnel, between Central Station and Beach Station.
 * To the south the player can enter the 'Adventure' area through the 
 * Treasure Dump Site. For added value, the light from the Dump Site
 * also lights this room.
 */

#include <properties.h>
#include <rooms.h>
#include "path.h"

inherit "/std/room";

/*-----------------------------------------------------------------------*/
/* Modify the room description and lighting according to the bright
 * of the niche.
 */

static int nichelight()
{
  object niche;
    
  niche = find_object(ADVENT("r/trdump"));
  return niche && niche->Query(P_INT_LIGHT);
}

string NicheExtraLong ()
{
  int    nlight;
    
  nlight = nichelight();
  if (nlight <= 10)
    return "To the south a niche has been carved into the side of the tunnel.\n";
  switch(nlight)
  {
    case 10..30:
      return "A niche to the south emanates a distinct glow into the tunnel.\n";
    case 31..70:
      return "A niche to the south casts some light into the tunnel.\n";
  }
  return "The tunnel is lighted by the light radiating from a niche to the south.\n";
}

string NicheDetail ()
{
  int    nlight;
  string msg;
    
  nlight = nichelight();
  msg = "The niche has been carefully carved into the stone.\n";
  if (nlight <= 10)
    return msg;
    
  switch(nlight)
  {
    case 10..30:
      msg += "A distinct glow emanates from within.\n";
      break;
    case 31..70:
      msg += "Some sort of sign within casts light into the tunnel.\n";
      break;
   default:
      msg += "It radiates enough light from within to light the tunnel.\n";
  }
  return msg;
}

int QueryIntLight()
{
  return ::QueryIntLight()+nichelight()/2;
}

/*-----------------------------------------------------------------------*/

void create()
{
  ::create();

  // Main description
  Set(P_INT_SHORT, "C.A.R.T. Tunnel");
  Set(P_INT_LONG,
"The old C.A.R.T. tunnel runs deep under the earth from east to west.\n"
     );
  Set(P_INT_SMELL, "The air smells stale and earthy.\n");
  Set(P_BRIGHT, 0);

  SetExtraEntry("niche extra long", P_INT_LONG, #'NicheExtraLong);

  // Exits
  AddExit("west", CART("yellow2"));
  AddExit("east", CART("yellow4"));
  AddExit("south", ADVENT("r/trdump"));

  // Miscellaneous
  Set(P_CARTROOM, 1);

  // Details
  AddDetail(({ "track", "tracks", "rail", "rail tracks", "rails" }),
"The tracks run through the tunnel. In former times, C.A.R.T. trains ran "
"along them, but nowadays they left to rust and dirt.\n"
           );
  AddDetail( ({"rust", "stains of rust", "rust stains", "rust stain" }),
"Stains of rust are visible everywhere on the tracks.\n"
           );
  AddDetail( "dirt",
"Stones, sand, splinters of wood and other debris, scattered over the tracks.\n"
"Nothing interesting at all.\n"
           );
  AddDetail( ({ "wall", "walls", "ceiling" }),
"Raw, unblended rock forms the walls and the ceiling of the tunnel.\n"
	   );
  AddDetail( ({ "stone", "rock" }),
"Stone, as known as rock, is the material continents are made of.\n"
	   );
  AddDetail("tunnel",
"The tunnel is quite old, but still trespassable. To the south, a niche "
"has been carved into the stone.\n"
           );
  AddDetail(({ "niche", "dumpsite", "dump site"
             , "treasure dumpsite", "treasure dump site" })
           , #'NicheDetail
           );

  // Items
}

/*************************************************************************/
