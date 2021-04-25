/*
** The /p/guilds/std/guild.c
**
** 01nov94 [t] Creation
** 08jun95 [t] ChangeSign, ChangeList added
** 12mar96 [t] reset starts update of the toplist
** 12feb97 [e] added a function to check weapons/armour when joining a guild
** 19Feb97 [r] moved the File to /p/guilds/std/guild
** 26apr97 [e] Introduced guild maintenenacers
**
** [t] Thragor
** [e] Eilan
** [r] Rover
*/

inherit "/std/room";
inherit "/p/guilds/std/guild/banish";
inherit "/p/guilds/std/guild/guildquests";
inherit "/p/guilds/std/guild/member";
inherit "/p/guilds/std/guild/stats";
inherit "/p/guilds/std/guild/level";

#include <guild.h>
#include <config.h>    // QUESTMASTER, NOTIFY-Priorities
#include <moving.h>    // move-methods
#include <rooms.h>     // REFRESH-Modes
#include <secure/wizlevels.h> // IS_IMMORTAL

#define LOSE_XP // Players lose XP leaving the guild

#define MYFILE "/p/guilds/std/guild"
#define REGISTER "GUILD"
#define STATS ({P_CON,P_STR,P_DEX,P_INT})
#define WIZFUNS "/d/archwiz/common/guilds/guildwizfuns"
#define STR "/lib/string"       // radjust, ladjust, ...
#define NMB "/lib/number"       // max, min
#define TP this_player()
#define ME this_object()
#define POS_IDS 0
#define POS_DESCR 1
#define POS_ROOM 2

#define TITLE_LEVEL 30 // from what level on you might use the 'title'-cmd

/* the purpose of the next three macros is to avoid the hash-tick notation,
 * which would confuse emacs c-mode indent engine _sigh_
 */
#define SF(x) #'x //'
#define SYM(x) 'x //'
#define CL_PROGN #', //'

private static string guildobject, // the filename of the guildobject
                      guildclass,  // e. g. Cleric
                      guildname;   // e. g. Cleric's Guild
private static mixed  sign,list;   // ({({<ids>}),<descr>,<room>})

protected int check_for_guildob(object ob)
{
  return (blueprint(ob)==guildobject);
}
  
/*
* Guildobject related help-funs
* =============================
*/

// Check if weapons/armour are allowed when joining the guild
int CheckEquipment(object guildobject, object pl)
{ 
  int i;
  object *idx;
  if (!pl || !guildobject) return 0;
  idx=pl->QueryWeapons() || ({});
//  idx -= ({0});
  for (i=0;i<sizeof(idx);i++)
  { 
    if (!idx[i]) continue;
    if (EQ_OK != guildobject->ChkWield(pl,idx[i],EWF_WEAPON))
	{
	  idx[i]->Unwield(EWF_WEAPON);
	}
  }
  idx=pl->QueryArmours() || ({});
//  idx -= ({0});
  for (i=0;i<sizeof(idx);i++)
  { 
    if (!idx[i]) continue;
    if (EQ_OK != guildobject->ChkWear(pl,idx[i],EWF_WEAPON))
	{
	  idx[i]->Unwear(EWF_WEAPON);
	}
  }
  return 1;
}

public object CreateGuildobject(object pl)
// Create the guildobject for the player <pl> if he doesn't have
// one and inform us about this event with
//    InformGuildobject(pl)
// If this function is present in the guild, it is called. E. g.
// you may define your own message here, what the player will
// get receiving this item.
// If the function returns a non-zero value, the default message
// when receiving the guildobject, won't be printed.
{
  object ob,*gobs;
  string err;
  int rc,i;
  string msg;

  if (!guildobject||!pl) return 0;
  gobs = filter_objects(all_inventory(pl),"IdGuildobject");
  if (sizeof(gobs))
    for(i=sizeof(gobs);i--;)
      if (   blueprint(gobs[i])==guildobject
          || blueprint(gobs[i])+".c"==guildobject
         )
	if (file_date(blueprint(gobs[i])+".c")>gobs[i]->QueryCreationTime())
	  {
	    gobs[i]->remove();
	    if (gobs[i]) destruct(gobs[i]);
	    tell_object(pl,"Your guildobject will get updated.\n");
	    break;
	  }
	else
	  return 0;
  if (err = catch(ob = clone_object(guildobject)))
  {
    msg = sprintf("%O->%O::CreateGuildobject() %O: %s"
                 , previous_object(), this_object(), guildobject, err);
    log_file("GUILD", msg);
    catch(load_object(CHANNEL_D)->SendTell("error", "GUILD", msg, 1));
    return 0;
  }

  // and now move it
  if (ME_OK != (rc = ob->move(pl,M_SILENT)))
  {
    msg = sprintf("%O->%O::CreateGuildobject() %O: move failed %d"
                 , previous_object(), this_object(), guildobject, rc);
    log_file("GUILD", msg);
    catch(load_object(CHANNEL_D)->SendTell("error","GUILD", msg, 1));
    return 0;
  }

  if (!ME->InformGuildobject(pl,ob))
    tell_object(pl,"The guildmaster gives "+
                (ob->QueryShort()||"the guildobject")+" to you.\n");
  return ob;
}

