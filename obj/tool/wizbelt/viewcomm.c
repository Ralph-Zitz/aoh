#include <properties.h>
#include <config.h>
#include <combat.h>
#include <secure/wizlevels.h>
#include <attributes.h>
#include <daemons.h>
#include "./wizbelt-defaults.h"

#if MUD_UL
#include "alias.h"
#endif

#define MAILSERVER "/mail/mailer"

string *turned_on;
int ShowObject(mixed arg);
int WFinger(string str);
object WFindObj(string str, object obj);
string WFindFile(string str);
string daytime(int sec);
string AdditionalInformation(string name, int flag, int gender);

int Write(string x) { 
    string text;
    if (text=x) { 
       "/lib/string"->smore(text,({int})this_player()->QueryPageSize()); 
       return 1;
    } 
    else return 0;
}

int wlist(string str) {
#if MUD_NF
  wizlist(str);
#endif
  return 1;
}

string WFindFile(string str) {
  string a, ret;
  ret = "";
  if (!str) return ret;
  if (str=="$h") return object_name(environment(this_player()));
  if (this_player() && ({string})this_player()->is_dir(str))
     return ({string})this_player()->is_dir(str);
  if (sscanf(str,"@^%s",a)==1) return object_name(environment(find_living(a)));
  if (sscanf(str,"+%s",a)==1) str="/d/"+a;
  if (sscanf(str,"~",a)==1) str="/players/"+a;
  ret=({string})this_player()->exp_fname(str); 
  if (sscanf(ret,"%s/%~s",a)<1) 
     ret = ({string})this_player()->QueryCurrentDir()+"/"+ret;
  if (resolve_file(ret)) return resolve_file(ret);
  return ret;
}

string blanks(string str, int i) {
   int x,bla;
   string ret;

   if (!str) str=" ---- no short description ----";
   ret=str;
   x=sizeof(str);
   bla=i-x;
   if (bla>0) {for (x=0; x<bla; x++) {ret=ret+" "; }};
   x=sizeof(str);
   if (x>0) return ret[0..i-1];
   return 0;
}

string cutlen(string str, int i) {
   if (i > sizeof(str)) return str;
   return str[0..i-1];
}

int wlook(string str) {
  string a, ausg;
  int x, num;
  object who, ob;
  if (!Allowed()) return 1;
  if (!str) who = environment(this_player());
  if (!who) who = find_living(str);
  if (!who) who = find_living(lower_case(str));
  if (!who) who = WFindObj(str, this_player());
  if (!who) a = WFindFile(str);
  if (!who && !a) { 
    if (sscanf(str,"/%s",a)<1) {
       write(str+" is not in " MUDNAME ".\n");
       return 1;
    }
  }
  x = -1;
  if (a) x = file_size(a);
  if (!who && a) who = find_object(a);
  if (!who && x >= 0) who = clone_object(a);
  if (!who) { write("Can't find file "+a+".\n"); return 1; }
  ausg = "Filename: " + object_name(who) +"\n";
  if (({int})who->QueryIsNPC() || ({int})who->QueryIsPlayer()) 
     ausg += ({string})who->QueryName()+"'s inventory:\n";
  else if (({string})who->QueryShort()) ausg += ({string})who->QueryShort()+" contains:\n";
  else ausg += ({string})who->QueryIntShort()+" contains:\n";
  ob = first_inventory(who);
  num = 0;
  while (ob) {
    num += 1;
    if (num<10) ausg += " ";
    ausg += num + ". " + blanks(({string})ob->QueryShort(),38)+" "+
	        cutlen("["+object_name(ob),35)+"]\n";
    ausg += " Value:"+({int})ob->QueryValue()+"\t";
    ausg += "Weight:"+({int})ob->QueryWeight()+"\t";
    ausg += "AC:"+({int})ob->QueryAC()+"\t";
    ausg += "WC:"+({int})ob->QueryWC()+"\t";
    if (({string})ob->QueryInfo()) ausg+=cutlen(explode(({string})ob->QueryInfo(), "\n")[0], 30);
    ausg += "\n";
    ob=next_inventory(ob);
  }
  Write(ausg);
  return 1;
}

