/*
** /std/guild/stats
** The handling of the stats
**
**   10feb95 [t] Creation
**   17apr96 [t] Stat-Calculation changed
**   07may96 [t] AdvanceCost-Calculation changed
**   09feb97 [e] Race specific MaxStat correction
**   05apr97 [e] Bugfix in Race data modification
**   26oct97 [a] Support new attribute-handling with base attributes
**
** [t] Thragor
** [e] Eilan
** [a] Analena
*/

#include <attributes.h>	// A_
#include <guild.h>
#include <races.h>	// RD_STAT
#include <config.h>	// RACEMASTER, NOTIFY_

#define ME	this_object()
#define TP	this_player()
#define STR	"/obj/lib/string"
#define SF(x)	#'x //'

inherit "/obj/lib/number";

private static mapping Pminstats,    // e. g. ([A_CON:1,A_STR:3,A_DEX:5,A_INT:1])
                       Pmaxstats,    // see minstats
                       Pmaxhlpstats, // max. for HLPs
                       Pskills;      // ([<skill>:<skill-startvalue>;
                                     // <skill-startlevel>])

public mapping GuildQuests();		// /std/guild/guildquests
public int GuildQuestLevel(object pl);	// /std/guild/guildquests
public int QueryLevelCost(int lvl);	// /std/guild/level

public varargs mapping QueryMaxStats(object pl);		// forward
public varargs int QueryMaxStat(string stat,object pl);		// forward
public varargs int QueryMaxHLPStat(string stat,object pl);	// forward

/*
** Stat related help-functions
** ===========================
*/

/*
** map_max:
** Returns the key of a mapping with the highest value.
*/

mixed map_max(mapping m)
{
  mixed key, *keys;
  int i;

  if (!mappingp(m) || !sizeof(keys = m_indices(m)))
    return 0;
  key = keys[<1];
  for (i=sizeof(keys)-1; i--; )
    if (m[keys[i]] > m[key])
      key = keys[i];
 return key;
}

/*
** get_race_modification:
** Return the race-specific modification of the attribute <stat>.
*/

protected int get_race_modification(object pl,string stat)
{
  mixed stats;
  int i;

  if (stats = ({mixed})RACEMASTER->RaceData(({string})pl->QueryBaseAttr(A_RACE)))
  {
    i=member(stats[RD_STAT][0],stat);
    return ( i==-1 ? 0 : stats[RD_STAT][1][i] );
  }
  return 0;
}

/*
** CalcFreeXP:
** Calculate the amount of XP the player may spend on stats (that is
**   the amount of XP above those needed for the current level)
*/

protected int CalcFreeXP(object pl)
{
  int tmp;

  if (!pl) return 0;
  if ((tmp=({int})pl->QueryBaseAttr(A_LEVEL)) < 0)
    pl->ChangeAttr(A_LEVEL,-tmp);
  return (({int})pl->QueryXP() - QueryLevelCost(({int})pl->QueryLevel()));
}

/*
** CalcAdvanceCost:
** Calculate the amount of xp the player <pl> has to spend to
**   advance the stat <stat>, where stat is one of the attributes
**   A_CON, A_STR, A_DEX or A_INT.
** The cost will be calculated by the following procedure:
**   * if the base-value of the attribute in question is below 1 it
**     will be set to one (stats may temporarily be below 1 through
**     modifications though)
**   * the basic cost is equal to 1/3 of the cost for advancing a level.
**     As the cost to reach level 2 is much higher than to advance from
**     level 2 to 3, a hack is included to reduce the cost for advancing
**     a stat to a value of 2 ;)
**   * the race-modifier of the attribute in question is next included
**     by multiplying the value with (15 - modifier)/15.
**     This way, a negative modifier on a stat will make advancing that
**     stat more expensive (e.g. a giant with a modifier of -10 on
**     intelligence will pay 166% of what a human will pay for advancing
**     intelligence in the same guild) while a positive modifier will
**     make advancing that stat cheaper (e.g. an oglon with a modifier of
**     +3 on dexterity will pay only 80% of what a human will pay).
**   * now the value is modified according to the stat's maximum value
**     in comparison to the guild's average maximum stat by a multiplier
**     of ( 2*sum_of_maxstats - 4*max_of_<stat> ) / sum_of_maxstats .
**     This way, advancing a stat the guild gives a bonus on is cheaper
**     than one that can't be raised up to the guild's average
**     stat-value (e.g. if a spellcaster guild has an average maxstat
**     of 20 and allows advancing intelligence to 25, advancing
**     intelligence will only cost 75% of what advancing a stat with
**     maximum 20 would cost).
**   * finally, the calculated cost is returned rounded to an integer
**     value.
*/