public string helpsign()
// description of the sign
{
  string h;
  string s;
  s="You may use the following commands in here:\n"
  "advance, join, list, message, cost, title, correct"+
  (is_leader(TP)?", banish, unbanish":"")+".\n"
  "To get help about any of these commands type 'help <topic>'.\n"
  "You may get help about 'minstats' and 'maxstats', too\n"+
  ((h = ME->QueryGuildleader())?
  "If you have problems talk to your guildleader "+capitalize(h)+
  ((h = ME->QueryViceGuildleader())?"\nor to your vice-guildleader "+
  capitalize(h):"")+".\n"
  :"")+
  (IS_IMMORTAL(TP)?"You may use the 'guild'-command in here. Just type\n"
  "'guild help' to see its effects.\n":"")+
  (sizeof(QueryCreators())?sizeof(QueryCreators())>1?
  "The creators of this guild are: ":"The creator of this guild is: ":"")+
  implode(map(QueryCreators(),SF(capitalize)),", ")+".\n"+
  (sizeof(QueryMaintainers())?sizeof(QueryMaintainers())>1?
    "The maintainers of this guild are: ":"The maintainer of this guild is: ":"")+
    implode(map(QueryMaintainers(),SF(capitalize)),", ")+".\n";
  return s;
}

public void banished_entered()
// If a banished member enters this room, kick him out of the guild
{
  object *obs;
  int i;

  write("You've been banished from this guild by "+Banisher(getuid(TP))+".\n");
  TP->SetGuild(0);
  TP->SetLevel(1);

  if (guildobject)
    if (i = sizeof(obs = filter(map(all_inventory(TP),
                                                SF(blueprint)
                                               ),
                                      lambda(({SYM(v)}),
                                             ({SF(==),SYM(v),guildobject})
                                            )
                                     )
                  )
       )
      while(i--)
        obs[i]->check_owner(TP);
  TP->SetTitle("the former "+guildclass);
  ME->RemoveMember(getuid(TP));
}

public void member_entered()
// This is called in the init of the guild.
// It does several checks. E. g. it updates the date, when
// a member entered, it restores the guildobject, if it
// was lost and it gives notes from other guildmembers
// to the player, if there are some.
{
  string uid,*msgs;
  int i;
  object ob;
  if (!TP) return;
  LoadGuild();
  if (!members) return;
  ME->UpdateTime(uid = getuid(TP));
  if ((msgs = members[uid,MEMBER_MSGS])&&
      (i=sizeof(msgs)))
    {
      write("Here "+(i>1?"are some notes":"is a note")+" for you.\n");
      while(i--)
        ME->create_note(msgs[i])->move(TP,M_GIVE);
      ME->RemoveMessages(uid);
    }
  if (ob = CreateGuildobject(TP))
    write("You've lost your "+(strip_article(ob->QueryShort())||"guildobject")+".\n"
      "Have it back!\n");
  CheckEquipment(ob, TP);	  
  ME->InformEnter(uid);
}

public void AddVotingbox()
// This should be called in the create() of the guild, to add
// a voting box, so that it is possible to vote for the guildleader.
// If you want to set guildleaders on your own, don't add this box.
{
  if (!ME->QueryGuildfile()) SetGuildfile(blueprint(ME));
  AddItem("/obj/gvotingbox",REFRESH_HOME,
          (["Savefile":ME->QueryGuildfile()+".votes"]));
}

public void ChangeSign(mixed ids,string descr,string roomdescr)
// As you might want another sign than the wooden one, you can
// change this.
// <ids> is a string or an array of strings containing the ids
//       the sign identifies to
// <descr> is the description you get when examining it (will be
//         followed by the normal helpsign-description
// <roomdescr> is the text appended to the roomdescr.
//             If the sign is described somewhere else in the IntLong
//             you may set it to "".
{
  if (sign)
    {
      RemoveDetail(sign[POS_IDS]);
      RemoveReadMsg(sign[POS_IDS]);
    }
  AddDetail(ids,
            lambda(0,({SF(+),
                       descr,
                       ({SF(call_other),ME,"helpsign"})
		     })
                  )
           );
  AddReadMsg(ids,SF(helpsign));
  sign = ({ids,descr,roomdescr});
}

