/* Advent - Subterranean Well -- Mateese, 13-Aug-96
 *
 * South of the Treasure Dump Site, the water stream gives his first
 * appearance here.
 * Also located here are several important objects:
 *  - the keys to open the grate in r/subtoutgrate, the entrance to the
 *    Cave
 *  - a bottle to carry water and oil around
 *  - tasty food to bribe the bear
 *  - a torch.
 * This room combines the original rooms Inside Building and End Of Road.
 */

#include <properties.h>
#include <rooms.h>
#include <moving.h>
#include "advent.h"

inherit ADVENT("i/stdroom");

/*-----------------------------------------------------------------------*/
/* Modify the room description and lighting according to the bright
 * of the treasure dump side
 */

static int trdumplight()
{
  object trdump;
    
  trdump = find_object(ADVENT("r/trdump"));
  return trdump && trdump->Query(P_INT_LIGHT);
}

string TrdumpExtraLong ()
{
  int nlight;
    
  nlight = trdumplight();
  if (nlight <= 10)
    return "From the north a faint glow is visible.\n";
  switch(nlight)
  {
    case 10..30:
      return "A distinct glow emanates from the north into the tunnel.\n";
    case 31..70:
      return "A sign in the north casts some light into the tunnel.\n";
  }
  return "The tunnel is lighted by light radiating from the north.\n";
}

string TrdumpDetail ()
{
  int    nlight;
  string msg;
    
  nlight = trdumplight();
  msg = "A faint yellow glow.\n";
  if (nlight <= 10)
    return msg;
    
  switch(nlight)
  {
    case 10..30:
      msg = "A distinct yellow glow emanating from the north.\n";
      break;
    case 31..70:
      msg = "Some sort of sign in the north casts light into the tunnel.\n";
      break;
   default:
      msg = "Some sort of sign in the north radiates enough light to light the tunnel.\n";
  }
  return msg;
}

int QueryIntLight()
{
  return ::QueryIntLight()+trdumplight()/2;
}

/*-----------------------------------------------------------------------*/
public void ConfigKey(object key, int rflags, int nr)
{
  if (!key)
    return;
  key->SetProperties(
([ P_SHORT: "a set of keys"
 , P_LONG: ({ "It's just a normal looking set of keys.\n"
, "A small label attached to the ring reads 'W. Crowther & D. Woods'.\n"
           })
 , P_KEYCODE: ADV_GRATE_KEYCODE
])
		    );
  key->AddId(({"key", "keyring", "keys"}));
  key->AddAdjective(({ "set", "set of", "ring", "ring of" }));
  /* The other properties are good as they are */
}

/*-----------------------------------------------------------------------*/
public void ConfigFood(object food, int rflags, int nr)
{
  if (!food)
    return;
  food->SetProperties(
([ P_SHORT: "some tasty food"
 , P_LONG: "Sure looks yummy!\n"
 , P_FOOD_KIND: "food"
 , P_FOOD_STUFF: 10
 , P_FOOD_ALC: 0
 , P_FOOD_SOAK: 0
 , P_FOOD_MSGS: ({ "Delicious!\n", "@@Pl_Name@@ eats some tasty food.\n" })
])
		    );
  food->AddAdjective(({ "some", "tasty" }));
  /* The other properties are good as they are */
}

/*-----------------------------------------------------------------------*/
public void ConfigBottle(object bottle, int rflags, int nr)
{
  if (!bottle)
    return;
  bottle->Set_Vessel_Name("bottle");
  bottle->Set_Vessel_Volume(350);
  bottle->Set_Vessel_Weight(300);
  bottle->Set_Vessel_Value(10);
  bottle->fill_vessel(0);
  bottle->Set(P_LONG, "An ordinary bottle.\n");
  bottle->AddId("bottle");
}

/*-----------------------------------------------------------------------*/
void create()
{
  ::create();

  // Main description
  Set(P_INT_SHORT, "Subterranean Well");
  Set(P_INT_LONG,
"You are in a low subterranean tunnel. A spring of clear water flows out "
"of a hole short above the floor and southwards along the tunnel.\n"
     );
  Set(P_INT_SMELL, "The air smells fresh and cool.\n");
  Set(P_INT_NOISE, "It is silent, except for the flow of the water.\n");
  Set(P_BRIGHT, 0);
  Set(P_NODWARF, 1);
  Set(P_NOORIG, 1);

  SetExtraEntry("trdump extra long", P_INT_LONG, #'TrdumpExtraLong);

  // Exits
  AddExit("north", ADVENT("r/trdump"));
  AddExit("south", ADVENT("r/subtstream"));
    
  // Details
  AddDetail("spring", 
"A spring of clear water, flowing out of a hole and southwards into the tunnel.\n"
	   );
  AddDetail("hole",
"A hole, about 1 foot in diameter. A constant stream of water is flowing "
"out of it.\n"
	   );
  AddDetail(({ "glow", "yellow glow", "sign", "north" }), #'TrdumpDetail);
    
  // Items
  AddItem(ADVENT("o/water"), REFRESH_REMOVE
         , ([ P_WATEREXTRA:
"It flows out of a hole and southwards into the tunnel.\n"
           ])
	 );
  AddItem("/obj/key", REFRESH_REMOVE, 1, #'ConfigKey);
  AddItem("/obj/torch", REFRESH_REMOVE);
  AddItem("/std/food", REFRESH_REMOVE, #'ConfigFood);
  AddItem("/std/drink", REFRESH_REMOVE, #'ConfigBottle);
  /* Using a /std/drink for the bottle is not optimal, especially as the
   * bottle is also to be filled with oil. But as it still should be possible
   * to drink from the bottle, /std/drink is better than a handcrafted bottle.
   */
}

/*************************************************************************/
