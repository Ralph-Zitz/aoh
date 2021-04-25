/* Plasma Gun  (Mateese, 19-Feb-92)
*/

#include <properties.h>
#include <combat.h>
#include <moving.h>
#include <secure/wizlevels.h>

inherit "/std/weapon";

#define ME this_object()
#define PL this_player()

#define WRITE(txt) tell_object(environment(),txt)

/*-------------------------------------------------------------------------*/

#define SHOT_ENERGY 75
#define SHOT_MINERG 20
#define SHOTS       10
#define ERG_PER_SEC  5

static int lastcheck;
static int energy;

static string lastdir;
static mixed lasttarget;
static object aim_env, *aim_data;

/*-------------------------------------------------------------------------
** Initialize us.
*/

create () {
  ::create();
  SetStandard (WT_CLUB, 10);
  SetShort ("a plasma rifle");
  SetLong (
"You see a bunch of metal, shaped like a heavy rifle.\n"+
"An opening at one end show the typical burnings of plasma.\n"+
"Anyway, it's heavy enough to bludgeon unfriendly beings as well.\n"
          );
  SetValue (1000);
  SetWeight (3000);
  SetInfo ("Experimental Plasma Gun, by Mateese");
  SetIds (({ "gun" }));
  AddAdjective ("plasma");
  lastcheck = time();
  energy = SHOTS * SHOT_ENERGY;
  seteuid(getuid(ME));
}

init() {
  ::init();
  if (!IS_IMMORTAL(PL)) return;
  add_action ("faim", "aim");
  add_action ("fcheck", "check");
  add_action ("ffire",  "fire");
}

/*-------------------------------------------------------------------------
** Update energy load of the gun.
*/

static int update_energy() {
  energy += (time() - lastcheck) * ERG_PER_SEC;
  lastcheck = time();
  if (energy >= SHOTS * SHOT_ENERGY) {
    energy = SHOTS * SHOT_ENERGY;
    return 1;
  }
  if (energy < SHOT_ENERGY)
    WRITE ("Your gun buzzes alarmingly.\n");
  return 0;
}

static void _buzz() {
  if (update_energy()) {
    write ("Your gun turns silent.\n");
    return;
  }
  write ("Your gun whines.\n");
  call_out ("_reload", 20);
}

/*-------------------------------------------------------------------------*/

int fcheck (string arg) {
  object *o;

  if (!arg)
    return (int) notify_fail ("Check what?\n");
  if (lower_case(arg) != "load" && lower_case(arg) != "energy") {
    o = (object *) PL->search_me_env(arg);
    if (!o || !sizeof(o))
      return (int) notify_fail ("There is no "+arg+".\n");
    if (o[0] != ME) return 0;
  }
  write ("The gun");
  if (update_energy())
    write (" is fully loaded.\nIt");
  else if (energy < SHOT_MINERG) {
    write ("'s load is completely drained.\n");
    return 1;
  }
  write (" has energy for "+(energy / SHOT_ENERGY + !!(energy % SHOT_ENERGY))+
         " shots.\n");
  return 1;
  
}

/*-------------------------------------------------------------------------
** Parse and perform any aiming. Return success.
*/

void _aim (string arg, mixed target, string dir) {
  string foo, bar;

  if (!arg || arg == "gun") {
    if (lasttarget || lastdir) {
      target = lasttarget;
      dir = lastdir;
    }
  }
  else {
    if (sscanf (arg, "gun %s", foo)) arg = foo;
    if (sscanf (arg, "at %s %s", foo, bar) == 2) {
      target = foo; dir = bar;
    }
    else if (sscanf (arg, "at %s", foo)) 
      target = foo;
    else if (   sscanf (arg, "%s at %s", foo, bar) == 2
             || sscanf (arg, "%s %s", foo, bar) == 2) {
      target = bar; dir = foo;
    }
    else {
      dir = arg;
      target = lasttarget;
    }
  }

  if (aim_env != environment(PL) || !target || target != lasttarget 
      || dir != lastdir) {
    aim_env = environment(PL);
    aim_data = 0;
  }
}

/*-------------------------------------------------------------------------*/

