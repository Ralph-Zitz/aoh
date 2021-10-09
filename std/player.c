/* -------------------------------------------------------------------------
 * /std/player.c -- The main player object
 *
 * Copyright (C) 1997 by One Step Beyond.
 * All Rights Reserved.
 * Permission granted for non-commercial use.
 * -------------------------------------------------------------------------
 * NOTE: only secure/login is allowed to clone this file!
 * [Softbyte] 16mar1998  Talent loading, remove
 * [Softbyte] 09oct1998  Unread mail check
 */

#include <attributes.h>
#include <properties.h>
#include <magic.h>
#include <ansi.h>
#include <combat.h>
#include <moving.h>
#include <search.h>
#include <stdrooms.h>
#include <news.h>
#include <classes.h>
#include <money.h>
#include <living.h>
#include <daemons.h>
#include <msgclass.h>
#include <secure/wizlevels.h>
#include <driver/object_info.h>
#include UDP_HEADER

#define TO this_object()
#define TP this_player()
#define TI this_interactive()
#define ME TO
#define PL TP
#define SF(x) #'x /*'*/
#define ENV(x) environment(x)

/* Define this to log the load of the players at every change in /log/WSTAT
 */

/* #define WSTAT 1 */

/* define here how many kills shall be saved
 */

#define NO_LASTKILLS 5

/* define this to change all old archwiz-domain homepathes
 */

#define FIX_ARCHWIZ_HOME

/* *************************************************************************
 * Inherits
 * *************************************************************************
 * NOTE: commands are inherited.
 */

inherit "std/player/commands";
inherit "std/living/description";
inherit "std/living/heart";
inherit "std/living/stats";
inherit "std/player/life";
inherit "std/living/moving";
inherit "std/player/restrictions";
inherit "std/living/combat";
inherit "std/living/commands";
inherit "std/living/attributes";

inherit "std/npc/putget";
inherit "std/npc/cmds";
inherit "std/npc/soul";

inherit "std/player/message";
inherit "std/player/comm";
inherit "std/player/filesys";
inherit "std/player/misc";
inherit "std/player/viewcmd";
inherit "std/player/objects";
/* inherit "std/player/soul"; */
inherit "std/player/telnet_neg";
inherit "std/player/gmcp";
inherit "std/player/mxp";
inherit "std/base";

/* *************************************************************************
 * Global variables - saved
 * *************************************************************************
 */

public string guild,                      /* name of player's guild, or 0 */
  quests,                         /* List of quests, the player fulfilled */
  mailaddr,                                              /* email address */
  called_from_ip,                    /* the ip-number we call/called from */
  called_from_ipname,                  /* the ip-name we call/called from */
  title,                                               /* title of player */
  home,                                   /* room where the player starts */
  citizen,                                /* domain the player belongs to */
  last_news_id,                                       /* the last news-id */
  explored,                       /* bit-string which things got explored */
  pTimeOptions;                                    /* How to display time */


public int pKills,                       /* how often a player got killed */
  saved_value,                   /* Collected autosave-value of inventory */
  pLastLogin,                                   /* Time we logged in last */
  pLastSave,                                         /* Time of last save */
  *pLastKills,                               /* Times of the last 5 Kills */
  pLastXP,                                        /* XP before last death */
  pFirstLogin;                                   /* time() of first login */

public mixed auto_load;                                 /* autoload-array */

public mapping pLastStats;                /* Last Stats before last death */

public mixed pIsNoVoter;           /* TRUE, if the player do not want to vote */
public int pIsWizard;              /* to get the wizard info from the plain */
                                   /* save file */
public int pShowVotes;             /* 1: if the vote shall be shown on login */

/* *************************************************************************
 * Global variables - not saved
 * *************************************************************************
 */

nosave string saved_where,           /* ??? used by move_player_to_startX */
  passw,                               /* temporarily for password change */
  passwold;                            /* temporarily for password change */

nosave int is_guest,                                        /* guest-flag */
#ifdef WSTAT
  last_weight,                           /* used by the weight statistics */
#endif
  player_started;   /* set to 1 once the player has been started properly */

nosave object dead_ob,                                /* the death object */
  *inform_death;                      /* inform these objects about death */

/* *************************************************************************
 * Prototypes
 * *************************************************************************
 */

public varargs int remove();                                   /* forward */
protected void password_entered(string str, int hidden);       /* forward */
protected void new_password_entered(string str, int hidden);   /* forward */
protected void new_password_verified(string str, int hidden);  /* forward */
protected void suicide_verified(string str, int hidden);       /* forward */
protected void gettty(string ttystr);                          /* forward */
private int setstty(string str);                               /* forward */
protected void getmailaddr(string maddr);                      /* forward */
protected void move_player_to_start(string where);             /* forward */
protected void move_player_to_start2(string where);            /* forward */
protected void move_player_to_start3(string where);            /* forward */
protected void move_player_to_start4(string where);            /* forward */
void vote_delay();
static varargs void process_gmcp(mapping attributes,string package, string message);
static int update_points_display();
static void init_gmcp();
public void announce_title(int news);

/* Temporary placement */
string presay;

string SetPresay(string s) {
#if 1
  if (   !this_player()
      || (    this_player() != this_object()
           && query_user_level(this_player())
           && !query_user_level(this_object())
         )
     )
    log_file("TITLES", ctime()+" Presay of "+getuid(this_object())+" set to '"
                      +s+"' by "+(this_player()?getuid(this_player()):"???")
                      +"\n");
#endif
  presay = s;
  announce_title(0);
  return presay;
}
string QueryPresay() { return presay || ""; }

/* *************************************************************************
 * Builtin properties / read-only
 * *************************************************************************
 */

/* -------------------------------------------------------------------------
 * P_CALLED_FROM_IP - last ip number the player called from
 * -------------------------------------------------------------------------
 */

public string QueryCalledFromIp() { return called_from_ip; }

/* -------------------------------------------------------------------------
 * P_CALLED_FROM_IPNAME - last ip name (or number if unresolved)
 * -------------------------------------------------------------------------
 */

public string QueryCalledFromIpName() { return called_from_ipname; }

/* -------------------------------------------------------------------------
 * P_FIRST_TIME - first time the player logged in
 * -------------------------------------------------------------------------
 */

public int QueryFirstLogin() { return pFirstLogin; }

/* -------------------------------------------------------------------------
 * P_LAST_LOGIN - last time the player logged in
 * -------------------------------------------------------------------------
 */

public int QueryLastLogin() { return pLastLogin; }

/* -------------------------------------------------------------------------
 * P_GUEST - set to 1 if the player is a guest character
 * -------------------------------------------------------------------------
 */

public int QueryGuest() { return is_guest; }

/* *************************************************************************
 * Builtin properties / read-write
 * *************************************************************************
 */

/* -------------------------------------------------------------------------
 * P_LAST_STATS - holds the last stats before death
 * -------------------------------------------------------------------------
 */

private mapping SetLastStats(mapping m) { return pLastStats = m; }
public mapping QueryLastStats() { return (pLastStats||([]))+([]); }

/* -------------------------------------------------------------------------
 * P_IS_NO_VOTER - Is the player/wizard an active voter ?
 * -------------------------------------------------------------------------
 */
public mixed QueryIsNoVoter() {return pIsNoVoter;}
private int SetIsNoVoter(mixed i) { pIsNoVoter=i; return 1; }

/* -------------------------------------------------------------------------
 * P_IS_NO_VOTER - Is the player/wizard an active voter ?
 * -------------------------------------------------------------------------
 */
public mixed QueryShowVotes() {return pShowVotes;}
public int SetShowVotes(mixed i) { pShowVotes=i; return 1; }

/* -------------------------------------------------------------------------
 * P_LAST_XP - holds the last xp-value before death
 * -------------------------------------------------------------------------
 */

private int SetLastXP(int i) { return pLastXP = i; }
public int QueryLastXP() { return pLastXP; }

/* -------------------------------------------------------------------------
 * P_KILLS - holds the total amount of kills for the player
 * -------------------------------------------------------------------------
 */

public int SetKills(int v) { return pKills = v; }
public int QueryKills() { return pKills; }

/* -------------------------------------------------------------------------
 * P_LAST_KILLS - holds the times of the last NO_LASTKILLS kills
 * -------------------------------------------------------------------------
 */

public int *SetLastKills(int *v) { return pLastKills = v; }
public int *QueryLastKills() { return (pLastKills||({}))+({}); }

public int AddKill() {
  if (!pLastKills) pLastKills = ({});
  pLastKills+=({time()});
  if (sizeof(pLastKills)>NO_LASTKILLS) pLastKills = pLastKills[<NO_LASTKILLS..];
  return ++pKills;
}

public int RemoveKill() {
  if (!pLastKills||!sizeof(pLastKills)) return 0;
  pLastKills = pLastKills[0..<2];
  --pKills;
  if (pKills<0)
    pKills = 0;
  return pKills;
}

/* -------------------------------------------------------------------------
 * P_DEAD_OB - ???
 * -------------------------------------------------------------------------
 */

