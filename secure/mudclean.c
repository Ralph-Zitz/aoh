/* /secure/mudclean
**
** Find and destruct old objects.
** This is a hotted up version of the /secure/destructor.
** The object is called by the GD via master::quota_demon() and ::daily_jobs(),
** but may be used directly by arches as well.
**
**   17-Mar-96 [mateese]
**   21-Jun-96 [mateese] Added one initial call to garbage_collection(),
**     to get some more mem to work in.
**     The internal array is derefd after use, so the final gc can reclaim
**     it as well.
**   30-Jul-96 [mateese] For experiments, added a MODEST variant, selected
**     by preprocessor. For now, it just destructs netdeads in the void.
**   01-Aug-96 [mateese] Blueprints with a filedate newer than the programtime
**     are gathered as well, even in MODEST mode.
**   06-Aug-96 [mateese] MODEST handling modified so again every object is
**     considered, but only clean_up() is called.
**   02-Nov-96 [mateese] MODEST handling is now a runtime option and selected
**     depending on the time since the last non-modest cleanup.
**   22-Apr-97 [deepthought] returns immediately (no mudclean)
*/

#include <secure/config.h>
#include <secure/wizlevels.h>

#define LOGFILE  "GAME_LOG"
#define SAVEFILE "/secure/save/.mudclean"

#define PO previous_object()
#define TO this_object()
#define TP this_player()
#define TI this_interactive()

// #define DEBUG

#ifdef DEBUG
#define DTELL(a) dtell("mateese", a)
#else
#define DTELL(a)
#endif

/*------------------------------------------------------------------------*/

int last_run;       /* Time of last run */
int last_thorough;  /* Time of last non-modest run */

/*------------------------------------------------------------------------*/

nosave int     bmodest;  // 0 if not modest, 1 if modest
nosave string  modest;   // "" if not modest, "modest " if modest
nosave int     idcode;   // code to tag the objects already visited

nosave int state;        // State of the destructor:
#define STATE_NONE   0   //   Destructor is not working.
#define STATE_START  1   //   Destructor has just been started.
#define STATE_START2 2   //   Destructor is initialized.
#define STATE_GATHER 3   //   Destructor is gathering interesting objects.
#define STATE_CLEAN  4   //   Destruct the gathered objects.
#define STATE_GC     5   //   Schedule the garbage_collection().
#define STATE_END    6   //   Destructor done.

nosave int age;  // Max. age of the objects to destruct

nosave mixed * objs;       // Array of array of objs
#define NUMOBJS  2000      // Number of objects per array.
nosave int     numobj;     // Number of objects to consider.
nosave int     numsecobj;  // Number of /secure-objects.
nosave int     numinter;   // Number of interactive objects.
nosave int     numyoung;   // Number of too-young objects.
nosave int     numupdate;  // Number of objects to update.

nosave int     numcarr;    // Number of carried objects.
nosave int     numvic;     // Number of objects near a player.
nosave int     numhold;    // Number of objects holding a player.
nosave int     numnegl;    // Number of objects neglecting destruction.
nosave int     numdest;    // Number of objects destructed.
nosave int     numrun;     // Number of runs needed.
nosave int     numhb;      // Number of heartbeats needed.

nosave object  lastobj;    // Last object considered in gather()
nosave int     diddest;    // Flag if clean() did destruct an object.
nosave int     lastix;     // Index of next object to consider in clean()

/*------------------------------------------------------------------------*/

void create() {
  seteuid(getuid(TO));

  if (-1 != member(object_name(TO), '#')
   || object_name(TO)+".c" != __FILE__
     ) {
    log_file (LOGFILE,
 ctime(time())+": mudclean: Illegal attempt!\n"
+"  this:        "+object_name(TO)+" ["+getuid(TO)+"]\n"
+"  previous:    "+(PO ? object_name(PO)+" ["+getuid(PO)+"]" : "<none>")+"\n"
+"  player:      "+(TP ? object_name(TP)+" ["+getuid(TP)+"]" : "<none>")+"\n"
+"  interactive: "+(TI ? object_name(TI)+" ["+getuid(TI)+"]" : "<none>")+"\n\n"
             );
    destruct (TO);
    return;
  }
  bmodest = 1;
  modest =  "modest ";
  state = STATE_NONE;
}

