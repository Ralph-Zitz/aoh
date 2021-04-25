/* quest control, by wurzel@nightfall '92
**
**            [Pepel]   converted to new Funname conventions
**  12-Oct-92 [Mateese] CheckAdvance()
**  14.10.92  [wurzel]  hints and names
**  14.10.92  [Pepel]   changed format of quest list
**  03.10.93  [Loco]    Mostly rewritten. Implemented:
**                      Location flag for long description display
**                      2nd list for Miniquests
**                      Several new and changed functions
**  06.10.93  [Mateese] Ensured that arrays are returned by value.
**  21.04.94  [Loco]    changed all_quests from alist to mapping
**                      Questlist now stored externally.
**                      Added XP values for Quests
**  12.06.94  [Loco]    Logging of any questsetting
**  25.06.94  [Loco]    QueryQuestShort(), QuestList with "short" parameter.
**  07.12.94  [Mateese] The min amounts of QPs needed to seer/wiz are now
**                      stored as fixed player props. [unused]
**  03.02.95  [Loco]    renamed questlist files
**  19.01.97  [Loco]    HLP quest support, QueryQuestsWritten, LockQuest
**  01.02.97  [Loco]    debugging a severe bug and installing repair functions
**                      (see #ifdef CLEAN_MULTIPLE)
*/

#include <config.h>
#include <properties.h>
#include <secure/wizlevels.h>
#include <quests.h>

/*-- CONFIGURATION -------------------------------------------------------*/
#define TOTAL              (off_total+mini_total)
#define OFF_SAVE_FILE      DAEMONSAVEPATH "questctrl/questlist"
#define HLP_SAVE_FILE      DAEMONSAVEPATH "questctrl/hlpquests"
#define MINI_SAVE_FILE     DAEMONSAVEPATH "questctrl/miniquests"
#define QUESTOBJLIST       "/log/QUESTLIST"
//#define MINI_MAX_QP        5     /* max. QP for a miniquest. */
                                   /* No limit, if undefined   */
#define TESTYLIB           "/obj/testylib"
#define LOCKCHAR           "-"

/* define if you want to use the longdesc restrictions */
#define RESTRICTED_QUEST_LONG

/* May be undefined if the list is stored sorted. If defined, actively
   sorts by QP value. Looks better, but may cause smaller confusion as
   the sorting sometimes turnes the order of same-valued quests. Also
   needs eval time, about 20 % of the usual create() eval time */
#undef SORT_QUESTLIST

// Define this macro if the necessary QP shall be determined just once
// at login. When changing this, be sure to change the macro in /std/player
// as well.

#undef FIXED_QP

/*-- The QP requirement. Might be adjusted ---------------------------------*/

#define BASE_VALUE             1000       /* At BASE_VALUE==off_total 90% of */
                                          /* off_total will be required.     */
#define MIN_QUEST_POINTS_WIZ   min_wiz_qp
#define WIZ_QP_FORMULA         ((off_total < BASE_VALUE*3/8) ? off_total : \
                               (int)(BASE_VALUE*2.1728*(sqrt((             \
			       (float)off_total)/BASE_VALUE +1.0) -1.0)))
// the old one: (((100-(off_total/100))*off_total)/100)
#define MIN_QUEST_POINTS_SEER  (MIN_QUEST_POINTS_WIZ*95)/100

/*--------------------------------------------------------------------------*/

/* the format of the quests array. In the questlist file they have to be  */
/* stored in same order with all data but Q_SHORT and Q_LONG in the first */
/* line, Q_SHORT in the second and Q_LONG takes the rest. Quests are      */
/* separated by empty lines. See the savefile for further information.    */

#define Q_NAME		0    /* queststring set in the player object      */
#define Q_CREATOR	1    /* programmer responsible for ideas and code */
#define Q_DOMAIN        2    /* where to display the quest hints          */
#define Q_POINTS	3    /* Questpoints given for this quest          */
#define Q_XP            4    /* Experience given                          */
#define Q_SHORT		5    /* shown in the guilds                       */
#define Q_LONG		6    /* hint shown in guilds                      */

/* the format of the miniquests list. The miniquests are loaded
   from file and only stored in the all_quests mapping. */