public int CalcAdvanceCosts(object pl,string stat)
{
  float val;
  int tmp;

  tmp = ({int})pl->QueryBaseAttr(stat);
  if (tmp < 1)
  {
    pl->ChangeAttr(stat,1-tmp);
    tmp = 1;
  }

  val = (QueryLevelCost(tmp+1) - (QueryLevelCost(tmp)||600))/3.0;
  // the '600' is a little hack, so that advancing a stat from 1 to 2
  // doesn't cost more than a stat from 2 to 3

  val = val * (15.0 - get_race_modification(pl, stat)) / 15.0;

  tmp = sum(m_values(QueryMaxStats()));
  val = val * ( 2 * tmp - 4 * QueryMaxStat(stat)) / tmp;

  return round(val);
}

public string CalcMinStat(object pl)
// Calculate the minimal stat of a player and return it (P_CON, ...)
// this is used to check, whether the player may advance a stat or
// not (e. g. a race with Str + 4 might advance Strength to the
// MaxStat for Strength of the Guild plus 4.
{
  string res;
  closure maxfun;
  int i;
  int rval, rmod;
  int sval, smod;

  maxfun =   ({int})pl->QueryBaseAttr(A_LEVEL)<20
	   ? symbol_function("QueryMaxStat",ME)
           : symbol_function("QueryMaxHLPStat",ME);
  for (i=sizeof(STATS); i--; )
  {
    sval = ({int})pl->QueryBaseAttr(STATS[i]);
    smod = (get_race_modification(pl,STATS[i]) + 1) / 2;
      // the modification for the currently checked stat
/* Disabled calculation due to a bug, Thragor, 16oct96
      if (   !res // if no minimum exists up to now
          || (   pl->Query(res)-rmod>pl->Query(STATS[i])-smod
              && !(pl->Query(STATS[i])>=apply(maxfun,STATS[i],pl))
             )
*/
    if (   ((rval - rmod) > (sval - smod))
	&& (sval < (apply(maxfun,STATS[i],pl) + smod))
       )
    {
      res = STATS[i];
      rval = sval;
      rmod = smod;
    }
  }
  return res;
}

/*
** check_level2stats:
** Return true if the average of a player's stats (without race-modifications)
**   is lower than his or her level plus a certain modifier (that is he/she
**   may still advance stats).
*/

protected int check_level2stats(object pl,int mod)
{
  int i, stat_sum, tmp;

  if (!pl)
    return 0;
  for (i=sizeof(STATS); i--; )
  {
    tmp =  2 * ({int})pl->QueryBaseAttr(STATS[i]) - get_race_modification(pl,STATS[i]);
    stat_sum += ( tmp<2 ? 2 : tmp );
  }
  return ((stat_sum + 4) / 8) < (({int})pl->QueryBaseAttr(A_LEVEL) + mod);
}