int wmore(string str) {
  string file;
  object obj;
  if (!str) { notify_fail("Missing argument"); return 0; }
  obj = WFindObj(str, this_player());
  if (obj) file=object_name(obj);
  if (!file) file = WFindFile(str);
  if (file) file = explode(file, "#")[0];
  if (!file || ((file_size(file + ".c") < 0) && (file_size(file) < 0))) {
     this_player()->more(str, 0, this_player()->QueryPageSize());
     return 1;
  }
  this_player()->more(file + ".c", 0, this_player()->QueryPageSize());
  return 1;
}

object WFindObj(string str, object obj) {
  object who, *objs;
  string *parts;
  int x;
  string such,newstr;
  // write("Trying to find object "+str+"\n");
  parts=explode(str,".");
  such=parts[0];
  if (such=="$m") { 
     if (sizeof(parts)<2) return this_player();
     parts-=({"$m"});
     newstr=implode(parts, ".");
     return WFindObj(newstr, this_player());
  }
  if (such=="$h") {
     if (sizeof(parts)<2) return environment(this_player());
     parts-=({such});
     newstr=implode(parts, ".");
     return WFindObj(newstr, environment(this_player()));
  }
  // Check if a special object number is specified
  if (!who && sscanf(such, "%d", x)>0) {
     objs = all_inventory(obj);
     if (sizeof(objs) < x || x < 0) {
        write("No object with number "+x+".\n");
        return 0;
     }
     return objs[x-1];
  }
  // Then check the environment of obj
  if (!who && environment(obj)) who = present(such, environment(obj));
  // Then check the inventory of obj
  if (!who && obj) who = present(such, obj);
  // If it can't be found in the environment or the inventory
  // check if a living with that name exists
  if (!who) who=find_living(such);
  if (!who) return who;
  if (sizeof(parts)<2) return who;
  parts-=({such});
  newstr=implode(parts, ".");
  return WFindObj(newstr, who);
}

int ShowObject(mixed arg) {
 object who;
 int i;
 mixed *pids;
 if (stringp(arg)) who=WFindObj(arg,this_player()); else who = arg;
 if (!who) return 0; // Can't find that object
 write(({string})who->QueryShort()+"\n");
 write(({string})who->QueryLong());
 if (({string})who->QueryInfo()) write("Info: "+({string})who->QueryInfo());
 if (({int})who->QueryBright()) write("Bright: "+({int})who->QueryBright()+"\n");
 pids=({string *})who->QueryIds();
 write("Ids: ");
 for (i=0; i<sizeof(pids); i++) {
     if (i < sizeof(pids)-1) write(pids[i]+", ");
     else write(pids[i]+"\n");
 }
 write("Weight: "+({int})who->QueryWeight()+"\n");
 write("Value: "+({int})who->QueryValue()+"\n");
 if (({int})who->QueryAC()) write("AC: "+({int})who->QueryAC()+"\n");
 if (({int})who->QueryWC()) write("WC: "+({int})who->QueryWC()+"\n");
 if (({string})who->QueryNoise()) write("Noise: "+({string})who->QueryNoise());
 if (({string})who->QuerySmell()) write("Smell: "+({string})who->QuerySmell());
 if (({string})who->QueryReadMsg()) write("Engraving: "+({string})who->QueryReadMsg());
 if (({int})who->QueryNoGet()) write("You can't take it.\n");
 if (({int})who->QueryNoDrop()) write("It can't be dropped.\n");
 if (({int})who->QueryNoGive()) write("It can't be given away.\n");
 if (({int})who->Query(P_MAGIC)) write("It is a magical item.\n");
 if (({object})who->QueryWielded()) write("It is wielded.\n");
 if (({string})who->QueryWeaponType()) write("This is a weapon of the type " +
     ({string})who->QueryWeaponType() + ".\n");
 if (({object})who->QueryWorn()) write("It is worn.\n");
 if (({string})who->QueryArmourType()) write("This is an armour of the type  " +
     ({string})who->QueryArmourType() + ".\n");
 return 1;
}