#define MQ_NAME         0
#define MQ_CREATOR      1
#define MQ_POINTS       2
#define MQ_XP           3
#define MQ_SIZEOF       4  /* To omit additional parameters, may be used
			      for comments */

/* the format of the all_quests mapping */
#define AQ_QP           0
#define AQ_XP           1
#define AQ_CREATOR      2
#define AQ_TYPE         3    /* official or hlp                           */

/* for quicker typing */
#define off_total       totalqp[QT_OFFICIAL]
#define mini_total      totalqp[QT_MINI]
#define hlp_total       totalqp[QT_HLP]

/* TEMPORARILY USED FOR AUTOMATICALLY CLEANING UP QUESTS SET MORE THAN ONCE */
#undef CLEAN_MULTIPLE
#ifdef CLEAN_MULTIPLE
#define CLEANED_SAVEFILE "/log/QUESTS.CLEANED"
#define CLEANLOG "QUESTS.CLEANLOG"
mapping cleaned;
#endif

mixed *questlist,*hlpquestlist;
mapping all_quests,quests_by_creator;
int *totalqp;
int mini_number,min_wiz_qp;

/* just prototypes of internal functions */

static mixed LoadQuestList(string file,int type);
static void LoadMiniQuestList();
static string GetDomain(mixed ob);
static string DomainName(string a);
static void log_questobj(string text,object pl);

/* Some more macros. Don't switch off! */
#define _DEBUG

#define TP this_player()
#define TI this_interactive()
#define PO previous_object()
#define ME this_object()
#define OB2STR(ob) ((ob) ? object_name(ob): "<none>")
#define GETUID(ob) ("["+((ob) ? getuid(ob):"<none>")+"]")
#define LOG(s) log_file ("QUESTCTRL", s)
#define DEBUGVAR(x) tell_object(find_player("loco"),sprintf("Value of x is %s\n","/lib/string"->mixed_to_string(x)))

/*-------------------------------------------------------------------------*/
/* the old quest list :
 * quests "find_kirk#orc_slayer#feed_ice_monster#cleanPhishsCastle#planetary#
 * wand_of_fire#beg_quest#adv_quest#vikingmania#sceptre_of_rocknroll#
 * mountain_sprite#pirates_cave#gaul_quest#king's_weapons#elementals"
 */

/*==========================================================================*/
/* CODE SEGMENT                                                             */
/*==========================================================================*/

void create() {
  int i,j;

  all_quests=([]);
  quests_by_creator=([]);
  totalqp=allocate(QT_ALL+1);

#ifdef CLEAN_MULTIPLE
  cleaned=mkmapping(explode(read_file(CLEANED_SAVEFILE),"\n"));
#endif

/* Don't see the plot of logging this. Loco Jul 14 1993
   Should be enough to log the initialisation in one line. Shorten the logfile
  LOG (ctime(time())+" create()\n");
  LOG ("    this    : "+OB2STR(ME)+" "+GETUID(ME)+"\n");
  LOG ("    previous: "+OB2STR(PO)+" "+GETUID(PO)+"\n");
  LOG ("    player  : "+OB2STR(TP)+" "+GETUID(TP)+"\n");
  LOG ("    iactive : "+OB2STR(TI)+" "+GETUID(TI)+"\n");
*/

  questlist=LoadQuestList(OFF_SAVE_FILE,QT_OFFICIAL);
  hlpquestlist=LoadQuestList(HLP_SAVE_FILE,QT_HLP);

#ifdef SORT_QUESTLIST
  questlist=sort_array(questlist,"MoreQP",this_object());
  hlpquestlist=sort_array(hlpquestlist,"MoreQP",this_object());
#endif

#ifdef MINI_SAVE_FILE
  LoadMiniQuestList();
#endif

  for (i=3;i<=QT_ALL;i++) for (j=1;j<i;j+=j) if (j&i) totalqp[i]+=totalqp[j];
  totalqp[0]=totalqp[QT_ALL];

  LOG (ctime(time())+": "+sizeof(questlist)+"+"+
       sizeof(hlpquestlist)+"+"+mini_number+"= "+
       (sizeof(questlist)+sizeof(hlpquestlist)+mini_number)+" Quests, "+
       off_total+"+"+hlp_total+"+"+mini_total+"= "+totalqp[0]+" QP."+
       (object_name(this_object())=="/obj/quest_control" ?"" : "[test]")+"\n");
  min_wiz_qp=WIZ_QP_FORMULA;
}