/*------------------------------------------------------------------------*/
void start_dragon(int arg, int mmode)

// Start the destruction of unused objects.
// arg <= 0: min age of objects to keep is derived from the time of the
//           last run.
// arg >  0: min age of objects to keep is computed as 'arg-1' hours.
// mmode > 0: do a modest run
// mmode = 0: autoselect modest mode or not
// mmode < 0: do a thorough run

{
  int tmp;
  return;
  if ((     PO != find_object(MASTER)
        || getuid(TO) != ROOTID
        || -1 != member(object_name(TO), '#')
	|| object_name(TO)+".c" != __FILE__
      )
      && (!TI || !IS_WIZARD(TI))
     ) {
    log_file (LOGFILE,
 ctime(time())+": mudclean:start_dragon(): Illegal attempt!\n"
+"  this:        "+object_name(TO)+" ["+getuid(TO)+"]\n"
+"  previous:    "+(PO ? object_name(PO)+" ["+getuid(PO)+"]" : "<none>")+"\n"
+"  player:      "+(TP ? object_name(TP)+" ["+getuid(TP)+"]" : "<none>")+"\n"
+"  interactive: "+(TI ? object_name(TI)+" ["+getuid(TI)+"]" : "<none>")+"\n\n"
             );
    destruct (TO);
    return;
  }

  if (state)
  {
    log_file (LOGFILE,
 ctime(time())+": mudclean:start_dragon(): Already working!\n"
+"  this:        "+object_name(TO)+" ["+getuid(TO)+"]\n"
+"  previous:    "+(PO ? object_name(PO)+" ["+getuid(PO)+"]" : "<none>")+"\n"
+"  player:      "+(TP ? object_name(TP)+" ["+getuid(TP)+"]" : "<none>")+"\n"
+"  interactive: "+(TI ? object_name(TI)+" ["+getuid(TI)+"]" : "<none>")+"\n\n"
             );
    return;
  }

  last_run = 0;
  last_thorough = 0;

  if (mmode > 0)
    bmodest = 1;
  else if (mmode < 0)
    bmodest = 0;

  restore_object(SAVEFILE);

  if (arg <= 0)
  {
    if (time() - last_run >= 3600)
      tmp = 6;
    else if (time() - last_run >= 1800)
      tmp = 4;
    else
      tmp = 2;
  }
  else
    tmp = arg-1;

  if (!mmode)
  {
    if (time() - last_thorough > 3600 * 24 * 7)
      bmodest = 0;
    else
      bmodest = 1;
  }

  modest = (bmodest ? "modest " : "");

  last_run = time();
  if (!bmodest)
    last_thorough = time();
  save_object(SAVEFILE);

  log_file(LOGFILE, ctime()+" "+modest+"mudclean started for age "+tmp+"\n");
  age = time() - tmp*3600;
  state = STATE_START;
  idcode = time();
  set_heart_beat(1);
}

