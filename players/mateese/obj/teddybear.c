/* A cute teddybear. Every Player should have one!                 22-Jan-92
**
** This teddybear is meant as an object to cuddle and it gives a certain
** feeling of security. This may be taken literally: if worn, the bear
** serves as an amulet.
** This item is auto-scattering.
*/

#include "/players/mateese/mydefs.h"

#include <properties.h>
#include <combat.h>

#define LOGFILE MPL+"teddy"

#define VERSION "920614"

inherit "/std/armour";

string longdesc;
string rname;

crate() {
  ::create();

  AddId ("teddy");
  AddId ("teddybear");
  AddId ("teddy");
  SetShort ("A cute teddybear");
  longdesc = "\nTouching it gives you a warm feeling of security.\n"+
             "You wonder, what it is good for.\n";
  SetLong ("This is a cute teddybear."+longdesc);
  Set(P_ARMOUR_TYPE, AT_AMULET);
  SetValue (200);
  SetWeight (500);
  Set(P_AC, 1);
  Set(P_INFO, "Mateese's teddybear, #"+VERSION);
}

init() {
  ::init();
  add_action ("fwear",    "wear");
  add_action ("fremove",  "remove");
  add_action ("fname",    "name");
  add_action ("finspect", "inspect");
  add_action ("fpet",     "touch");
  add_action ("fpet",     "pet");
  add_action ("fpet",     "stroke");
  add_action ("fcuddle",  "ruffle");
  add_action ("fcuddle",  "cuddle");
  add_action ("fcuddle",  "hug");
  add_action ("fpress",   "squeeze");
  add_action ("fpress",   "press");
  add_action ("fpress",   "push");
  add_action ("fplay",    "play");
  add_action ("ffeed",    "feed");
  add_action ("fadmire",  "admire");
  add_action ("fbounce",  "bounce");
  add_action ("fflip",    "flip");
  add_action ("ffondle",  "fondle");
  add_action ("fkiss",    "kiss");
  add_action ("flick",    "lick");
  add_action ("flove",    "love");
  add_action ("fnose",    "nose");
  add_action ("fpraise",  "praise");
}

/* Some special messages on armourlike usage. */

fwear (str) {
  if (str) str = lower_case(str);
  if (!::wear(str)) return 0;
  /* the following msg may be wrong in case another amulet is already worn,
  ** but it doesnt matter.
  */
  write ("You get a warm feeling of security.\n");
  return 1;
}

fremove (str) {
  if (str) str = lower_case(str);
  if (!::remove(str)) return 0;
  write ("You feel a bit less secure.\n");
  return 1;
}

/* This makes the description like "his teddy" or "<name>, her teddy" */

static makedesc() {
  string s;
  s = PL->query_possessive()+" teddy";
  if (rname) s = rname + ", " + s;
  return s;
}

/* Whats the player doing with us ? */

fname (str) {
  string n, v;

  if (!str) return 0;
  sscanf (str, "%s %s", v, n);
  if (!isitme(v, "What do you want to give a name?\n")) return 0;

  rname = n;
  if (n) {
    AddId (lower_case(n));
    SetShort (n+", a cute teddybear");
    SetLong (n+", a cute teddybear."+longdesc);
  } else {
    SetShort ("A cute teddybear");
    SetLong ("This is a cute teddybear."+longdesc);
  }
  return 1;
}

finspect (str) {
  if (!isitme(str, "What do you want to inspect?\n")) return 0;
  write ("A small sign reads:\n"+
         "  Cute Teddybear -- #"+VERSION+"\n"+
         "  Biontic Magics & Darkness\n"
        );
  return 1;
}

fadmire (str) {
  if (!isitme(str, "Admire what?\n")) return 0;
  say (PL_NAME+" "+query_verb()+"s "+makedesc()+".\n");
  write ("What a nice teddy you have!\n");
  return 1;
}