/*------------------------------------------------------------------*/
/* Query several values                                             */

varargs int QueryTotalQuestPoints(int types) {
  return totalqp[types]; // default is 0, contains same value as QT_ALL
}

/* How many QP do you need to get wiz or HLP? */
int QueryWizQuestPoints() {
  return MIN_QUEST_POINTS_WIZ;
}
int QueryHLPQuestPoints() {
  return MIN_QUEST_POINTS_SEER;
}
/*----------------------------------------------------------------------*/
/* Return the QP and XP associated with a quest                         */

int QueryQuestQP(string qname) {
  return all_quests[qname];
}

int QueryQuestXP(string qname) {
  return all_quests[qname,AQ_XP];
}

/*------------------------------------------------------------------------*/
/* Don't use anyway, only for debugging purposes.                         */
/* QueryQuests() returns only the official ones.                          */
/* QueryAllQuests() the mapping with all questnames                       */


varargs mixed QueryQuests(int types) {
  mixed *rc;
  int i;

  if (!types) types=QT_ALL;
  if (!sizeof(questlist)) {
    LOG (ctime(time())+" QueryQuests() on empty questlist\n");
    LOG ("    this    : "+OB2STR(ME)+" "+GETUID(ME)+"\n");
    LOG ("    previous: "+OB2STR(PO)+" "+GETUID(PO)+"\n");
    LOG ("    player  : "+OB2STR(TP)+" "+GETUID(TP)+"\n");
    LOG ("    iactive : "+OB2STR(TI)+" "+GETUID(TI)+"\n");
    return 0;
  }

  // Produce a value-copy of the array to annoy cheaters.
  rc = ({});
  if (types&QT_OFFICIAL)
    for (i = 0; i < sizeof(questlist); i++)
      rc += ({ questlist[i]+({}) });
  if (types&QT_HLP)
    for (i = 0; i < sizeof(hlpquestlist); i++)
      rc += ({ hlpquestlist[i]+({}) });
  return rc;
}

mixed QueryAllQuests() {
  mixed rc,ind;
  int i;

//return all_quests;

  rc=([]);
  ind=m_indices(all_quests);
  // Produce a value-copy of the alist to annoy cheaters.
  for (i=sizeof(ind)-1;i>=0;--i)
    rc+=([ind[i]:all_quests[ind[i]];all_quests[ind[i],1];
	  all_quests[ind[i],2]]);
  return rc;
}


/*-------------------------------------------------------------------------
** string *QueryQuestNames()
**   Return the names of all valid official quests
** string *QueryAllQuestNames()
**   Return the names of all quests, including miniquests.
*/

varargs string *QueryQuestNames(int types) {
  string *qnames;
  int i;
  qnames = ({});

  if (!types) types=QT_OFFICIAL;
  if (types&QT_OFFICIAL)
    for (i=0; i<sizeof(questlist); i++)
      qnames += ({ questlist[i][Q_NAME] });
  if (types&QT_HLP)
    for (i=0; i<sizeof(hlpquestlist); i++)
      qnames += ({ hlpquestlist[i][Q_NAME] });
  return qnames;
}