public void ChangeList(mixed ids,string descr,string roomdescr)
// As you might want another topscore-list than the given one, you can
// change this.
// <ids> is a string or an array of strings containing the ids
//       the list identifies to
// <descr> is the description you get when examining it (will be
//         followed by the normal toplist)
// <roomdescr> is the text appended to the roomdescr.
//             If the list is described somewhere else in the IntLong
//             you may set it to "".
{
  if (list)
    {
      RemoveDetail(list[POS_IDS]);
      RemoveReadMsg(list[POS_IDS]);
    }
  AddDetail(ids,
            lambda(0,({SF(+),
                       descr,
                       ({SF(call_other),ME,"toplist"})
		     })
                  )
           );
  AddReadMsg(ids,
             lambda(0,({CL_PROGN,
                        ({SF(call_other),STR,"smore",
                          ({SF(call_other),ME,"toplist"}),
                          ({SF(call_other),
                            ({SF(this_player)}),
                            "QueryPageSize"})
                        }),
                        ""
		      })
                   )
            );
  list = ({ids,descr,roomdescr});
}

public void check_leave(string answer,object pl)
// This function is called by input_to. It waits for an answer,
// if the player really wants to leave the guild.
// If so, the Guild-Property will be set to 0, the XP reduced
// by one third of their amount and the title set to 'former ...'
// Last but not least the message will be spread on the news-
// channel, if the player is visible.
{
  int i;
  object *obs;
  if (!pl) return;
  if (member(({"yes","y"}),lower_case(answer||""))==-1)
    write("Ok. You'll stay a member of this guild.\n");
  else
    {
#ifdef LOSE_XP
      if (pl->QueryGuild()!=GC_ADVENTURER) // only adventurers don't loose xp
        {
          int decxp;
          decxp = CALC_DEC(pl->QueryXP());
          if (decxp>MAX_DEC_XP) decxp = MAX_DEC_XP;
          pl->AddXP(-decxp);
        }
#endif
      pl->SetGuild(0);
      pl->SetLevel(0);
      pl->SetTitle("the former "+guildclass);
      ME->RemoveMember(getuid(pl));
      if (guildobject)
        if (i = sizeof(obs = filter_objects(all_inventory(pl),
                                            "IdGuildobject")))
          while(i--)
            obs[i]->check_owner(pl,1);
      // The check of the guildobject might look a bit complicated,
      // but this allows to use the std-guildobject without an extra
      // file and no problems when destructing it (because the guildobject
      // is asked to remove itself, when the owner is not a member of the
      // right guild.
      write("You leave the guild.\n");
      if (!pl->QueryInvis())
        CHANNEL_D->SendEmote("news",capitalize(getuid(pl)),
                          "left the "+strip_article(guildname)+".",1);
   }
}

/*
* ----------------------------------------------------------------------
* Queries, Settings, Adds
* ----------------------------------------------------------------------
*/

public string SetGuildclass(string cl)
// The guildclass. This will be used to set the Guild-Property in a
// player. Examples: Cleric, Adventurer, Headhunter, ...
{
  return guildclass = cl;
}

public string QueryGuildclass()
{
  return guildclass;
}

public string SetGuildname(string gn)
// The guildname. This will be used in some messages, e. g. in the
// Top-Score-List. Examples: Temple of Fahr, Adventurer Guild,
// Headhunter
{
  return guildname=gn;
}

public string QueryGuildname()
{
  return guildname;
}

public string SetGuildobject(string gfile)
// Set the guildobject, this guild uses, which will be
// automatically cloned, if a member enters, automatically
// destructed, if a member leaves, and so on
{
  return guildobject = gfile;
}

public string QueryGuildobject()
{
  return guildobject;
}

/*
* ----------------------------------------------------------------------
* Functions
* ----------------------------------------------------------------------
*/

public void input_done(string str,string towhom,string fromwhom)
// called when finished writing a message to a member
{
  string *text;

  if (!str||str=="") return;
  text = ({"A message from "+capitalize(fromwhom||"")+" ["+ctime()+"]:"})+
         explode(str,"\n");
  AddMessage(towhom,text);
}

public object create_note(string *msg)
// Create a note from a message
{
  object ob;
  ob = clone_object("/std/thing");
  ob -> SetShort("a note");
  ob -> SetLong("This is a small note from your guild, which you may read.\n");
  ob -> SetHelpMsg("This is a small note from your guild, which you may read.\n");
  ob -> SetValue(0);
  ob -> SetWeight(0);
  ob -> SetIds(({"note","paper"}));
  ob -> SetAds(({"small","a","the"}));
  ob -> SetReadMsg(implode(msg,"\n")+"\n");
  ob -> Set(P_NOSELL,1);
  return ob;
}

public int ForbidJoin (object pl)

