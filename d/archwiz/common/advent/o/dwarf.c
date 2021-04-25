/* Advent - The Threatening Little Dwarf -- Mateese, 22-Aug-96
 *
 * The dwarf appears at random for the player, a static one is started
 * in the Hall of Mists.
 * It implements the same question/answer spiel as the Dragon, but can't
 * be killed with bare hands.
 * Regarding the different combat systems, I reduced the attack chance
 * from 75% to 50%.
 * The state of the answer is stored in the player as 'AdventDwarfQuestion':
 *   0: didn't try to kill yet.
 *   1: tried to kill, didn't answer yet.
 *   2: second try to kill, maybe answered no.
 *
 * Properties:
 *   "AdventDwarfFollow": the player this dwarf follows.
 */

#include <config.h>
#include <properties.h>
#include <attributes.h>
#include <living.h>
#include <moving.h>
#include <rooms.h>
#include "advent.h"

inherit "/std/living";
inherit ADVENT("i/advlib");

#define P_QUESTION "AdventDragonQuestion"
#define P_FOLLOW   "AdventDwarfFollow"

#define FOLLOW_CHANCE 96
#define MIRRORCANYON ADVENT("r/mirrorcanyon")

/*-----------------------------------------------------------------------*/
public void create()
{
  if (::create()) /* Don't setup the blueprint */
    return;

  // Description
  SetInitLevel(15, 1);
  Set(P_NAME, "Dwarf");
  Set(P_SHORT, "a threatening little dwarf");
  Set(P_LONG, 
"It's probably not a good idea to get too close. Suffice it to say the little "
"guy's pretty aggressive.\n"
     );
  Set(P_WEIGHT, 40000+random(20000));
  Set(P_VALUE, 0);
  Set(P_RACE, "dwarf");
  Set(P_GENDER, GENDER_MALE);
  Set(P_WIMPY, 10);
  Set(P_GO_CHANCE, 10);
  Set(P_AGGRESSIVE, 1);
  Set(P_ATTACK_CHANCE, 5000);
  SetAttr(A_CORPSE, 0);
  AddDefence(0, 10);

  AddId( ({ "dwarf", ADV_DWARF }) );
  AddAdjective( ({"threatening", "little" }) );
    
  Set(ADV_DWARF, 1);

  /* Catch the saying of 'yes' or 'no'.
   */
  InitMatch(
       ({ "_catch_yesno" }),  /* Function        */
       ({ "says:"        }),  /* type            */
       ({ 0              })   /* match (0 == "") */
           );

  /* Weaponry */
  AddItem("/std/weapon", REFRESH_REMOVE|REFRESH_FUNC
         , ([ P_SHORT: "a nasty little knife"
            , P_LONG: "A little dwarfen knife, quite sharp.\n"
            , P_VALUE: 100
            , P_WEIGHT: 200
            , P_ADS: ({ "little", "dwarven", "nasty" })
            , P_WEAPON_TYPE: WT_KNIFE
            , P_DAM_TYPE: DT_SLASH
            , P_WC: 5
            , P_NUMBER_HANDS: 1
           ])
         , #'wieldme
         );
}

/*-----------------------------------------------------------------------*/
public void Follow (object obj)

{
  object old;

  if (obj && !obj->Query(P_IS_PLAYER))
    return;

  old = Query(P_FOLLOW);
  if (old == obj)
    return;
    
  if (old)
    old->RemoveFollower(this_object());
  Set(P_FOLLOW, obj);
  if (obj)
    obj->AddFollower(this_object());
}

/*-----------------------------------------------------------------------*/
public void deep_remove()

/* Destruct the bear and all the inventory.
 */