int on(string str) {
 if (!str) return 0;
 if (member(turned_on, str) > -1) return 1;
 return 0;
}

int stat(string str) {
 mixed abili;
 mixed abili2;
 mixed *arm, *weap;
 object who,snp;
 string aus, name;
 int n, i, wizlev, ali, qp, nqp, y, acs, xpgiven, gender;
 if (!Allowed()) return 1;
 if (!str) { write("Missing parameter.\n"); return 1; }
 if (str=="$m") str=({string})this_player()->QueryRealName();
 who = WFindObj(str, this_player());
 if (who) write(object_name(who)+"\n");
 if (!living(who) && ShowObject(who)) return 1;
 if (!who) { WFinger(str); return 1; }
 wizlev=({int})MASTER->query_user_level(getuid(who));
 aus = "";
 aus += " * * * "+({string})who->QueryShort()+" * * * \n";
 if (living(who) && environment(who) && on("environment"))
    aus += "Environment: " + object_name(environment(who)) + "\n";
 if (!({int})who->QueryIsNPC() && on("title")) aus += "Title: "+({string})who->QueryTitle()+"\n";
 aus += "Player-Level: "+({int})who->QueryLevel()+"\t Wiz-Level: "+
	wizlev+"\n";
 aus += "SP: "+({int})who->QuerySP()+" ("+
       ({int})who->QueryMaxSP()+") , HP: "+
       ({int})who->QueryHP()+" ("+({int})who->QueryMaxHP()+") ,\t";
 aus += "XP: "+({int})who->QueryXP()+"\t";
 xpgiven = ({int})who->QueryXP()/35;
 if (xpgiven > 20000)
    xpgiven = 20000+(xpgiven-20000)/10;
 if (xpgiven > 50000)
    xpgiven = 50000+(xpgiven-50000)/100;
 aus += "KillXP: "+xpgiven+"\n";
 abili=({mapping})who->QueryStats();
#if MUD_UL
 abili = ({ ({"Str","Dex","Int","Con","IVision","UVison"}),
         ({who->query("str"), who->query("dex"), 
           who->query("int"), who->query("con"),
           who->query("infravision"), who->query("ultravision") }) });
#endif
 if (abili) {
   for (i=0; i<sizeof(abili[0]); i++) {
       aus += abili[0][i]+":"+abili[1][i]+"\t"; }
   aus += "\n";  }
 if (on("attributes")) {
    aus += "Attributes:     ";
    abili2=({mapping})who->QueryAttributes();
    if (!abili2) aus += "none\n";
    if (abili2) {
       n=6;
       abili = m_indices(abili2);
       for (i=0; i<sizeof(abili); i++) {
          n+=1;
          if (n==7) { n=0; aus += "\n"; }
          if (stringp(abili[i]) && 
             member(LIVING_ATTRS, abili[i]) < 0) {
             // Ignore builtin attributes ^
             name = abili[i];
             if (sizeof(name) > 3) name = name[0..2];
             if (intp(abili2[abili[i]]))
	         aus += blanks(name+":"+abili2[abili[i]],11);
             else aus += blanks(name+": skill",11);
          } // else aus += blanks("???: error",11);
       }
       aus += "\n";
    }
 }
 if (({int})who->QueryIsPlayer()) {
    /* aus += "Skills: \n";
     * who->flspells("skills");
     */
    aus += "Guild: ";
    if (({string})who->QueryGuild()) aus += ({string})who->QueryGuild()+"\n";
    else aus += "none\n";
 }
 name=({string})who->QueryName();
 aus += "soaked: "+({int})who->QueryDrink()+"\t";
 aus += "stuffed: "+({int})who->QueryFood()+"\t";
 aus += "intoxicated: "+({int})who->QueryAlcohol()+"\n";
 aus += "Race: "+({string})who->QueryRace()+"\tWeight: "+({int})who->QueryWeight()+" ("+
       ({int})who->QueryMaxWeight()+")\n";
 if (on("messages")) {
    if (stringp(({mixed})who->QueryMsgIn())) aus+="MSGIN: "+({string})who->QueryMsgIn()+"\n";
    if (stringp(({mixed})who->QueryMsgOut())) aus+="MSGOUT: "+({string})who->QueryMsgOut()+"\n";
    if (stringp(({mixed})who->QueryMMsgIn())) aus+="MMSGIN: "+({string})who->QueryMMsgIn()+"\n";
    if (stringp(({mixed})who->QueryMMsgOut())) aus+="MMSGOUT:"+({string})who->QueryMMsgOut()+"\n";
 }
 arm = ({object *})who->QueryArmours();
 weap = ({object *})who->QueryWeapons();
 acs = 0;
 if (arm && sizeof(arm)) for (i=0; i<sizeof(arm); i++)
     if (arm[i]) {
        acs += ({int})arm[i]->QueryAC();
        if (on("armours") && ({string})arm[i]->QueryShort()) 
           aus += "Wearing " + ({string})arm[i]->QueryShort() + "(AC:" + 
               ({int})arm[i]->QueryAC() + ")\n";
     }
 if (weap && sizeof(weap)) for (i=0; i<sizeof(weap); i++)
     if (weap[i]) {
        if (on("weapons") && ({string})weap[i]->QueryShort()) 
           aus += "Wielding " + ({string})weap[i]->QueryShort() + "(AC:" + 
               ({int})weap[i]->QueryWC() + ")\n";
     }
 aus += "Armourclass: "+acs+"\n";
 gender = ({int})who->QueryGender();
 aus += "Gender: "+({string})who->QueryGenderString()+"\n";
 if (!({int})who->QueryIsNPC() && on("quests")) {
    aus += "Quests: "+({string})who->QueryQuests()+"\n";
    qp= ({int})QUESTMASTER->QueryQuestPoints(who);
    y= ({int})who->QueryLevel();
    nqp= ({int})QUESTMASTER->QPointLevel(y+1);
    y=0;
    aus += name+" has "+qp+" questpoints and needs "+nqp+" to advance.\n";
 }
 if (interactive(who)) {
     y=query_idle(who);
     if (y>30) aus += name+" has been idle for "+y+" seconds now.\n";
 }
/*
    snp=query_snoop(who);
This function has to be disabled as long as there is no check that only the
valid snoop checks are displayed. That means,
if a wl 30 snoops and a wl 20 queries, this MUST NOT be displayed, e.g.
*/
  snp=0;
  if (!({int})who->QueryIsNPC()) {  
    if (({int})who->QueryGhost()) aus += name+" is a ghost.\n";
    if (({int})who->QueryFrog()) aus += name+" is a frog.\n";
    if (({int})who->QueryNetdead()) aus += name+" is netdead.\n";
    if (({int})who->QueryInvis()) aus += name+" is invisible.\n";
    if (snp) aus += name+" is snooped by "+
             capitalize(({string})snp->QueryRealName())+".\n";
    ali=({int})who->QueryAlign();
    aus += "Alignment: "+({string})this_player()->QueryAlignString(ali)+"\n";
    aus += "IP: "+query_ip_number(who)+"\n";
#if MUD_NF
    aus += "Mail: "+({string})who->QueryMailaddr()+"\n";
#endif
#if MUD_UL
    aus += "Mail: "+({string})who->QueryMailAddr()+"\n";
#endif
    aus += "Age: "+daytime(({int})who->QueryAge()*2)+".\n";
    aus += AdditionalInformation(({string})who->QueryRealName(), 0, gender);
    if (on("mails") && n = ({int})MAILSERVER->FingerMail(str)) 
       aus += "There are " + n + " new mails waiting for this player.\n";
 }
 Write(aus);
 return 1;
}

