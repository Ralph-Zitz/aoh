/* Death  (Mateese, 28-Aug-92)
** slighlty updated, Pepel, 8-Jan-95
**
** The final judge itself.
*/

#include <properties.h>   /* for the property names */

inherit "/std/npc";

/*-------------------------------------------------------------------------*/

create () {

  if (::create()) return 1;

  SetName ("Balgarion");
  AddId ("moot");
  AddId ("sword");
  AddId ("robe");

  SetShort ("Your rules teacher Balgarion");

  SetRace ("immortal");

  Set(P_DEFENCES, ({ 50 }) );
  Set(P_HANDS, ({ ({ "right hand", 0, 25 }), ({"left hand", 0, 25 }) }));
}

/*-------------------------------------------------------------------------*/

init() {
  ::init();
  add_action("take_it", "take");
}

/*-------------------------------------------------------------------------*/

/* hack alert */
static private string lastid;
id(s) { if (::id(s)) { lastid = s; return 1; } }

Long (str) {

  if (lastid == "balgarion" || lastid == "moot")
    return 
 "Balgarion looks like all the time when he was teaching you zhe rules of\n"
"honour and obedience. He is dressed in his old stitched robe and he is as\n"
"dirty as possible. Only the sword in his hand confuses you and something\n"
"must be happened with his eyes.......\n"
"They are looking like ..... nope that cant be...... black STONES.\n";
  
  if (lastid == "sword")
    return 
"An extremly sharpened Ninjato. It's so sharp that gusts of wind actually try\n"
"to turn away from the edge rather than be sliced in two by the wicked\n"
"looking blade. It does strange things with light as well as unlucky photons\n"
"split into their sub-components when they hit the blade.\n";
  
  if (lastid == "robe")
    return 
"A very dirty robe with numerous stitches. It doesn't seem to fit you very\n"
"well however. It seems to have been tailored a very long time befor and an\n"
"VERY lean customer...\n";
  
  return 0;
}

/*-------------------------------------------------------------------------*/

take_it(str) {

  if (str == "sword" || str == "robe") {
    int extra;
    extra = random(90) + 10;
    write (
"You take a firm grip on the " + str + " and try to pull it towards you.\n\n"
"STUPID MORTAL. YOU JUST EARNED " + extra + " EXTRA YEARS IN THE DOJO!\n"
"Balgarion frowns and slaps you smartly across your fingers with a part of his robe.\n"
          );
    this_player()->SetExplored();
    return 1;
  }
}

/***************************************************************************/



