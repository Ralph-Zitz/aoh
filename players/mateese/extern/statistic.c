/* Statistisches Bundesamt
 * Version 2.5
 * (C) Loco 1993/94
 */

inherit "std/thing";
inherit "lib/string";
#include <properties.h>
#include <secure/wizlevels.h>
#include <config.h>

#define LINE "---------------------------------------+---------------------------------------\n"
#define ALPHABET " abcdefghijklmnopqrstuvwxyz"
#define PGREPOUT pgreppath+"pgrep.out"
#define PGREP_PER_HB 4
#define TP this_player()
#define NEEDEUID (pgrepmode)
#define SETEUID0 if (!NEEDEUID) seteuid(0);
#define TESTY(n) ("/obj/testylib"->CheckTesty(n))
#define CHECKWIZLEVEL(l)  if (query_user_level(this_player())<l)    \
    return (int)notify_fail("You can't use this function yet.\n"); 
#define FILEDATE(file,i)  ((i=file_date(file)) ? ctime(i) : "Oops, file not found!")
#define QUESTLIST_FILE "/obj/questctrl/questlist.o"
#define MINI_SAVE_FILE "/obj/questctrl/miniquests.o"
#define QCTRLFILE "/obj/quest_control.c"
#define STR(x) ("/lib/string"->mixed_to_string(x))
#define SC_FILE      0
#define SC_SIZE      1
#define SAVEVERSION  1
#define SAVEDATA     "/players/loco/save/stat/"
#define TOWN(p)      GetTown(p)

static int count,counth,timeset,countlen,
  lagcountmode,pgrepmode,
  alphpos,direc,dirpos,filecount,hitcount;
static object user;
static string pgrepstring,pgreppath,name;
string *scopes;


create() {
  seteuid(getuid());
  (::create());
  SetShort("a statistics tool");
/* SetLong replaced by level-dependent QueryLong() */
  SetIds(({"statistics","statistics tool","statistic","pstats","tool","statistic tool"}));
  pgrepmode=lagcountmode=0;
  if (!scopes) scopes = ({});
  if (!name) name=this_player()->QueryRealName();
  SetAutoObject();
}


SetAutoObject(mixed arg) {
  if (restore_object(SAVEDATA+name)) return; /* ok */
  if (!pointerp(arg)) return; /* bad */
  if (sizeof(arg)>=2) scopes=arg[1]; /* old */
}


mixed QueryAutoObject() {
  if (sizeof(scopes)) save_object(SAVEDATA+name);
  return ({SAVEVERSION});
}


init()
{
  (::init());
  add_action("gdlagindex","gdlag");
  if (IS_IMMORTAL(TP)) {
    add_action("pwhere","pwhere");
  }
  if (IS_WIZARD(TP)) {
    add_action("pstatistics","pstats");
    add_action("playerinfo","pinfo");
    add_action("playerinfo","stats");
    add_action("testycheck","ptesty");
    /*
      add_action("hipinfo","hip");
      add_action("rwhere","rwhere");
    */
    add_action("queststatus","qcstatus");
    add_action("logcheck","logview");
    add_action("questinfo","qinfo");
  }
  if (IS_WIZARD(TP)) {
    add_action("pgrep","pgrep");
    add_action("pgrepstop","pgrepstop");
  }
}


QueryLong() {
  string s;
  s="Commands defined:\n";
  if (!IS_IMMORTAL(TP)) s+=
"(There are more commands, but these are the ones you can use.)\n";
  if (IS_IMMORTAL(TP)) s+="\
pwhere [name]  lists the towns where the player(s) call from\n\
pstats         gives a statistic about interactive players\n\
pstats arg     same, but a bit more information (arg = any argument)\n\
pinfo <pl>     information about an specific player (also stats <pl>)\n\
ptesty         gives a list of known testcharacters online.\n\
ptesty <pl>    tries to find out whether a player is a testcharacter.\n\
               Please mail wrong guesses to Loco!\n\
qcstatus       returns information about the quest controller's status.\n\
qinfo [?|[mini|off][short]]   Statistical questinfo\n\
logview        watch logs. logview ? for more information.\n";
  s+="\
gdlag [len]    measures the GameDriver LAG using two different methods.\n\
               Optional arg is measurement duration (default 10).\n\
               A result of around 2.0 means little or no lag.\n";
  if (IS_WIZARD(TP)) s+="\
pgrep <regexp> searches all players save files for regexp. Type pgrep\n\
               without any parameter for further information.\n\
pgrepstop      cancels a running pgrep process.\n";
  return s;
}