public int CanAdvance(object pl,string stat)
// Check if the player can advance the current stat (for one point)
// The results are defined in <guild.h>:
// ADV_NOT_XP, ADV_NOT_LEVEL, ADV_OTHER_FIRST,
// ADV_TOO_HIGH, ADV_OK
{
  int sval, mval;
  string minstat;
  closure maxfun;

  maxfun = ({int})pl->QueryLevel()<20?symbol_function("QueryMaxStat",ME)
                              :symbol_function("QueryMaxHLPStat",ME);
  if (({int})pl->QueryBaseAttr(stat) >= apply(maxfun,stat,pl))
  {
    notify_fail("You can't advance it any further.\n",
		NOTIFY_NOT_VALID);
    return ADV_TOO_HIGH;
  }
  if (!check_level2stats(pl,2))
  {
    notify_fail("Your level is not high enough.\n",NOTIFY_NOT_VALID);
    return ADV_NOT_LEVEL;
  }
  if (minstat = CalcMinStat(pl))
  {
    sval = ({int})pl->QueryBaseAttr(stat) - (get_race_modification(pl,stat)+1)/2;
    mval = ({int})pl->QueryBaseAttr(minstat) - (get_race_modification(pl,minstat)+1)/2;
    if ( (sval - mval) > 2 )
    // is the difference between the minimal stat and the stat you
    // want to advance too high?
    {
      notify_fail("You are not "+
          ([A_INT: "intelligent",
            A_DEX: "dexterous",
            A_STR: "strong",
            A_CON: "tough"])[minstat]+" enough.\n",
          NOTIFY_NOT_VALID);
      return ADV_OTHER_FIRST;
    }
  }
  if (CalcFreeXP(pl) < CalcAdvanceCosts(pl,stat))
  {
    notify_fail("You don't have enough XP to advance your "+
	([A_INT: "intelligence",
	  A_DEX: "dexterity",
	  A_STR: "strength",
	  A_CON: "constitution"])[stat]+".\n",
	NOTIFY_NOT_VALID);
    return ADV_NOT_XP;
  }
  return ADV_OK;
}

public int AdvanceStat(object pl,string stat)
// Try to advance the stat of a player and give the corrosponding
// message (ok or notify_fail ;-) )
{
  if (!pl)
    return 0;
  if (CanAdvance(pl,stat) != ADV_OK)
    return 0;
  pl->AddXP(-CalcAdvanceCosts(pl,stat));
  pl->ChangeAttr(stat,1);
  write("Ok.\n");
  return 1;
}

/*
** comment_cost:
** Select the comment that shows why you can't advance a certain stat
**   (in the AdvanceCostList).
**   Return values:  0 possible
**                   1 possible, but not now
**                  -1 not possible (reached maximum for level)
**   <comment> is given by reference and will return the string to print
*/

protected int comment_cost(object pl,string stat,string comment)
{
  switch(CanAdvance(pl,stat))
  {
    case ADV_TOO_HIGH:    comment = " not possible"; return -1;
    case ADV_OK:          comment = "   ";           return  0;
    case ADV_NOT_XP:      comment = "(x)";           return  1;
    case ADV_NOT_LEVEL:   comment = "(l)";           return  1;
    case ADV_OTHER_FIRST: comment = "(o)";           return  1;
    default:              comment = "   ";           return  1;
  }
  comment = "   ";
  return 1;
}

public string AdvanceCostList(object pl)
// return a printable list of Stat-Advancecosts
{
  string c, res, cost, *stats;
  int i;

  res = "";
  stats = sort_array(STATS,SF(<));
  for (i=sizeof(stats); i--; )
  {
    if (comment_cost(pl,stats[i],&c) == -1)
    {
      cost = c[1..];
      c = "";
    }
    else
      cost = to_string(CalcAdvanceCosts(pl,stats[i]));
    res += sprintf("  %s (%02d): %'.'20s %s\n",
		   stats[i],({int})pl->QueryAttr(stats[i])," "+cost,c);

  }
  return res;
}