/*------------------------------------------------------------------------*/
static void shout (string msg)
{
  filter(users(), #'tell_object /*'*/, msg);
}

/*------------------------------------------------------------------------*/
static object setobj (int index, object obj)

// Set object <obj> into objs[] at <index>.
// Index may be <numobjs>, thus extending the array.

{
  int arr, ix;

  arr = index / NUMOBJS;
  ix = index % NUMOBJS;
  if (arr == sizeof(objs))
    objs += ({ allocate(NUMOBJS) });
  objs[arr][ix] = obj;
  if (index >= numobj)
    numobj = index+1;
  return obj;
}

/*------------------------------------------------------------------------*/
static object getobj (int index)

// Get object <obj> from objs[] at <index> and return it.

{
  return objs[index / NUMOBJS][index % NUMOBJS];
}

/*------------------------------------------------------------------------*/
static int gather()

// Collect all cleanable objects in objs[].
// Return 1 if no more objects are to be collected.
// The functions judges just on object name, age and interactivity,
// but ignores inventory relations and similar.

{
  int mincost, fage;
  object o;
  string name;

  // Stop at max(50%, 100) eval cost left.
  mincost = get_eval_cost() / 20;
  if (mincost < 100)
    mincost = 100;

  for ( o = debug_info(2)
      ; o && get_eval_cost() >= mincost
      ; o = debug_info(2, o)
      )
  {
    name = to_string(o);
    // Netdead things in the void are garbage.
    if (interactive(o) || (query_once_interactive(o) && environment(o)))
    {
      DTELL("Gather: "+to_string(o)+" is interactive.\n");
      numinter++;
      if (interactive(o) && ({int})o->Query("mudclean_id") != idcode)
      {
        tell_object(o,
"The Gamedriver hurries in, scrutinizes you and makes a note on his padd.\n"
+"He winks at you and frinks away.\n"
                   );
	o->Set("mudclean_id", idcode);
      }
    }
    else if (name[0..6] == "/secure" || o == TO)
    {
      numsecobj++;
      DTELL("Gather: "+to_string(o)+" is secure.\n");
    }
    else
    {
      setobj(numobj, o);
      DTELL("Gather: "+to_string(o)+" gathered.\n");
    }
  }

  lastobj = o;
  return !o;
}

/*------------------------------------------------------------------------*/
static int clean()

// Destruct all cleanable objects in objs[].
// Return 1 if no more objects are to be destructed.

{
  int mincost;
  object o, e;
  int isclone, rc;
  mixed tmp;

  // Stop at max(5%, 100) eval cost left.
  mincost = get_eval_cost() / 20;
  if (mincost < 100)
    mincost = 100;

  for (; lastix < numobj && get_eval_cost() >= mincost; lastix++)
  {
    o = getobj(lastix);
    if (!o)
    {
	/* Maybe it's a side effect of our actions, maybe someone else
	 * destructed it - anyway, it's gone.
	 */
      continue;
    }

    setobj(lastix, 0);

    if (!is_clone(o)
     && file_size(to_string(o)+".c") > 0
     && file_date(to_string(o)+".c") > program_time(o)
       )
    {
      numupdate++;
      DTELL("Clean: "+to_string(o)+" gets updated.\n");
    }
    else if (program_time(o) > age)
    {
      numyoung++;
      DTELL("Clean: "+to_string(o)+" is too young.\n");
      continue;
    }
#if 0
    else if (bmodest && (!query_once_interactive(o) || interactive(o) || environment(o)))
    {
      numnegl++;
      DTELL("Clean modest: "+to_string(o)+" no interactive in the void.\n");
      continue;
    }
#endif

    e = environment(o);
    if (e)
    {
      // Don't destruct objects carried by players.
      for ( ; e && !query_once_interactive(e); e = environment(e));
      if (e)
      {
        DTELL(to_string(o)+" carried.\n");
        numcarr++;
        continue;
      }

      // Don't destruct objects in the vicinity of players.
      if (sizeof(filter( all_inventory(environment(o))
                             , #'interactive)))
      {
        DTELL(to_string(o)+" around player.\n");
        numvic++;
        continue;
      }
    }

    // Don't destruct objects containing players.
    if ("/room/netdeath" == to_string(o)
    ||  sizeof(filter(deep_inventory(o), #'query_once_interactive /*'*/)))
    {
      DTELL(to_string(o)+" contains player.\n");
      numhold++;
      continue;
    }

    isclone = (0 <= member(to_string(o), '#'));

    DTELL("Destructing "+to_string(o)+".\n");

    catch(o->clean_up(2));
    rc = 1;
    if (!bmodest)
    {
      if (o /* && (!first_inventory(o) || isclone) */)
        catch(rc = call_resolved(&tmp, o, "remove", -1));
      // Netdead things in the void are garbage, too.
      // The gather() run won't pass us true interactives, so we
      // don't need to check for that.
      if (o && (!rc || (!environment(o) && query_once_interactive(o)))
            /* && (!first_inventory(o) || isclone) */
         )
        destruct(o);
    }
    if (!o)
    {
      numdest++;
      diddest = 1;
    }
    else
      numnegl++;
  }

  rc = (lastix >= numobj && !diddest);
  if (lastix >= numobj)
  {
    numrun++;
    if (diddest)
    {
      diddest = 0;
      lastix = 0;
    }
  }

  return rc;
}

/*------------------------------------------------------------------------*/
void heart_beat()

// Main state machine of the destructor.

{
  string errmsg;
  int    tmp;

  errmsg = 0;

  switch(state)
  {
    case STATE_NONE:
      errmsg = "Heartbeat in state NONE.";
      break;

    case STATE_START:
      shout(
"Gamedriver shouts: Oh, what a mess you made! If only I knew where I put\n"
"  my broom and dustpan...\n"
"The Gamedriver leaves into his tool shack.\n"
           );
      garbage_collection(); /* gives us some more memory to work in */
      objs = ({});
      numobj = 0;
      numsecobj = 0;
      numinter = 0;
      state = STATE_START2;
      break;

    case STATE_START2:
      shout(
"The Gamedriver reappears, wiping away some random webs sticking to his hair,\n"
"in his hands carrying a broom and a large dustpan (and a notepadd).\n"
"Gamedriver shouts: I'll clean this place up a bit - please watch your\n"
"  step meanwhile (it won't last long).\n"
"He grins at you and walks away, eagerly working his broom.\n"
           );
      state = STATE_GATHER;
      /* FALLTHROUGH */
    case STATE_GATHER:
      if (gather())
      {
        state = numobj ? STATE_CLEAN : STATE_GC;
        numcarr = 0;
        numvic = 0;
        numhold = 0;
        numnegl = 0;
        numdest = 0;
        numrun = 0;
        numhb = 0;
        numupdate = 0;
	numyoung = 0;

        diddest = 0;
        lastix = 0;

        tmp = numsecobj+numinter+numobj;
        log_file(LOGFILE,
ctime()+" "+modest+"mudclean: "+numobj+" of "+tmp+" objects gathered.\n"
+"  "+numsecobj+" game objs, "+numinter+" interactives.\n");
      }
      break;

    case STATE_CLEAN:
      numhb++;
      if (numhb % 2 && numhb / 2 < 2)
      {
        shout(
({ "You hear quiet puffs from everywhere as garbage disappears.\n"
 , "The fabric of space shivers and the air tingles.\n"
 })[numhb/2]
             );
      }
      if (clean())
      {
        state = STATE_GC;
        log_file(LOGFILE,
 ctime()+" "+modest+"mudclean: "+numdest+"("+numupdate+") objects destructed ("
        +numrun+" runs, "+numhb+" hbs).\n"
+"  "+(numcarr+numvic+numhold+numnegl+numyoung)+" left: "
     +numyoung+" too young, "+numcarr+" carried, "+numvic+" near pl, "
     +numhold+" around pl, "+numnegl+" persistant.\n"
                );
      }
      break;

    case STATE_GC:
      shout("For a short while, everything freezes.\n");
      objs = 0; /* so the GC can reclaim this memory as well */
      garbage_collection();
      state = STATE_END;
      break;

    case STATE_END:
      shout(
"Finally the Gamedriver puts the broom into the pan, waves merrily and\n"
"strolls away.\n"
"You feel your body regaining its agility.\n"
           );
      set_heart_beat(0);
      destruct(TO);
      break;

    default:
      errmsg = "Illegal state "+state+".\n";
      break;
  }

  if (errmsg)
  {
    log_file (LOGFILE,
 ctime(time())+": "+modest+"mudclean:heart_beat(): "+errmsg+"\n"
+"  this:        "+object_name(TO)+" ["+getuid(TO)+"]\n"
+"  previous:    "+(PO ? object_name(PO)+" ["+getuid(PO)+"]" : "<none>")+"\n"
+"  player:      "+(TP ? object_name(TP)+" ["+getuid(TP)+"]" : "<none>")+"\n"
+"  interactive: "+(TI ? object_name(TI)+" ["+getuid(TI)+"]" : "<none>")+"\n"
+"  Dragon destructed.\n\n"
             );
    set_heart_beat(0);
    destruct(TO);
  }
}

/*========================================================================*/
