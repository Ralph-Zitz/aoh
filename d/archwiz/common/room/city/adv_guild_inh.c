/*
** The good old adventurer's guild. It is mostly for demonstration.
** It will serve only players that have not choosen any `real' guild yet.
** Players can come here to advance their stats and their level. Note that
** for this guild, advancing the level *purely* means getting a new title,
** nothing else. Other guilds could choose to couple the player level to
** guild-specific mini-quests or the like, thus haveing no need for
** explicitly advancing the player level.
**
** Pepel, Mon Oct 12 21:19:08 MET 1992
**
** This file is the basic guild-file for all adventurer-guilds and should
** be inherited by them.
*/

/*
** 13-06-94 [c]   removed all the exits from this file
**                and changed it to a real guild again :-)
** 15-06-94 [l]   added setting of visits
** 30-09-94 [c]   small rewrite
** 13-10-94 [c]   implement check of correct gender  in advanve()
** 01-11-94 [r]   added a comment about leaving the guild again :)
** 20-01-05 [t]   adapted to /std/guild
**
** [c] Cadra
** [l] Loco
** [r] Rover
** [t] Thragor
*/   

inherit "/std/guild";

#include "path.h"
#define FILE "adv_guild_inh"
#define MYFILE CITY(FILE)
#define MYGROUPS ({"players","players.\\.*"})
#include <guild.h>
#include <secure/wizlevels.h>
#include <rooms.h>
#include <config.h>
#include <attributes.h>
#include <properties.h>
#include <moving.h>
#include <messages.h>

#define TP          this_player()
#define ME          this_object()
#define GUILDSTR    "adventurer"
#define GUILDID     "adv020169"
#define BANISHER    "/secure/banisher"
#define XP_FOR_VISIT 500
#define SF(x) #'x //'
#define CL_PROGN #', //'

private static string location;

/*
** You can't banish players from this guild.
** The only available banish-command in this guild is to
** banish players/names from the mud.
*/

public int cmd_banish(string str)
{
  if (!IS_IMMORTAL(TP))
    return notify_fail("Sorry, but you can't use this command.\n",
                       NOTIFY_NOT_OBJ),0;
  return BANISHER->banish(str);
}

public int cmd_unbanish(string str)
{
  return notify_fail("You can't use this command.\n",
                     NOTIFY_NOT_OBJ),0;
}

/*
** Don't check for the Minstats as a player should be able to enter
** the guild at any time.
*/

public int cmd_join(string str)
// RoomCmd "join"
{
  if (str&&lower_case(str)!="guild")
    return notify_fail("Join what?\n",NOTIFY_NOT_OBJ),0;
  if (is_member(TP)&&TP->QueryGuild()==QueryGuildclass())
      return notify_fail("You are already member of this guild.\n",
			 NOTIFY_NOT_OBJ),0;
  else if (TP->QueryGuild())
      return notify_fail("You are already member of a guild.\n",
			 NOTIFY_NOT_OBJ),0;
  if (IsBanished(getuid(TP)))
    return notify_fail(capitalize(Banisher(getuid(TP)))+
		       " banished you from this guild.\n",
		       NOTIFY_NOT_VALID),0;
  TP->SetGuild(QueryGuildclass());
  TP->SetLevel(1);
  TP->SetTitle(GuildLeveltoTitle(1,TP->QueryGender()));
  CorrectStats(TP);
  InitSkills(TP);
  AddMember(getuid(TP),1,G_MEMBER);
  write("You are now a member of the "+
	(strip_article(QueryGuildname())||"guild")+".\n");
  if (!TP->QueryInvis())
    {
      say(capitalize(getuid(TP))+" is now a member of the "+
	  (QueryGuildname()||"guild")+".\n");
      if (QueryGuildname())
   CHANNEL_D->SendEmote("news", TP->QueryRealName(),
                            " has become a member of the" +
				 strip_article(QueryGuildname())+".",1);
    }
  CreateGuildobject(TP);
  return 1;
}

/*
** We want that all players can see without problems in this guild.
*/