public object SetDeadOb(object o) { return dead_ob = o; }
public object QueryDeadOb() { return dead_ob; }

/* -------------------------------------------------------------------------
 * P_INFORM_OBJS - the objects that are informed about death of the player
 * -------------------------------------------------------------------------
 */

public object *SetInformObjs(object *ob) { return inform_death=ob||({}); }
public object *QueryInformObjs() { return inform_death+({}); }

public object *AddInformObj(object ob) { return inform_death+=({ob}); }
public object *RemoveInformObj(object ob) { return inform_death-=({ob}); }

/* -------------------------------------------------------------------------
 * P_LAST_NEWSID - the last newsid
 * -------------------------------------------------------------------------
 */

public string QueryLastNewsId() { return last_news_id; }
public string SetLastNewsId(string s) { return last_news_id = s; }

/* -------------------------------------------------------------------------
 * P_MAILADDR - the players mail address
 * -------------------------------------------------------------------------
 */

public string SetMailAddr(string addr) { return mailaddr = addr; }
public string QueryMailAddr() { return mailaddr; }

/* -------------------------------------------------------------------------
 * P_LAST_SAVE - last time the player got saved
 * -------------------------------------------------------------------------
 */

public int QueryLastSave() { return pLastSave; }
public int SetLastSave(int i) { return pLastSave; }

/* -------------------------------------------------------------------------
 * P_PLAYER_STARTED - returns 1 upon the player object got properly started.
 * -------------------------------------------------------------------------
 */

int QueryPlayerStarted() { return player_started; }
int SetPlayerStarted(int i) { return player_started; }

/* -------------------------------------------------------------------------
 * P_TITLE - the players title
 * -------------------------------------------------------------------------
 */

public void announce_title(int news) {
  if (player_started && !QueryInvis())
    CHANNEL_D->SendEmote( news ? "news" : "title", QueryName(1),
   "is now "+Short(), 1 );
}

public string SetTitle(string t) {
  title = t;
  announce_title(0);
  return title;
}

public string QueryTitle() { return title || ""; }

/* -------------------------------------------------------------------------
 * P_GUILD - the players guild
 * -------------------------------------------------------------------------
 */

public string SetGuild(string g) { return guild = g; }
public string QueryGuild() { return guild; }

/* -------------------------------------------------------------------------
 * P_QUESTS - the players queststring
 * -------------------------------------------------------------------------
 */

public string SetQuests(string q) {
  reallevel = 0;
  return object_name(previous_object()) == QUESTMASTER ? quests = q : 0;
}
public string QueryQuests() { return quests; }

/* -------------------------------------------------------------------------
 * P_EXPLORED - bitmask holding the explorerpoints found
 * -------------------------------------------------------------------------
 */

