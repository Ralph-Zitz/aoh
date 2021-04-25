/* Boozon
**
** A autochanging food object.
**
**   14-Feb-92 [m]
**   14-Jun-92 [m] Adapted for native mode.
**
**  [m] Mateese
*/

#include "/players/mateese/mydefs.h"
#include <properties.h>

inherit "/std/thing";

#define LOGFILE MPL+"boozon"
#define VERSION "920614"

static string name, alias, dscr;
static string boozemsg, boozermsg;
static int full;
static int weight, value, type, strength, detox;
#define SOFT  0 /* Softdrink */
#define DETOX 1 /* detoxicating softdrink */
#define BOOZE 2 /* Booze */
#define FOOD  3 /* Food */
  /* if not detoxi-softdrink, strength is also heal */
static string selections; /* Array of possible selections */
#define S_NAME   0
#define S_ALIAS  1
#define S_DSCR 2  /* Also used in the offers list */
#define S_SHORT  3
#define S_LONG   4
#define S_DMSG   5
#define S_DRMSG  6
#define S_WEIGHT 7
#define S_VALUE  8
#define S_TYPE   9
#define S_STREN 10
#define S_DETOX 11
#define S_INCR   12

_resetme() {
  Set(P_IDS, ({"object"})); AddId("boozon");
  SetShort ("a boozon");
  SetLong (
"A boozon, the mythical elementary particle\n"+
"out of which every good booze is made.\n"+
"Maybe you can transform it into something delicious?\n"+
"\n"+
"You can 'zone <what>', 'eat [<what>]', 'drink [<what>]' or\n"+
"list the 'offers [detail]'.\n"
         );
  SetValue(0);
  SetWeight(0);
  full = 0;
}

create() {
  thing::create();
  _resetme();
  selections = ({
 "beer", 0, "First Class Beer",
    "A bottle of beer", "A bottle of good beer.\n",
    0, "That feels good.\n",
    1, 6, BOOZE, 2, 0
,"guiness", 0, "Pint of Guinness",
    "A pint of Guiness", "A pint of real Guiness.\n",
    0, "That feels good.\n",
    1, 8, BOOZE, 4, 0
,"special", 0, "Special of the House",
    "A Special of the House", "A Special in a special glass.\n",
    0, "A tingling feeling goes through your body.\n",
    1, 50, BOOZE, 10, 0
,"firebreather", "fire", "Firebreather",
    "A glass of Firebreather", "A glass glowing with a Firebreather.\n",
    0, "A shock wave runs through your body.\n",
    1, 140, BOOZE, 15, 0
,"zombiekiller", "zombie", "Zombiekiller",
   "A steaming Zombiekiller", "A glass filled with a steaming Zombiekiller.\n",
    0, "Yeah! Where are those damn zombies ?!\n",
    1, 250, BOOZE, 30, 0
,"coffee", 0, "Cup of Coffee",
    "A cup of coffee", "A cup with really strong coffee.\n",
    0, "Yeech! That tastes awful!\n",
    1, 6, DETOX, 2, 2
,"chili", 0, "Chili con Carne",
    "A portion chili con carne", 
    "A plate with some steaming Chili con Carne.\n",
    0, "Woah, hot and zapped, as it should be.\n",
    1, 50, FOOD, 20, 0
,"mousse", 0, "Mousse'o'Chocolat",
    "A portion Mousse'o'Chocolat", "Some Mousse'o'Chocolat in a small bowl.\n",
    0, "Mhm, this slides down most pleasantly.\n",
    1, 10, FOOD, 10, 0
   });
}


init() {
   thing::init();
  add_action ("fset"    , "zone");
  add_action ("foffers" , "offers");
  add_action ("finspect", "inspect");
  add_action ("fdrink"  , "drink");
  add_action ("feat"    , "eat");
}


finspect (str) {
  if (!str) return 0;
  if (!id(str) && present (str, PL) != ME) return 0;
  write ("The boozon quirks and stretches a bit and then forms the words:\n");
  write ("  Boozon     ---    v"+VERSION+"\n");
  write ("  Biontic Magics & Darkness\n");  
  write ("Then it collapses back again to its former shape.\n");
  return 1;
}


fset (str) {
  if (!str) {
    notify_fail("What shall the boozon form ?\n");
    return 0;
  }
  str = lower_case(str);
  if (id(str)) {
    write ("Your boozon is that already.\n");
    return 1;
  }
  if (str == "boozon") {
    full = 0;
    write ("Your boozon sighs in relief and appears in its own shape.\n");
    say (PL_NAME+" holds a single boozon.\n");
    return 1;
  }
  set_boozon(str);
  write ("Your boozon multiplies and clusters into "+lower_case(dshort)+".\n");
  say (PL_NAME+" is shortly hidden in a cloud of boozons.\n");
  return 1;
}