string daytime(int sec) {
  string ret;
  ret = "";
  if (sec >= 86400) { // days
     ret += sec/86400+" days ";
     sec %= 86400;
  }
  if (sec < 36000) ret += "0";
  ret += sec/3600+" hours "; // hours
  sec %= 3600;
  if (sec < 600) ret += "0";
  ret += sec/60+" minutes "; // minutes
  sec %= 60;
  if (sec < 10) ret += "0";
  ret += sec+" seconds"; // seconds
  return ret;
}

int ULFinger(string str) {
 string gener, saved, r, name, cap, guild;
 string race, mail, age;
 string *info, *dummy, *stat, aus;
 int idletime, gender, alt;
 if (!str) { notify_fail("No such player.\n"); return 0; }
 if (sizeof(explode(str, " ")) > 1) {
    notify_fail("Don't use spaces within a name.\n");
    return 0;
 }
 cap = lower_case(str);
 name = "/save/"+cap[0..0]+"/"+cap+".o";
 dummy = ({""});
 aus = "";
 cap = capitalize(cap);
 if (file_size(name) < 0) {
    write("I can't find a save file for that player.\n");
    return 0; 
 }
 info = explode(read_file(name), "\n");
 idletime = file_date(name);
 name = cap;
 saved = (regexp(info, "^life_saved")+dummy) [0][12..<2];
 gener = (regexp(info, "^general")+dummy) [0][9..<2];
 sscanf(saved, "%~sgender\":%d,%~s",gender);
 stat=({});
 sscanf(saved, "%~slevel\":%s,%~s",r);
 stat+=({r});
 sscanf(saved, "%~shit_points\":%s,%~s",r);
 stat+=({r});
 sscanf(saved, "%~sspell_points\":%s,%~s",r);
 stat+=({r});
 sscanf(saved, "%~sexperience\":%s,%~s",r);
 stat+=({r});
 sscanf(saved, "%~smax_hit_points\":%s,%~s",r);
 stat+=({r});
 sscanf(saved, "%~smax_spell_points\":%s,%~s",r);
 stat+=({r});
 sscanf(gener, "%~sinfravision\":%s,%~s",r);
 stat+=({r});
 sscanf(gener, "%~sultravision\":%s,%~s",r);
 stat+=({r});
 sscanf(saved, "%~sdrink\":%s,%~s",r);
 stat+=({r});
 sscanf(saved, "%~sfood\":%s,%~s",r);
 stat+=({r});
 sscanf(saved, "%~salcohol\":%s,%~s",r);
 stat+=({r});
 sscanf(saved, "%~sage\":%s,%~s",r);
 stat+=({r});
 sscanf(gener, "%~scon\":%s,%~s",r);
 stat+=({r});
 sscanf(gener, "%~sstr\":%s,%~s",r);
 stat+=({r});
 sscanf(gener, "%~sdex\":%s,%~s",r);
 stat+=({r});
 sscanf(gener, "%~sint\":%s,%~s",r);
 stat+=({r});
 sscanf(gener, "%~scalled_from_ip\":\"%s\",%~s",r);
 stat+=({r});
 sscanf(gener, "%~spresay\":%s,%~s",r);
 stat+=({r});
 sscanf(gener, "%~stitle\":%s,%~s",r);
 stat+=({r});
 sscanf(gener, "%~srace\":%s,%~s",race);
 sscanf(gener, "%~sguild\":%s,%~s",guild);
 sscanf(gener, "%~smailaddr\":%s,%~s",mail);
 if (!guild || guild=="" || guild=="0") guild = "none";
 if (!race || race=="0" || race=="") race = "none";
 if (!mail || mail=="0" || mail=="") mail = "none";
 aus = "* * * "+stat[17]+cap+" "+stat[18]+" * * *\n";
 r = "Player level: "+stat[0]+"\n";
 aus += r;
 aus += "SP: "+stat[2]+" ("+stat[5]+") , HP: "+stat[1]+" ("+stat[4]+") , XP: "+
       stat[3]+"\n";
 aus += "Dex: "+stat[14]+" Con: "+stat[12]+" Int: "+stat[15]+" Str: "+
       stat[13]+" IVision: "+stat[6]+" UVision: "+stat[7]+"\n";
 aus += "Guild: "+guild+"\n";
 aus += "Race: "+race+"\n";
 aus += "soaked: "+stat[8]+"\tstuffed: "+stat[9]+
       "\tintoxicated: "+stat[10]+"\n";
 switch(gender) {
     case 1: aus += "Gender: male\n"; break;
     case 2: aus += "Gender: female\n"; break;
     default: aus += "Gender: neuter\n";
 }
 aus += "Last login was from "+stat[16]+" ["+({string})COUNTRY_D->country(stat[16])+"].\n";
 aus += "Last logout was on "+ctime(idletime)+".\n";
 aus += "Mail: "+mail+"\n";
 alt = to_int(stat[11]) * 2;
 age = daytime(alt);
 aus += "Age: "+age+".\n";
 aus += AdditionalInformation(lower_case(str), 1, gender);
 Write(aus);
 return 1;
}