/* Return non-zero if <pl> is not allowed to join the guild.
 * A suitable notify_fail() must be set in that case, too.
 */
{
  return 0;
}

/*
* ----------------------------------------------------------------------
* Commands
* ----------------------------------------------------------------------
*/

public int cmd_cost(string str)
// RoomCmd "cost"
{
  if (!TP) return 0;
  if (str)
    return (int)notify_fail("'cost' doesn't support arguments.\n",
                            NOTIFY_ILL_ARG)&&0;
  if (!is_member(getuid(TP)))
    return (int)notify_fail("You're not a member of this guild.\n",
                            NOTIFY_NOT_VALID)&&0;
  PrintAdvanceCosts(TP);
  return 1;
}

public int cmd_message(string str)
// RoomCmd "message"
{
  if (!check_member(TP))
    return (int)notify_fail("Only guildmembers may use this.\n",
                            NOTIFY_NOT_OBJ)&&0;
  if ((str = lower_case(str||""))=="")
    return (int)notify_fail("To whom do you want to write a message?\n",
                            NOTIFY_ILL_ARG)&&0;
  if (!is_member(str))
    return (int)notify_fail(capitalize(str)+
                            " is not member of the guild.\n",
                            NOTIFY_NOT_VALID)&&0;
  write("You may now enter the message you want to send to "+
        capitalize(str)+":\n");
  STR->input_text(({str,getuid(TP)}));
  return 1;
}
  
public int cmd_advance(string str)
// RoomCmd "advance"
{
  int nlvl,rlvl;
  mixed h;

  if (!TP) return 0;
  if (!is_member(TP)) // the player needs to be a member of our guild
    return (int)notify_fail("You are not a member of this guild.\n",
                            NOTIFY_NOT_OBJ)&&0;
  if (sscanf(TP->QueryRealName(),"guest%d",h)) // guests can't advance
    return (int)notify_fail(
             "You need to create your own character first.\n",
             NOTIFY_NOT_OBJ)&&0;
  if (TP->QueryRace()=="visitor") // visitors can't advance
    return (int)notify_fail(
     "You have to go to Dr. Frankenstein first.\n"
     "You need to choose a race.\n",
     NOTIFY_NOT_OBJ)&&0;
  if (TP->QueryGhost()) // ghost can't advance
    return (int)notify_fail("You can't do that in your current state!\n",
                            NOTIFY_NOT_OBJ)&&0;
  rlvl = ((rlvl = TP->QueryRealLevel())>(h = GuildQuestLevel(TP)))?h:rlvl;
  if (!sscanf(str = lower_case(str||""),"to %d",nlvl)
      &&!sscanf(str,"to level %d",nlvl)
      &&!sscanf(str,"level to %d",nlvl)
      &&!sscanf(str,"level to level %d",nlvl)
      &&str!="level"
      &&str!="")
    if ((member(STATS,capitalize(str))==-1)
        &&member(STATS, (["strength":"Str",
                               "dexterity":"Dex",
                               "constitution":"Con",
                               "intelligence":"Int"])[str])==-1)
      return ((int)notify_fail("Advance what?\n",
                               NOTIFY_ILL_ARG)&&0)||ME->InformAdvance(TP,str);
      // if the std/guild can't identify what the player wants to advance
      // and the guild itself doesn't either (InformAdvance) then give
      // the notify_fail-message
    else
      // the player wants to advance one of the stats
      {
	if (h = AdvanceStat(TP,capitalize(str)))
	  ME->InformAdvance(TP,capitalize(str));
	return h;
      }
  if (!nlvl)
    nlvl = rlvl;
  if (nlvl>rlvl)
    return (int)notify_fail("You can't advance to level "+to_string(nlvl)+".\n")&&0;
  if (nlvl==TP->QueryLevel() && NMB->max(m_indices(G_COSTS))==nlvl)
    return (int)notify_fail("You can't advance anymore.\n",
                            NOTIFY_NOT_VALID)&&0;
  if (nlvl<TP->QueryLevel())
    return (int)notify_fail("You can't decrease your level.\n")&&0;
  if (nlvl==TP->QueryLevel())
    return (int)notify_fail("You can't advance right now.\n")&&0;
  if (nlvl>rlvl)
    if (TP->QueryLevel()<rlvl)
      return (int)notify_fail("You can only advance up to level "+
                              to_string(rlvl)+"\n",
                              NOTIFY_NOT_VALID)&&0;
    else
      return (int)notify_fail("You're not ready to advance.\n",
                              NOTIFY_NOT_VALID)&&0;
  TP->SetLevel(nlvl);
  ChangeLevel(getuid(TP),nlvl);
  InitSkills(TP);
  TP->SetTitle(GuildLeveltoTitle(nlvl,TP->QueryGender()));
  if (nlvl>=TITLE_LEVEL||!QueryLevelTitles()[nlvl])
    write("You may choose your own title now.\n"
          "Try 'help title' in here.\n");
  else
    write("You are now allowed to call yourself "+
          capitalize(getuid(TP))+" "+
          GuildLeveltoTitle(nlvl,TP->QueryGender())+".\n");
  ME->InformAdvance(TP,"level"); // InformAdvance may do additional things
  AnnounceNewLevel();
  return 1;
}

