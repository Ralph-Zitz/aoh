/* /p/daemon/races/racemaster
*
* Provide the specific data about/of the races and perform control operations.
*
*  28-Sep-92 [Mateese]
*  01-Oct-92 [Mateese] Redone.
*  04-Oct-92 [Mateese] Stats are hardcoded now.
*  07-Oct-92 [Mateese] Skills, Spells are 'gone'. Again 20 levels under 1M XP.
*  12-Oct-92 [Mateese] XPfromLevel(), CheckAdvance(), CostsAdvance()
*  21-Oct-92 [Mateese] Copy of references in InitRace() fixed
*  20-Nov-92 [Mateese] Wizards can have private races
*  03-Dec-92 [Mateese] Ugly bug in InitRace() fixed
*  05-Aug-94 [Kelson] Added the ability to have an initialize function in
*          each race object that is called from InitRace()
*  07-Dec-94 [Mateese] Adapted to new XP scale.
*  19-Jan-95 [Mateese] RD_SIZE added.
*  06-Feb-95 [Mateese] RD_MAXSTAT added for experiments.
*  11-Apr-95 [Mateese] RestoreRace() also sets the P_SIZE of the player.
*  20-Jun-95 [Mateese] InitRace() setzt von visitors die move-messages um.
*  14-Jan-96 [Mateese] EquipPlayer() new;
*  28-Oct-97 [Mateese] RacePlural() new.
*  18-Dec-97 [Softbyte] RaceInfo new
*/

#include <races.h>
#include <attributes.h>
#include <secure/wizlevels.h>
#include <guild.h>

#define TP this_player()
#define TI this_interactive()

/*-------------------------------------------------------------------------*/

nosave object player;    /* For the local Pl_funs() */
nosave int    *xplimits;  /* Array of the xp scale limits */

/*-------------------------------------------------------------------------*/

void create() {
  mapping c;
  int i;

  if (member( object_name(this_object()), '#') != -1) {
    destruct (this_object());
    return;
  }
  seteuid(getuid());
  c = G_COSTS;
  xplimits = allocate(sizeof(c));
  for (i = sizeof(c); i > 0; i--)
    xplimits[i-1] = c[i];
}

/*-------------------------------------------------------------------------
 * mixed copy_value (mixed arg)
 *
 *   Return a copy of the given arg, while taking special care for arrays.
 */

mixed copy_value (mixed arg) {
  mixed *cp;
  int i;
  if (!pointerp(arg)) return arg;
  cp = ({});
  for (i = 0; i < sizeof(arg); i++) cp += ({ copy_value (arg[i]) });
  return cp;
}

/*-------------------------------------------------------------------------
 * int XPtoLevel (int xp)
 *   Convert the experience <xp> into an approbiate general level.
 * int XPfromLevel (int level)
 *   Convert the <level> into the min. experience associated.
 */

int XPtoLevel (int xp) {
  int i;

  for (i = 0; i < sizeof(xplimits); i++)
    if (xplimits[i] > xp)
      break;
  // Here i is one too high - but otoh the level scale starts at 1.
  return i;
}

int XPfromLevel (int level) {
  int xp;

  if (level < 1)
    return 0;
  if (level <= sizeof(xplimits))
    return xplimits[level-1];
  xp = xplimits[<1];
  level -= sizeof(xplimits);
  while (level--)
    xp += (xp * 7) / 100;
  return xp;
}

/*-------------------------------------------------------------------------
 * string *AllRaceNames ()
 *   Return the names of all know races.
 */

protected int cmp_str (string s1, string s2) { return s1 > s2; }
protected string strip_dotc (string fname) { return fname [0..<3]; }