public int _display_filter(object rec, mixed msg)
{
  string txt;
 
  if (!is_member(rec))
    return (::_display_filter(rec,msg));
  rec->catch_msg(({ MSG_SEE, msg[0], msg[0] }));
  return 1;
}

public int QueryIntLight()
{
  if (!is_member(previous_object()))
    return (::QueryIntLight());
  return (previous_object()->QueryUVision()-1);
}

/*
** As this is a kind of Explorer-Guild, you may explore all the different
** Adventurer-Guilds in Nightfall.
** Use this function to describe where your guild is.
*/

public string SetLocation(string str) { return location=str; }

public string QueryLocation() { return location; }


public int gocenter()
{
  if (!TP) return 0;
  if (!query_once_interactive(TP)||
      !IS_IMMORTAL(TP->QueryRealName()))
    return notify_fail(
      lambda(0,
        ({CL_PROGN,
          ({SF(write),"A strong magic force stops you.\n"}),
          ({SF(show),
            ({SF(+),
              ({SF(call_other),
                ({SF(this_player)}),
                "QueryName"
              }),
              " tries to go through the magic field but fails.\n"
            })
          })
        }) )),0;
  write("You wriggle through the force field...\n");
  TP->move(TOWER("center"),M_GO,"south");
  return 1;
}


public int allow_enter(mixed method,mixed extra)
{
  int i;
  object ob;
  if (  !(i=(::allow_enter(method,extra)))
      ||object_name()==MYFILE)
  {
    return 0;
  }
  if (method!=M_TELEPORT &&
      previous_object()->QueryGuild()==QueryGuildclass() &&
      location &&
      !QUESTMASTER->QueryQuestSolved(location,previous_object()))
    {
      QUESTMASTER->SetPlayerQuest(location,previous_object());
      previous_object()->AddXP(XP_FOR_VISIT);
    }
  return i;
}

/*
public varargs void update_toplist(int state,mapping data)
{
  while(remove_call_out(SF(update_toplist))!=-1);
  if (object_name()==MYFILE) (::update_toplist(state,data));
}
*/

public string toplist()
{
  if (object_name()==MYFILE) return (::toplist());
  else return (MYFILE)->toplist();
}

