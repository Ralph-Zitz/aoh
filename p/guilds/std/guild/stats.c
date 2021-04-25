/*
** /std/guild/stats
** The handling of the stats
**
**   10feb95 [t] Creation
**   17apr96 [t] Stat-Calculation changed
**   07may96 [t] AdvanceCost-Calculation changed
**   09feb97 [e] Race specific MaxStat correction
**   05apr97 [e] Bugfix in Race data modification
**
** [t] Thragor
** [e] Eilan
*/

#include <guild.h>
#include <races.h>     // RD_STAT
#include <config.h>    // RACEMASTER, NOTIFY_

#define ME this_object()
#define TP this_player()
#define STATS ({P_CON,P_STR,P_DEX,P_INT})
#define STR "/lib/string"
#define SF(x) #'x //'

inherit "/lib/number";

private static mapping Pminstats,    // e. g. ([P_CON:1,P_STR:3,P_DEX:5,P_INT:1])
                       Pmaxstats,    // see minstats
                       Pmaxhlpstats, // max. for HLPs
                       Pskills;      // ([<skill>:<skill-startvalue>;
                                    // <skill-startlevel>])

/*
* Prototypes from /std/guild/member.c
*/

/*
** Stat related help-functions
** ===========================
*/

protected int get_race_modification(object pl,string stat)
// Return the modification from the Racefile, so we may
// check this for the guild, too.
{
  mixed stats;
  int idx;
  if (!stats = RACEMASTER->RaceData(pl->QueryRace()))
    return 0;
  stats = stats[RD_STAT];
  idx=member(stats[0],stat);
  if (idx < 0) return 0;
  return stats[1][idx];
}

protected int CalcFreeXP(object pl)
// How much XP may the player spend on Stats
{
  if (pl->QueryLevel()<0) pl->SetLevel(0);
  return pl->QueryXP()-ME->QueryLevelCost(pl->QueryLevel());
}
  
public int CalcAdvanceCosts(object pl,string stat)
// return the amount of xp the player <pl> has to spend to advance
// the stat <stat>, where stat is P_CON, P_STR, P_DEX or P_INT
// the amount is 1/3 * (xp for level (==stat+1) - xp for level (==stat))
// then it is modified by the race-stats and the MaxStats of the
// guild. So an intelligent race in a spellcaster-guild has to pay
// very few XP for advancing Int, but a weak race in a cleric-guild
// has to pay very much XP for advancing Str.
{
  float val,percent;
  int racemod,newcost,oldcost;
  if (pl->Query(stat)<1) pl->Set(stat,1);
  // get the costs for the LEVEL (to pay the costs for a stat
  // will be 1/3 of it).
  newcost = ME->QueryLevelCost(pl->Query(stat)+1);
  oldcost = ME->QueryLevelCost(pl->Query(stat))||600;
  // the '600' is a little hack, so that advancing a stat from 1 to 2
  // doesn't cost more than a stat from 2 to 3
  val = (newcost-oldcost)/3.0;
  val -= get_race_modification(pl,stat)*val/15.0;
  // modify the xp you have to pay by the race-modifications
  percent = (ME->QueryMaxStat(stat)*100.0)/
    a_avarage(m_values(ME->QueryMaxStats()));
  percent = 100.0-percent;
  val = val+(val*percent/100.0);
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
  int i,rmod,smod;
  maxfun = pl->QueryLevel()<20?symbol_function("QueryMaxStat",ME)
                              :symbol_function("QueryMaxHLPStat",ME);
  for (i=0;i<sizeof(STATS);i++)
    {
      smod = round(get_race_modification(pl,STATS[i])/2.0);
      // the modification for the currently checked stat
      if (res) rmod = round(get_race_modification(pl,res)/2.0);
      // if we have alread a minimal stat, get the modification
      // which belongs to this stat
/* Disabled calculation due to a bug, Thragor, 16oct96
      if (   !res // if no minimum exists up to now
          || (   pl->Query(res)-rmod>pl->Query(STATS[i])-smod
              && !(pl->Query(STATS[i])>=apply(maxfun,STATS[i],pl))
             )
*/
      if (   pl->Query(res)-rmod>pl->Query(STATS[i])-smod
          && !(pl->Query(STATS[i])>=apply(maxfun,STATS[i],pl)+smod
         )
         )
       res = STATS[i];
     }
  return res;
}