public int cmd_join(string str)
// RoomCmd "join"
{
  object ob;
  if (str&&lower_case(str)!="guild")
    return (int)notify_fail("Join what?\n",NOTIFY_NOT_OBJ)&&0;
  if (is_member(TP)&&TP->QueryGuild()==guildclass)
      return (int)notify_fail("You are already a member of this guild.\n",
                              NOTIFY_NOT_OBJ)&&0;
  else if (TP->QueryGuild())
      return (int)notify_fail("You are already member of a guild.\n",
                              NOTIFY_NOT_OBJ)&&0;
  if (QueryMinStat(P_INT,TP)>TP->QueryInt())
     return (int)notify_fail("You're not intelligent enough.\n",
                             NOTIFY_NOT_VALID)&&0;
  if (QueryMinStat(P_CON,TP)>TP->QueryCon())
    return (int)notify_fail("You're not tough enough.\n",
                            NOTIFY_NOT_VALID)&&0;
  if (QueryMinStat(P_STR,TP)>TP->QueryStr())
    return (int)notify_fail("You're not strong enough.\n",
                            NOTIFY_NOT_VALID)&&0;
  if (QueryMinStat(P_DEX,TP)>TP->QueryDex())
    return (int)notify_fail("You're not dexterous enough.\n",
                            NOTIFY_NOT_VALID)&&0;
  if (IsBanished(getuid(TP)))
    return (int)notify_fail(capitalize(Banisher(getuid(TP)))+
                            " banished you from this guild.\n",
                            NOTIFY_NOT_VALID)&&0;
  if (ForbidJoin(TP))
    return 0;
  TP->SetGuild(guildclass);
  TP->SetLevel(1);
  TP->SetTitle(GuildLeveltoTitle(1,TP->QueryGender()));
  CorrectStats(TP);
  InitSkills(TP);
  AddMember(getuid(TP),1,G_MEMBER);
  if (!ME->InformJoin(TP))
    {
      write("You are now a member of the "+
            (strip_article(guildname)||"guild")+".\n");
      if (!TP->QueryInvis())
	{
	  say(capitalize(getuid(TP))+" has become a member of the "+
              (guildname||"guild")+".\n");
          if (guildname)
            CHANNEL_D->SendEmote("news",capitalize(getuid(TP)),
                              "has become a member of the "+
                              strip_article(guildname)+".",1);
        }
    }
  ob = CreateGuildobject(TP);
  CheckEquipment(ob, TP);
  return 1;
}

public int cmd_list(string str)
// RoomCmd "list"
{
  int which;
  if (!str||(str=lower_case(str))=="quests")
    {
      STR->smore(QUESTMASTER->ListQuests(TP),TP->QueryPageSize());
      return 1;
    }
  if (str=="guildquests")
    {
      STR->smore(ListGuildquests(TP),TP->QueryPageSize());
      return 1;
    }
  if (sscanf(str,"quest %d",which)||sscanf(str,"%d",which))
    {
      which--;
      STR->smore(QUESTMASTER->QueryQuestLong(which),TP->QueryPageSize());
      if (QUESTMASTER->QueryQuestSolved(which,TP)) 
        write("You have already solved this quest.\n"
              "Special congrats from "+
               QUESTMASTER->QueryQuestCreator(which)+".\n");
      return 1;
    }
  if (sscanf(str,"guildquest %d",which)||sscanf(str,"#%d",which))
    {
      mapping quests;
      ListGuildquests(TP,0,&quests);
      if (!mappingp(quests))
        return notify_fail("No guildquests known.\n",NOTIFY_ILL_ARG),0;
      if (!member(quests,which))
        return notify_fail("No such guildquest.\n",NOTIFY_ILL_ARG),0;
      STR->smore(ME->QueryGuildquests()[quests[which],Q_LONG]);
      return 1;
    }
  if (str=="members")
    {
      STR->smore(MemberList(),TP->QueryPageSize());
      return 1;
    }
  if (str=="banished")
    {
      STR->smore(ListBanished(),TP->QueryPageSize());
      return 1;
    }
  notify_fail("List what?\n",NOTIFY_ILL_ARG);
  return ME->InformList(TP,str);
}