varargs string *QueryAllQuestNames(int types) {
  if (!types) types=QT_ALL;
  return m_indices(filter_indices(all_quests,
     lambda(({'x,'types,'all_quests}),
            ({#'&,'types,({#'[,'all_quests,'x,AQ_TYPE})})),
     types,all_quests));
}


/*-------------------------------------------------------------------------
 ** string *QueryPlayerQuests(mixed player)
 **   Return an array of the names of the quests that the player already has
 **   solved. The structure of the player's quests variable is hidden in this
 **   and the next function.
 */

varargs string *QueryPlayerQuests(mixed player, int types) {
  object pl;
  string *ret, q;
  mapping tdata;

  if (objectp(player))
    ret=(q = ({string})player->QueryQuests())? explode(q, "#") : ({});
  else if (stringp(player)) {
    if (pl = find_player(player))
      ret=QueryPlayerQuests(pl);
    else if (tdata = ({mapping})"/obj/player_dummy"->QueryProperties(player) ) {
      ret=(q = ({string})tdata[P_QUESTS])? explode(q, "#") : ({});
    }
  }
  if (!ret) return ({});
  if (!types) return ret;
  if (types==QT_ALL) return ret-(ret-m_indices(all_quests));
  return filter(ret,
     lambda(({'x,'types,'all_quests}),
            ({#'&,'types,({#'[,'all_quests,'x,AQ_TYPE})})),
     types,all_quests);
}


/*------------------------------------------------------------------------*/
/* Check whether a player already has solved a specific quest.            */

int QueryQuestSolved(mixed qname,mixed pl) {
  if (intp(qname)) {
    if (qname>=0 && qname<sizeof(questlist)) qname=questlist[qname][Q_NAME];
    else return 0;
  }
  return (member(QueryPlayerQuests(pl), qname) != -1);
}

/*------------------------------------------------------------------------*/
/* return the creator of a quest. Number or string allowed. Not for       */
/* miniquests, look them up yourself...                                   */

string QueryQuestCreator(mixed q) {
  if (stringp(q)) return all_quests[q,AQ_CREATOR];
  if (q<0 || q>=sizeof(questlist)) return "Nobody";
  return capitalize(questlist[q][Q_CREATOR]);
}

/*-------------------------------------------------------------------------
 ** int SetPlayerQuest(string qname, object player)
 **   Sets the quest in the player. This function has some potential for
 **   security checks (but doing real security is really hard !)
 **   Currently we allow setting of non-official quests, might be useful for
 **   guild-specific mini-quests. Non-official quests will just not be counted
 **   for quest points.
 **   Return 0 for failure, string of player's quests for success.
 */

mixed SetPlayerQuest(string qname, object pl) {
  string *q;

    if (!query_once_interactive(pl)) return 0;
    if (QueryQuestSolved(qname,pl)||QueryQuestSolved(LOCKCHAR+qname,pl)) return 0;
  if ((pl != this_interactive())) {
    int qt;
    object o;

    if (!this_interactive()) for (qt=0; ((o=previous_object(qt)) && (blueprint(o)!="/obj/qtyper")); ++qt);
      else if (blueprint(this_interactive())=="/secure/login") o=this_player();
    if (!o) {
      /* quest setting by xcall is forbidden. (Loco May 07 1993) */
      if (IS_WIZARD(this_player()))
	LOG (ctime(time())+" Legally set quest '"+qname+"' for "+({string})pl->QueryRealName()+"\n");
      else
	LOG (ctime(time())+" Illegal attempt to set quest '"+qname+"' for "+({string})pl->QueryRealName()+"\n");
      qt=0;
      while (previous_object(qt)) {
	LOG ("    previous: "+OB2STR(previous_object(qt))+" "+GETUID(previous_object(qt))+"\n");
	++qt;
      }
      LOG ("    player  : "+OB2STR(TP)+" "+GETUID(TP)+"\n");
      LOG ("    iactive : "+OB2STR(TI)+" "+GETUID(TI)+"\n");
      if (!IS_WIZARD(this_player())) return 0;
    }
  }
  log_questobj(qname+" ["+blueprint(previous_object())+"] ",pl);
  q=({string *})pl->SetQuests(implode(QueryPlayerQuests(pl) + ({qname}), "#"));
  if (q) pl->AddXP(all_quests[qname,AQ_XP]);
  pl->save_me(1,0);
  log_file("QUESTS",ctime()[4..18]+": "+capitalize(getuid(pl))+" : "+qname+
	   (q ? "" : " - unsuccessful -")+"\n");
  return q;
}

/*-------------------------------------------------------------------------
** string ListQuests(object player)
**   Return an string with all quest descriptions. If a pointer to a player
**   is given, also give info if the player already has solved the quest.
**   (Writing out the quest points might be debatable).
**   If this_player() is HLP or wizard, the list also contains HLP quests
*/

static string ListQuestsFromOneList(mixed list,mixed q,int shortflag,int start);

varargs string ListQuests(mixed pl,int shortflag) {
  string s;
  mapping q;

  q = pl ? (mkmapping(QueryPlayerQuests(pl))) : ([]);
  s=(shortflag?"":"Official Quests:\n");
  s+=ListQuestsFromOneList(questlist,q,shortflag,1);
  if (IS_HLP(TP)||IS_IMMORTAL(TP))
     s+=(shortflag?"":"HLP Quests:\n")+
         ListQuestsFromOneList(hlpquestlist,q,shortflag,sizeof(questlist)+1);
  return s;
}

static string ListQuestsFromOneList(mixed list,mixed q,int shortflag,int start) {
  int i;
  string s;
  if (!shortflag) {
    for (s = ""; i<sizeof(list); i++) {
      s += sprintf("%2d. %s (%d)",(i+start),list[i][Q_SHORT],
		   list[i][Q_POINTS]);
      if (member(q,list[i][Q_NAME])) s += " (solved)";
      s += "\n";
    }
  } else {
    for (s = ""; i<sizeof(list); i++)
      if (member(q,list[i][Q_NAME]))
	s += list[i][Q_SHORT]+"\n";
  }
  return s;
}


/*-------------------------------------------------------------------------
** string * QueryQuestsWritten (string wizname [,int type] );
**   Return an array of the queststrings for the quests the wizard is
**   registered as creator. Default for type: QT_ALL
*/

varargs string *QueryQuestsWritten (string wizname,int types) {
  mixed ret;

  wizname=lower_case(wizname);
  ret=quests_by_creator[wizname];
  if (!ret||!pointerp(ret)) return ({});
  if (!types||types==QT_ALL) return ret;
  return filter(ret,
     lambda(({'x,'types,'all_quests}),
            ({#'&,'types,({#'[,'all_quests,'x,AQ_TYPE})})),
     types,all_quests);
}

/*-------------------------------------------------------------------------
** string QueryQuestLong(int n)
**   Return the hint to a given quest
**   Check for restricted information if wanted.
*/

#ifndef RESTRICTED_QUEST_LONG

/* This is the old one. */
/* CAUTION! REMAINED UNCHANGED, WON'T WORK WITH HLP QUESTS! */
string QueryQuestLong(int n) {
  return
    (n < sizeof(questlist) && n >= 0) ?
      (questlist[n][Q_LONG] ) :
	"No such quest.\n";
}

#else

#define QENTRY(n) ((n>=sizeof(questlist)) ? hlpquestlist[n-sizeof(questlist)] : questlist[n])
#define QLONG(x) (QENTRY(x)[Q_LONG]+"This quest was created by "+QENTRY(x)[Q_CREATOR]+".\n")
#define QDOM(n) (QENTRY(n)[Q_DOMAIN])

string QueryQuestLong(int nr) {
  string s;
  int i;

  if ((nr>=sizeof(questlist)+sizeof(hlpquestlist)) ||
       (!IS_HLP(TP) && !IS_IMMORTAL(TP) && nr >= sizeof(questlist))
       || nr < 0) return "No such quest.\n";
    /* wizards may query any hint */
  if (IS_IMMORTAL(this_player())) return QLONG(nr);
  if (!QDOM(nr))
    return "Sorry, no information available.\n";
  if (intp(QDOM(nr)) || QDOM(nr)=="*")
    return QLONG(nr);
  s=GetDomain(previous_object());
  if ((stringp(QDOM(nr)) && QDOM(nr)==s)
      || (pointerp(QDOM(nr)) && member(QDOM(nr), s)!=-1))
    return QLONG(nr);

  /* well, info may not be queried by previous_object... */

  s="";
  if (stringp(QDOM(nr))) s=DomainName(QDOM(nr));
  else for (i=sizeof(QDOM(nr))-1;i>=0;--i) {
    s+=DomainName(QDOM(nr)[i]);
    if (i>1) s+=", ";
    else if (i==1) s+=" or ";
  }

  return implode(({string *})"/lib/string"->break_line("Sorry, no information available here. Go to "+s+" and ask there.\n"),"\n");
/* Use this if you don't want to use lib/string:
  return "Sorry, no information available here.\nGo to "+s+" and ask there.\n";
*/
}

#endif

/*-----------------------------------------------------------------------*/
/* QueryQuestShort(string|int quest)                                     */
/* Return the Questlist line of a specific offical or hlp quest          */

string QueryQuestShort(mixed quest) {

  int i;
  if (intp(quest)) {
    if (quest<=0) return 0;
    if (quest<=sizeof(questlist)) return questlist[quest-1][Q_SHORT];
    if ((quest-=sizeof(questlist))<=sizeof(hlpquestlist))
       return hlpquestlist[quest-1][Q_SHORT];
    return 0;
  }
  i=sizeof(questlist)-1;
  while(i>=0 && questlist[i][Q_NAME]!=quest) i--;
  if (i>=0) return questlist[i][Q_SHORT];
  i=sizeof(hlpquestlist)-1;
  while(i>=0 && hlpquestlist[i][Q_NAME]!=quest) i--;
  if (i>=0) return hlpquestlist[i][Q_SHORT];
  return 0;
}

/*-------------------------------------------------------------------------
** int QueryQuestPoints (mixed player)
**   Return the amount of collected qp for player.
*/

int QueryQuestPoints (mixed pl) {
  string *q;
  int i, points;

  q=QueryPlayerQuests(pl);

#ifdef CLEAN_MULTIPLE
  if (objectp(pl) && this_player()==pl && this_player()==this_interactive()) {
    if (!member(cleaned,geteuid(pl))) {
      string *qq;
      log_file(CLEANLOG,"Cleaning "+geteuid(pl));
      qq=q+({"*MARKER*"});
      while (qq[0]!="*MARKER*") qq=qq-({qq[0]})+({qq[0]});
      qq=qq[1..];
      log_file(CLEANLOG,": "+(sizeof(q)-sizeof(qq))+" quests removed.\n");
      if (sizeof(qq)!=sizeof(q)) {
	log_file(CLEANLOG,"  old: "+implode(q,"#")+"\n  new: "+implode(qq,"#")+"\n");
	q=qq;
	pl->SetQuests(implode(q,"#"));
      }
      write_file(CLEANED_SAVEFILE,geteuid(pl)+"\n");
      cleaned+=([geteuid(pl)]);
    }
  }
#endif

  for (i=0;i<sizeof(q);++i) points+=all_quests[q[i]];
  return points;
}

/*-------------------------------------------------------------------------
** int QPointLevel (int level)
**   Return the amount of qp associated with the given 'general <level>'.
** Well, it's something like this: Up to 4 its zero. Then it is going up
** slightly, but accelerating. For Lv. 20 it's the HLP value. Linear
** slope until 25 which is the total of the officials. Then raising
** asymptotical in direction total of all quests.
*/

int QPointLevel (int level) {
  if (level<5) return 0;
  if (level<20) return (level-4)*level*MIN_QUEST_POINTS_SEER/320;
  if (level==20) return MIN_QUEST_POINTS_SEER;
  if (level<26) return MIN_QUEST_POINTS_SEER+(off_total-MIN_QUEST_POINTS_SEER)*(level-20)/5;
  return (off_total+mini_total)-mini_total/(level-24);
}

/*-------------------------------------------------------------------------
** int QueryWizQuests (object player)
**   Return non-zero if <player> has enough questpoints to become wizard.
** int QuerySeerQuests (object player)
**   same for seers/HLPs
*/

int QueryWizQuests (object player) {
  int minpoints;
#ifdef FIXED_QP
  minpoints = player->QueryWizQP();
  if (!minpoints || minpoints > MIN_QUEST_POINTS_WIZ
#endif
    minpoints = MIN_QUEST_POINTS_WIZ;
  return QueryQuestPoints (player) >= minpoints;
}

int QuerySeerQuests(object player) {
  int minpoints;
#ifdef FIXED_QP
  minpoints = player->QuerySeerQP();
  if (!minpoints || minpoints > MIN_QUEST_POINTS_SEER)
#endif
    minpoints = MIN_QUEST_POINTS_SEER;
  return QueryQuestPoints (player) >= minpoints;
}

/*-------------------------------------------------------------------------
** int QueryQuestFactor(object player)
**   Returns 1 if <player> hasn't got enough questpoints for his/her amount
**   of accumulated experience, else zero.
**   Might be used by guilds, is not used by the RACEMASTER
*/

int QueryQuestFactor(object player) {
  return QPointLevel(({int})RACEMASTER->XPtoLevel(({int})player->QueryXP())) >=
    QueryQuestPoints(player);
}

/*-------------------------------------------------------------------------
** int CheckAdvance (object player, string stat, int delta)
**   Check if player has enough questpoints to advance its <stat> by <delta>.
**   Return the amount of qp missing, or zero if advancing is ok.
*/

int CheckAdvance (object pl, string stat, int delta) {
  int at, cost;
  at = ({int})pl->QueryAttribute (stat) - ({int})pl->QueryBonus (stat);
  cost = QPointLevel (at+delta) - QueryQuestPoints (pl);
  return (cost <= 0 ? 0 : cost);
}

/*-------------------------------------------------------------------------*/
/* qshout. Something similar to shout, but checks for earmuffs. More       */
/* checks may be implemented if useful                                     */

void qshout(string text) {
  object *u;
  int i;
  u=users()-({this_player()});
  for (i=sizeof(u)-1;i>=0;i--)
    if (!({int})u[i]->QueryEarmuffs()) tell_object(u[i],text);
}

/*-------------------------------------------------------------------------*/
/* Some internal functions. You'll never need them.                        */
/* Declared static they cannot be called from outside                      */


static void RegisterCreator(string crea,string qname) {
  mixed a;
  int i;
  if (member(crea,'&')>=0) crea=implode(explode(crea,"&")," ");
  if (member(crea,'/')>=0) crea=implode(explode(crea,"/")," ");
  if (member(crea,',')>=0) crea=implode(explode(crea,",")," ");
  if (strstr(crea," and ")>0) crea=implode(explode(crea," and ")," ");
  a=explode(lower_case(crea)," ")-({""});
  for (i=sizeof(a)-1;i>=0;i--) {
    if (quests_by_creator[a[i]]) quests_by_creator[a[i]]=quests_by_creator[a[i]]+({qname});
    else quests_by_creator[a[i]]=({qname});
  }
}


static void LoadMiniQuestList() {
  mixed list,q;
  int i;
  list=explode(read_file(MINI_SAVE_FILE),"\n");
  for (i=0;i<sizeof(list);++i) {
    q=explode(list[i],"#");
    if (q[0]!="") {
      if (sizeof(q)>=MQ_SIZEOF) {
	++mini_number;
	q[MQ_POINTS]=to_int(q[MQ_POINTS]);
	q[MQ_XP]=to_int(q[MQ_XP]);
#ifdef MINI_MAX_QP
	if (q[MQ_POINTS] > MINI_MAX_QP) {
	  LOG("Illegal QP value "+q[MQ_POINTS]+" for miniquest '"+q[MQ_NAME]+"'\n");
	  q[MQ_POINTS]=0;
	}
#endif
	mini_total+=q[MQ_POINTS];
	all_quests+=([q[MQ_NAME]:q[MQ_POINTS];q[MQ_XP];q[MQ_CREATOR];QT_MINI]);
	RegisterCreator(q[MQ_CREATOR],q[MQ_NAME]);
      } else
	LOG( "Bogus Miniquest Entry\n"+list[i]+"s\n");
    }
  }
}


static int NoComment(string blubb) {
  return (blubb && blubb!="" && blubb[0]!='#');
}

static mixed LoadQuestList(string file,int type) {
  mixed list,h;
  int i;
  list=explode(read_file(file),"\n\n");
  for (i=sizeof(list)-1;i>=0;i--) {
    list[i]=filter(explode(list[i],"\n"),#'NoComment /*'*/);
    if ( sizeof(list[i])<3 || ( ! list[i][0] )  || ( list[i][0][0]=='-') ) {
      if ( sizeof( list[i] ) && ( ( ! list[i][0] ) || ( list[i][0][0]!='-') ) )
	LOG(sprintf("Bogus Quest Entry %d:\n%s\n",
		    i,({string})"lib/string"->mixed_to_string(list[i])));
      list[i]=0;
    }
    else {  /* Seems to be loadable */
      list[i]=explode(list[i][0],"#")+({list[i][1]})+
	({implode(list[i][2..],"\n")+"\n"});
      if (list[i][Q_DOMAIN]=="*") list[i][Q_DOMAIN]=1;
      else
	list[i][Q_DOMAIN]=(sscanf(list[i][Q_DOMAIN],"%d",h) ? h :
			   explode(list[i][Q_DOMAIN],","));
      list[i][Q_POINTS]=(sscanf(list[i][Q_POINTS],"%d",h) ? h : 0);
      list[i][Q_XP]=(sscanf(list[i][Q_XP],"%d",h) ? h : 0);
    }
    if (list[i]){
      totalqp[type] += (list[i][Q_POINTS]);
      all_quests+=([list[i][Q_NAME]:
		  list[i][Q_POINTS];
		  list[i][Q_XP];
		  list[i][Q_CREATOR];
                  type]);
      RegisterCreator(list[i][Q_CREATOR],list[i][Q_NAME]);
    }
  }
  return list-({0});
}

static string GetDomain(mixed a) {
  string *ex, loc;
  ex=explode(blueprint(a),"/");
  if (ex[1]==DDIR) return ex[2];
  if (ex[1]=="p" && ex[2]=="guilds")
    if (loc = ({string})a->QueryDomain()) return loc;
  return "NoDomain";
}

static string DomainName(string a) {
  switch(a) {
  case "beyond":    return "One Step Beyond central area";
  case "silvere":   return "The City of Silvere";
  case "sykorath":  return "The City of Sykorath";
  default: return capitalize(a);
  }
  return capitalize(a);
}

static void log_questobj(string text,object pl) {
  /* partly stolen from /std/food */
  string list;

  if (file_size(QUESTOBJLIST) > 0) {
    list = read_file(QUESTOBJLIST);
    if (sscanf(list,"%~s"+text+"%~s")) return;
  }
  if (IS_IMMORTAL(pl) || ({mixed})TESTYLIB->CheckTesty(pl)) return;
  write_file(QUESTOBJLIST,text+" ("+({string})pl->QueryRealName()+")\n");
}

#ifdef SORT_QUESTLIST
static int MoreQP(mixed a,mixed b) {
  return ( a[Q_POINTS] > b[Q_POINTS] );
}
#endif

string RemoveQuest(mixed pl, string q) {
  if (stringp(pl)) pl=find_player(pl);
  if (!IS_WIZARD(previous_object()) || !IS_WIZARD(TI)) {
    LOG("Unsuccessful attempt to delete quest "+q+" for "+OB2STR(pl)+"\n");
    LOG ("  player  : "+OB2STR(TP)+" "+GETUID(TP)+"\n");
    LOG ("  iactive : "+OB2STR(TI)+" "+GETUID(TI)+"\n");
    return 0;
  }
  if (!pl) return 0;
  pl->SetQuests(implode(explode((({string})pl->QueryQuests()||""),"#")-({q}),"#"));
  LOG(ctime()+": Deleted Quest "+q+" for "+OB2STR(pl)+ "\n");
  LOG ("  previous: "+OB2STR(previous_object())+" "+GETUID(previous_object())+"\n");
  LOG ("  player  : "+OB2STR(TP)+" "+GETUID(TP)+"\n");
  LOG ("  iactive : "+OB2STR(TI)+" "+GETUID(TI)+"\n");
  return q;
}

string LockQuest(mixed pl, string q) {
  if (stringp(pl)) pl=find_player(pl);
  if (!IS_WIZARD(previous_object()) || !IS_WIZARD(TI)) {
    LOG("Unsuccessful attempt to lock quest "+q+" for "+OB2STR(pl)+"\n");
    LOG ("  player  : "+OB2STR(TP)+" "+GETUID(TP)+"\n");
    LOG ("  iactive : "+OB2STR(TI)+" "+GETUID(TI)+"\n");
    return 0;
  }
  if (!pl) return 0;
  LOG(ctime()+": Locked Quest "+q+" for "+OB2STR(pl)+ "\n");
  LOG("  has previously "+(QueryQuestSolved(q,pl) ? "":"not ")+"been set.\n");
  pl->SetQuests(implode(explode(({string})pl->QueryQuests(),"#")-({q})+({LOCKCHAR+q})-({""}),"#"));
  LOG ("  previous: "+OB2STR(previous_object())+" "+GETUID(previous_object())+"\n");
  LOG ("  player  : "+OB2STR(TP)+" "+GETUID(TP)+"\n");
  LOG ("  iactive : "+OB2STR(TI)+" "+GETUID(TI)+"\n");
  return q;
}