{
  map_objects(deep_inventory(this_object()), "remove");
  map(deep_inventory(this_object()), #'destruct);
  remove();
}

/*-----------------------------------------------------------------------*/
public int dwarf_heartbeat()

/* If nothing happens for a while, remove myself.
 */

{
  object obj, *enems;
    
  obj = Query(P_FOLLOW);
  enems = Query(P_ENEMIES) || ({ });
    
  if ((!obj || environment(obj) != environment(this_object()))
   && !sizeof(enems)
   && !random(3)
     )
  {
    Follow(0);
    show("Tiring of this, the dwarf slips away.\n");
    deep_remove();
    return 0;
  }

  /* Just in case, check if to attack the person we are following */
  if (obj 
   && environment(obj) == environment(this_object())
   && -1 == member(enems, obj) 
   && CheckAutoAttack(obj)
     )
  {
    if (to_string(environment(this_object())) == MIRRORCANYON)
    {
      show_room(environment(this_object())
	       , "The dwarf admires himself in the mirror.\n"
	       );
      Follow(0);
    }
    else
      Kill(obj);
  }

  return 1;
}

/*-----------------------------------------------------------------------*/
public void init()
{
  object obj;

  ::init();
  AddHeart("dwarf_heartbeat");
  this_player()->Set(P_QUESTION, 0);
  add_action("fkill", "kill");
  add_action("fkick", "kick");
  add_action("fyesno", "yes");
  add_action("fyesno", "no");

  if (to_string(environment(this_object())) == MIRRORCANYON)
  {
    write("The dwarf admires himself in the mirror.\n");
    Follow(0);
  }
  else
  {
    obj = Query(P_FOLLOW);
    if (!obj && obj != this_player() && random(100) < FOLLOW_CHANCE)
      Follow(this_player());
  }
}

/*-----------------------------------------------------------------------*/
public void Death (object * killers)
{
  int i;

  show_room(environment(this_object()),
"The body vanishes in a cloud of greasy black smoke.\n"
	   );
  for (i = 0; i < sizeof(killers); i++)
    killers[i]->Set(P_DWARVES, killers[i]->Query(P_DWARVES)+1);
  ::Death(killers);
}

/*-----------------------------------------------------------------------*/
public int IsFriend (object obj)

/* Don't attack players which have killed more than ADV_DWARVES of us.
 */

{
  if (obj->Query(P_DWARVES) >= ADV_DWARVES
   || to_string(environment(this_object())) == MIRRORCANYON
     )
    return 1;
  return ::IsFriend(obj);
}

/*-----------------------------------------------------------------------*/
public void follow (mixed dest, int method, mixed extra)

/* The dwarf follows only M_GO movements, and this just with a 96% chance.
 */
{
  object here;
    
  here = environment(this_object());
    
  if (method == M_GO || random(100) < FOLLOW_CHANCE)
    ::follow(dest, method, extra);
    
  if (here == environment(this_object()))
    Follow(0);
  else
    write("The dwarf stalks after you.\n");
}

/*-----------------------------------------------------------------------*/
public varargs int move (mixed dest, int method, mixed extra)

/* Don't move into several places.
 * Also, with a 75% chance, get stuck in the mirror canyon.
 */

{
  if (!this_object())
    return ME_DESTRUCTED_SELF;

  if (to_string(environment(this_object())) == MIRRORCANYON
   && random(100) < 75
     )
  {
    Follow(0);
    return 0;
  }

  if (!objectp(dest))
    dest = load_object(dest);

  if (!dest)
    return 0;
    
  if (dest->Query(P_NODWARF)
   || dest->Query(P_NONPC)
   || dest->Query(P_INT_LIGHT) > 10
   || present(ADV_BEAR, dest)
   || present(ADV_TROLL, dest)
   || present(ADV_DRAGON, dest)
     )
  {
    Follow(0);
    return 0;
  }

  AddHeart("dwarf_heartbeat");
  return ::move(dest, method, extra);
}

/*-----------------------------------------------------------------------*/
public int fkick (string arg)

/* Intercept the 'kick' for a nice message.
 */

{
  int question;
    
  if (!arg || -1 == strstr("dwarf", arg))
    return 0;

  write(
"You boot the dwarf across the room. He curses, then gets up and brushes "
"himself off. Now he's madder than ever!\n"
       );
  show(
this_player()->Query(P_NAME)
+" boots the dwarf across the room. He curses, then gets up "
"and brushes himself off. Now he's madder than ever!\n"
      , "You hear some vicious curses.\n"
       );
  Set(P_ATTACK_CHANCE, Query(P_ATTACK_CHANCE)+1); /* ! */
  return 1;
}

/*-----------------------------------------------------------------------*/
public int fkill (string arg)

/* Intercept the 'kill' command to ask the question before.
 */

{
  int question;
    
  if (!arg || -1 == strstr("dwarf", arg))
    return 0;

  question = this_player()->Query(P_QUESTION);
  
  /* Player didn't answer the question or answered 'no' before.
   */
  if (question >= 1)
  {
    this_player()->Set(P_QUESTION, 2);
    return 0;
  }
    
  /* Ask the question and wait for the answer. */
  this_player()->Set(P_QUESTION, 1);
  write("With what? With your bare hands?\n");
  return 1;
}

/*-----------------------------------------------------------------------*/
public int GotAnswer (int bYes)
{
  if (1 == this_player()->Query(P_QUESTION))
  {
    this_player()->Set(P_QUESTION, 2);
    if (bYes)
      write("You wish.\n");
    else
      write("Smart move.\n");
    return 1;
  }
  return 0;
}

/*-----------------------------------------------------------------------*/
/* The reception of 'yes' and 'no'.
 */

void _catch_yesno (string msg)
{
  int i;
  string *msgs;

  i = member(msg, ':');
  msgs = norm_id(msg[i+1..<2], 1);
  if ("yes" == msgs[0])
    GotAnswer(1);
  if ("no" == msgs[0])
    GotAnswer(0);
}

public int fyesno (string arg)
{
  return GotAnswer("yes" == query_verb());
}

/*-----------------------------------------------------------------------*/
public varargs int allow_enter (int method, mixed extra)

/* Print a message if someone gives us something.
 * The handling is a bit crude, but for sake of compatibility with
 * the original...
 */

{
  if (M_GIVE == method)
  {
    write(
"The dwarf is not at all interested in your offer. (The reason being, "
"perhaps, that if he kills you he gets everything you have anyway.)\n"
	 );
    return 0;
  }
  return ::allow_enter(method, extra);
}

/*************************************************************************/