public int cmd_help(string str)
{
  int what;
  string txt;
  int i;

  if (!str)
    return (int)notify_fail("Help about what?\n",NOTIFY_NOT_OBJ)&&0;
  what = member(({"advance","join","list","message","title",
                  "banish","unbanish","minstats","maxstats","cost",
		  "correct"}),
                  lower_case(str));
  switch(what)
    {
    case 0: // advance
            txt = "Usage: advance <con|int|dex|str>\n\n"
                  "This advances one of your stats (Con, Int, Dex, Str) one\n"
                  "point, but costs a certain amount of XP.\n\n"
                  "Usage: advance [level]\n\n"
                  "Advance up to the maximum level you might reach with\n"
                  "your QP and XP\n\n"
                  "Usage: advance [to [level] <nr>]\n\n"
                  "If possible, advance up to level <nr>.\n";
            break;
    case 1: // join
            txt = "Usage: join\n\n"
                  "Become a member of this guild. You can only join this\n"
                  "guild if you are not member in another one.\n"
                  "You have to have stats of a certain amount to become\n"
                  "a member of this guild (-> help minstats, help maxstats)\n"
                  "If your stats are higher than the HLP-maxstats of the\n"
                  "guild, joining the guild will adjust them!\n";
            break;
    case 2: // list
            txt = "Usage: list [quests]\n\n"
                  "List all available quests\n\n"
                  "Usage: list [quest] <nr>\n\n"
		  "List the description for the quest no. <nr>.\n\n"
                  "Usage: list guildquests\n\n"
                  "List all guildquests of the guild.\n\n"
                  "Usage: list [guildquest <nr>|#<nr>]\n\n"
                  "List the description for the guildquest no. <nr>.\n\n";
            if (is_member(TP)||IS_IMMORTAL(TP))
              txt+="Usage: list members\n\n"
                   "Lists all members of the guild.\n";
            if (is_leader(TP)||IS_IMMORTAL(TP))
              txt+="Usage: list banished\n\n"
                   "Lists all banished players of the guild.\n";
            break;
    case 3: // message
            txt = "Usage: message <player-name>\n\n"
                  "Creates a note for another member of this guild.\n"
                  "The player will get the note the next time, (s)he\n"
                  "enters the guild.\n";
            break;
    case 4: // title
            txt = "Usage: title <your title>\n\n"
                  "If you were informed about this while advancing, you may\n"
                  "change your title with this command.\n"
	          "If you enter no title this will set your title to the\n"
	          "guild-title defined for your level. This option may be\n"
	          "used from any level on.\n";
            break;
    case 5: // banish
            txt = "Usage: banish <name>[,<reason>]\n\n"
                  "If you are allowed to, i. e. if you are (Vice-)\n"
                  "Guildleader or Creator of the guild, you may\n"
                  "forbid a player to join the guild or to be member\n"
                  "of the guild. If banished and the player is a member\n"
                  "of this guild, (s)he will be removed from it.\n"
                  "Use it wisely. Banishes are logged!\n";
            break;
    case 6: // unbanish
            txt = "Usage: unbanish <name>\n\n"
                  "If you are allowed to, i. e. if you are (Vice-)\n"
                  "Guildleader or Creator of the guild, you may\n"
                  "allow a player to become a member of this guild\n"
                  "again.\n";
            break;
    case 7: // minstats
            txt = "These are the stats you need to be able to join\n"
                  "this guild:\n";
            for (i=0;i<sizeof(STATS);i++)
              txt+=STATS[i]+": "+
                   STR->radjust(to_string(QueryMinStat(STATS[i],TP)),2)+"   ";
            txt+="\n";
            break;
    case 8: // maxstats
            txt = "These are the stats up to which you can advance:\n";
            for (i=0;i<sizeof(STATS);i++)
              txt+=STATS[i]+": "+
                   STR->radjust(to_string(TP->QueryLevel()<20
		     ?QueryMaxStat(STATS[i],TP)
		     :QueryMaxHLPStat(STATS[i],TP)),2)+"   ";
            txt+="\n";
            break;
    case 9: // cost
            txt = "Usage: cost\n\n"
                  "It will show you detailed information about what you\n"
                  "have to do or need to advance stats or your level.\n"
                  "First line is your level, in the next line your QP\n"
                  "and your maximum QP and your actual XP.\n"
                  "The third line shows the XP which are not bound to\n"
                  "your current level.\n"
                  "The following lines show the stats, at which value they\n"
                  "are and how much XP you need to advance them\n"
                  "(if possible).\n"
                  "The next line shows how far you may advance without\n"
                  "any problems (just type 'advance') or what you have\n"
                  "to do to reach your next level.\n";
            break;
    case 10: // correct
             txt = "Usage: correct\n\n"
	            "Sometimes bugs 'damage' players, like e. g. their\n"
	            "short-descriptions get screwed up. This command\n"
	            "will check for bugs and try to fix them. If you know\n"
	            "bugs, which aren't fixed by this command, tell it\n"
	            "to a wizard.\n"
	            "Current problems which get fixed:\n"
	            "- Your short is 'Someone' but you are not invis.\n";
	     break;
    default:
             notify_fail("There's no help available about '"+str+"'.\n",
                         NOTIFY_ILL_ARG);
             return ME->InformHelp(TP,str);
    }
  STR->smore(txt,TP->QueryPageSize());
  return 1;
}