queststatus() {
  int i,j,xp,give_xp;
  mixed h,ql;

/*
  CHECKWIZLEVEL(LEARNER_LVL)
*/  
  ql=QUESTMASTER->QueryAllQuestNames();
  for (i=sizeof(ql)-1;i>=0;i--)
    if (h=QUESTMASTER->QueryQuestXP(ql[i])) {
      give_xp++;
      xp+=h;
    }
  write(sprintf("\
Number of quests: official %7d, miniquests%7d, together%7d.\n\
Questpoints:      officials%7d, miniquests%7d, together%7d.\n\
To get HLP a player needs %d QP, for wizzing %d QP\n\
There are %d XP given for %d quests.\n\
Last updates: Questcontroller:      %s\n\
              Quests save file:     %s\n\
              Miniquests save file: %s\n",
  i=((h=QUESTMASTER->QueryQuestNames()) ? sizeof(h) : 0),
  j=(sizeof(ql)-i),
  i+j,
  QUESTMASTER->QueryOfficialQP(),
  QUESTMASTER->QueryMiniQP(),
  QUESTMASTER->QueryTotalQuestPoints(),
  QUESTMASTER->QuerySeerQuestPoints(),
  QUESTMASTER->QueryWizQuestPoints(),
  xp,give_xp,
  FILEDATE(QCTRLFILE,j),
  FILEDATE(QUESTLIST_FILE,j),
  FILEDATE(MINI_SAVE_FILE,j)
  ));

  return 1;
}


#define QM_SHOWMINI 1
#define QM_SHOWOFF 2
#define QM_SHOWALL QM_SHOWOFF | QM_SHOWMINI
#define QM_SHORT 4