public void PrintAdvanceCosts(object pl)
// the result of the 'cost'-command
// it will show you:
// 1. The QP and XP you have
// 2. The 'free' XP (not needed for the current level)
// 3. The list of stats and how much it costs to advance them
// 4. The level up to which you might advance
// 5. The QP you need to advance
// 6. The XP you need to advance
// 7. The next guildquest you need to advance
{
  int rlvl, lvl, qp, xp, tmp;
  string res;

  if (!pl) return;
  if ((tmp = GuildQuestLevel(pl)) < (rlvl = ({int})pl->QueryRealLevel()))
    rlvl = tmp;
  if ((lvl = ({int})pl->QueryBaseAttr(A_LEVEL)) < 0)
  {
    pl->ChangeAttr(A_LEVEL,-lvl);
    lvl = 0;
  }
  qp = ({int})QUESTMASTER->QueryQuestPoints(pl);
  xp = ({int})pl->QueryXP();
  res = sprintf("You are level %d.\n"
		"You have %d/%d QP and %d XP.\n",
		lvl, qp, ({int})QUESTMASTER->QueryTotalQuestPoints(), xp);
  if ((tmp = xp - QueryLevelCost(lvl)) > 0)
    res += sprintf("You have %d XP left to spend.\n", tmp);
  res += AdvanceCostList(pl);
  if (lvl < rlvl)
    res += sprintf("You may advance up to level %d.\n", rlvl);
  else
    if (lvl == max(m_indices(G_COSTS)))
      res += "You have the highest possible level.\n";
    else
    {
      if ((tmp=({int})QUESTMASTER->QPointLevel(lvl+1)) > qp)
        res += sprintf("You need %d (%d) QP to advance.\n", tmp-qp, tmp);
      if ((tmp = QueryLevelCost(lvl+1)-xp) > 0)
        res += sprintf("You need %d XP to advance.\n", tmp);
      if ((tmp = GuildQuestLevel(pl)) < lvl+1)
        res += ({string})STR->wrap(sprintf("You still have to solve the guildquest "
			         "\"%s\".\n",
				 GuildQuests()[tmp+1]), 50, 0);
    }
  tell_object(pl,res+"\n"
                 "(x) ... not enough XP to advance\n"
                 "(l) ... level not high enough to advance\n"
                 "(o) ... other stat is too low to advance this one\n");
}

protected void CorrectStats(object pl)
// Called when joining the guild. If the stats of the joining
// player are too high they are set to the maximum possible.
{
  int i, tmp;

  for (i=sizeof(STATS); i--; )
  {
    tmp = ({int})pl->QueryBaseAttr(STATS[i]) - QueryMaxHLPStat(STATS[i],pl);
    if (tmp > 0)
      pl->ChangeAttr(STATS[i],-tmp);
  }
}

/*
* ----------------------------------------------------------------------
* Queries, Settings, Adds
* ----------------------------------------------------------------------
*/

/*
* Stats
* =====
*/

public varargs mapping QueryMinStats(object pl)
{
  int i, tmp;
  string *s;
  mapping stats;

  stats = copy(Pminstats||G_MINSTATS);
  if (!pl)
    return stats;
  s = m_indices(stats);
  for (i = sizeof(s); i--; )
  {
    tmp = stats[s[i]] + (get_race_modification(pl,s[i])+1)/3;
    stats[s[i]] = ( tmp<1 ? 1 : tmp );
  }
  return stats;
}

public varargs int QueryMinStat(string stat,object pl)
{
  return QueryMinStats(pl)[stat];
}

public mapping SetMinStats(mapping stats)
// Set the minimum stats you need to enter this guild, e. g.
// ([P_CON:1,P_DEX:1,P_STR:1,P_INT:1])
// Purpose is to make it impossible for Newbies to enter a
// guild (except adventurers)
{
  if (   !mappingp(stats)
      || min(m_values(stats))<1
      || sizeof(stats)!=sizeof(STATS)
      || sizeof(m_indices(stats)-STATS)
      || sizeof(STATS-m_indices(stats))
     )
    return QueryMinStats();
  Pminstats = copy(stats);
  return QueryMinStats();
}

public mapping AddMinStat(string stat,int value)
// Add a minimum stat, e. g. AddMinStat(P_DEX,3)
{
  if ((value < 1) || (member(STATS,stat) == -1))
    return copy(Pminstats||G_MINSTATS);
  if (!Pminstats)
    Pminstats = G_MINSTATS;
  Pminstats[stat]=value;
  return copy(Pminstats);
}

public varargs mapping QueryMaxStats(object pl)
{
  string *s;
  int i, summe;
  mapping stats;

  stats = copy(Pmaxstats||G_MAXSTATS);
  if (!pl)
    return stats;
  summe=0;
  s = m_indices(stats);
  for (i=sizeof(s); i--; )
  {
    stats[s[i]] += (get_race_modification(pl,s[i])+1)/2;
    summe += stats[s[i]];
  }
  if (summe > G_LLP_LIMIT)
    for ( i=(summe-G_LLP_LIMIT); i--; )
      stats[map_max(stats)]--;
  return stats;
}

public varargs int QueryMaxStat(string stat,object pl)
{
  return QueryMaxStats(pl)[stat];
}