public int cmd_correct(string str)
  // This is a 'growing function'. It is meant to be able to fix known
  // bugs which happen to players and can be fixed 'in general', i. e.
  // the error is for everyone the same.
{
  status fixed;
  if (!TP) return 0;
  fixed = 0;
  // 1. Problem: Sometimes the short of players is 'Someone' although they
  // are not invis.
  // If for another reason the short gets screwed up, you just need to
  // expand the if-clause.
  if (   (!TP->QueryInvis()&&TP->QueryShort()=="Someone")
     )
    {
      TP->SetShort(0);
      fixed = 1;
      write("Your short-description got corrected.\n");
    }
  fixed = fixed||ME->InformCorrect(TP,str);
  if (!fixed)
    {
      write("There's nothing known to be wrong with you. If there IS\n"
	    "something wrong with you, inform a wizard about the case.\n");
    }
  return 1;
}

public int cmd_leave(string str)
{
  if (!TP) return 0;
  if (lower_case(str||"")!="guild")
    return (int)notify_fail("Leave what?\n",NOTIFY_NOT_OBJ)&&0;
  if (!is_member(getuid(TP)))
    return (int)notify_fail("You are not a member of this guild.\n",
                            NOTIFY_NOT_VALID)&&0;
  write("Do you really want to leave the guild? (Y/N) ");
  input_to("check_leave",0,TP);
  return 1;
}

public int cmd_banish(string str)
{
  string whom,why,err;
  object pl,*obs;
  int i;

  if (!TP) return 0;
  if (!str)
    return (int)notify_fail("Whom do you want to banish?\n",NOTIFY_ILL_ARG)&&0;
  if (sscanf(lower_case(str),"%s %s",whom,why)!=2)
    whom = lower_case(str),why = 0;
  if ((err = AddBanish(whom,why))!=NO_ERR)
    return (int)notify_fail(err,NOTIFY_NOT_VALID)&&0;
  if (pl = find_player(whom))
    {
      tell_object(pl,"You've been banished from "+guildname+" by "+
                     Banisher(getuid(TP))+".\n");
      if (pl->QueryGuild()==guildclass)
        {
          pl->SetGuild(0);
          pl->SetLevel(0);
          if (guildobject)
            if (i = sizeof(obs = filter(map(all_inventory(pl),
                                                        SF(blueprint)
                                                       ),
                                              lambda(({SYM(v)}),
                                                     ({SF(==),SYM(v),
                                                       guildobject})
                                                    )
                                            )
                         )
              )
              while(i--)
                obs[i]->check_owner(pl);
          pl->SetTitle("the former "+guildclass);
	}
    }
  RemoveMember(whom);
  write("Ok.\n");
  SaveGuild();
  return 1;
}

public int cmd_unbanish(string str)
{
  string err;
  if (!TP) return 0;
  if (!str)
    return (int)notify_fail("Whom do you want to unbanish?\n",NOTIFY_ILL_ARG)&&0;
  str = lower_case(str);
  if ((err = RemoveBanish(str))!=NO_ERR)
    return (int)notify_fail(err,NOTIFY_NOT_VALID)&&0;
  write("Ok.\n");
  return 1;
}

public int cmd_title(string str)
{
  if (!TP) return 0;
  if (!is_member(TP))
    return notify_fail("You are not a member of this guild.\n",
                            NOTIFY_NOT_OBJ),0;
  if (!str)
    {
      TP->SetTitle(GuildLeveltoTitle(TP->QueryLevel(),TP->QueryGender()));
      write("Title changed to default-title at this level.\n");
      return 1;
    }
  if (TP->QueryLevel()<TITLE_LEVEL && QueryLevelTitles()[TP->QueryLevel()])
    return notify_fail(
      "You are not allowed to change your title.\n",NOTIFY_NOT_OBJ),0;
  TP->SetTitle(str);
  write("Ok. Title changed.\n");
  return 1;
}
/*
** Disabled, Thragor 231096

public int cmd_quit()
{
  object *inv,pl;
  pl = previous_object();
  if (!IS_IMMORTAL(pl)
      &&sizeof(inv = filter(all_inventory(pl),SF(check_for_guildob)))
      &&inv[0]->QuerySource())
    inv[0]->gtp();
  return 0;
}
*/

