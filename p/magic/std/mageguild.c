// A Generic mageguild which allows to raise Wisdom and Stamina */
//
// [S] Written on 02jul96
// [S] Advance stuff  on 04feb97
//
// Wizards:
// [S] Softbyte

#include <guild.h>
#include <skills.h>
#include <config.h>
#include <properties.h>
#include <moving.h>
#include <rooms.h>

inherit G_GUILDOFFICE;

#define D_STAT_COST 5
#define  D_MAX_STAT 25
#ifndef A_WIS
#define A_WIS "Wis"
#endif
#ifndef A_STA
#define A_STA "Sta"
#endif

#define TP this_player()

private static string *localstats;

string *QueryExtraStats() {return localstats;}
int SetExtraStats(string *stats) {localstats=stats;return 1;}


varargs void AddPlayerBoard()
{
  AddItem("/obj/guild/guildboard",REFRESH_HOME,
   ([P_SHORT:"The Central Player Board",
     P_LONG:"You see a black board of onyx. There are some letters on\n"
            "it and sometimes the letters on the board change.\n"
            "There seem to be some informations on it.\n"
            "At one side you can see a bookmark for the group\n"
            "players, where all players-related stuff may be stored.\n",
     "Groups":({"players"}),
     "LevelGroups":(["players.hlp":20])
   ]));
/*
  AddItem("/std/board",REFRESH_HOME,
   ([P_SHORT:"The Central Player Board",
   P_LONG:"You see a black board of onyx. There are some letters on\n"
            "it and sometimes the letters on the board change.\n"
            "There seem to be some informations on it.\n",
     "Groups":({"players","players.([^h]\\.*|(h[^l]\\.*|hl[^p]\\.*))"})
   ]));
*/
}
public void create()
{
  (::create());
  SetExtraStats( ({A_WIS,A_STA})  );  /* Set Local Stats */
  AddRoomCmd("wjoin",#'cmd_join);  /* Somtimes Wizards cannot use 'join' */
}

public int InformAdvance(object pl,string stat)
{
  if (pl!=TP) 
     return (int)notify_fail("Only YOU can advance your stats!\n",NOTIFY_NOT_VALID);
  if (stat=="wisdom" || stat=="wis") stat=A_WIS;
  if (stat=="stamina" || stat=="sta") stat=A_STA;
  if (-1==member(QueryExtraStats(),stat)) 
     return (int)notify_fail("You cannot advance that!\n",NOTIFY_NOT_VALID);
  if (CanAdvance(pl,stat)!=ADV_OK) return 0;
  pl->AddXP(-CalcAdvanceCosts(pl,stat));
  pl->SetAttr(stat,pl->QueryAttr(stat)+1);
  if (stat==A_STA) pl->SetAttribute("MEP", 8 * (pl->QueryAttr(stat)+1) + 50);

  write("Ok.\n");
  return 1;

 return 1;
}
public int CanAdvance(object pl,string stat)
{
  if (-1==member(QueryExtraStats(),stat)) return ::CanAdvance(pl,stat);
  /* Needs to set a notify_fail if advance is not allowed !!! */
  if (pl->QueryAttr(stat)>D_MAX_STAT) 
  {
    notify_fail("You cannot advance your "+stat+" anymore.\n",NOTIFY_NOT_VALID);
    return ADV_TOO_HIGH;
  }
  if (stat==A_WIS && ((4*pl->QueryInt()+pl->QueryDex())/5)<pl->QueryAttr(stat))
  {
    notify_fail("You are not intelligent enough.\n",NOTIFY_NOT_VALID);
    return ADV_OTHER_FIRST;
  }
  if (stat==A_STA && ((2*pl->QueryCon()+pl->QueryStr())/3)<pl->QueryAttr(stat))
  {
    notify_fail("You are not though enough.\n",NOTIFY_NOT_VALID);
    return ADV_OTHER_FIRST;
  }
  return ADV_OK;
}

public int CalcAdvanceCosts(object pl,string stat)
{
int cost,base;
  if (-1==member(QueryExtraStats(),stat)) return ::CalcAdvanceCosts(pl,stat);
  base=pl->QueryAttr(stat);
  if (base==0) return 5;
  if (base==1) return 18;
  if (base<0) base= -2 * base;
  if (base>=30) return -1; 
  cost=(QueryLevelCost(base) - QueryLevelCost(base-1))/D_STAT_COST;
  return cost;
}

public string AdvanceCostList(object pl)
{
string res,c,cost;
int i;
  res=::AdvanceCostList(pl);
  for (i=0;i<sizeof(QueryExtraStats());i++)
  {
      if (comment_cost(pl,QueryExtraStats()[i],&c)==-1)
        {
          cost = "not possible";
          c = "";
        }
      else
        cost = to_string(CalcAdvanceCosts(pl,QueryExtraStats()[i]));
      res+=sprintf("  %s (%02d): %'.'20s %s\n",QueryExtraStats()[i],
            pl->QueryAttr(QueryExtraStats()[i]),
                   " "+cost,c);
  }
  return res;
} 
/* Compat only */
string *QueryD_Stats() {return localstats;}
int SetD_Stats(string *stats) {localstats=stats;return 1;}