fbounce (str) {
  if (!isitme(str, "Bouncing what?.\n")) return 0;
  say (PL_NAME+" "+query_verb()+"s with "+makedesc()+".\n");
  write ("You bounce your teddy around the room.\n");
  return 1;
}

fflip (str) {
  if (!isitme(str, "Flip what?\n")) return 0;
  say (PL_NAME+" "+query_verb()+"s "+makedesc()+".\n");
  write ("You flip your teddy upside down.\n");
  return 1;
}

ffondle (str) {
  if (!isitme(str, "Fondle who?n")) return 0;
  say (PL_NAME+" tries to "+query_verb()+" "+makedesc()+".\n");
  write ("Funny idea.\n");
  return 1;
}

fkiss (str) {
  if (!isitme(str, "Kiss who?\n")) return 0;
  say (PL_NAME+" "+query_verb()+"es "+makedesc()+".\n");
  write ("You give your teddy a soft kiss.\n");
  return 1;
}

flick (str) {
  if (!isitme(str, "Lick who?\n")) return 0;
  say (PL_NAME+" "+query_verb()+"s "+makedesc()+".\n");
  write ("Yeech! You get a mouthful of teddy fluff\n");
  return 1;
}

flove (str) {
  if (!isitme(str, "Love who?\n")) return 0;
  say (PL_NAME+" really"+query_verb()+"s "+makedesc()+".\n");
  write ("If nobody else would love you, this teddy does.\n");
  return 1;
}

fnose (str) {
  if (!isitme(str, "Nose who?\n")) return 0;
  say (PL_NAME+" "+query_verb()+"s "+makedesc()+".\n");
  write ("You rub your nose at you teddy. Wonderful!\n");
  return 1;
}

fpraise (str) {
  if (!isitme(str, "Praise who?\n")) return 0;
  say (PL_NAME+" "+query_verb()+"s "+makedesc()+".\n");
  write ("Your teddy is the best!\n");
  return 1;
}

fpet (str) {
  if (!isitme(str, "Touching the pure air isn't quite helpful.\n")) return 0;
  say (PL_NAME+" "+query_verb()+"s "+makedesc()+".\n");
  write ("You feel the warm teddy skin under your fingers.\n");
  return 1;
}

fcuddle (str) {
  if (!isitme(str, "It needs two for that.\n")) return 0;
  say (PL_NAME+" "+query_verb()+"s "+makedesc()+".\n");
  write ("Aaah, you feel more secure now.\n");
  return 1;
}

fpress (str) {
  if (!isitme(str, "What?\n")) return 0;
  say (PL_NAME+" "+query_verb()+"es "+makedesc()+".\nBAAAAH!\n");
  write ("BAAAAH!\n");
  return 1;
}

fplay (str) {
  string s;

  if (str && !isitme(str, "You can't play with that!\n")) return 0;
  say (PL_NAME+" has fun playing with "+makedesc()+".\n");
  s = "your teddy";
  if (rname) s = rname;
  write ("You play a while with "+s+". Wow, what fun!\n");
  return 1;
}

ffeed (str) {
  if (!isitme(str, "Who do you want to feed?\n")) return 0;
  say (PL_NAME+" ridiciously tries to feed "+PL->query_possessive()+
       " teddy.\n");
  write ("This is just a normal teddybear! You can't feed it!\n");
  return 1;
}

static isitme(str, errmsg) {
  if (!str) {
    notify_fail (errmsg);
    return 0;
  }
  if (present(lower_case(str), environment()) != ME) return 0;
  return 1;
}

/* This function is called from the reset-routine of our castle's scatter-room
** with its argument passed. We scatter ourselves around!
*/

query_auto_scatter() { return 1; }

auto_scatter (scroom, arg) {
  string msg;
  int rc, nrob, flen;

  msg = "A cute teddybear is here.\n";
  flen = 2000;
  nrob = 0;
  rc=scroom->lscatter (ME, "/players/mateese/room/home", 0, msg);
  if (rc == 0 || rc == -2 ) nrob++;
  return nrob;
}