public void create()
{
  (::create());
log_file("guilds/advguild.log","Adv.Guild cloned by "+ QueryCreator() +" on "+ ctime() +" .\n");
// Should the register object be loaded by something else than an archwizard
// object, it won't be able to write /d/archwiz/common/log anymore. 
// Besides: what ... use should this registering have? [Mateese]
//  "/obj/lib/register"->Register(this_object(),
//                                "/d/archwiz/common/log/"+FILE+"s");
  SetGuildfile("/log/adventurer/member"); // need to put it to this global
                                          // place to allow all adventurer-
                                          // guilds to update it.
  SetLoadForce(1); // the guild needs to load the guild-data on every query,
                   // because another adv-guild might have changed the data
                   // meanwhile
  SetIntShort("The adventurer's guild");
  SetIntLong(
"You are standing in the famous adventurer's guild of Nightfall. "
"In former times many adventurers came here to chat and inform themselves. "
"You can still see the old bulletin board standing there. "
"The interiour is shabby and worn out, but you have the impression that "
"there has been some restauration effort recently.\n"
"There is a doorway to the south.\n"
"The wall to the east is slightly damaged. Apparently a dinosaur ran "
"into it quite recently.\n");
  SetGuildclass("adventurer");
  SetGuildname("Adventurer Guild");
  SetGuildobject(GUILDOBJ("staff"));
  SetCreators(({"joan", "tune"}));
  AddMinStat(P_CON,1);
  AddMinStat(P_STR,1);
  AddMinStat(P_DEX,1);
  AddMinStat(P_INT,1);
  AddMaxStat(P_CON,20);
  AddMaxStat(P_STR,20);
  AddMaxStat(P_DEX,20);
  AddMaxStat(P_INT,20);
  AddMaxHLPStat(P_CON,30);
  AddMaxHLPStat(P_STR,30);
  AddMaxHLPStat(P_DEX,30);
  AddMaxHLPStat(P_INT,30);
  
  AddItem("/obj/guild/guildboard",REFRESH_HOME,ME->QueryBoardProps()||
   ([P_SHORT:"The Central Player Board",
      P_ADS:({"wooden","bulletin","old"}),
     P_LONG:"This is the Central Player Board of Nightfall.\n",
     "Groups":({"players","flames"}),
     "LevelGroups":(["players.hlp":20])
   ]));
  AddId("advguild");
  AddDetail( ({"door", "doorway"}),
	    "There is some shimmering blue light in the doorway.\n");

  AddLevelTitles( 1,"the utter creature",
                    "the utter novice",
                    "the utter novice");
  AddLevelTitles( 2,"the simple creature",
                    "the simple wanderer",
                    "the simple wanderer");
  AddLevelTitles( 3,"the furry creature",
                    "the lowrank ranger",
                    "the lowrank ranger");
  AddLevelTitles( 4,"the bugbear",
                    "the master ranger",
                     "the master ranger");
  AddLevelTitles( 5,"the wraith",
                    "the small fighter",
                    "the siren");
  AddLevelTitles( 6,"the shadow",
                    "the experienced fighter",
                    "the charming siren");
  AddLevelTitles( 7,"the threatening shadow",
                    "the small adventurer",
                    "the small adventuress");
  AddLevelTitles( 8,"the small monster",
                    "the experienced adventurer",
                    "the experienced adventuress");
  AddLevelTitles( 9,"the medium monster",
                    "the great adventurer",
                    "the great adventuress");
  AddLevelTitles(10,"the experienced monster",
                    "the warrior",
                    "the amazon");
  AddLevelTitles(11,"the great monster",
                    "the champion",
                    "the deadly amazon");
  AddLevelTitles(12,"the beholder",
                    "the conjurer",
                    "the conjuress");
  AddLevelTitles(13,"the small demon",
                    "the apprentice magician",
                    "the apprentice magicienne");
  AddLevelTitles(14,"the powerful demon",
                    "the magician",
                    "the magicienne");
  AddLevelTitles(15,"the small dragon",
                    "the enchanter",
                    "the enchantress");
  AddLevelTitles(16,"the devious dragon",
                    "the warlock",
                    "the witch");
  AddLevelTitles(17,"the vicious dragon",
                    "the apprentice sorcerer",
                    "the apprentice sorceress");
  AddLevelTitles(18,"the small tyrannosaur",
                    "the master sorcerer",
                    "the master sorceress");
  AddLevelTitles(19,"the ferocious tyrannosaur",
                    "the grand master sorcerer",
                    "the grand master sorceress");
  AddLevelTitles(20,"the experienced tyrannosaur",
                    "the experienced sorcerer",
                    "the experienced sorceress");
  AddLevelTitles(21,"the hydra",
                    "the apprentice traveller",
                    "the apprentice traveller");
  AddLevelTitles(22,"the experienced hydra",
                    "the small traveller",
                    "the small traveller");
  AddLevelTitles(23,"the deadly hydra",
                    "the experienced traveller",
                    "the experienced traveller");
  AddLevelTitles(24,"the small minotaur",
                    "the one who's seen the world",
                    "the most famous traveller");
  AddLevelTitles(25,"the strong minotaur",
                    "the apprentice travelling master",
                    "the apprentice travelling mastress");
  AddLevelTitles(26,"the deadly minotaur",
                    "the travelling master",
                    "the travelling mastress");
  AddLevelTitles(27,"the small manticore",
                    "the grand master of travelling",
                    "the Lady of travelling");
  AddLevelTitles(28,"the strong manticore",
                    "the wanderer",
                    "the grand Lady of travelling");
  AddLevelTitles(29,"the wandering creature",
                    "the wandering Lord",
                    "the wandering Lady");
  AddLevelTitles(30,"the high wandering creature",
                    "the high Lord of the adventurer guild",
                    "the high Lady of the adventurer guild");
}