public void SetExplored() {
  string str;
  object cookie;

  /* if SetExplored() is invoked by an unregistered object, EXPLOREMASTER
   * sets bit 0. Thus we need to ignore bit 0 when evaluating the
   * explored variable.
   * The EXPLOREMASTER uses our previous_object() to check for validity
   */

  str = explored || "";
  explored = ({string})EXPLOREMASTER->SetExplored(str);
  if (str != explored)
  {
    if (getuid(previous_object()) == "daydream")
      switch(random(2))
      {
        case 1:
          str = "\n...and no sign of the Spanish Inquisition!\n"; break;
        case 0:
        default:
          str = "\n42.\n"; break;
      }
    else
      switch(random(4))
      {
        case 3:
          str = "\nYou are sure you've never encountered anything quite like this before.\n"; break;
        case 2:
          str = "\nShocking, isn't it?\n"; break;
        case 1:
          str = "\nQuite an experience, indeed.\n"; break;
        case 0:
        default:
          str = "\nInteresting, isn't it?\n"; break;
      }
    RegisterCmdFun(lambda(({'msg,'unwind})
      , ({#'||, 'unwind
      , ({#'tell_object, this_object(), 'msg }) })
    ), str);
    if (!catch(cookie = clone_object("/std/food")) && cookie)
    {
      cookie->AddId("cookie");
      cookie->SetShort("cookie");
      cookie->SetLong("A small but very delicious cookie.\n");
      cookie->SetValue(0);
      cookie->SetWeight(1);
      cookie->SetFoodKind("food");
      cookie->SetFoodStuff(1);
      cookie->SetFoodMsgs(({"..knrrps..knrrps.. mmmmh!\n",
        "@@Pl_Name@@ eats a cookie.\n"}));
      if (({int})cookie->move(this_object()) != ME_OK)
        cookie->remove();
    }
  }
}

public string QueryExplored() { return explored; }

/* -------------------------------------------------------------------------
 * P_HOME - the players home where he/she starts on logon
 * -------------------------------------------------------------------------
 */

public string SetHome(string str) { return home = str; }
public string QueryHome() { return home; }

/* -------------------------------------------------------------------------
 * P_TIME_OPTIONS  -  display OSB time, RL time, or both
 * -------------------------------------------------------------------------
 */

string SetTimeOptions( string s ) { return pTimeOptions = s; }
string QueryTimeOptions()  { return pTimeOptions; }

/* *************************************************************************
 * Property overwrites
 * *************************************************************************
 */

/* -------------------------------------------------------------------------
 * P_NAME - living name must not be set here
 * -------------------------------------------------------------------------
 */

public varargs string SetName(string n) {
  return ::SetName(n, 1);
}

/* -------------------------------------------------------------------------
 * P_SHORT - prevent setting of P_SHORT by everything/everybody
 * -------------------------------------------------------------------------
 */

public string SetShort(string str) {
  string rc;
  if (   !this_interactive()
      || this_interactive() == this_object()
      || IS_WIZARD(this_interactive())
      || object_name(this_interactive())[0..7] == "/secure/"
      || !str
      || str == capitalize(getuid(this_object()))
     )
    rc = ::SetShort(str);
  else
    rc = ::QueryShort();
  return rc;
}

/* -------------------------------------------------------------------------
 * P_NOSELL - players cannot be sold
 * -------------------------------------------------------------------------
 */

public string QueryNoSell() { return "You can't sell this."; }

/* -------------------------------------------------------------------------
 * P_NOBUY - players cannot be bought too
 * -------------------------------------------------------------------------
 */

public string QueryNoBuy() { return "You can't buy this."; }

/* *************************************************************************
 * Autoloader handling
 * *************************************************************************
 */

private void save_autoobject(object ob, int destruct_autos) {
  mixed args;
  string err;

  if (!objectp(ob))
    log_file("AUTOLOAD",sprintf("%s Object already destructed, user: %s\n",
  ctime(),object_name()) );
  else {
    if (err = catch(args = ({mixed})ob->Query(P_AUTOOBJECT)))
      log_file("AUTOLOAD",sprintf("%s file: %s\nuser %s, save error: %s",
    ctime(),object_name(ob),object_name(),err) );
    else if (args) {
      auto_load[blueprint(ob)] = args;
      if (destruct_autos)
        catch(ob->remove()), ob && destruct(ob);
    }
  }
}

private void restore_autoobject(string file, mixed args) {
  object ob;
  string err;
  int mret;

  if (err = catch(ob = clone_object(file),
    ob->Set(P_AUTOOBJECT, args),
    mret = ({int})ob->move(this_object(), M_NOCHECK))) {
    log_file("AUTOLOAD",sprintf("%s file: %s\nuser %s, load error: %s",
                                ctime(),file,object_name(),err));
    if (ob)
      destruct(ob);
  }
  else if (!ob || mret != ME_OK || environment(ob) != this_object()) {
    log_file("AUTOLOAD",sprintf("%s file: %s\nuser; %s, bad autoobject: %s, "
                                "move: %d\n",
                                ctime(), file, object_name(),
                                (ob ? object_name(ob) : "- selfdestruct -"),
                                mret) );
    if (ob)
      destruct(ob);
  }
}

/* *************************************************************************
 * save_me - saves the player object
 * *************************************************************************
 * NOTES:
 *  save character: we also need to build the autoload list here.
 *  Scan through all objects in the inventory and put objects with
 *   Query(P_AUTOOBJECT) <> 0 into the list.
 */

public void save_me(status value_items, status destruct_autos) {
  if (this_player() != ME)
    return;

  update_age();
  //RecomputeStats();

  auto_load = ([]);
  saved_value = 0;
  pLastSave = time();

  if (value_items) {
    object *inv, ob;
    int value, i;
    mixed args;

    inv = all_inventory();
    for (i = sizeof(inv); i--; ) {
      ob = inv[i];
      if (ob && !catch(args = ({mixed})ob->Query(P_AUTOOBJECT)) && !args
          && !catch(value = ({int})ob->Query(P_VALUE)) && intp(value)
          && value > 0
          ) {
        saved_value += (value < 1000 ? value : 1000);
      }
    }
  }

  filter(all_inventory(), SF(save_autoobject), destruct_autos);

  if (!is_guest)
    save_object("/" SAVEPATH+QueryRealName()[0..0]+"/" + QueryRealName());

  auto_load = 0;
  saved_value = 0;
}

/* *************************************************************************
 * SaveStats - the stats before last death.
 * *************************************************************************
 */

public void SaveStats() {
  SetLastStats(
    ([
      P_CON:QueryCon(),
      P_DEX:QueryDex(),
      P_STR:QueryStr(),
      P_INT:QueryInt(),
      P_WIS:QueryWis(),
      P_CHA:QueryCha(),
      P_QUI:QueryQui(),
      P_AGI:QueryAgi()
    ])
  );
  SetLastXP(QueryXP());
}

/* *************************************************************************
 * Ressurrect - restore last-stats after death
 * *************************************************************************
 */

public void Resurrect() {
  int i;
  string *idx;
  object po;
  mapping laststats;
  po = previous_object();

  if (   !(QueryGhost() || IS_WIZARD(this_player()))
      || !pKills
     )
    return;

  log_file("RESURRECT",sprintf("%s %O resurrected by %O [TP: %O]\n",
         ctime(),ME,po,this_player()));
  idx = m_indices(laststats = QueryLastStats());
  for(i=sizeof(idx);i--;)
    Set(idx[i],laststats[idx[i]]);
  ME->SetXP(QueryLastXP());
  SetLevel(QueryLevel()+1);
  SetGhost(0);
}

/* *************************************************************************
 * Miscellaneous overwrites of inherited functions
 * *************************************************************************
 */

public varargs string Short(string what) {
  string rc;
  int i;
  mixed away;

  rc = ::Short(what);
  if (!rc || what || !query_once_interactive(ME))
    return rc;
  if (QueryNetdead())
    return "a statue of "+capitalize(QueryName());
  if (rc == QueryName())
    rc = QueryPresay()+rc+" "+QueryTitle();
  /* else we're a frog, a ghost, or something else non-natural */
  i = query_idle(ME) / 60;
  away = Query(P_AWAY);
  if (IS_WIZARD(PL))
  {
    if (i || away)
      rc += " (";
    if (away)
      rc += "away";
    if (i && away)
      rc += ", ";
    if (i)
      rc += i+" min. passive";
    if (i || away)
      rc += ")";
  }
  else
  {
    if (i>4 || away)
      rc += " (";
    if (away)
      rc += "away";
    if (i>4 && away)
      rc += ", ";
    if (i>4)
      rc += "passive";
    if (i>4 || away)
      rc += ")";
  }
  return rc;
}

static string * _long(string str) {
  string * rc;
  int i;
  rc = ::_long(str);
  if (i = query_idle(ME) / 60) {
    rc[2] = rc[2][0..<3]; // Cut off trailing ".\n"
    if (IS_WIZARD(PL))
      rc[2] += ", but hasn't moved for "+i+" minutes";
    else if (i > 4)
      rc[2] += ", but hasn't moved for a long time.";
    rc[2] += ".\n";
  }
  return rc;
}

public varargs string Long() {
  if (QueryNetdead() || (QueryGhost() && QueryRace() != "visitor"))
    return "You see "+(Short()||"nothing")+".\n";

  return ::Long();
}

/* *************************************************************************
 * Commands
 * *************************************************************************
 */

/* -------------------------------------------------------------------------
 * add_standard_commands - commands for all kinds of users
 * -------------------------------------------------------------------------
 */

void add_standard_commands() {
  add_action("cmd_prompt","prompt");
  add_action("cmd_save","save");
  add_action("cmd_quit","quit");
  add_action("cmd_petrify", "petrify");
  add_action("cmd_kill", "kill");
  add_action("cmd_password", "password");
  add_action("cmd_suicide", "suicide");
  add_action("cmd_wimpy", "wimpy");
  add_action("cmd_stop","stop");
  add_action("cmd_title","title");
  add_action("cmd_presay","presay");
  add_action("cmd_voting","voting");
  add_filesys_commands();
  add_object_commands();
  add_misc_commands();
}

/* -------------------------------------------------------------------------
 * add_ancient_commands - commands for ancients and wizards
 * -------------------------------------------------------------------------
 */

void add_ancient_commands() {
  if (ME != PL) return;
  add_action("cmd_force", "force");
  add_action("cmd_heal", "heal");
  add_action("cmd_in", "in");
  add_action("cmd_review", "review");
  add_action("cmd_shutdown", "shutdown");
  add_action("cmd_snoop", "snoop");
  add_action("cmd_trans", "trans");
  add_action("finvis", "invis",1);
  add_action("fvis", "vis",1);
  add_action("cmd_wizmode", "wizmode");
}

/* -------------------------------------------------------------------------
 * add_wiz_commands - commands for full wizards
 * -------------------------------------------------------------------------
 */

void add_wiz_commands() {
  if (ME != PL) return;
  add_action("cmd_home", "home");
  add_action("cmd_wizlist", "wizlist");
}

/* -------------------------------------------------------------------------
 * cmd_password - the password command
 *  This has to stay here always because the password must not be a prop.
 * -------------------------------------------------------------------------
 */

static int cmd_password(string str) {
  int hidden;

  hidden = 1;
  if (stringp(str))
  {
    str = norm_id(str);
    if ("hidden" == str) hidden = 1;
    else if ("open" == str) hidden = 0;
    else
    return notify_fail("Usage: password [hidden|open]\n"
     "Nothing changed.\n",
     NOTIFY_NOT_VALID);
  }
  msg_write(CMSG_GENERIC|MMSG_DIRECT, "Please enter old password: ");
  input_to(#'password_entered, hidden, hidden); /*'*/
  return 1;
}

/* password_entered - called from cmd_password
 */

protected void password_entered(string str, int hidden) {
  if (hidden)
    write("\n");
  if (!str) str == "";
  if (({int})MASTER->update_password(str,str) == 0)
    return msg_write(CMSG_GENERIC|MMSG_DIRECT, "Wrong password!\n");
  passwold = str;
  msg_write(CMSG_GENERIC|MMSG_DIRECT, "Please enter new password: ");
  input_to(#'new_password_entered /*'*/,hidden,hidden);
}

/* new_password_entered - called from password_entered
 */

protected void new_password_entered(string str, int hidden) {
  if (hidden)
    msg_write(CMSG_GENERIC|MMSG_DIRECT, "\n");
  if ((!str) || str == "")
    return msg_write(CMSG_GENERIC|MMSG_DIRECT, "Aborted!\n");
  if (sizeof(str) < 6)
  {
    msg_write(CMSG_GENERIC|MMSG_DIRECT, "Password must have at least 6 characters!\n"
    "Please enter new password: ");
    input_to(#'new_password_entered,hidden,hidden); /*'*/
    return;
  }
  passw = str;
  msg_write(CMSG_GENERIC|MMSG_DIRECT, "To ensure, that you typed the new password correctly please "
  "type it again: ");
  input_to(#'new_password_verified,hidden,hidden); /*'*/
}

/* new_password_verified - called from new_password_entered
 */

protected void new_password_verified(string str, int hidden) {
  if (hidden)
    msg_write(CMSG_GENERIC|MMSG_DIRECT, "\n");
  if (!str || str != passw)
    return msg_write(CMSG_GENERIC|MMSG_DIRECT, "That was different! Password not changed.\n");
  if (({int})MASTER->update_password(passwold,passw))
    msg_write(CMSG_GENERIC|MMSG_DIRECT, "Your password has been changed.\n");
}

/* -------------------------------------------------------------------------
 * cmd_suicide - the suicide command
 *  This has to stay here always because the password must not be a prop.
 * -------------------------------------------------------------------------
 */

static int cmd_suicide(string arg) {
  int _hidden;

  /* You mustn't be forced to suicide! */
  if (TI != ME || TI != PL || !TI)
    return 0;

  _hidden = 1;
  if (stringp(arg)) {
    arg = norm_id(arg);
    if ("hidden" == arg) _hidden = 1;
    else if ("open" == arg) _hidden = 0;
    else
      return notify_fail("Usage: suicide [hidden|open]\nNothing changed.\n",
                         NOTIFY_NOT_VALID);
  }
  msg_write(CMSG_GENERIC|MMSG_DIRECT, "You are going to commit suicide. Do you really want that?\n"
    "If yes, enter your password as verification.\n"
    "If not, enter something else or just press return to abort.\n"
    "Password? ");
  input_to(#'suicide_verified, _hidden, _hidden); /*'*/
  return 1;
}

/* suicide_verified - called from cmd_suicide
 */

protected void suicide_verified(string str, int hidden) {
  int rc;

  if (hidden)
    write("\n");
  if (!str || str == "" || !({int})MASTER->update_password(str,str))
    return msg_write(CMSG_GENERIC|MMSG_DIRECT, "Well, staying alive is much more fun, isn't it?\n");

  msg_write(CMSG_GENERIC|MMSG_DIRECT, "Well...\n\n");
  rc = ({int})"/secure/remover"->remove_player(getuid(ME));
  if (!rc)
    msg_write(CMSG_GENERIC|MMSG_DIRECT, "You can't do it now.\n");
  else
  {
    msg_write(CMSG_GENERIC|MMSG_DIRECT,
      "\nWe thank you for visiting " MUDNAME " and hope you "
      "enjoyed it here.\n"
      "\n            === Live Long And Prosper! ===\n\n"
    );
    catch(remove());
    if (ME)
      destruct(ME);
  }
}

/* -------------------------------------------------------------------------
 * cmd_save - the save command
 * -------------------------------------------------------------------------
 */

public int cmd_save(string arg) {
  save_me(1, 0);
  msg_write(CMSG_GENERIC|MMSG_DIRECT, "Ok.\n");
  return 1;
}

/* -------------------------------------------------------------------------
 * cmd_quit - the quit command
 * -------------------------------------------------------------------------
 */

public int cmd_quit(string arg) {
  if (arg) return 0;
  msg_write(CMSG_GENERIC|MMSG_DIRECT, "Saving "+capitalize(QueryRealName())+".\n");
  auto_load = ([]);
  pLastSave = time();
  filter(all_inventory(), SF(save_autoobject), 1 /* destruct them */);
  map_objects(filter_objects(all_inventory(), "Query", P_NO_QUITDROP),
        "remove");
  if (!query_user_level(ME) && !is_guest)
    Drop("really all");

  if (environment(this_object()))
    environment(this_object())->PlayerNetdead(-1);
  catch(map_objects(deep_inventory(this_object()), "PlayerNetdead", -1));
  catch(map_objects(QueryFollowers()||({}), "PlayerNetdead", -1));
  PARTYOBJECT->PlayerNetdead(-1);

  if (!is_guest)
    save_object("/"+SAVEPATH+QueryRealName()[0..0]+"/" + QueryRealName());
  auto_load = 0;
  process_gmcp((["msg":"Goodbye!"]),"Core","Goodbye");
  if (!QueryInvis()) say(QueryName() + " left the world.\n");
  log_file("ENTER", ctime()[4..15]+" quit \t"+query_ip_name()+" \t"
                    +getuid(this_object())+"\n");
  remove();
  return 1;
}

/* -------------------------------------------------------------------------
 * cmd_petrify - the petrify-command
 * -------------------------------------------------------------------------
 */

public int cmd_petrify(string arg) {
  if (arg) return 0;
  if (is_guest) return cmd_quit(0); /* Do not petrify guests */

  msg_write(CMSG_GENERIC|MMSG_DIRECT, "Saving "+capitalize(QueryRealName())+".\n");
  save_me(1,0);
  if (!QueryInvis()) say(capitalize(QueryName()) + " petrifies..\n");
  log_file("ENTER", ctime()[4..15]+" petrify \t"+query_ip_name()+" \t"
                    +getuid(this_object())+"\n");
  remove_interactive(PL);
  QueryNetdead();
  return 1;
}

/* -------------------------------------------------------------------------
 * cmd_kill - the kill command
 * -------------------------------------------------------------------------
 */

public int cmd_kill(string str) {
  object eob, *eobs = ({});
  if (!str || str == "") { return notify_fail("Kill WHOM?\n", NOTIFY_ILL_ARG); }
  if (QueryGhost())
    return notify_fail("You can't do that in your present state.\n", NOTIFY_NOT_VALID);
  if ("all" == lower_case(str)) {
    string *names = ({});
    eobs = (search(ME, lower_case(str), SEARCH_ENV) || ({})) - ({ ME });
    eobs -= filter(eobs, #'query_once_interactive /*'*/);
    eobs = filter(eobs, #'living /*'*/);
    if (!sizeof(eobs))
      return notify_fail("You can't see any creatures here to attack!\n", NOTIFY_ILL_ARG);
    names = map(eobs, (: return ({string})$1->Short(); :));
    msg_write(CMSG_COMBAT_SELF, "You attack " + implode(names[..<2], ", ") + ", and " + names[<1] + "!\n");
    msg_room(ENV(ME), CMSG_COMBAT_OTHERS|MMSG_SEE,
      ({ Query(P_NAME) + " attacks " + implode(names[..<2], ", ") + ", and " + names[<1] + "!\n",
         "You hear the sounds of a fight breaking out around you from multiple sources!\n"
      }),
      (({ ME }) + eobs)
    );
    map(eobs, #'Kill /*'*/);
    return 1;
  }
  else
    eob = SearchEnv(lower_case(str));
  if (!CanSee() || !eob || !objectp(eob) || !living(eob))
    return notify_fail("You can't see that creature here!\n", NOTIFY_ILL_ARG);
  if (!living(eob) || !({int})eob->Query(P_IS_LIVING))
    return notify_fail("That is dead already. In fact, it never was alive.\n", NOTIFY_NOT_VALID);
  if (eob == this_object())
  {
    switch(random(3))
    {
      case 0:
        return notify_fail("Ouch, that would hurt!\n", NOTIFY_NOT_VALID);
        break;
      case 1:
        return notify_fail("You think suicide's an alternative? You might be sorry.\n", NOTIFY_NOT_VALID);
        break;
      case 2:
        return notify_fail("Interesting concept.\n", NOTIFY_NOT_VALID);
        break;
      default:
        return notify_fail("That's not logical.\n", NOTIFY_NOT_VALID);
    }
    return notify_fail("That's not logical.\n", NOTIFY_NOT_VALID);
  }
  if (!query_once_interactive(eob)) {
    msg_write(CMSG_COMBAT_SELF, "You attack " + ({string})eob->Short() + "!\n");
    msg_room(ENV(ME), CMSG_COMBAT_OTHERS|MMSG_SEE,
      ({ Query(P_NAME) + " attacks " + ({string})eob->Short() + "!\n",
         "You hear sounds of a fight breaking out around you!\n"
      }),
      ({ ME, eob })
    );
    Kill(eob);
  }
  return 1;
}

/* -------------------------------------------------------------------------
 * cmd_stop - the stop command
 * -------------------------------------------------------------------------
 */

public int cmd_stop(string str) {
  if (str == "?")
  {
    notify_fail("stop [attack]\tStops current attacking.\n"
      "stop hunt\tStops all hunts.\n"
      "stop cast\tTries to stop current spellcasting.\n");
    return 0;
  }
  if (!str || str == "attack") { enemies = ({}); write ("Ok.\n"); return 1;}
  if (str == "hunt") { hunters = ({}); write ("Ok.\n"); return 1;}
  if (str != "cast") return 0;
  if (!magicobj) { write ("You are not casting any spell.\n"); return 1;}

    /* It's task of the spell object to output messages... */
  if (({int})magicobj->StopCast())
    magicobj = 0;
  return 1;
}

/* -------------------------------------------------------------------------
 * cmd_trans - the trans command
 * -------------------------------------------------------------------------
 */

static int cmd_trans(string str) {
  object living;

  if (!str)
    return notify_fail("Trans WHAT?\n",NOTIFY_NOT_OBJ);
  if (!IS_IMMORTAL(this_object()) && QuerySP() < TELEPORT_COST)
    return notify_fail("Too low on power.\n",NOTIFY_NOT_VALID);
  if (!(living = find_living(str)))
    return write("That creature doesn't exist!\n"),1;
  tell_object(living,"You feel a strange sensation...\n");
  if (({int})living->move(environment(ME), M_TELEPORT) != ME_OK)
  {
    write(({string})living->QueryName()+" resisted to come!\n");
    tell_object(living, QueryName()
      +" tried to teleport you, but failed!\n");
    if (!IS_IMMORTAL(this_object())) ReduceSP(TELEPORT_COST/2, -1);
      return 1;
  }
  tell_object(living,"You feel a bit unoriented...\n");
  write("OK.\n");
  if (!IS_IMMORTAL(this_object())) ReduceSP(TELEPORT_COST, -1);
  return 1;
}

/* -------------------------------------------------------------------------
 * cmd_force - the force command
 * -------------------------------------------------------------------------
 */

static int cmd_force(string str) {
  string *words;
  object living;
  string what;
  string living_name;

  if (!str)
    return notify_fail("Force WHAT?\n");
  words = explode(str," ");
  if (sizeof(words) < 2)
    return notify_fail("Force WHO WHAT?\n");
  living_name = words[0];
  what = implode(words[1..sizeof(words)-1]," ");
  if (!(living = find_living(living_name)))
  {
    write("That creature doesn't exist!\n");
    return 1;
  }
  write((({int})living->command_me(what)?"Success!\n":"Failed!\n"));
  return 1;
}

/* -------------------------------------------------------------------------
 * cmd_snoop - the snoop command
 * -------------------------------------------------------------------------
 */

static int cmd_snoop(string str) {
  object snoopee;
  object *u;
  int i;

  if (str == "?")
    return notify_fail("snoop\n  Say who you are actually snooping.\n"
         "snoop <name>\n  Start snoop on player <name>\n"
         "snoop off\n  Stop current snoop.\n  Syn.: snoop stop\n"
         "snoop check\n  List all users currently snooped.\n"),0;

  /* Check who is snooped by us */
  u = users();
  snoopee = 0;
  for (i = 0; i < sizeof(u); i++)
    if (query_snoop(u[i]) == this_object())
    {
      snoopee = u[i];
      break;
    }
    if (!str)
    {
      write ("You are snooping "+
       (snoopee ? capitalize(({string})snoopee->QueryRealName()) : "nobody")+".\n");
      return 1;
    }
  str = lower_case (str);
  if (str == "check")
  {
    object *user, snooper;
    mixed data;

    for (i = 0, user = users(), data = ({}); i < sizeof(user); i++)
      if (snooper = query_snoop (user[i])) data += ({({ user[i], snooper })});
    if (!sizeof (data)) write ("Nobody is snooped.\n");
    else
    {
      write (sizeof(data)+" of "+sizeof(user)+" users are snooped.\n");
      for (i = 0; i < sizeof(data); i++)
        write ("  "+capitalize(({string})data[i][0]->QueryRealName())
          +"\t by "+capitalize(({string})data[i][1]->QueryRealName())+"\n");
    }
    return 1;
  }

  if ("stop" == str || "off" == str)
  {
    if (!snoopee)
      return notify_fail("You aren't snooping anyone.\n");
    if (!snoop (this_object()))
      return notify_fail ("Oops, can't stop snoop on "
        +capitalize(({string})snoopee->QueryRealName())+"?\n");
    write ("Stopped snoop on "+capitalize(({string})snoopee->QueryRealName())+".\n");
    return 1;
  }

  if (snoopee && !snoop (this_object()))
    return notify_fail ("Oops, can't stop snoop on "
    +capitalize(({string})snoopee->QueryRealName())+"?\n");

  if (!(snoopee = find_player(str)))
    return notify_fail ("Can't find player '"+str+"'.\n");

  if (!snoop (this_object(), snoopee))
  {
    notify_fail ("Oops, can't snoop "+capitalize(({string})snoopee->QueryRealName())
      +".\n");
    snoopee = 0;
    return 0;
  }
  write("Snooping "+capitalize(({string})snoopee->QueryRealName())+".\n");
  return 1;
}

/* -------------------------------------------------------------------------
 * cmd_in - execute a command in another room
 *  The player will move to another destination, execute a command
 *  and return to the old destination then.
 * -------------------------------------------------------------------------
 */

static int cmd_in(string str) {
  object room;
  object old_room;
  string cmd;
  if (!str) return 0;
  if (sscanf(str, "%s %s", room, cmd) != 2)
    return write("Usage: in ROOM CMD\n"),1;
  if (!room)
    return write("Invalid file name.\n"),1;
  old_room = environment();
  move_object(ME, room);
  command(cmd);
  if (old_room) move_object(ME, old_room);
  else write("Could not go back again.\n");
  return 1;
}

/* -------------------------------------------------------------------------
 * cmd_shutdown - the shutdown command
 * -------------------------------------------------------------------------
 */

static int cmd_shutdown(string str) {
  int tim;
  string foo;

  foo = 0;
  if ( str && (    sscanf (str, "time %d %s", tim, foo) == 2
  || sscanf (str, "%d %s", tim, foo) == 2
        )
     )
    str = foo;
  else
    tim = 5;

  if (!str || !sizeof(str))
    return notify_fail("Shutdown why?\n"),0;

  if (geteuid(this_object()) == "superj")
    write("\nBe afraid ... be very afraid...\n\n");

  if (tim <= 0)
  {
    write("Ok, shutting down immediately.\n");
    shout("Game is shut down by " + QueryRealName() + ".\n");
    log_file("GAME_LOG", ctime(time())+" Game shutdown by "+
       QueryRealName()+".\n");
    if (str) log_file ("GAME_LOG", "  Reason: "+str+"\n");
    else log_file ("GAME_LOG", "  No reason given.\n");
    shutdown();
  }
  else
  {
    object arma;
    arma = load_object(SHUTDEMON);
    write("Ok, shutting down in "+tim+" minutes.\n");
    arma->slow_shut(tim);
    if (str)
    {
      arma->set_reason(str);
      log_file ("GAME_LOG", "  Reason: "+str+"\n");
    }
    else log_file ("GAME_LOG", "  No reason given.\n");
  }
  return 1;
}

/* -------------------------------------------------------------------------
 * cmd_wizmode - the wizmode command
 * -------------------------------------------------------------------------
 */

static int cmd_wizmode(string str) {
  if (!str)
  {
    if (Query(P_NOWIZ))
      write("Wizmode is off, acting as a player.\n");
    else
      write("Wizmode is on, acting as a normal wizard.\n");
    return 1;
  }
  if (str == "off")
  {
    write("Wizmode is off, acting as a player.\n");
    Set(P_NOWIZ, 1);
    return 1;
  }
  if (str == "on")
  {
    write("Wizmode is on, acting as a normal wizard.\n");
    Set(P_NOWIZ, 0);
    return 1;
  }
  write("Usage: wizmode [on | off]\n");
  return 1;
}

/* -------------------------------------------------------------------------
 * cmd_review - the review command
 * -------------------------------------------------------------------------
 */

public int cmd_review(string arg) {
  string *how, *who;
  mixed msg;
  object pl;
  if (!arg) pl = PL;
  else
    pl = find_living(arg);
  if (!pl)
    return notify_fail("No living with name "+arg+".\n",
         NOTIFY_NOT_VALID);
  who = ({ capitalize(({string})pl->QueryRealName()||({string})pl->QueryName()), "Something" });
  if (pl==PL)
    write("Your current moving messages are:\n");
  else
    write(add_gen(who[0])+" current moving messages are:\n");
  msg = ({mixed})pl->QueryMsgOut();
  how = pointerp(msg) ? msg : ({ msg, "leaves" });
  write ("mout:  "+who[0]+" "+how[0]+" <direction>.\n"
  +"       "+who[1]+" "+how[1]+".\n"
  );
  msg = ({mixed})pl->QueryMsgIn();
  how = pointerp(msg) ? msg : ({ msg, "arrives" });
  write ("min:   "+who[0]+" "+how[0]+".\n"
  +"       "+who[1]+" "+how[1]+".\n"
  );
  msg = ({mixed})pl->QueryMMsgOut();
  how = pointerp(msg) ? msg : ({ msg, "disappears" });
  write ("mmout: "+who[0]+" "+how[0]+".\n"
  +"       "+who[1]+" "+how[1]+".\n"
  );
  msg = ({mixed})pl->QueryMMsgIn();
  how = pointerp(msg) ? msg : ({ msg, "comes into existance" });
  write ("mmin:  "+who[0]+" "+how[0]+".\n"
  +"       "+who[1]+" "+how[1]+".\n"
  );
  return 1;
}

/* -------------------------------------------------------------------------
 * cmd_prompt - the prompt command
 * -------------------------------------------------------------------------
 */

public int cmd_prompt(string arg) {
  if (!arg)
    msg_write(CMSG_GENERIC, "Your prompt is: \""+QueryPrompt()+"\".\n");
  else
    SetPrompt(arg);
  return 1;
}

/* -------------------------------------------------------------------------
 * cmd_title - the title command
 * -------------------------------------------------------------------------
 */

public int cmd_title(string t) {
  if (!t)
    return msg_write(CMSG_GENERIC, "Your title is " + title + ".\n"),1;
  SetTitle(t);
  return 1;
}

/* -------------------------------------------------------------------------
 * cmd_presay - the presay command
 * -------------------------------------------------------------------------
 */

public int cmd_presay( string t ) {
  if ( ! t )
    return msg_write(CMSG_GENERIC, "Your presay is \""+QueryPresay()+"\".\n" ), 1;

  if ( t == "0" )
    SetPresay( "" );
  else {
    if ( t[<1] != ' ' )
      t += " ";
    SetPresay( t );
  }
  return 1;
}

/* -------------------------------------------------------------------------
 * cmd_heal - the heal command
 * -------------------------------------------------------------------------
 */

static int cmd_heal(string name) {
  object ob;
  string it;

  if (!name) return 0;
  it = lower_case(name);
  ob = find_living(it);
  if (!ob)
    return notify_fail("No such person is playing now.\n", NOTIFY_ILL_ARG);
  ob->Heal(100000);
  msg_object(ob, CMSG_COMBAT, "You are healed by " + QueryName() + ".\n");
  msg_write(CMSG_GENERIC, "Ok.\n");
  return 1;
}

/* -------------------------------------------------------------------------
 * cmd_wimpy - the wimpy command
 * -------------------------------------------------------------------------
 */

static int cmd_wimpy(string arg) {
  int w;
  if (arg && (!sscanf(arg, "%d", w) || w < 0))
    return notify_fail("Usage: wimpy <positive number>\n",
         NOTIFY_ILL_ARG);
  if (arg)
    SetWimpy(w);
  if (w = QueryWimpy())
    msg_write(CMSG_GENERIC, "Wimpy mode ("+w+").\n");
  else
    msg_write(CMSG_GENERIC, "Brave mode.\n");
  return 1;
}

/* -------------------------------------------------------------------------
 * cmd_home - the home command
 * -------------------------------------------------------------------------
 */

static int cmd_home(string str) {
  int rc;
  if (   !home ||  home == ""
      || catch(rc = move(home, M_TELEPORT))
      || rc != 1)
    rc = move( STARTROOM, M_TELEPORT);
  return rc;
}

static int cmd_wizlist(string str) {
  wizlist(str);
  return 1;
}

/* *************************************************************************
 * command_me - let the player do a command
 *  Force the player to do a command. The force_us() function isn't
 *  always good, because it checks the level of the caller, and this
 *  function can be called by a room.
 * *************************************************************************
 */

public int command_me(string cmd) {
  if (ME != TI) {
    if (TI) msg_object(ME, CMSG_GENERIC, ({string})TI->QueryName()+" forces you to "+cmd+".\n");
    else if (!TI && getuid(previous_object()) != getuid(ME))
      msg_object(ME, CMSG_GENERIC, getuid(previous_object())+" forces you to "+cmd+".\n");
  }
  return ::command_me(cmd);
}

/* *************************************************************************
 * Start and restart player functions
 *  start_player: This function is called by the login object
 *  restart_player: as well, to restart once netdead players
 *  move_player_to_startX : used by start_player
 *  plus misc. helpfuns
 * *************************************************************************
 */

public void restart_player(string name) {
  int nmails,umails;

  set_no_buffering( 0 ); /* IMPORTANT! */
  QueryNetdead(); /* does all the restarting */
  if (!IS_IMMORTAL(ME) || !QueryInvis())
    CHANNEL_D->SendEmote("news"
   , capitalize(getuid(ME)), "becomes alive again", 1);
  if (called_from_ip && query_ip_number() != called_from_ip)
    write("Your last login was from " + called_from_ipname +
          ( ( called_from_ip != called_from_ipname ) ?
            " (" + called_from_ip + ")" :
            "" ) +
          ".\n" );

  called_from_ipname = query_ip_name();
  called_from_ip = query_ip_number();

  pLastLogin = time(); /* store our login time */

  autosave(1);
  log_file("ENTER", ctime()[4..15]+" restart \t"+query_ip_name()+" \t"
                    +getuid(this_object())+"\n");
  //NOSTY
#if 0
#if __EFUN_DEFINED__(set_prompt)
  efun::set_prompt( #'print_prompt /*'*/, this_object() );
#else
  set_prompt( #'print_prompt /*'*/, this_object() );
#endif
#endif
  // set_prompt("> ", this_object() );
  promptpend = 1; /* we need to print a prompt here sometimes */
  GAMECLOCK->StartBing();

  if (nmails = ({int})MAILDEMON->FingerMail(QueryRealName()))
  {
    umails=({int})MAILDEMON->FingerUnreadMail(QueryRealName());
    write("There are "+nmails+" new mails ("+umails+" unread) waiting for you.\n");
  }
  // Softbyte dec97 for voting system
  if (IS_IMMORTAL(ME)) pIsWizard=1;
  else pIsWizard=0;
  call_out("vote_delay",1);
  call_out(#'init_gmcp, 1);
  call_out(function void()
  {
    process_gmcp(
      ([ // Char.Name initially. (wizard)
        "name":capitalize(Query(P_REALNAME)),
        "fullname":Short(),
        "gender":QueryGenderString(),
        "wizard":IS_WIZARD(this_object()),
      ]),"Char","Name"); // Explain fields
    process_gmcp(([]),"Char","StatusVars"); // Explain fields once
    update_points_display();
  }, 2);
#ifdef FIX_ARCHWIZ_HOME
  if ( ( home ) &&
       ( home != "" ) &&
       ( strstr( home, "archwiz" ) != -1 ) ) {
    write( "Your home-location got changed to the Silvere harbour.\n" );
    home = STARTROOM;
  }
#endif
}

private int setstty(string str)
  // called from gettty
{
  str = lower_case(str||"");
  switch(str)
    {
    case "vt100":
      msg_write( CMSG_GENERIC, "This should be "+ANSI_BOLD+" bold%^RESET%^\n");
      SetTTY("vt100");
      return 1;
    case "ansi":
      SetTTY("ansi");
      msg_write( CMSG_GENERIC, "This should be %^RED%^red%^RESET%^\n");
      return 1;
    case "bsx":
      write("Sorry, bsx is only a nice daydream yet.\n");
      return 0;
    case "dumb":
      write("How boring! Get a new terminal!\n");
      SetTTY("dumb");
      return 1;
    default:
      write("This terminal type is not supported. Use one of the following:\n"
      "vt100, ansi, bsx or dumb.\n");
      return 0;
    }
  return 0;
}

protected void output_wiz_news()
{
  object news;
  mixed headers, *note;
  string text, line;
  int    nr;

  if (   !(news = load_object(NEWSSERVER))
      || !pointerp(headers = ({mixed})news->GetHeaders("wiznews"))
      || !sizeof(headers)
     )
    return;
  nr = 0;
  if (stringp(last_news_id))
    while (nr < sizeof(headers) && headers[nr][M_ID] <= last_news_id)
      nr++;
  if (nr >= sizeof(headers))
    return;
  line =
"--------------------------------------------------------------------------\n";
  text = line[0..31]+" WIZARD NEWS "+line[45..];
  for (; nr < sizeof(headers); nr++)
    {
      note = ({mixed *})news->GetNote("wiznews", nr+1);
      last_news_id = note[M_ID];
      text += note[<1]+line;
    }
  "/lib/string"->smore( text, QueryPageSize()
      , this_object(), "QueryLastId" // dummy
      , 1);
}

protected void move_player_to_start4(string where)
{
  int nmails,umails;
  enable_commands();

#ifdef MWHOD_DEMON
  /* call mudwho to register player */
  MWHOD_DEMON->send_user_login(this_player());
#endif

  set_no_buffering( 0 ); /* Paranoia .) */
  // NOSTY
#if 0
#if __EFUN_DEFINED__(set_prompt)
  efun::set_prompt( #'print_prompt /*'*/, this_object() );
#else
  set_prompt( #'print_prompt /*'*/, this_object() );
#endif
#endif
  AddHeart(HEART_BODY); // to be sure
  ValidizeHeart();
/*  config_soul(); */
  add_bin_commands();
  add_standard_commands();
  add_view_commands();
  add_comm_commands();
  add_npc_cmds();
  add_putget_cmds();
  add_soul_cmds();
//  add_abil_commands(); // Softbyte 19nov97
  if (IS_IMMORTAL(PL))
  {
    add_ancient_commands();
    Set(P_CURRENTDIR,WPATH+QueryRealName());
  }
  if (IS_WIZARD(PL))
    add_wiz_commands();
  cat("/etc/NEWS");

#ifdef FIX_ARCHWIZ_HOME
  if ( ( home ) &&
       ( home != "" ) &&
       ( strstr( home, "archwiz" ) != -1 ) ) {
    write( "Your home-location got changed to the Silvere harbour.\n" );
    home = STARTROOM;
  }
#endif

  if (   !home
      || home == ""
      || catch(nmails = move(home, M_TELEPORT))
      || nmails != 1
     )
  {
    move( STARTROOM, M_TELEPORT);
  }

  CHANNEL_D->RegisterUser( this_object() );
  if (QueryInvis() && !IS_WIZARD(PL))
    fvis();
  if (!QueryInvis())
  {
    say(capitalize(QueryShort()) + " enters the world.\n");
    CHANNEL_D->SendEmote("news", capitalize(QueryRealName()),
   "enters the world", 1);
  }
  else
  {
    CHANNEL_D->SendEmote("general", capitalize(QueryRealName()),
  	         "enters the world (invis)", 1);
    write("YOU ARE INVISIBLE !\n\n");
  }

  if (called_from_ip && query_ip_number() != called_from_ip && !is_guest)
    write("Your last login was from " + called_from_ipname +
          ( ( called_from_ip != called_from_ipname ) ?
            " (" + called_from_ip + ")" :
            "" ) +
          ".\n" );

  called_from_ipname = query_ip_name();
  called_from_ip = query_ip_number();

  pLastLogin = time(); /* store our login time */

  set_living_name(QueryRealName());

  if (pointerp(auto_load))
    auto_load = apply(#'mkmapping, auto_load); // ');
  if (mappingp(auto_load))
    walk_mapping(auto_load, #'restore_autoobject); // ');
  auto_load = 0;

#if 0
  /* Commented due to:
   * Going linkdead will let players clone more money without actually Having
   * lost any in the first place
   */
  if (saved_value) /* Player lost possessions since last autosave */
  {
    object ob;

    ob=present(MONEY_NAME,PL);
    if (!ob)
    {
      ob=clone_object(MONEY);
      ob->move(PL);
    }
    ob->AdjustMoney((mixed *)MONEY_DEMON->CreateMoney(saved_value, 0),0);
    if (ob && !sizeof((mapping)ob->QueryMoney()))
      ob->remove();
    write("You find "+saved_value+" coins.\n");
    saved_value = 0;
  }
#endif

  if (IS_IMMORTAL(PL))
    output_wiz_news();
  if (nmails = ({int})MAILDEMON->FingerMail(QueryRealName()))
  {
    umails=({int})MAILDEMON->FingerUnreadMail(QueryRealName());
    write("There are "+nmails+" new mails ("+umails+" unread) waiting for you.\n");
  }

  // Softbyte dec97 for voting system
  if (IS_IMMORTAL(ME)) pIsWizard=1;
  else pIsWizard=0;
  call_out("vote_delay",1);

  player_started = 1;
  RACEMASTER->EquipPlayer(PL);
  autosave(1);

  /* In case, reactive the clocktower (and in December Santa Claus, too) */
  GAMECLOCK->StartBing();

  /* write( "\n+++ Divide By Cucumber Error. Please Reinstall Universe And Reboot +++\n\n" ); */

  /* PrintPrompt(); */
}

protected void move_player_to_start3(string where)
{
  if (tty == "none")
  {
    write("Please select a terminal type (You can change later with "
    "<config stty>)\n"
    "vt100, ansi, bsx, dumb [dumb] ");
    input_to(#'gettty, 0); /*'*/
    return;
  }
  move_player_to_start4(where);
}

protected void move_player_to_start2(string where)
{
/* Genders are set together with the race
  if (gender <= 0) {
    write("Are you male or female: ");
    input_to("getgender", 0);
    return;
  }
*/
  move_player_to_start3(where);
}

/*
 * Checks for several player info:
 * - email address
 * - gender
 * - terminal
 */

protected void move_player_to_start(string where)
{
  if (!is_guest && (!mailaddr || mailaddr == ""))
  {
    write("Please enter your email address (or 'none'): ");
    saved_where = where;
    input_to(#'getmailaddr); /*'*/
    return;
  }
  move_player_to_start2(where);
}

public void start_player(string str, int isg)
  // <isg> = is_guest
{
  int newflag;  /* could player be restored? */
  string str1, realname;
  mixed rc;


  sscanf(object_name(previous_object()), "%s#",str1);
  if (str1 != "/secure/login" && str1 != "/secure/vote") return 0;

  is_guest = isg > 0;
  ::SetName(capitalize(str), 1);
  realname = QueryRealName();

  /* try to restore player. If it doesn't exist, set the new flag */
  if (!is_guest)
  {
    string fname;

    fname = SAVEPATH+realname[0..0]+"/"+realname;
    if (file_size(fname+".o") < 0)
      pFirstLogin = time();
    rc = catch(newflag = !restore_object(fname));
  }
  else
    rc = catch(newflag = !restore_object(SAVEPATH "g/guest"));

  // TODO: Remove this compat after everyone is converted
  if (!stringp(pTimeOptions))
    pTimeOptions = TIME_ALL;

  if (!QueryGhost() || newflag)
    // This needs to be done to have 'ghost' autoloading but e. g.
    // 'frog' not.
    SetLState(LSTATE_NORMAL);

  if (rc) write ("***** You're character couldn't be restored properly.\n");
  if (is_guest)
  {
    // Re-Init channels in case previous guest modified them.
    ChannelOn("players");
    ChannelOn("news");
    ChannelOn("title");
  }
  Set(P_CLEAN_UP, 0);  /* Make sure that players are here to stay! */
  Set(P_SHORT, capitalize(realname)); // If it got corrupted

#if 0
  /* Savefile conversion deactivated */
  catch( fconv = clone_object("/std/player/saver")
   , fconv->convert_savefile(realname)
       );
#endif

  attributes::RestoreAttrs();

  if (catch(RACEMASTER->RestoreRace(ME)))
  {
    write ("***** Your character had an invalid race.\n");
    SetRace("visitor");
    rc = 1;
  }

  if (is_guest) title = "the " MUDNAME " visitor";

  if (QueryRace() == "visitor")
  {
    title = "the new player";
    //SetMMsgIn("drifts around");
    //SetMMsgOut("blows");
  }

  /* If this is a new character, we call the adventurers guild to get
   * our first title !
   */

  if (QueryLevel() < 0)
  {
    SetLevel(1);
    SetHP(QueryMaxHP());
    SetFood(QueryMaxFood());
    SetDrink(QueryMaxDrink());
  }

  InitColourTranslation();
  InitLoadedTalents();  // Call all talents
  telnet_neg::add_actions();
  init_gmcp();
  call_out(function void()
  {
    process_gmcp(
      ([ // Char.Name initially. (wizard)
        "name":capitalize(Query(P_REALNAME)),
        "fullname":Short(),
        "gender":QueryGenderString(),
        "wizard":IS_WIZARD(this_object()),
      ]),"Char","Name"); // Explain fields
    process_gmcp(([]),"Char","StatusVars"); // Explain fields once
    update_points_display();
  }, 2);
  move_player_to_start(0);

  log_file("ENTER", ctime()[4..15]+" start \t"+query_ip_name()+" \t"
                    +getuid(this_object())+"\n");

  if (rc) write ("***** Contact a wizard!\n");
}

/* -------------------------------------------------------------------------
 * Input-Functions for login
 * -------------------------------------------------------------------------
 */

protected void getmailaddr(string maddr)
{
  mailaddr = maddr;
  move_player_to_start2(saved_where);
}

/*  This function is called using input_to, and sets the
 *  tty of this player.
 */
protected void gettty(string ttystr)
{
  if (!ttystr || ttystr == "") ttystr = "dumb";
  if (!setstty(ttystr))
  {
    input_to(#'gettty, 0); /*'*/
    return;
  }
  move_player_to_start4(saved_where);
}

/*  This function is called using input_to, and sets the
 *  gender of this player.
 */
protected void getgender(string str)
{
  str = lower_case(str);
  if (str[0] == 'm')
  {
    write("Welcome, Sir!\n");
    SetGender(GENDER_MALE);
  }
  else if (str[0] == 'f')
  {
    write("Welcome, Madam!\n");
    SetGender(GENDER_FEMALE);
  }
  else
  {
    write("Sorry, but that is too weird!\n");
    write("Are you male or female (type m or f): ");
    input_to(#'getgender); /*'*/
    return;
  }
  move_player_to_start3(saved_where);
}

/* *************************************************************************
 * Die Hard, Die Harder ... everything 'needed' to die
 * *************************************************************************
 */

public void log_kill() {
  int i;
  object attacker_ob;
  string myname, mylevel, klevel, kname, rname;
  string kname2;

  myname = QueryRealName();
  if (({int})TESTY_D->CheckTesty(ME))
    myname + "testy "+myname;
  if (i = query_user_level(this_object()))
    mylevel = "w"+i;
  else
    mylevel = QueryLevel()+"/"+(({int})RACEMASTER->XPtoLevel(QueryXP()));

  if (!PL || PL == ME)
    /* Scan back until we find the killing object.
    ** Note that previous_object(0) is mostly the player itself
    ** due to the call from die().
    */
    for (i=0; (attacker_ob = previous_object(i)) == ME; i++);
  else
    attacker_ob = PL;
  if (attacker_ob)
  {
    if (query_user_level(ME) && query_user_level(attacker_ob))
      return; // Nothing to log.
    if (!(kname = ({string})attacker_ob->QueryRealName())
      && !(kname = ({string})attacker_ob->QueryName())
      && !(kname = (({string})attacker_ob->QueryShort() || ({string})attacker_ob->QueryIntShort()))
    )
    {
      kname = "<invis>";
      kname2 = "Something";
    }
    else
      kname2 = kname;
    if (!({int})attacker_ob->QueryIsPlayer())
      kname += " ["+object_name(attacker_ob)+"]";
    else if ((rname = ({string})attacker_ob->QueryRealName()) && kname != rname)
      kname += " ["+rname+"]";
    if (query_once_interactive(attacker_ob)
      && (i = query_user_level(attacker_ob))
      )
      klevel = "w"+i;
    else
      klevel = ({int})attacker_ob->QueryLevel()+"/"
        +(({int})RACEMASTER->XPtoLevel(attacker_ob->QueryXP()));
    if (({int})TESTY_D->CheckTesty(attacker_ob))
    {
      kname = "testy "+kname;
      kname2 = "testy "+kname2;
    }
  }
  else
  {
    klevel = kname = "???";
    kname2 = "Something";
  }

  if (attacker_ob && query_once_interactive(attacker_ob))
  {
    rname = ctime()+" "+kname+"("+klevel+") killed "
      +myname+"("+mylevel+")"
      +(query_verb() ? (" by '"+query_verb()+"'"):"") +"\n";

    write_file("/log/KILLER", rname);
    attacker_ob->SetKills(({int})attacker_ob->QueryKills()+1);
  }
  else
  {
    write_file("/log/KILLS", myname+"("+mylevel+") killed by "+
      kname+"("+klevel+"), creator: "+
      (attacker_ob?getuid(attacker_ob):"<none>")
      +((environment() && ({int})environment()->QueryKillZone())
      ? " (LEGAL)"
      : "")
      +"\n");
  }
  CHANNEL_D->SendEmote("news",
     capitalize(myname),"was killed by "+kname2+".", 1);
  if (!attacker_ob)
    raise_error(capitalize(myname)+" killed by no attacker_ob.\n");
}

public int second_life() {
  int i;
  object death_mark;

  /* Prevent against any direct calls */
  if (find_player(QueryRealName()||QueryName()) && !interactive())
  {
      /* This player is linkdead. */
    write(QueryName()
      +" is not here. You cannot kill a player who is not logged in.\n");
    StopAllHunting();
    return 1;
  }
  /* Don't damage wizards too much ! */
  if (IS_IMMORTAL( /*this_object()*/ QueryRealName())
      && !Query(P_NOWIZ)
     )
  {
    tell_object(this_object(), "Your wizardhood protects you from death.\n");
    return 1;
  }

#ifdef LOG_KILLS
  log_kill();
#endif

  i = sizeof(inform_death);
  while(i--)
    if (inform_death[i]) inform_death[i]->inform_death();

  if (   dead_ob
      && ({int})dead_ob->player_died())
    return 1;

  /* make_scar(); */
  SetGhost(1);

  // SaveStats() is done in /std/living/body:Die()

  AddKill();
  if ((i = QueryLevel()) > 1) SetLevel(i-1);
  // Softbyte 19nov97: New stats added
  if (QueryCha() > 1) SetCha(QueryCha()-1);
  if (QueryWis() > 1) SetWis(QueryWis()-1);
  if (QueryAgi() > 1) SetAgi(QueryAgi()-1);
  if (QueryQui() > 1) SetQui(QueryQui()-1);
  if (QueryStr() > 1) SetStr(QueryStr()-1);
  if (QueryCon() > 1) SetCon(QueryCon()-1);
  if (QueryInt() > 1) SetInt(QueryInt()-1);
  // TODO remove dex
  if (QueryDex() > 1) SetDex(QueryDex()-1);
  SetAlcohol(0);
  SetFood(0);
  SetDrink(0);
  tell_object(ME, "\nYou die.\nYou have a strange feeling.\n" +
        "You can see your own dead body from above.\n\n");
  death_mark = clone_object("/room/death_mark");
  death_mark->move(ME, M_SILENT);
  return 1;
}

/* *************************************************************************
 * create, remove, heart_beat, reset
 * *************************************************************************
 */

public void illegal_patch(string what) {
  write("You are struck by a mental bolt from the interior of the game.\n");
  log_file("ILLEGAL", ctime(time()) + ":\n");
  log_file("ILLEGAL", ({string})TI->QueryRealName() + " " + what + "\n");
}

void init() {
  stats::init();
  combat::init();
}

public void create() {
  string str1;
  int i;

  base::create();
  heart::create();
  soul::create(); /* needed to initialize the default verbs/adverbs */
  telnet_neg::create();

  /* dont mess with the blueprint */
  if (!clonep()) return;
  //if (sizeof(explode(object_name(ME),"#")) != 2) return;

  /* Find loading object */
  str1 = previous_object() ? object_name(previous_object()) : "<none>";
  i = member(str1, '#');
  if (i < 0)
    i = sizeof(str1);
  if (str1[0..i-1] != "/secure/login")
  {
    write("Loaded by '"+str1+"'\n");
    illegal_patch("Clone of /std/player");
    destruct(ME);
    return 0;
  }

  filesys::create();
  life::create();
  combat::create();
  restrictions::create();
  stats::create();

  Set(P_HELP_MSG, "");

  AddClassId(C_PLAYER);
  SetLevel(-1);
  ::SetName("<logon>", 1);
  title = "the title less";
  SetAlign(0);
  inform_death = ({});
  player_started = 0;
  incmsg = "";
  /* Yeah, tty, we should handle that by telnet negotiation */
  tty = "none";
  RACEMASTER->InitRace(ME, "visitor");
  ChannelOn("players");
  ChannelOn("news");
  ChannelOn("title");
  Set(P_TIME_OPTIONS, TIME_ALL);
  SetDrink(100);
  SetFood(100);
  SetAlcohol(0);
  SetWimpy(15);
  SetGhost(1);
  SetEchoMode(1);
  SetHeart(1);
  Set(P_PROMPT, 0);
  promptpend = 0;
}

public void reset() {
}

/* Having a heart_beat function means that our heart_beat will
 * run all the time
 */
public void heart_beat() {
#ifdef WSTAT
  int weight;
#endif

#ifdef WSTAT
  weight = QueryWeight();
  if (weight != last_weight && !QueryLState())
  {
    string msg;
    msg = sprintf("%s %O %d/%d Str %d Content %d MaxContent %d Load %d\n"
   , ctime(), this_object(), query_user_level(this_object())
   , QueryRealLevel(), QueryStr()
   , QueryWeightContent()
   , QueryMaxWeight()-QueryWeight()+QueryWeightContent()
   , QueryLoad()
   );
    log_file("WSTAT", msg);
    last_weight = weight;
  }
  else if (QueryLState())
    last_weight = 0;
#endif

  ResetCmdDataStack();
  QueryNetdead();
  message::heart_beat(); /* prompt handling */
  heart::heart_beat();
}

public varargs int remove() {
  object *inv;
  int i;
  string err;

  inv = all_inventory(ME);
  if (!query_user_level(ME) && !is_guest)
    // Normal players drop things on destruct
  {
    for (i = 0; i < sizeof(inv); i++)
      if (!({int})inv[i]->Query(P_NODROP))
      {
        inv = inv[0..i-1]+inv[i+1..];
        i--;
      }
  }
  err = catch(filter_objects(inv, "remove"));
  if (err && IS_IMMORTAL(this_object()))
    log_file("NDWIZARD",
       sprintf("%s: %O failed remove: %s  inv left: %O\n",
       ctime(), this_object(), err,
       all_inventory(this_object()) )
      );
  while (first_inventory())
    destruct(first_inventory());

  stats::remove(); // talents
  i = moving::remove();
  return i;
}

/* *************************************************************************
 * Identify ourself.
 * special temp handling for ghosts and invis
 * *************************************************************************
 */

#if 0
public mixed id(str, lvl) {
  if (!str) return 0;
  if (::id(str) || lower_case(str) == lower_case((string)QueryName()))
    return 1;
  if (QueryInvis() && lvl < level) return 0;
  if (QueryGhost()) return str == "ghost of "+ capitalize(QueryRealName());
}
#endif


/* **************************************************************************
 * prevent_shadows - restrict shadowing
 *  std/player's may only be shadowed by files from std/player/shadows/...
 * **************************************************************************
 */

public int prevent_shadow(object obj) {
  string what;

  if (obj) {
    what=object_name(obj);
    if (what && what != ""
      && sscanf(what,"std/player/shadows/%s#",what)==1)
      return 0;
  }
  return 1;
}

/* -------------------------------------------------------------------------
 * Voting stuff
 * [Softbyte 8dec97]
 * -------------------------------------------------------------------------
 */
/* -------------------------------------------------------------------------
 * cmd_voting - the vote command
 * -------------------------------------------------------------------------
 */
int cmd_voting(string str)
{
  notify_fail("Voting on/off/show/noshow/status?\n",NOTIFY_NOT_CMD);
  if (!str) return 0;
  if (str=="on")
  {
    SetIsNoVoter(0);
    msg_write( CMSG_GENERIC, "You are now an active voter.\n");
  }
  else if (str=="off")
  {
    SetIsNoVoter(1);
    msg_write( CMSG_GENERIC, "You are no longer taking part at votes.\n");
  }
  else if (str=="show")
  {
    SetShowVotes(1);
    msg_write( CMSG_GENERIC, "New votes are shown on login.\n");
  }
  else if (str=="noshow")
  {
    SetShowVotes(0);
    msg_write( CMSG_GENERIC, "New votes are NOT shown on login.\n");
  }
  else if (str=="status")
  {
    if (QueryIsNoVoter())
      msg_write( CMSG_GENERIC, "You are not taking part at votes.\n");
    else
    {
      if (QueryShowVotes())
        msg_write( CMSG_GENERIC,
          "You are taking part at votes and new votes are shown on login.\n");
      else
        msg_write( CMSG_GENERIC,
          "You are taking part at votes and nothing is shown on login.\n");
    }
  }
  else return 0;
  return 1;


}
// Show which votes are pending
void vote_delay()
{
  if (PL && IS_WIZARD(PL))
  {
    if (QueryIsNoVoter()) return ;   // no voter
    if (({int})VOTESERVER->ShallShowVotes("wizards",this_object()))
    {
      if (QueryShowVotes())
        VOTESERVER->ListVotes("wizards",1,this_object(),1);
      else
        msg_write( CMSG_GENERIC, "There are new votes open.\n");
    }
  }
}

/* *********************************************************************** */