int WFinger(string str) {
 string r, name, ip, cap, presay, title, race, guild, mail, age;
 string *info, *dummy, *stat, aus;
 int n, idletime, gender, alt;
#if MUD_UL
 return ULFinger(str);
#endif
 if (!str) { write("No such player.\n"); return 0; }
 if (sizeof(explode(str, " ")) > 1) {
    notify_fail("Don't use spaces within a name.\n");
    return 0;
 }
 cap = lower_case(str);
 name = "/save/"+cap[0..0]+"/"+cap+".o";
 dummy = ({""});
 aus = "";
 cap = capitalize(cap);
 if (file_size(name) < 0) {
    write("I can't find a save file for that player.\n");
    return 0; 
 }
 info = explode(read_file(name), "\n");
 idletime = file_date(name);
 ip = (regexp(info, "^called_from_ip")+dummy) [0][16..<2];
 presay = (regexp(info, "^presay")+dummy) [0][8..<2];
 title = (regexp(info, "^title")+dummy) [0][7..<2];
 race = (regexp(info, "^race")+dummy) [0][6..<2];
 guild = (regexp(info, "^guild")+dummy) [0][7..<2];
 mail = (regexp(info, "^mailaddr")+dummy) [0][10..<2];
 gender = to_int((regexp(info, "^gender")+dummy) [0][7..]);
 stat = ({});
 r = (regexp(info, "^level")+dummy) [0][6..];         // level      0
 stat += ({ r });
 r = (regexp(info, "^hit_point")+dummy) [0][10..];    // hp         1
 stat += ({ r });
 r = (regexp(info, "^spell_points")+dummy) [0][13..]; // sp         2
 stat += ({ r });
 r = (regexp(info, "^experience")+dummy) [0][11..];   // xp         3
 stat += ({ r });
 r = (regexp(info, "^max_hp")+dummy) [0][7..];        // maxhp      4
 stat += ({ r });
 r = (regexp(info, "^max_sp")+dummy) [0][7..];        // maxsp      5
 stat += ({ r });
 r = (regexp(info, "^ivision")+dummy) [0][8..];       // ivision    6
 stat += ({ r });
 r = (regexp(info, "^uvision")+dummy) [0][8..];       // uvision    7
 stat += ({ r });
 r = (regexp(info, "^soaked")+dummy) [0][6..];        // soaked     8
 stat += ({ r });
 r = (regexp(info, "^stuffed")+dummy) [0][7..];       // stuffed    9
 stat += ({ r });
 r = (regexp(info, "^intoxicated")+dummy) [0][11..];  // intox     10
 stat += ({ r });
 r = (regexp(info, "^age")+dummy) [0][4..];        // age       11
 stat += ({ r });
 r = (regexp(info, "^Con")+dummy) [0][4..];        // con       12
 stat += ({ r });
 r = (regexp(info, "^Str")+dummy) [0][4..];        // str       13
 stat += ({ r });
 r = (regexp(info, "^Dex")+dummy) [0][4..];        // dex       14
 stat += ({ r });
 r = (regexp(info, "^Int")+dummy) [0][4..];        // int       15
 stat += ({ r });
 /* att = (regexp(info, "^attra")+dummy) [0][5..]; */
 if (!guild || guild=="") guild = "none";
 if (!race || race=="") race = "none";
 if (!mail || mail=="") mail = "none";
 aus = "* * * "+presay+cap+" "+title+" * * *\n";
 r = "Player level: "+stat[0]+"\n";
 aus += r;
 aus += "SP: "+stat[2]+" ("+stat[5]+") , HP: "+stat[1]+" ("+stat[4]+") , XP: "+
       stat[3]+"\n";
 aus += "Dex: "+stat[14]+" Con: "+stat[12]+" Int: "+stat[15]+" Str: "+
       stat[13]+" IVision: "+stat[6]+" UVision: "+stat[7]+"\n";
 aus += "Guild: "+guild+"\n";
 aus += "Race: "+race+"\n";
 aus += "soaked: "+stat[8]+"\tstuffed: "+stat[9]+
       "\tintoxicated: "+stat[10]+"\n";
 switch(gender) {
     case 1: aus += "Gender: male\n"; break;
     case 2: aus += "Gender: female\n"; break;
     default: aus += "Gender: neuter\n";
 }
 aus += "Last login was from "+ip+" ["+({string})COUNTRY_D->country(ip)+"].\n";
 aus += "Last logout was on "+ctime(idletime)+".\n";
 aus += "Mail: "+mail+"\n";
 alt = to_int(stat[11]) * 2;
 age = daytime(alt);
 aus += "Age: "+age+".\n";
 cap = lower_case(str);
 aus += AdditionalInformation(cap, 1, gender);
 if (on("mails") && n = ({int})MAILSERVER->FingerMail(cap)) 
       aus += "There are " + n + " new mails waiting for this player.\n";
 Write(aus);
 return 1;
}

string AdditionalInformation(string name, int flag, int gender) {
  mixed *userinfo;
  string *data, fname, ret;
  int i, wizlev;
  userinfo = ({mixed *})MASTER->find_userinfo(name);
  if (!userinfo) return "";
  wizlev = userinfo[2];
  data = userinfo[3];
  ret = "";
  if (flag) ret += "Wizard-level: "+wizlev+"\n";
  if(sizeof(data) && on("domains")) {
      ret += "Domains: ";
      for(i=0; i<sizeof(data); i++) {
        if(i)
          ret += ", ";
        switch(data[i][0]) {
          case '+': ret += "Vice ";
          case '*': ret += LORDSTR(gender)+" of "+capitalize(data[i][1..]);
               break;
          default: ret += "Member of "+capitalize(data[i]);
        }
      }
      ret += ".\n";
  }
  fname="/players/"+name+"/.project";
  if (file_size(name)>=0 && on("project"))
     ret += "Project: "+explode(read_file(name), "\n")[0]+"\n";
  fname="/players/"+name+"/.plan";
  if (file_size(fname)>=0 && on("project"))
     ret += "Plan:\n"+read_file(fname);
  else if (on("plan"))
     ret += "No plan.\n";
  return ret;
}