set_boozon(what) {
  int i;
  if (!what || what == "boozon") {
    full = 0;
    return 1;
  }
  _resetme();
  for (i = 0; i < sizeof(selections); i += S_INCR) {
    if ( what == selections[i+S_NAME] || what == selections[i+S_ALIAS] ) {
      name     = selections[i+S_NAME];
      alias    = selections[i+S_ALIAS];
      dscr   = selections[i+S_DSCR];
      dshort   = selections[i+S_SHORT];
      dlong    = selections[i+S_LONG];
      boozemsg  = selections[i+S_DMSG];
      boozermsg = selections[i+S_DRMSG];
      weight   = selections[i+S_WEIGHT];
      value    = selections[i+S_VALUE];
      type     = selections[i+S_TYPE];
      strength = selections[i+S_STREN];
      detox    = selections[i+S_DETOX];
      break;
    }
  }
  if (i >= sizeof(selections)) {
    name     = what;
    alias    = what;
    dscr     = what;
    boozermsg = "Bleargh! Thats not quite that what you wanted!\n";
    weight   = 1;
    value    = 1+random(200);
    type     = random(4);
    strength = 6+random(10);
    detox    = 2;
    if (type == FOOD) { 
      boozemsg = PL_NAME+" eats some "+what+" and shakes in disgust.\n";
      dshort   = "A piece of "+what;
      dlong    = 
        "A piece of something almost, but not quite, entirely unlike "+
        what+".\n";
    }
    else {
      boozemsg = PL_NAME+" drinks some "+what+" and shakes in disgust.\n";
      dshort   = "A glass of "+what;
      dlong    = 
       "A glass filled with something almost, but not quite, entirely unlike "+
        what+".\n";
    }
  }
  AddId(name); if (alias) AddId(alias);
  if (!dshort) SetShort (name); else SetShort (dshort);
  if (!dlong) SetLong(dshort+".\n"); else SetLong (dlong);
  SetValue (value/2);
  SetWeight (weight*1000);
  if (!boozemsg) 
    if (type == FOOD) boozemsg = PL_NAME+" eats "+lower_case(dshort)+".\n";
    else              boozemsg = PL_NAME+" drinks "+lower_case(dshort)+".\n";
  if (!boozermsg) boozermsg = "Aah, that feels good.\n";
  full = 1;
  return 1;
}


foffers(str) {
  if (str && str != "detail") return 0;
  write_selections(str && str == "detail");
  return 1;
}

write_selections(detail) {
  int i, val;
  string sv1, sv2;
  write ("  Currently the boozon offers:\n");
  write (
"------------------------------------------------------------------\n");
  for (i = 0; i < sizeof(selections); i += S_INCR) {
    write ("    ");
    write (ladjust(capitalize(selections[i+S_DSCR]), 25));
    if (detail) {
      val = selections[i+S_TYPE];
      sv1 = ""+selections[i+S_STREN];
      sv2 = ""+selections[i+S_DETOX];
      if (val == FOOD) 
        write (" : food  "+radjust (sv1,2)+"    ");
      else if (val == SOFT) 
        write (" : soft  "+radjust (sv1,2)+"    ");
      else if (val == DETOX) 
        write (" : detox "+radjust (sv1,2)+", "+radjust (sv2,2));
      else if (val == BOOZE) 
        write (" : alc   "+radjust (sv1,2)+"    ");
      sv1 = ""+selections[i+S_VALUE];
      write (" : "+radjust (sv1,4)+" coins");
    }
    write ("\n");
  }
  write (
"------------------------------------------------------------------\n");
  write ("  If you don't find your favorite, don't panic. Try ordering it.\n");
}

ladjust (str, n) {
  return sprintf("%-*s", n, str);
}

radjust (str, n) {
  return sprintf("%*s", n, str);
}


fdrink (str) {
  int was_full;
  string last_dshort;

  was_full = full; last_dshort = dshort;
  if (str) str = lower_case(str);
  if (str == "boozon") str = 0;
  if (!full && !str) {
    write ("You can't drink a single boozon.\n");
    return 1;
  }
  if (full && !str) str = name;
  if (!full || !id(str)) {
    set_boozon(str);
    if (str == "boozon") {
      write ("You can't drink a single boozon.\n");
      return 1;
    }
  }
  if (!was_full || last_dshort != dshort) {
  write ("Your boozon multiplies and clusters into "+lower_case(dshort)+".\n");
  say (PL_NAME+" is shortly hidden in a cloud of boozons.\n");
  }
  if (type == FOOD) {
    write ("You can't drink "+lower_case(dshort)+"\n");
    return 1;
  }
  if (do_consume()) _resetme();
  return 1;
}

feat (str) {
  int was_full;
  string last_dshort;

  was_full = full; last_dshort = dshort;
  if (str) str = lower_case(str);
  if (str == "boozon") str = 0;
  if (!full && !str) {
    write ("You can't eat a single boozon.\n");
    return 1;
  }
  if (full && !str) str = name;
  if (!full || !id(str)) {
    set_boozon(str);
    if (str == "boozon") {
      write ("You can't eat a single boozon.\n");
      return 1;
    }
  }
  if (!was_full || last_dshort != dshort) {
  write ("Your boozon multiplies and clusters into "+lower_case(dshort)+".\n");
  say (PL_NAME+" is shortly hidden in a cloud of boozons.\n");
  }
  if (type != FOOD) {
    write ("You can't eat "+lower_case(dshort)+".\n");
    return 1;
  }
  if (do_consume()) _resetme();
  return 1;
}

do_consume() {
  if (type == FOOD && !PL->eat_food(strength)) {
    write ("You cannot eat that much!\n");
    return 0; 
  }
  else if ((type == SOFT || type == DETOX) && !PL->drink_soft(strength)) {
    write ("You can't possibly drink that much right now!\n");
    return 0; 
  }
  else if (type == BOOZE) {  /* booze */
    if (strength / 2 >= 12 && PL_LEVEL < 10) {
      write ("You sputter liquid all over the room.\n");
      say (PL_NAME+" tries "+dshort+" but coughs and sputters\n"+
           "all over you.\n");
      return 0;
    }

    if (strength / 2 >= 8 && PL_LEVEL < 5) {
      write ("You throw it all up.\n");
      say (PL_NAME+" tries to drink "+dshort+" but throws up.\n");
      return 0;
    }

    if(!PL->drink_alco(strength)) {
      write ("You can't drink that much!\n");
      return 0; 
    }
  }

  if (type != DETOX) PL->heal_self(strength);
  else PL->drink_alco(-detox);

  write (boozermsg);
  say (boozemsg);
  return 1;
}