protected int check_level2stats(object pl,int mod)
// Calculate if <pl> may advance the stats further
// depending on the avarage stats (race-modifications
// will be ignored) compared to the level of the player
{
  int i;
  float *res,val;

  i = sizeof(STATS);
  res = ({});
  while(i--)
    {
      val = pl->Query(STATS[i])-get_race_modification(pl,STATS[i])/2.0;
      if (val<1.0) val = 1.0;
      res+=({val});
    }
  return round(a_avarage(res))<pl->QueryLevel()+mod;
}

public int CanAdvance(object pl,string stat)
// Check if the player can advance the current stat (for one point)
// The results are defined in <guild.h>:
// ADV_NOT_XP, ADV_NOT_LEVEL, ADV_OTHER_FIRST,
// ADV_TOO_HIGH, ADV_OK
{
  int modify;
  string minstat,maxstat;
  closure maxfun;

  modify = get_race_modification(pl,stat);
  minstat = CalcMinStat(pl);
  maxfun = pl->QueryLevel()<20?symbol_function("QueryMaxStat",ME)
                              :symbol_function("QueryMaxHLPStat",ME);
  if (pl->Query(stat)>=apply(maxfun,stat,pl))
    return notify_fail("You can't advance it any further.\n",
		       NOTIFY_NOT_VALID),ADV_TOO_HIGH;
  if (!check_level2stats(pl,2))
    return notify_fail("Your level is not high enough.\n",
		       NOTIFY_NOT_VALID),ADV_NOT_LEVEL;
  if (minstat && (pl->Query(stat)-round(modify/2.0))-
      (pl->Query(minstat)-
       round(get_race_modification(pl,minstat)/2.0))>2)
    // is the difference between the minimal stat and the stat you
    // want to advance too high?
    {
      notify_fail("You are not "+
        (["Int":"intelligent",
          "Dex":"dexterous",
          "Str":"strong",
          "Con":"tough"])[minstat]+" enough.\n",
        NOTIFY_NOT_VALID);
      return ADV_OTHER_FIRST;
    }
  if (CalcFreeXP(pl)<CalcAdvanceCosts(pl,stat))
    return notify_fail("You don't have enough XP to advance "+stat+".\n",
		       NOTIFY_NOT_VALID),ADV_NOT_XP;
  return ADV_OK;
}

public int AdvanceStat(object pl,string stat)
// Try to advance the stat of a player and give the corrosponding
// message (ok or notify_fail ;-) )
{
  if (CanAdvance(pl,stat)!=ADV_OK) return 0;
  TP->AddXP(-CalcAdvanceCosts(pl,stat));
  TP->Set(stat,TP->Query(stat)+1);
  write("Ok.\n");
  return 1;
}

protected int comment_cost(object pl,string stat,string comment)
// needed to show why you can't advance a certain stat (in the
// AdvanceCostList
// results: 0  possible
//          1  possible, but not now
//         -1  not possible (maximum reached)
{
  int result;
  result = CanAdvance(pl,stat);
  comment = "   ";
  if (result==ADV_OK) return 0;
  if (result==ADV_TOO_HIGH)
    {
      comment = " not possible";
      return -1;
    }
  if (result==ADV_NOT_XP) comment = "(x)";
  else if (result==ADV_NOT_LEVEL) comment = "(l)";
  else if (result==ADV_OTHER_FIRST) comment = "(o)";
  return 1;
}