string *AllRaceNames() {
  return sort_array (map (get_dir(RACEDIR+"*.c"), #'strip_dotc
                               , this_object())
                    , #'cmp_str, this_object());
}

/*-------------------------------------------------------------------------
 * object RaceObject (string race, [object pl])
 *   Search the race-object for given race.
 */

varargs object RaceObject (string race, object pl) {
  string fname;
  object rc;

  if (!stringp(race) || race == 0)
    return 0;
  if (!pl)
    pl = TP;
  if (sscanf(race,"%~s %~s") == 2) return 0;
  if (IS_WIZARD(pl)) {
    fname = WPATH+getuid(pl)+"/race/"+race;
    if (file_size(fname+".c") <= 0) fname = RACEDIR+race;
  }
  else fname = RACEDIR+race;
  if (file_size(fname+".c") <= 0) return 0;
  if (rc = find_object (fname)) return rc;
  call_other (fname, "???");
  return find_object (fname);
}

/*-------------------------------------------------------------------------
 * mixed *RaceData (string race, [object pl])
 *   Return the data for <race>.
 */

varargs mixed *RaceData (string race, object pl) {
  object rc;
  if (rc = RaceObject (race, pl))
    return copy_value(({mixed *})rc->RaceData());
  return 0;
}

/*-------------------------------------------------------------------------
 * string RaceDescr (string race, [object pl])
 *   Returns a descriptional string for <race>.
 */

varargs string RaceDescr (string race, object pl) {
  object rc;
  if (rc = RaceObject (race, pl))
    return ({string})rc->RaceDescr();
  return 0;
}

/*-------------------------------------------------------------------------
 * mixed *RacePlural (string race, [object pl])
 *   Return the plural name for <race>.
 */

varargs string RacePlural (string race, object pl) {
  object rc;
  mixed * data;

  if (!(rc = RaceObject (race, pl)))
    return lower_case(race)+"s";
  data = ({mixed *})rc->RaceData();
  if (!data || !stringp(data[RD_PLURAL]))
    return lower_case(race)+"s";
  return lower_case(data[RD_PLURAL]);
}
/*-------------------------------------------------------------------------
 * string RaceInfo (string race, [object pl])
 *   Returns a info|help string for <race>.
 */

varargs string RaceInfo (string race, object pl) {
  object rc;
  if (rc = RaceObject (race, pl))
    return ({string})rc->RaceInfo();
  return 0;
}


/*-------------------------------------------------------------------------
 * int CheckStat (object pl, string stat, int val)
 *   Check the given <stat> with value <val> for <pl> according
 *   to it's level and race against the max. allowed value, and return
 *   the lesser of both.
 */

int CheckStat (object pl, string stat, int val) {
  int at2;
  mixed *rdata;

  if (!(rdata = RaceData (({string})pl->QueryRace()))) return 0;

  at2 = XPtoLevel (({int})pl->QueryXP()) + rdata[RD_STAT][stat]
       + ({int})pl->QueryBonus(stat);
  if (val <= at2) at2 = val;
  if (mappingp(rdata[RD_MAXSTAT])
     && (val = rdata[RD_MAXSTAT][stat])
     && val <= at2)
    at2 = val;
  if (at2 < 1) at2 = 1;
  return at2;
}

/*-------------------------------------------------------------------------
 * int AdvanceCosts (string stat, int base, int delta)
 *   Return amount of experience an increase of <stat> from the current
 *   value <base> by <delta> has to cost.
 */

int AdvanceCosts (string stat, int base, int delta) {
  if (delta <= 0) return 0;
  return (XPfromLevel (base+delta) - XPfromLevel (base)) / 10;
}

/*-------------------------------------------------------------------------
 * int CheckAdvance (object pl, string stat, int delta)
 *   Check if the <pl>ayer has enough experience to advance this <stat>
 *   by the given <delta> value. Returns the costs in experience
 *   if advancing is ok, else zero.
 */

int CheckAdvance (object pl, string stat, int delta) {
  int at, costs, xp;

  at = ({int})pl->QueryAttr(stat) - ({int})pl->QueryBonus(stat) + delta;
  if (at != CheckStat (pl, stat, at)) return 0;
  costs = AdvanceCosts (stat, at-delta, delta);
  xp = ({int})pl->QueryXP();
  if (XPfromLevel(({int})pl->QueryLevel())+costs > xp) return 0;
  return costs;
}

/*-------------------------------------------------------------------------
 * void AdjustStats (object pl)
 *   Adjust the stats for <pl> according to it's level and race.
 */

void AdjustStats (object pl) {
  int level, i, at, at2;
  mixed *rdata, attr, attb;
  mixed stat, *statlist;

  if (!(rdata = RaceData (({string})pl->QueryRace()))) return 0;

  level = XPtoLevel (({int})pl->QueryXP());

  attr = ({mixed})pl->QueryStats() || ([]);
  attb = ({mixed})pl->QueryBoni() || ([]);
  if (pointerp(attb))
    attb = mkmapping(attb[0], attb[1]);

  if (pointerp(attr))
    raise_error(sprintf("Illegal call to AdjustStats(%O): attr is pointer\n"));
  else
  {
    statlist = STATS;
    for (i = 0; i < sizeof(statlist); i++) {
      stat = statlist[i];
      if (m_contains(&at, attr, stat))
      {
        if ( at
             <= (at2 = level + rdata[RD_STAT][stat] + attb[stat])
           ) at2 = at;
        if (at2 < 1) at2 = 1;
        if (at2 != at) attr[stat] = at2;
      }
    }
  }
  pl->SetStats (attr);
}

/*-------------------------------------------------------------------------*/
int EquipPlayer(object pl)

/* Clone the basic equipment for a player, depending on his race.
 * Return 0 on success, !0 on failure.
 */

{
  int res;
  object ro;

  if (!pl)
    pl = TP||TI||previous_object();
  if (!pl)
    return 1;
  ro = RaceObject(({string})pl->Query(P_RACE), pl);
  if (!ro)
    return 1;
  res = ({int})ro->EquipPlayer(pl);
  return res;
}

/*-------------------------------------------------------------------------
 * void InitRace (object pl, string race)
 *   Sets the <race> of the <pl> and also all inferred values.
 */

int InitRace (object pl, string race) {
  mixed *data;
  object race_obj;
  int con, itl, dex, str;

  if (!(data = RaceData (race,pl))) {
    if (({int})pl->QueryIsPlayer()) // Catch wrong inits of player objects
      log_file ("ILLEGAL", ctime(time())+(TI ? (" ("+getuid(TI)+")"):"")
                +" Illegal race '"+race+"'\n");
    return 0;
  }

  player = pl; /* for functions that are called via process_string() */

    /* If necessary, rescue old stat settings */
  if ("visitor" != ({string})pl->QueryRace())
    con = itl = dex = str = 1;
  else {
    if ((con = ({int})pl->QueryCon()) < 1) con = 1;
    if ((itl = ({int})pl->QueryInt()) < 1) itl = 1;
    if ((dex = ({int})pl->QueryDex()) < 1) dex = 1;
    if ((str = ({int})pl->QueryStr()) < 1) str = 1;
    pl->SetMsgIn(0);
    pl->SetMMsgIn(0);
    pl->SetMsgOut(0);
    pl->SetMMsgOut(0);
  }

  pl->SetAlign(data[RD_ALIGN]);
  pl->SetMaxFood(data[RD_FOOD]);
  pl->SetMaxDrink(data[RD_DRINK]);
  pl->SetMaxAlcohol(data[RD_ALC]);
  pl->SetWeight(data[RD_WEIGHT]);

  pl->SetHands(copy_value(data[RD_HANDS]));
  pl->SetLegs(copy_value(data[RD_LEGS]));
  pl->SetDefences(copy_value(data[RD_DEFENCES]));
  pl->SetHome(data[RD_HOME]);
  pl->SetLong(process_string(data[RD_LONG]));
  // if (!call_resolved(&tmp2, pl, "SetAttrs", mkmapping(tmp[0], tmp[1])))
  pl->SetAttrs(data[RD_ATTR]);
  pl->SetAttrs(data[RD_STAT]);
  pl->SetStats(data[RD_STAT]);
  pl->SetCon(con+data[RD_STAT][A_CON]);
  pl->SetInt(itl+data[RD_STAT][A_INT]);
  pl->SetStr(str+data[RD_STAT][A_STR]);
  pl->SetAgi(str+data[RD_STAT][A_AGI]);
  pl->SetQui(str+data[RD_STAT][A_QUI]);
  pl->SetWis(str+data[RD_STAT][A_WIS]);
  pl->SetCha(str+data[RD_STAT][A_CHA]);
  pl->SetDex(dex+data[RD_STAT][A_DEX]);
  pl->SetSize(data[RD_SIZE]);
  pl->SetRace(race);
  pl->cmd_title("the new "+capitalize(race));

  /* Some stats might have gone negative... */
  AdjustStats (pl);

  /* do any initialization stuff necessary for the race */
  if (race_obj = RaceObject(race))
    race_obj->InitRace(pl);

  return 1;
}

/*-------------------------------------------------------------------------
 * void RestoreRace (object pl)
 *   Restores the not-saved values of <pl>ayer which are concerned by its
 *   race.
 */

void RestoreRace (object pl) {
  mixed *data;
  string race;

  race = ({string})pl->QueryRace();
  if (!(data = RaceData (race))) race = "visitor";
  if (!(data = RaceData (race))) return;

  player = pl;

  pl->SetWeight (data[RD_WEIGHT]);
  pl->SetHands (copy_value (data[RD_HANDS]));
  pl->SetDefences (copy_value(data[RD_DEFENCES]));
  pl->SetLong (process_string(data[RD_LONG]));
  pl->Set(P_SIZE, data[RD_SIZE]);
}

/* For simple process_string()s, return the players name and gender_stuff */

string Pl_name() { return ({string})player->QueryName(); }
string Pl_pro() { return ({string})player->QueryPronoun(); }
string Pl_poss() { return ({string})player->QueryPossessive(); }
string Pl_obj() { return ({string})player->QueryObjective(); }

string Pl_Name() { return capitalize(({string})player->QueryName()||""); }
string Pl_Pro() { return capitalize(({string})player->QueryPronoun()||""); }
string Pl_Poss() { return capitalize(({string})player->QueryPossessive()||""); }
string Pl_Obj() { return capitalize(({string})player->QueryObjective()||""); }