int faim (string arg) {
  mixed target;
  string dir;
  object bolt;
  int s,i;

  if (environment() != PL) return 0;
  if (arg == "?")
    return 
     (int) notify_fail ("Usage: aim [gun] [<direction>] [[at] <target>]\n");

  if (!wielded)
    return (int) notify_fail("You need to wield your gun first.\n");

  _aim (arg, &target, &dir);

  if (!target) {
    if (dir) {
      write ("You turn your gun "+dir+".\n");
      show ((string)PL->QueryName()+" turns "+(string)PL->QueryPossessive()+
            " gun "+dir+".\n", PL);
    }
    else {
      write ("You wave your gun.\n");
      show ((string)PL->QueryName()+" waves "+(string)PL->QueryPossessive()+
            " gun.\n", PL);
    }
    return 1;
  }

  write ("You aim your gun"+
         (stringp(dir) ? " "+dir : "") +
         (target ? " at "+(stringp(target) ? capitalize(target)
                                           : (string) target->QueryShort())
                 : "")+"");
  show ((string)PL->QueryName()+" aims "+(string)PL->QueryPossessive()+
         " gun"+
         (stringp(dir) ? " "+dir : "") +
         (target ? " at "+(stringp(target) ? capitalize(target)
                                           : (string) target->QueryShort())
                 : "")+".\n");

  bolt = clone_object("/players/mateese/obj/plasmabolt");
  bolt->move(environment(PL), M_SILENT);
  aim_data = (object *) bolt->aim (target, dir, SHOT_ENERGY);
  bolt->remove();
  
  if (!aim_data[0]) {
    lasttarget = 0;
    WRITE(", but you can't see a it.\n");
  }
  else {
    lasttarget = aim_data[0];
    s = sizeof(aim_data);
    WRITE (",\n and you see it ");
    if (aim_data[s-1] == environment(environment())) 
      WRITE ("right besides you.\n");
    else WRITE("in "+(string)aim_data[s-1]->QueryIntShort()+".\n");
  }
  return 1;
}

/*-------------------------------------------------------------------------*/

int ffire (string arg) {
  mixed target;
  string dir, olddir;
  object bolt, pl;
  int erg,i;

  if (environment() != PL) return 0;
  if (arg == "?")
    return 
     (int) notify_fail ("Usage: fire [gun] [<direction>] [[at] <target>]\n");

  if (!wielded)
    return (int) notify_fail("You need to wield your gun first.\n");

  olddir = lastdir;
  _aim (arg, &target, &dir);

  pl = PL;
  if (target) {
    if (!aim_data) {
      bolt = clone_object("/players/mateese/obj/plasmabolt");
      bolt->move(environment(PL), M_SILENT);
      aim_data = (object *) bolt->aim (target, dir, SHOT_ENERGY);
      bolt->remove();
    }
    if (!aim_data[0])
      WRITE ("You don't see any target!\n");
    target = aim_data[0];
  }

  update_energy();

  if (energy < SHOT_MINERG) {
    write ("You pull the trigger, but nothing happens.\n");
    show (({(string)pl->QueryName()+" pulls the trigger of "+
            (string)pl->QueryPossessive()+" gun, but nothing happens.\n", 
          "You hear a hard 'click'.\n"}), pl);
    return 1;
  }

  WRITE ("Your gun rocks hard as you fire a plasma bolt"+
         (dir ? " "+dir:"")+
         (target ? " at "+
          capitalize((objectp(target)?(string)target->QueryShort():target))
                 : ""
         )+".\n");
  if (olddir == dir)
    say ((string)pl->QueryName()+
         " fires a bright bolt of pure energy"+
         (dir ? " "+dir : "")+".\n", pl);
  else
    say ((string)pl->QueryName()+" turns "+
         (string)pl->QueryPossessive()+" gun "+
         (dir ? " "+dir : "")+
         " and fires a bright bolt of pure energy.\n", pl);

  erg = (energy >= SHOT_ENERGY) ? SHOT_ENERGY : energy;
  energy -= erg;
  if (find_call_out("_reload") == -1) {
    WRITE ("Your gun emits a faint but high pitched whine.\n");
    call_out ("_reload", 20);
  }
  bolt = clone_object("/players/mateese/obj/plasmabolt");
  bolt->move(environment(pl), M_SILENT);
  bolt->fire (dir, aim_data, erg, pl);
  return 1;
}

/*************************************************************************/