public string AdvanceCostList(object pl)
// return a printable list of Stat-Advancecosts
{
  string c,res,cost,*stats;
  int i;
  
  res = "";
  stats = sort_array(STATS,SF(>));
  for (i=0;i<sizeof(stats);i++)
    {
      if (comment_cost(pl,stats[i],&c)==-1)
	{
	  cost = "not possible";
	  c = "";
	}
      else
	cost = to_string(CalcAdvanceCosts(pl,stats[i]));
      res+=sprintf("  %s (%02d): %'.'20s %s\n",stats[i],pl->Query(stats[i]),
		   " "+cost,c);
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
  int rlvl,lvl,qp,xp,h;
  string res;
  if (!pl) return;
  rlvl = ((rlvl = pl->QueryRealLevel())>(h = ME->GuildQuestLevel(pl)))?h:rlvl;
  if ((lvl = pl->QueryLevel())<0) pl->SetLevel(0);
  qp = QUESTMASTER->QueryQuestPoints(pl);
  xp = pl->QueryXP();
  res = sprintf("You are level %d.\n"
		"You have %d/%d QP and %d XP.\n",lvl,qp,
		QUESTMASTER->QueryTotalQuestPoints(),xp);
  if ((h = xp-ME->QueryLevelCost(lvl))>0)
    res+=sprintf("You have %d XP left to spend.\n",h);
  res+=AdvanceCostList(pl);
  if (lvl<rlvl)
    res+=sprintf("You may advance up to level %d.\n",rlvl);
  else
    if (lvl==max(m_indices(G_COSTS)))
      res+="You have the highest possible level.\n";
    else
        {
          if ((h=(int)QUESTMASTER->QPointLevel(lvl+1))>qp)
            res+=sprintf("You need %d (%d) QP to advance.\n",h-qp,h);
          if ((h = ME->QueryLevelCost(lvl+1)-xp)>0)
            res+=sprintf("You need %d XP to advance.\n",h);
          if ((h = ME->GuildQuestLevel(pl))<lvl+1)
            res+=STR->wrap(sprintf("You still have to solve the guildquest "
			      "\"%s\".\n",ME->GuildQuests()[h+1]),50,0);
        }
  tell_object(pl,res+"\n"
                 "(x) ... not enough XP to advance\n"
                 "(l) ... level not high enough to advance\n"
                 "(o) ... other stat is too low to advance this one\n");
}

// Calculate the largest stat in the mapping
string FindLargestStat(mapping st)
{
  string *idx;
  string maxstat;
  int oldstat,i;
  idx=m_indices(st);
  oldstat=0;
  for (i=0;i<sizeof(idx);i++)
  {
    if (st[idx[i]] > oldstat )
    {
      oldstat=st[idx[i]];
      maxstat=idx[i];
    }
  }
  return maxstat;
}

// Reduce largest stat in mapping until sum <= limit
mapping ReduceMaxStats(mapping st,int summe,int limit)
{
  string plstat;
  while (summe>limit)
  {
    plstat=FindLargestStat(st);
    st[plstat]=st[plstat]-1;
    summe=summe-1;
  }
  return st;
}

protected void CorrectStats(object pl)
// Called when joining the guild. If the stats of the joining
// player are too high they are set to the maximum possible.
{
  int i,m;
  mixed attr;

  for (i=0;i<sizeof(STATS);i++) {
    m = ME->QueryMaxHLPStat(STATS[i],pl);
    if (pl->Query(STATS[i]) > m)
      pl->Set(STATS[i],m);
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

public mapping SetMinStats(mapping stats)
// Set the minimum stats you need to enter this guild, e. g.
// ([P_CON:1,P_DEX:1,P_STR:1,P_INT:1])
// Purpose is to make it impossible for Newbies to enter a
// guild (except adventurers)
{
  if (   !mappingp(stats)
      || min(m_values(stats))<1
      || sizeof(stats)!=sizeof(STATS)
      || sizeof(m_values(stats)-STATS)
      || sizeof(STATS-m_values(stats))
     )
    return Pminstats||G_MINSTATS;
  return Pminstats = stats;
}

public varargs mapping QueryMinStats(object pl)
{
  string *idx;
  int i;
  mapping st;
  st = (Pminstats||G_MINSTATS)+([]);
  if (!pl) return st;
  i = sizeof(idx = m_indices(st));
  while(i--)
    st[idx[i]]=max(({1,round(get_race_modification(pl,idx[i])/3.0)+st[idx[i]]}));
  return st;
}

public varargs int QueryMinStat(string stat,object pl)
{
  return QueryMinStats(pl)[stat];
}

public mapping AddMinStat(string stat,int value)
// Add a minimum stat, e. g. AddMinStat(P_DEX,3)
{
  if ((value<1)
      ||(member(STATS,stat)==-1)
     )
    return Pminstats||G_MINSTATS;
  if (!Pminstats) Pminstats = G_MINSTATS;
  Pminstats[stat]=value;
  return Pminstats;
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
      || sizeof(m_values(stats)-STATS)
      || sizeof(STATS-m_values(stats))
     )
    return Pmaxstats||G_MAXSTATS;
  return Pmaxstats = stats;
}

public varargs mapping QueryMaxStats(object pl)
{
  string *idx;
  int i,summe;
  mapping st;
  summe=0;
  st = (Pmaxstats||G_MAXSTATS)+([]);
  if (!pl) return st;
  i = sizeof(idx = m_indices(st));
  while(i--)
  {
    st[idx[i]]=max(({1,round(get_race_modification(pl,idx[i])/2.0)+st[idx[i]]}));
    summe=summe+st[idx[i]];
  }
  
  st=ReduceMaxStats(st,summe,G_LLP_LIMIT);
  return st;
}

public varargs int QueryMaxStat(string stat,object pl)
{
  return QueryMaxStats(pl)[stat];
}

public mapping AddMaxStat(string stat,int value)
{
  if ((value<1)
      ||(member(STATS,stat)==-1)
     )
    return Pmaxstats||G_MAXSTATS;
  if (!Pmaxstats) Pmaxstats = G_MAXSTATS;
  Pmaxstats[stat]=value;
  return Pmaxstats;
}

public mapping SetMaxHLPStats(mapping stats)
// Set the maximum stats for the guild, a HLP
// might get.
{
  if (   !mappingp(stats)
      || min(m_values(stats))<1
      || sizeof(stats)!=sizeof(STATS)
      || sizeof(m_values(stats)-STATS)
      || sizeof(STATS-m_values(stats))
     )
    return Pmaxstats||G_MAXSTATS;
  return Pmaxhlpstats = stats;
}

public varargs mapping QueryMaxHLPStats(object pl)
{
  string *idx;
  int i,summe;
  mapping st;
  summe=0;
  st = (Pmaxhlpstats||G_MAXSTATS)+([]);
  if (!pl) return st;
  i = sizeof(idx = m_indices(st));
  while(i--)
  {
    st[idx[i]]=max(({1,round(get_race_modification(pl,idx[i])/2.0)+st[idx[i]]}));
    summe=summe+st[idx[i]];
  }
  st=ReduceMaxStats(st,summe,G_HLP_LIMIT);
  return st;
}

public varargs int QueryMaxHLPStat(string stat,object pl)
{
  return QueryMaxHLPStats(pl)[stat];
}

public mapping AddMaxHLPStat(string stat,int value)
{
  if ((value<1)
      ||(member(STATS,stat)==-1)
     )
    return Pmaxhlpstats||G_MAXSTATS;
  if (!Pmaxhlpstats) Pmaxhlpstats = G_MAXSTATS;
  Pmaxhlpstats[stat]=value;
  return Pmaxhlpstats;
}

public mapping SetSkills(mapping sk)
{
  return Pskills = sk;
}

public varargs mapping QuerySkills(object pl)
{
  mapping res;
  string *idx;
  int i;
  
  i = sizeof(idx = m_indices(res = (Pskills||m_allocate(0,2))+([])));
  if (!pl) return res;
  while(i--)
    if (res[idx[i],1]>pl->QueryLevel()) m_delete(res,idx[i]);
  return res;
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
  return Pskills;
}

public void InitSkills(object pl)
{
  string *idx,*new;
  int i,old;
  mapping skills;
  skills = QuerySkills(pl);
  i = sizeof(idx = m_indices(skills));
  if (!pl) return;
  new = ({});
  while(i--)
    if (   (   !intp(pl->QueryAttr(idx[i],1))
            || pl->QueryAttr(idx[i],1) < skills[idx[i],0]
           )
        && (pl->QueryLevel() >= skills[idx[i],1])
       )
      {
	new+=({idx[i]});
        pl->SetAttr(idx[i],skills[idx[i],0],1);
      }
  if (sizeof(new))
    tell_object(pl,"You learn the skills "+
      implode(new[0..<2],", ")+(sizeof(new[0..<2])?" and ":"")+new[<1]+".\n");
}