public mapping SetMaxStats(mapping stats)
// Set the maximum stats you might get when
// you are Low-Level-Player;
// the following functions just work like those
// for the minstats
{
  if (   !mappingp(stats)
      || min(m_values(stats))<1
      || sizeof(stats)!=sizeof(STATS)
      || sizeof(m_indices(stats)-STATS)
      || sizeof(STATS-m_indices(stats))
     )
    return QueryMaxStats();
  Pmaxstats = copy(stats);
  return QueryMaxStats();
}

public mapping AddMaxStat(string stat,int value)
{
  if ((value < 1) || (member(STATS,stat) == -1))
    return copy(Pmaxstats||G_MAXSTATS);
  if (!Pmaxstats)
    Pmaxstats = G_MAXSTATS;
  Pmaxstats[stat] = value;
  return copy(Pmaxstats);
}

public varargs mapping QueryMaxHLPStats(object pl)
{
  string *s;
  int i, summe;
  mapping stats;

  stats = copy(Pmaxhlpstats||G_MAXSTATS);
  if (!pl)
    return stats;
  summe=0;
  s = m_indices(stats);
  for (i=sizeof(s); i--; )
  {
    stats[s[i]] += (get_race_modification(pl,s[i])+1)/2;
    summe += stats[s[i]];
  }
  if (summe > G_HLP_LIMIT)
    for ( i=(summe-G_HLP_LIMIT); i--; )
      stats[map_max(stats)]--;
  return stats;
}

public varargs int QueryMaxHLPStat(string stat,object pl)
{
  return QueryMaxHLPStats(pl)[stat];
}

public mapping SetMaxHLPStats(mapping stats)
// Set the maximum stats for the guild, a HLP
// might get.
{
  if (   !mappingp(stats)
      || min(m_values(stats))<1
      || sizeof(stats)!=sizeof(STATS)
      || sizeof(m_indices(stats)-STATS)
      || sizeof(STATS-m_indices(stats))
     )
    return Pmaxstats||G_MAXSTATS;
  Pmaxhlpstats = copy(stats);
  return QueryMaxHLPStats();
}

public mapping AddMaxHLPStat(string stat,int value)
{
  if ((value < 1) || (member(STATS,stat) == -1))
    return copy(Pmaxhlpstats||G_MAXSTATS);
  if (!Pmaxhlpstats)
    Pmaxhlpstats = G_MAXSTATS;
  Pmaxhlpstats[stat]=value;
  return copy(Pmaxhlpstats);
}

public varargs mapping QuerySkills(object pl)
{
  mapping skills;
  string *s;
  int i;

  skills = copy(Pskills||m_allocate(0,2));
  if (pl)
  {
    s = m_indices(skills);
    for (i=sizeof(s); i--; )
      if (skills[s[i],1] > ({int})pl->QueryBaseAttr(A_LEVEL))
        m_delete(skills,s[i]);
  }
  return skills;
}

public mapping SetSkills(mapping sk)
{
  Pskills = ( sk ? copy(sk) : 0 );
  return QuerySkills();
}

public varargs mapping AddSkill(string name,int value,int level)
// Add the skill <name>. The initial value when
// joining the guild is <value>.
// If using the guildobj this should be a percentage.
{
  if (!Pskills)
    Pskills = m_allocate(0,2);
  Pskills[name,0] = value;
  Pskills[name,1] = level;
  return copy(Pskills);
}

public void InitSkills(object pl)
{
  mapping skills;
  string *s, *new;
  int i;
  mixed tmp;

  if (!pl)
    return;
  skills = QuerySkills(pl);
  s = m_indices(skills);
  new = ({ });
  for (i=sizeof(s); i--; )
  {
    tmp = ({int})pl->QueryBaseAttribute(s[i]);
    if (   (!intp(tmp) || (tmp < skills[s[i],0]))
	&& (({int})pl->QueryBaseAttr(A_LEVEL) >= skills[s[i],1])
       )
    {
      new += ({ s[i] });
      pl->ChangeAttribute(s[i], skills[s[i],0] - (intp(tmp) ? tmp : 0) );
    }
  }
  if (sizeof(new))
    tell_object(pl,implode_list(new,
		   "You learn the skill"+(sizeof(new)>1 ? "s " : " ")));
}