public int wiz_funcs(string str)
// Called with action 'guild'. These are special commands for wizards to check
// this and other guilds.
{
  int i, number;
  string *rega;

  if (!str) str = "help";
  str = lower_case(str);

  return WIZFUNS->startfun(TP,str,REGISTER);
}


/*
* ----------------------------------------------------------------------
* create, init, reset .. the global functions
* ----------------------------------------------------------------------
*/

public varargs string QueryIntLong(mixed what)
{
  if (what) return (::QueryIntLong(what));
  return (::QueryIntLong())+(sign?sign[POS_ROOM]:"")+
         (list?list[POS_ROOM]:"");
}

public void create()
{
  (::create());
  call_out(SF(update_toplist),2+random(5));
  SetIntShort("a guild");
  SetIntLong("You are in a guild.\n");
  Set(P_CANSEE,1);

  AddDetail("guild","You are in it.\n");
  ChangeSign(({"sign","wooden sign"}),
    "It's a wooden sign with golden letters on it:\n",
    "There's a wooden sign ");
  ChangeList(({"list","toplist","small list"}),"",
    "and a small list at the wall.\n");
  AddDetail("wall","There's a sign fixed to it.\n");
  AddRoomCmd("leave",SF(cmd_leave));
  AddRoomCmd("banish",SF(cmd_banish));
  AddRoomCmd("unbanish",SF(cmd_unbanish));
  AddRoomCmd("cost",SF(cmd_cost));
  AddRoomCmd("join",SF(cmd_join));
  AddRoomCmd("advance",SF(cmd_advance));
  AddRoomCmd("help",SF(cmd_help));
  AddRoomCmd("list",SF(cmd_list));
  AddRoomCmd("message",SF(cmd_message));
  AddRoomCmd("title",SF(cmd_title));
  AddRoomCmd("correct",SF(cmd_correct));
  //  AddRoomCmd("quit",SF(cmd_quit)); // to prevent cheating for guildleaders
#ifndef LOSE_XP
  AddItem("/std/thing",REFRESH_HOME,
    ([P_SHORT:"a big red note",
      P_LONG:"It's an important information about the guilds.\n"
             "You should read it.\n",
      P_IDS:({"information","note"}),
      P_ADS:({"big","red","important"}),
      P_READ_MSG:
       "The new guilds got implemented now. Please report every bug you\n"
       "recognize. Some guildobjects offer something like 'gbug', 'gtypo'\n"
       "or 'gidea' to report things about the guild. If not, just go to\n"
       "your guild and use 'bug', 'idea' and 'typo' there.\n"
       "For a short while (perhaps about two months) changing guilds wouldn't\n"
       "cost you any experience points. This is a feature to be able to test\n"
       "different guilds but will vanish in the future again.\n\n"
       "Thanks,\n"
       "Thragor\n\n"
       "P.S.: If there are bugs you may also mail to Rover and me.\n",
      P_NOGET:"It's magically fixed.\n"
    ]));
#endif
  "/obj/lib/register"->Register(ME,REGISTER);
}

public void init()
{
  (::init());
  if (TP&&TP->QueryGuild()==guildclass&&IsBanished(getuid(TP)))
    call_out(SF(banished_entered),1);
  if (TP&&is_member(getuid(TP))&&TP->QueryGuild()==guildclass)
    call_out(SF(member_entered),1);
  if (IS_IMMORTAL(TP)) add_action("wiz_funcs","guild");
}

public int prevent_leave(mixed dest,int method,mixed extra)
{
  object *inv,pl;
  pl = previous_object();
  if (!IS_IMMORTAL(pl)
      &&sizeof(inv = filter(all_inventory(pl),SF(check_for_guildob)))
      &&inv[0]->QuerySource())
    return ME_NO_LEAVE;
  return (::prevent_leave(dest,method,extra));
}

public void reset()
{
  string *m,h;
  m = m_indices(members||([]));
  m -= filter(m,SF(find_savefile));
  map(m,SF(RemoveMember));
  if ((h = QueryGuildleader())&&IS_IMMORTAL(h))
    {
      ChangeStat(h,G_MEMBER);
      if (h=QueryViceGuildleader())
        ChangeStat(h,G_LEADER);
    }
  if ((h = QueryViceGuildleader())&&IS_IMMORTAL(h))
    ChangeStat(h,G_MEMBER);
  call_out(SF(update_toplist),2);
  (::reset());
}

public int allow_enter(int method,mixed extra)
{
  if (object_name(ME) == MYFILE) return ME_NO_ENTER;
  return (::allow_enter(method,extra));
}