questinfo(mixed s) {
  mixed mqnames,oqnames,h1,h2;
  int i,off_xp,mini_xp,mode;
  string out;

  if (s && s=="?") {
    write("\
Usage: qinfo [options], where options are:\n\
 short    only show the questnames, no further information.\n\
 off      only show official quests\n\
 mini     only show miniquests\n\
 alph     sort officials by alphabet\n\
"/* <questname> show some information about quest <questname>\n\
"*/);
    return 1;
  }
  out="";
  if (!s) s=({});
  else s=explode(s," ");
  if (member(s,"mini")==-1 && member(s,"off")==-1) mode |= QM_SHOWALL;
  if (member(s,"mini")!=-1) mode |=QM_SHOWMINI;
  if (member(s,"off")!=-1) mode |=QM_SHOWOFF;
  if (member(s,"short")!=-1) mode |=QM_SHORT;
  oqnames=QUESTMASTER->QueryQuestNames();
  if (member(s,"alph")!=-1) oqnames=sort_array(oqnames,#'>); /*')*/
  mqnames=sort_array(QUESTMASTER->QueryAllQuestNames() - oqnames,#'>); /*')*/
  if (mode & QM_SHOWOFF) {
    out+="Official quests:\n";
    for (i=0;i<sizeof(oqnames);i++) {
      off_xp+=(h1=QUESTMASTER->QueryQuestXP(oqnames[i]));
      h2=QUESTMASTER->QueryQuestQP(oqnames[i]);
      if (mode & QM_SHORT)
	out+=sprintf("%-25s",oqnames[i])+((i%3)==2 ? "\n" : "");
      else
	out+=sprintf("%-25s %4d QP %6d XP %s\n",
	     oqnames[i],h2,h1,QUESTMASTER->QueryQuestCreator(oqnames[i]));
    }
  }
  if (mode & QM_SHOWMINI) {
    out+="\nMiniquests:\n";
    for (i=0;i<sizeof(mqnames);i++) {
      mini_xp+=(h1=QUESTMASTER->QueryQuestXP(mqnames[i]));
      h2=QUESTMASTER->QueryQuestQP(mqnames[i]);
      if (mode & QM_SHORT)
	out+=sprintf("%-25s",mqnames[i])+((i%3)==2 ? "\n" : "");
      else
	out+=sprintf("%-25s %4d QP %6d XP %s\n",
	     mqnames[i],h2,h1,QUESTMASTER->QueryQuestCreator(mqnames[i]));
    }
  }
  if (mode & QM_SHOWOFF) out+=sprintf("\n\
Official quests: %4d quests, %5d QP, %7d XP\n",
	       sizeof(oqnames),QUESTMASTER->QueryOfficialQP(),off_xp);
  if (mode & QM_SHOWMINI) out+=sprintf("\n\
Miniquests     : %4d quests, %5d QP, %7d XP\n",
	       sizeof(mqnames),QUESTMASTER->QueryMiniQP(),mini_xp);
  smore(out,TP->QueryPageSize());
  return 1;
}


logcheck(string arg) {
  mixed data;
  int i,size;
  string str;
/*
  CHECKWIZLEVEL(LEARNER_LVL)
*/
  if (!arg) {
    str="";
    for (i = 0; i < sizeof(scopes); i++) {
      data=scopes[i];
      size=file_size(data[SC_FILE]);
      if (size!=data[SC_SIZE]) {
	str += ((ladjust ("--- "+data[SC_FILE]+" ("+data[SC_SIZE]+"/"+size+") ", 75, "-"))+"\n");
	if (size<data[SC_SIZE]) {
	  if (size<0) str+="File has disappeared!\n";
	  else str+="File has shrunk since last view!\n";
	}
	else str+=read_bytes (data[SC_FILE], (data[SC_SIZE]>0 ? data[SC_SIZE] : 0) , size-data[SC_SIZE]);
	scopes[i][SC_SIZE]=size;
      }
    }
    smore(str,TP->QueryPageSize());
    return 1;
  }
  if (arg=="update") {

    for (i = 0; i < sizeof(scopes); i++) scopes[i][SC_SIZE]=file_size(scopes[i][SC_FILE]);
    write("Updating filesizes without reading.\n");
    return 1;
  }
  if (arg=="?") {
    write("\
The command logview will watch over a list of logs and show\n\
you any new lines on request, storing the length of the files.\n\
logview               show changes since last view\n\
logview filename      add filename to list of logs to watch\n\
logview ?             this helppage\n\
logview update        updates filesizes without reading\n\
logview list          lists viewed logs\n\
logview remove <nr>   removes log <nr> from the list\n\
");
    return 1;
  }
  if (arg=="list") {
    for (i=0;i<sizeof(scopes);++i) write(sprintf("%3i: %s\n",i+1,scopes[i][SC_FILE]));
    return 1;
  }

  if (sscanf(arg,"remove %d",i)) {
    string was;
    if (i<0 || i>sizeof(scopes)) return (int)notify_fail("Illegal value\n");
    was=scopes[i-1][SC_FILE];
    scopes=scopes[0..i-2]+scopes[i..<1];
    write("Removed "+was+" from viewlist.\n");
    if (!sizeof(scopes)) rm (SAVEDATA+name+".o");
    return 1;
  }
  if (sizeof(explode(arg," "))>1) return notify_fail("Too many arguments to logview - just 1 argument allowed (except remove <nr>)\n"),0;
  arg=this_player()->get_path(arg);
  scopes+=({({arg,file_size(arg)})});
  write("Added "+arg+" to viewlist.\n");
  return 1;
}


static string pwheredataline(mixed pl) {
  return 
    sprintf("%s%2d %-12s%-23.23s",
	    (IS_IMMORTAL(pl[0]) ? "w" : " "),
	    (IS_IMMORTAL(pl[0]) ? query_user_level(pl[0]) : pl[0]->QueryLevel()),
	    capitalize(pl[0]->QueryRealName()),
	    pl[1]   
	    );
}


static int pwheregreater(mixed pl1, mixed pl2) {
  return (pl1[1] > pl2[1]);
}


pwhere(string arg) {
  mixed p,pl;

  int i,halfusers,nrusers;
  string outstr;
/*
  CHECKWIZLEVEL(LEARNER_LVL)
*/  
  if (arg) {
    if (!(p=find_player(arg))) 
      return (int)notify_fail("Sorry, can't find this player.");
    write(pwheredataline(({p,TOWN(p)}))+" "+query_ip_name(p)+"\n");
    return 1;
  }
  p=users();
  pl=({});
  for (i=sizeof(p)-1;i>=0;--i) {
    pl+=({({p[i],TOWN(p[i])})});
  }
  pl=sort_array(pl,"pwheregreater",this_object());
  nrusers=sizeof(p);
  halfusers=(nrusers+1)/2;
  outstr=LINE;
  for (i=0; i<halfusers; ++i) {
    outstr+=pwheredataline(pl[i]);
    outstr+="|";
    if (i+halfusers<nrusers) outstr+=pwheredataline(pl[i+halfusers]);
    outstr+="\n";
  }
  outstr+=LINE;
  write(outstr);
  return 1;
}

  
testycheck(string arg) {
  mixed pl;
  string s,owner;
  int i;
/*
  CHECKWIZLEVEL(LEARNER_LVL)
*/
  if (arg) {
    if (pl=TESTY(arg)) 
      write(capitalize(arg)+" is registered as testcharacter of "+capitalize(pl)+".\n");
    else 
      write(capitalize(arg)+" seems not to be a testcharacter.\n");
    return 1;
  }
  pl=users();
  s="";
  for (i=sizeof(pl)-1;i>=0;--i) {
    if (owner=TESTY(pl[i])) s+=
      sprintf("%-15s from %-25.25s Owner: %-20s\n",
	      capitalize(pl[i]->QueryRealName()),TOWN(pl[i]),capitalize(owner));
  }
  if (s=="") write("There seem to be no testcharacters online.\n");
  else write("These players online are registered as testcharacters:\n"+s);
  return 1;
}


gdlagindex(str) {
  
  user=this_player();
  if (!str || !sscanf(str,"%d",countlen)) countlen=10;
  write("measuring GameDriver Lag ("+countlen+" turns)\n");
  count=-countlen;
  counth=-1;
  call_out("lagcount",0);
  lagcountmode=1;
  set_heart_beat(1);
  return 1;
}


lagcount() {
  if (count<0) {
    timeset=time();
    count=-count+1;
  }
  --count;
  if (!count) {
    tell_object(user,"\nAverage call_out time  : "+divide(time()-timeset,countlen)+" sec.\n");
  }
  else call_out("lagcount",0);
}


pgrepstop() {
  if (!pgrepmode) return 
    (int)notify_fail("No pgrep process running!\n");
  pgrepmode=0;
  write("pgrep process cancelled.\nPresent results written to "+PGREPOUT+"\n");
  write_file(PGREPOUT,"*** CANCELLED ***\n");
  return 1;
}


pgrep(string s) {
/*
  CHECKWIZLEVEL(LORD_LVL)
*/
  if (!s || s=="?") {
/* 456789012345678901234567890123456789012345678901234567890123456 */
    write("\
Usage: pgrep <regexp>\n\
pgrep searches ALL player save-files for matching lines according\n\
<regexp>. See man regexp for further info on regular expressions.\n\
This function runs asyncronous (reading a certain number of files\n\
per heartbeat) and saves the results in a file.\n\
The results of the search will be stored in\n\
/open/<name>.pgrep.out\n\
where name is your name. Please do not forget to remove the file.\n\
If the file already exists the new results will be appended.\n\
A started pgrep process may be cancelled with 'pgrepstop'.\n");

/* Well, it would be possible directly to write to the players 
   dir using my VAW uid. Should I do this?
   Another way would be to put the tool to /obj/tool so it has the 
   cloners uid anyway.
                                         - Loco, fighting wih (e)uids
*/
    return 1;
  }
  if (getuid() != getuid(this_player())) 
    pgreppath="/open/"+getuid(this_player())+".";
  else pgreppath="/players/"+getuid(this_player())+"/";

  if (pgrepmode) 
    return (int)notify_fail("\
Only one pgrep process may be run at once. Please wait until it is finished\n\
or stop with pgrepstop.\n");
  if (!(regexp(({"dummy"}), s)))
    return (int)notify_fail("Bad regular expression");
  pgrepmode=1;
  hitcount=0;
  alphpos=0;
  filecount=0;
  direc=({});
  dirpos=-1;
  pgrepstring=s;
  write_file(PGREPOUT,
"PGREP results searching for: "+s+"\nsearch date: "+ctime(time())+"\n\n");
  write("Starting PGrep search for "+s+",\n\
("+PGREP_PER_HB+" files per heart_beat, storing results in "+PGREPOUT")\n");
  user=this_player();
  set_heart_beat(1);
  return 1;
}

DoPGrep() {
  int i;
  
  for (i=PGREP_PER_HB; (i) ;--i) {
    ++dirpos;
    if (dirpos==sizeof(direc)) {
      ++alphpos;
      if (alphpos>26) {
	pgrepmode=0;
	tell_object(user,"\n\
pgrep: ready. Read "+filecount+" player files with "+hitcount+" matching lines.\n\
results written to "+PGREPOUT+"\n");
	if ((PGREPOUT)[0..4]=="/open") tell_object(user,"Please remove the result file - mv "+PGREPOUT+" ~/pgrep.out\n");
	write_file(PGREPOUT,"*** FINISHED "+ctime(time())+", "+filecount+" files, "+hitcount+" matching lines ***\n");
	return;
      }
      direc=get_dir("/save/"+ALPHABET[alphpos..alphpos]+"/*.o");  
      tell_object(user,"[pgrep: reading "+ALPHABET[alphpos..alphpos]+":"+sizeof(direc)+" files]\n");
      dirpos=-1;
      filecount+=sizeof(direc);
    }
    else GrepPlayerFile(direc[dirpos][0..<3]);
  }
}

GrepPlayerFile(string file) {
  int i, f;
  string *ts, t;
  
  if (!(t=read_file("/save/"+file[0..0]+"/"+file+".o"))) return;
  
  if(f=sizeof(ts=regexp(explode(t, "\n"), pgrepstring))) {
    for (i=0;i<f;++i)
      write_file(PGREPOUT, sprintf("%-12s: ",file)+ts[i]+"\n");
    hitcount+=f;
  }
}

heart_beat() {
  if (lagcountmode) {
    ++counth;
    if (!counth) timeset=time();
    if (counth==countlen) {
      tell_object(user,"\nAverage heart_beat time: "+divide(time()-timeset,countlen)+" sec.\n");
      lagcountmode=0;
    }
  }
  if (pgrepmode) DoPGrep();
  if (!(lagcountmode || pgrepmode)) set_heart_beat(0);
}

static string divide(int i1, int i2) { 
  int i;
  string s;
  if (i2==0) return "not calculable";
  i=i1*10/i2;
  if (i<10) return "0."+i;
  s=(string)(i);
  return s[0..<2]+"."+s[<1..<1];
}

static timestr(int i) {
  string s;
  s="";
  if (i/43200) { s+= (i/43200) + " days "; i = i - (i/43200)*43200; }
  if (i/1800)  { s+= (i/1800) + " hours "; i = i  - (i/1800)*1800; }
  if (i/30)    { s+= (i/30) + " minutes "; i = i - (i/30)*30; }
  if (i) s+= (i*2) + " seconds.";
  return s;
}

playerinfo(string name) {
  object p;
  mixed h;
  string s;
  int i;
/*  
  CHECKWIZLEVEL(LEARNER_LVL)
*/
  if (!name) 
    return (int)notify_fail("Please give name of player as argument.\n");
  if (!(p=find_player(name))) {
    if (!(p=find_living(name)))
      return (int)notify_fail("Can't find a player named "+name+".\n");
    else write(name+" is no player but an npc. Anyway, here are the stats:\n");
  }
  write(sprintf("\
Name:      %-15sRealName: %-15sGender: %s\n\
Full Name: %s\n\
Host:      %s\n\
Race:      %-15sGuild:    %-15s\n\
Age:       %s\n\
Wizlevel:  %-7dPlayerlevel:%5d   RealLevel: %3d \n\
XP:%12d   QP:   %5d (%4d)  (QP for Level: %d, next: %d)\n\
HP:  %5d (%3d)  SP:   %5d  (%3d)  Alignment:%7d\n\
Food:%5d (%3d)  Drink:%5d  (%3d)  Alc:%5d (%3d)\n",

  ((h=p->QueryName()) ? h : "- none -"),
  ((h=p->QueryRealName()) ? h : "- none -"),
  ((h=p->QueryGender())==1 ? "male" : (h==2 ? "female" : "other")),
  ((h=p->QueryShort()) ? h : "- invisible -" ),
  ((h=query_ip_name(p)) ? h+" ["+TOWN(p)+"]" : "- netdead -" ),
  ((h=p->QueryRace()) ? h : "- none (?!?) -"),
  ((h=p->QueryGuild()) ? h : "- none -"),
  timestr(p->QueryAge()),
  query_user_level(p),p->QueryLevel(),p->QueryRealLevel(),
  p->QueryXP(),
  (QUESTMASTER->QueryQuestPoints(p)),
  (QUESTMASTER->QueryTotalQuestPoints(p)),
  (QUESTMASTER->QPointLevel(p->QueryLevel())),
  (QUESTMASTER->QPointLevel(p->QueryLevel()+1)),
  p->QueryHP(),p->QueryMaxHP(),p->QuerySP(),p->QueryMaxSP(),
  p->QueryAlign(),
  p->QueryFood(),p->QueryMaxFood(),p->QueryDrink(),p->QueryMaxDrink(),
  p->QueryAlcohol(),p->QueryMaxAlcohol())
	);
  s="Stats:\t";
  h=p->QueryStats();
  if (!h || !pointerp(h) || !sizeof(h[0])) s+="none";
  else for (i=0;i<sizeof(h[0]);++i) s=s+h[0][i]+": "+STR(h[1][i])+",  ";
  s+="\nAttrib:\t";
  h=p->QueryAttributes();
  if (mappingp(h)) h = ({ m_indices(h), m_values(h) });
  if (!h || !pointerp(h) || !sizeof(h[0])) s+="none";
  else for (i=0;i<sizeof(h[0]);++i) s=s+h[0][i]+": "+STR(h[1][i])+",  ";
 
  write(s+"\n");
  return 1;
}

static string location(string fn) {
  string *ex;
  ex=explode(fn,"/");
  switch (ex[1]) {
    case "players": return "~"+ex[2]; break;
    case "d"      : return "+"+ex[2]; break;
    default       : return "/"+ex[1];
  }
}


static string GetTown(mixed p) {
  mixed h;
  h=("/lib/country"->resolve_ip_number(query_ip_number(p)));
  if (lower_case(h)=="unknown" && query_ip_number(p)!=query_ip_name(p))
    h=implode(explode(query_ip_name(p),".")[<2..<1],".");
  return h;
}


pstatistics(string arg)
{
/* nrs: 0=all, 1=players, 2=wizards, 3=testies, 4=male, 
        5=female, 6=seers, 7=neuter
*/
  string *countries,*domains,p_country;
  int a_wizlevel,a_playerlevel,*nrs,*nr_countries,*nr_domains;
  int a_pmoney,a_wizmoney,a_invent,a_idle,
      i,cpos;
  object *players,env;
  string name;
/*
  CHECKWIZLEVEL(LEARNER_LVL)
*/
  countries=({}); nr_countries=({});
  domains=({}); nr_domains=({});
  nrs=allocate(8);
  players=users();
  for (i=0; i<sizeof(players); ++i) {
    name=lower_case(players[i]->QueryRealName());
    if ((name!="mudlink") && (name!="<logon>")) {
      if (arg) {
	a_invent+=sizeof(all_inventory(players[i]));
	a_idle+=query_idle(players[i]);
      }
      p_country="/lib/country"->country(players[i]);
      cpos=member(countries, p_country);
      if (cpos==-1) {
	countries+=({p_country});
	nr_countries+=({1});
      } else 
	++nr_countries[cpos];
      switch (players[i]->QueryGender()) {
	case 0:++nrs[7]; break;
        case 1:++nrs[4]; break;
        case 2:++nrs[5]; break;
      }
      if (IS_IMMORTAL(players[i])) {
	++nrs[2];
	a_wizlevel+=query_user_level(players[i]);
	if (arg) catch(a_wizmoney+="/lib/money"->QuerySomeonesValue(players[i]));
      } else {
	++nrs[1];
        if TESTY(name) ++nrs[3];
	if (IS_IMMORTAL(players[i])) ++nrs[6];
	a_playerlevel+=players[i]->QueryLevel();
	if (arg) a_pmoney+="/lib/money"->QuerySomeonesValue(players[i]);
	env=environment(players[i]);
	if (env) {
	  p_country=location(file_name(env));
	  cpos=member(domains, p_country);
	  if (cpos==-1) {
	    domains+=({p_country});
	    nr_domains+=({1});
	  } else
	    ++nr_domains[cpos];
	}
      }
    }
  }
  nrs[0]=nrs[2]+nrs[1];

  /*--- Evaluation ------------------------------------------------------------*/

  write(
"\n                       Player Statistics\n\
-------------------------------------------------------------------------\n\
Players: "+nrs[1]+", average playerlevel: "+divide(a_playerlevel,nrs[1])+"\
 (including "+nrs[6]+" seers)\n\
Wizards: "+nrs[2]+", average wizlevel: "+divide(a_wizlevel,nrs[2])+"\n\
"+nrs[3]+" of the players are known as Testcharacters.\n\
male: "+nrs[4]+", female: "+nrs[5]+(nrs[7] ? (", neuter: "+nrs[7]) : "")+"\n\
Players per Wizard : "+divide(nrs[1],nrs[2])+"\n\
Wizlevel per player: "+divide(a_wizlevel,nrs[1])+"\n\
There are players from "+sizeof(countries)+" different countries:\n\
");
  for (cpos=0; cpos<sizeof(countries); ++cpos) 
    write(countries[cpos]+": "+nr_countries[cpos]+", ");
  write("\n\
Where the players (not wizards, but also seers) are:\n\
");
  for (cpos=0; cpos<sizeof(domains); ++cpos) 
    write(domains[cpos]+": "+nr_domains[cpos]+", ");
  write("\n");
  if (arg) write("\
Average money: Players "+divide(a_pmoney,nrs[1])+", Wizards "+divide(a_wizmoney,nrs[2])+
", together "+divide(a_pmoney+a_wizmoney,nrs[0])+" cc.\n\
Average inventory: "+divide(a_invent,nrs[0])+" objects.\n\
Average idle time: "+divide(a_idle,nrs[0])+" seconds.\n\
");
  return 1;
}
