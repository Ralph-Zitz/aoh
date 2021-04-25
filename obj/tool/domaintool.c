/*   Domain Tool
**
** Allows easy setting/removing of Domain Members/Vice/Lords.
** It must be trusted to do so.
**
**   v.0 20-Jun-92 [m]
**       22-Jun-92 [m] query_user_level() gone
**       24-Jun-92 [m] query_user_level() back
**       30-Jun-92 [m] Security Bug fixed
**       15-Jul-92 [m] Builtin prop Default-Domain "def_domain" added
**       29-Jul-92 [m] Now notifies with the /obj/domaininfo
**       31-Aug-92 [m] Adapted for new property system.
**       17-Nov-92 [m] Backdoor-wizarding disabled
**       22-Aug-93 [m] Vices may add domain members.
**       01-Oct-93 [m] args are now properly lower_cased.
**       09-Dec-93 [t] New dlist and new command "dbugs" and make the tool
**                     autoloading, saving the defaultdomain
**       15-Dec-93 [t] any changes to domaininfo will be logged
**       27-Jan-94 [t] new properties for the domaintool and the domain-
**                     tool is not autoloading, when you're not the owner
**                     and destructs itself when the environment is not a
**                     wizard
**       18-Jul-95 [c] Supports us ladies now...
**       27-Jan-96 [m] create() calls domaininfo->check_state() to cause
**                     a reload when necessary.
**       14-Feb-96 [m] Now handles projects as well.
**       28-Mar-96 [m] No automatic creation of wizard-dirs anymore.
**       11-feb-97 [s] QueryAC overwritten
**       15-Mar-98 [m] New level system.
**
** [m] Mateese, Biontic Magics & Darkness
** [t] Thragor
** [c] Charity
** [s] Softbyte
*/

#include <secure/userinfo.h>
#include <secure/wizlevels.h>
#include <properties.h>
#include <secure/domaininfo.h>

#define PL this_player()
#define ME this_object()
#define STR "/lib/string"
#define LOG "DOMAINTOOL"

#define VERSION "960328"
#define VERSNR  5

inherit "/std/thing";
inherit "/std/living";

private nosave string actdomain;        // the domain the tool defaults to
private nosave int    defaulted;        // Startup defaults set?

/*
* -----------------------------------------------------------------------------
* create()
* Sets the Ids, weight and value.
* -----------------------------------------------------------------------------
*/

void create ()
{
 thing::create();

 SetIds(({"domaintool","tool","dtool"}));
 SetNoDrop(1);
 SetNoGive(1);
 Set(P_NOBUY,"Cannot be bought");
 Set(P_NOSELL,"Cannot be sold");
 SetValue(0);
 SetWeight(0);
 SetReadMsg("\
A small sign reads:\n\
  Domaintool --- v."+VERSNR+"-"+VERSION+"\n\
  Biontic Magics & Darkness\n"
);

 SetInfo("Domaintool, v."+VERSNR+"-"+VERSION+", by Mateese");
 actdomain = 0;
 defaulted = 0;
 DOMAININFO->check_state();
}

/*--------------------------------------------------------------------------*/
static int IsProject (string dom)

/* Return TRUE if the <dom>ain is in fact a project.
 */

{
  return (-2 == file_size(PPATH+dom));
}

/*--------------------------------------------------------------------------*/
static int HasDir (string name)

/* Return TRUE if the <name> exists as /d/<name> or /p/<name>.
 */

{
  return (-2 == file_size(DPATH+name)) || (-2 == file_size(PPATH+name));
}

/*--------------------------------------------------------------------------*/
static string Describe (string dom)

/* Return "domain" if <dom> is a domain, or "project".
 */

{
  return (-2 == file_size(PPATH+dom)) ? "project" : "domain";
}

/*
* -----------------------------------------------------------------------------
* Set/QueryDefDomain()
* Handle the Default-Domain property "def_domain"
* -----------------------------------------------------------------------------
*/

string SetDefDomain(string val)
{
 if (({int})DOMAININFO->QueryDomain(val)==-1)
  return 0;
 defaulted = 1;
 return actdomain = lower_case(val);
}

/*
* string QueryDefaultDomain()
* This returns the domain where the player has the highest level,
* if no def-domain is set. The default-domain for wizards of no
* domain is set to the first Domain in QueryAllDomains().
*/

string QueryDefDomain()
{
 mixed data;
 int i,maxstat;
 string domain,*domains;

 if (defaulted==1&&actdomain&&actdomain!="") return actdomain;

 data = ({mixed})DOMAININFO->QueryWizardData(PL->QueryRealName());

i = pointerp(data)?sizeof(data[DW_NAME]):0;
 if (!i)
 {
  if (!domains = (({string *})DOMAININFO->QueryAllDomains()+({}))||!sizeof(domains))
  {
   write("Uh Oh ... no domains available!\n");
   return 0;
  }
  return domains[0];
 }

 maxstat = data[DW_DSTAT][0];
 domain = data[DW_NAME][0];

 while(i--)
  if (maxstat<data[DW_DSTAT][i])
  {
   maxstat = data[DW_DSTAT][i];
   domain = data[DW_NAME][i];
  }
 return domain;
}

/*
* -----------------------------------------------------------------------------
* string dtime(int time)
* This returns the time in the format: ddd hh:mm:ss
* (last login of wizard)
* -----------------------------------------------------------------------------
*/

private string dtime(int time)
{
 string ret;

 ret="";
 if(time>=86400)                // days
 {
  ret+=time/86400+"d ";
  time%=86400;
 }

 if(time<36000) ret+="0";       // hours
 ret+=time/3600+":";
 time%=3600;

 if(time<600) ret+="0";         // minutes
 ret+=time/60+":";
 time%=60;

 if(time<10) ret+="0";          // seconds
 ret+=time+"";

 return ret;
}

/*
* -----------------------------------------------------------------------------
* check_euid()
* Check the euid of this_player() against ours.
* If we have none, try to set it to this_players().
* If this fails, the player can't use us, our euid is reset and tried
* to be set to the players one.
* -----------------------------------------------------------------------------
*/

private varargs int check_euid (int silently, object pl)
{
 if (!pl) pl = PL;
 if (!pl || !living(pl)) return 0;
 if (!previous_object() || geteuid(previous_object()) != geteuid(ME))
 {
  if (!silently) write ("Sorry, this domaintool is not yours.\n");
  return 0;
 }

 if (!geteuid(ME) && !seteuid(geteuid(pl)))
 {
  if (!silently) write ("Sorry, this domaintool is not yours.\n");
  return 0;
 }

 if (!geteuid(ME) || geteuid(ME) != geteuid(pl))
 {
  seteuid(0);
  if (!seteuid(geteuid(pl)))
  {
   if (!silently) write ("You have no right to do that!\n");
   return 0;
  }
 }
 return 1;
}

/*
* -----------------------------------------------------------------------------
* check_status()
* returns 1 if the domaininfo is fully initialised otherwise 0
* -----------------------------------------------------------------------------
*/

private varargs int check_status()
{
 notify_fail(
"Database isn't loaded up to now. Please wait and try again later.\n\
Use 'dinfo' to check the actual state.\n");
 return ({int})DOMAININFO->QueryState()==DI_INITED;
}

/*
* -----------------------------------------------------------------------------
* check_dir()
* Checks if the calculated hasdir results in the same value as the flag set in
* the domaininfo. If not: Set the flag in the domaininfo new.
* -----------------------------------------------------------------------------
*/

private int check_dir(string domain)
{
 mixed *ddata;
 int hasdir;

 ddata = ({mixed *})DOMAININFO->QueryDomain(domain);

 hasdir = HasDir(domain);
 if (hasdir != ddata[DD_HASDIR])
 {

#ifdef LOG
  log_file(LOG,"\
Update of domaininfo via check_dir "+ctime()+"\n\
> domain: "+domain+"\n\
> DirFlag: "+to_string(ddata[DD_HASDIR])+" changed to "+to_string(hasdir)+"\n");
#endif

  ddata[DD_HASDIR] = hasdir;
  DOMAININFO->SetDomain(domain, ddata);
 }
 return hasdir;
}

/*
* -----------------------------------------------------------------------------
* _get_state()
* Decode the state-Values of the Domaininfo into strings.
* -----------------------------------------------------------------------------
*/

private string _get_state(int state,string name)
{
object ob;
int gender;
string filename;
string *data;
  ob=find_player(name);
  if(!ob){
    filename=SAVEPATH+name[0..0]+"/"+name+".o";
    data=explode(read_file(filename),"\n");
    gender=to_int((regexp(data,"^aGender")+({""}))[0][8..]);
  }else{
   gender=(({int})ob->QueryGender());
  }

 if (gender != 2){
 if (state == D_NOMEMBER) return "no member";
 if (state == D_MEMBER)   return "Member";
 if (state == D_VICE)     return "Vice Lord";
 if (state == D_LORD)     return "Lord";
 }else{
 if (state == D_NOMEMBER) return "no member";
 if (state == D_MEMBER)   return "Member";
 if (state == D_VICE)     return "Vice Lady";
 if (state == D_LORD)     return "Lady";
 }
 return "[code "+state+"]";
}

/*
* -----------------------------------------------------------------------------
* Set/QueryAutoObject()
* The tool is autoloading and saves the default-domain
* -----------------------------------------------------------------------------
*/

void SetAutoObject(mixed data)
{
 if (stringp(data))
 {
  actdomain = data;
  defaulted = (actdomain && actdomain!="");
 }
 else
 {
  actdomain = 0;
  defaulted = 0;
 }
}

string QueryAutoObject()
{
 if (!check_euid(1, environment(ME))) return 0;
 return actdomain||"";
}

/*
* -----------------------------------------------------------------------------
* QueryShort/Long()
* Make Long and Short description depending on the actual setting of
* euid and actdomain.
* -----------------------------------------------------------------------------
*/

varargs string QueryShort()
{
 string euid;

 check_euid(1, environment(ME));
 if(euid = geteuid(ME)) euid = capitalize(euid);

 if (!euid) return "a domaintool";
 return add_gen(euid)+" domaintool";
}

varargs string QueryLong()
{
 string euid, uid;
 string long;

 check_euid (1, environment(ME));
 if(uid = getuid(ME)) uid = capitalize(uid);
 if(euid = geteuid(ME)) euid = capitalize(euid);
 long =
"This tool allows adding/removing of domain members.\n\
It is set to domain '"+capitalize(QueryDefDomain())+"'.\n\
Use 'help dtool' for specific information.\n";
 if (euid != geteuid(PL))
  if (!euid)
   long += "But this instance must be trusted to to be useful,\n"+
           "since only "+uid+" can use it now fully.\n";
  else
   long += "This special instance may be used fully only by "+euid+".\n";
 return long;
}

/*
* -----------------------------------------------------------------------------
* fhelp()
* Command 'help'
* -----------------------------------------------------------------------------
*/

int fhelp (string arg)
{
 if (!check_euid(1,environment(ME))) return 1;
 if (!arg || !id(arg)) return 0;
 write (
"Domaintool v."+VERSNR+"-"+VERSION+" - Mateese, Biontic Magics & Darkness.\n\n\
This tool is for checking and manipulation of wizard's domain memberships.\n\
Projects are treated as domains in this context!\n\
The commands are:\n\
  ddef   : sets/shows the default domain of the tool.\n\
  dwhich : shows the domain membership of a wizard.\n\
  dlist  : lists all members of a domain; or gives a list of all domains.\n\
  dbugs  : lists all members with the size of their report and .err-files\n\
  dset   : sets/change the domain membership of a wizard.\n\
  drm    : removes a wizard from a domain.\n\
  dcr    : creates a new domain.\n\
  dext   : removes a domain (including all memberships).\n\
  dinfo  : gives information about the domaininfo-object\n\
All commands give specific info when given as '<cmd> ?', e.g. 'dset ?'.\n\
Additionally, the (e)uid of the tool must be set properly.\n\
The tool ids as 'tool', 'dtool' and 'domaintool'.\n\n");
 return 1;
}

/*
* -----------------------------------------------------------------------------
* fddef()
* Command 'ddef'
* -----------------------------------------------------------------------------
*/

int fddef (string arg)
{
 if (!check_euid(1,environment(ME))) return 1;
 if (arg == "?")
  return notify_fail(
"ddef\n\
  Shows default domain.\n\
ddef <name>\n\
  Sets default domain to <name> if /["+DDIR","+PDIR+"]/<name> exists.\n\
ddef reset, ddef me\n\
  Resets the default domain to your calculated default domain depending\n\
  on your highest level or if you're in no domain it's set to the first\n\
  domain.\n");

 if (!check_status()) return 0;

 if (arg)
  if (arg == "me"|| arg == "reset")
   actdomain = QueryDefDomain();
  else
   if (!actdomain=SetDefDomain(arg))
   {
    write ("No such domain '"+arg+"'.\n");
    actdomain = QueryDefDomain();
   }

 if (!actdomain)
  write("The tool defaults to no domain.\n");
 else
  write("The tool defaults to the "+capitalize(actdomain)+" "
       +Describe(actdomain)+".\n");
 return 1;
}

/*
* -----------------------------------------------------------------------------
* fdwhich()
* Command 'dwhich'
* -----------------------------------------------------------------------------
*/

int fdwhich(string arg)
{
 mixed *domain;
 int i;

 if (!check_euid(1,environment(ME))) return 1;
 if (!arg || arg == "?")
  return notify_fail(
"dwhich <name>\n\
  Shows domain(s) of wizard <name>.\n\
dwhich me\n\
  Shows your domain(s).\n");

 if (!check_status()) return 0;

 arg = lower_case(arg);
 if (arg == "me") domain = ({mixed *})DOMAININFO->QueryWizardData(geteuid(PL));
 else domain = ({mixed *})DOMAININFO->QueryWizardData(arg);
 if (!domain)
  return notify_fail("There's no data for wizard '"+arg+"'.\n");

 write(arg=="me"?"You are ":capitalize(arg)+" is ");
 if (!i = sizeof(domain[0]))
  write ("no member of any domain.\n");
 else
 {
  while(i--)
  {
   write (_get_state (domain[DW_DSTAT][i],arg)+" of "+
          capitalize(domain[DW_NAME][i]));
   if (i) write (", ");
  }
  write (".\n");
 }
 return 1;
}

/*
* -----------------------------------------------------------------------------
* fdlist()
* Command 'dlist'
* -----------------------------------------------------------------------------
*/

/*
* online(string wizard)
* filter to check if wizards of the domain are online
*/

int online(string wizard)
{
  object w;
  return (w = find_player(wizard))?interactive(w):0;
}

/*
* list_domains()
* is called when the parameter given to dlist is 'domains'
*/

private string *list_domains()
{
 string *output,*domains,dname;
 mixed *ddata;
 int i; //,hasdir ,online;

 domains = ({string *})DOMAININFO->QueryAllDomains();
 output = ({});
 if (!domains)
 {
  notify_fail("No domain information available.\n");
  return 0;
 }
 if (!sizeof(domains))
 {
  notify_fail("No domains registered with domaininfo (yet).\n");
  return 0;
 }
 for (i = 0; i < sizeof(domains); i++)
 {
  ddata = ({mixed *})DOMAININFO->QueryDomain(dname = domains[i])+({});
  //online = sizeof(filter(ddata[DD_WIZ][DW_NAME],#'online /*'*/));
  output+=({"  "+({string})STR->ladjust(capitalize(dname)+" ",20,".")+
            " "+({string})STR->radjust(""+sizeof(ddata[DD_WIZ][DW_NAME]),2," ")+
              " members"+
 //            " members (online "+STR->radjust(to_string(online),2," ")+
 //            ")  "+
            ((HasDir(dname)) ? "" : " (NO DIR)")
          });
  check_dir(dname);
 }
 output = sort_array(output,#'> /*'*/);
 i = sizeof(domains);
 return ({"Domains:"})+output
        +({"There "+(i==1?"is ":"are ")+to_string(i)+" domain"
           +(i==1?"":"s")+"."});
}

/*
* list_members()
* is called to return a list of the members of the specified domain
*/

private string *list_members(string domain)
{
 mixed *ddata,*wdata,*userinfo;
 string *output,*hlog,name,logd,filename,dir,str,path;
 object pl;
 int i,j, isp;
string logi;

 ddata = ({mixed *})DOMAININFO->QueryDomain(domain);

 if (!pointerp(ddata))
 {
  notify_fail("No "+capitalize(domain)+" domain!\n");
  return 0;
 }
 wdata = ddata[DD_WIZ];

 isp = IsProject(domain);
 path = isp ? PPATH : DPATH;

 output = ({});

 for (i=0;i < sizeof(wdata[DW_NAME]); i++)
 {
  name = wdata[DW_NAME][i];
  userinfo=({mixed *})MASTER->find_userinfo(name);
  filename=SAVEPATH+name[0..0]+"/"+name+".o";
  logi = 0;

  if(!userinfo || file_size(filename)<0)        // last logout
   logd = "- NOT EXISTING!!! -";
  else
  {
   j=file_date(filename);
   if (pl = find_player(name))
   {
    if (interactive(pl)) logd = "- currently on -";
    else logd = "- netdead -";
   }
   else
   {
    hlog = explode(ctime(j)," ");
     if (hlog[2]!="")
      logd = hlog[2]+lower_case(hlog[1])+hlog[4];
     else
      logd = hlog[3]+lower_case(hlog[1])+hlog[5];
     logi = " ("+dtime(time()-j)+")";
    }
   }

   filename = path+domain+"/"+name+"/*";
   if (!userinfo = get_dir(filename))           // dir-entries
    dir = "- NO DIR -";
   else
   {
    j = sizeof(userinfo)-2;
    if (!j)
     dir = "- no entry -";
    else
     dir = ({string})STR->radjust(to_string(j),3," ")+" entr"+(j-1?"ies":"y  ");
   }

  filename = (j=({int})DOMAININFO->QueryWizard(name,domain))?(j-1?" L":"VL"):" M";

  str = ({string})STR->ladjust(capitalize(name),11," ")+" ("+filename+")"+"|";    // output

  if (logi)     // if not logged in give the last login-time
   str += ({string})STR->radjust(logd,9," ")+" "+({string})STR->radjust(logi,16," ")+"|";
  else
   str += ({string})STR->cadjust(logd,26)+"|";
  if (dir[0]=='-')
   str += ({string})STR->cadjust(dir,20," ");
  else
   str+=({string})STR->radjust(dir,20," ");
  output += ({str});
 }
 output = sort_array(output,#'> /*'*/);
 output=({"The list of domainmembers","",
"Name            | Last logout              | Files in Domain-Dir",
"----------------+--------------------------+--------------------"})+output+
({"","There are "+sizeof(wdata[DW_NAME])+" members in this domain."});

 filename = path+domain;

 output+=({check_dir(domain)?
"A directory "+filename+" exists.":"There's no directory "+filename+".\n"});
 return output;
}

int fdlist (string domain)
{
 string *output;

 if (!check_euid(1,environment(ME))) return 1;

 if (domain=="?")
  return notify_fail("\
dlist <domain> lists the domain-members of a domain, their level: Member,\n\
  Vice Lord or Lady, Lord, or Lady, their last logout and the time from then \n\
  up to now and their files in their domain-dir (no recursive counting, it's \n\
  just to check if the wiz already has done something, or if his dir is \n\
  present).\n\
  If <domain> is not given, the default domain querying your highest wizlevel\n\
  in the domains will be calculated.\n\
dlist domains\n\
  Lists all available domains.");

 if (!check_status()) return 0;

 if (!domain) domain = QueryDefDomain();

 output = ({});

 if ((domain = lower_case(domain)) == "domains")
  output = list_domains();
 else
  output = list_members(domain);

 if (!output) return 0;

 STR->smore(output,PL->QueryPageSize());
 return 1;
}

/*
* -----------------------------------------------------------------------------
* fdbugs()
* Command 'dbugs
* -----------------------------------------------------------------------------
*/

int fdbugs(string domain)
{
 mixed *ddata, *wdata;
 int i, size, esize;
 string name,*lines;

 if (domain=="?"||domain=="help")
  return notify_fail("\
dbugs <domain> lists the bug-files and .err files of the given domain. If no\n\
domain is given, the default-domain is calculated depending on your highest\n\
level in the domains.\n");

 if (!check_status()) return 0;

 if (!domain) domain = QueryDefDomain();
 ddata = ({mixed *})DOMAININFO->QueryDomain(domain = lower_case(domain));

 if (!pointerp(ddata))
  return notify_fail("No "+domain+" domain!\n");

 i = sizeof((wdata = ddata[DD_WIZ])[DW_NAME]);

 lines = ({});

 while(i--)
 {
  size = file_size("/log/rep/"+(name = wdata[DW_NAME][i]));
  esize = file_size("/players/"+name+"/.err");
  if (size !=-1 || esize!=-1)
  {
   if (size == -1) size = 0;
   if (esize == -1) esize = 0;
   lines += ({({string})STR->ladjust(capitalize(name),11," ")+"|"+
              ({string})STR->radjust(" "+to_string(size)+" ",7," ")+"|"+
              ({string})STR->radjust(" "+to_string(esize)+" ",7," ")});
  }
 }
 if ((size = file_size("/d/"+domain+"/.err"))==-1) size = 0;
 lines = sort_array(lines,#'> /*'*/)+ ({"",capitalize(domain)+" error-log: "+
         ({string})STR->radjust(to_string(size),5," ")});
 if ((size = file_size("/log/rep/"+domain))==-1) size = 0;
 lines+=({capitalize(domain)+" rep-log:   "+
          ({string})STR->radjust(to_string(size),5," ")});
  if ((size = file_size("/log/rep/"+domain+".help"))==-1) size = 0;
  lines+=({capitalize(domain)+" help-log:  "+
           ({string})STR->radjust(to_string(size),5," ")});
 if (domain == "daydream")
 {
  if ((size = file_size("/log/daydream.h.err"))==-1) size = 0;
  lines += ({"Daydream.h errors:  "+({string})STR->radjust(to_string(size),5," ")});
 }

 lines = ({"Bug files of the domain-members:","",
            "Name       | reps  | errs",
            "-----------+-------+-------"})+lines;

 check_dir(domain);

 STR->smore(lines,({int})PL->QueryPageSize());
 return 1;
}

/*
* -----------------------------------------------------------------------------
* fdset()
* Command 'dset'
* -----------------------------------------------------------------------------
*/

int fdset (string arg)
{
  string domain, name, euid;
  int    ix, isme, rc;
  int    stat;  /* 0: member, 1: vice, 2: lord */
  mixed *dom;
  object ob;
  int gender;
  string filename;
  string *data;

 if (!check_euid(1,environment(ME))) return 1;
 if (!arg || arg == "?")
  return notify_fail(
"dset [[domain|project] <dname>] [member|vice|lord] <name>\n\
  Adds to domain <dname> (or the default domain) the wizard <name>\n\
  as member (default), vice or lord. Existing entries are overwritten.\n\
  If necessary, the directory /["+DDIR","+PDIR+"]/<dname>/<name> will be created.\n\
  The <name> 'me' defaults to yourself.\n\
  You must be in position to do that, of course.\n");

 if (!check_status()) return 0;
 if (!check_euid()) return 1;

 euid = geteuid(ME);
 arg = lower_case(arg);

 /* Parse arg */

 if      (sscanf (arg, "domain %s lord %s"  , domain, name) == 2) stat = 2;
 else if (sscanf (arg, "domain %s vice %s"  , domain, name) == 2) stat = 1;
 else if (sscanf (arg, "domain %s member %s", domain, name) == 2) stat = 0;
 else if (sscanf (arg, "domain %s %s"       , domain, name) == 2) stat = 0;
 else if (sscanf (arg, "project %s lord %s"  , domain, name) == 2) stat = 2;
 else if (sscanf (arg, "project %s vice %s"  , domain, name) == 2) stat = 1;
 else if (sscanf (arg, "project %s member %s", domain, name) == 2) stat = 0;
 else if (sscanf (arg, "project %s %s"       , domain, name) == 2) stat = 0;
 else if (sscanf (arg, "%s lord %s"         , domain, name) == 2) stat = 2;
 else if (sscanf (arg, "%s vice %s"         , domain, name) == 2) stat = 1;
 else if (sscanf (arg, "%s member %s"       , domain, name) == 2) stat = 0;
 else if (sscanf (arg, "lord %s", name) == 1) domain = actdomain, stat = 2;
 else if (sscanf (arg, "vice %s", name) == 1) domain = actdomain, stat = 1;
 else if (sscanf (arg, "member %s", name) == 1) domain =actdomain, stat = 0;
 else if (sscanf (arg, "%s %s"              , domain, name) == 2) stat = 0;
 else name = arg, domain = actdomain, stat = 0;
 if (name == "me") name = geteuid(PL), isme = 1;
 else isme = 0;

 /* Check validities */

 if (!domain) return notify_fail("No domain specified.\n");

 if (!pointerp(({mixed *})DOMAININFO->QueryDomain(domain)))
  return notify_fail("No such domain '"+arg+"'.\n");

 if (   (stat == 2 && euid != ROOTID
                   && ({int})MASTER->query_user_level(euid) < LVL_WIZARD)
     || (stat == 1 && euid != ROOTID && !({int})MASTER->domain_master(euid,domain)
                     && ({int})MASTER->query_user_level(euid) < LVL_WIZARD)
     || (stat == 0 && euid != ROOTID
                     && !(   ({int})MASTER->domain_master(euid,domain)
                          || ({int})MASTER->domain_vice(euid,domain))
                     && ({int})MASTER->query_user_level(euid) < LVL_WIZARD)
    )
   return notify_fail("You are not of high enough level to do that.\n");

 if (!({mixed *})MASTER->find_userinfo(name = lower_case(name))
     || ({int})MASTER->query_user_level(name) < LVL_WIZARD)
  return notify_fail("There's no wizard '"+name+"'.\n");

 /* Do it */

 dom = ({mixed *})DOMAININFO->QueryWizardData(name);
 if (dom && pointerp(dom) && sizeof(dom[0]) &&
     (ix = member(dom[0], domain)) != D_NOMEMBER
    )
 {
  ix = dom[1][ix];
  write((isme?"You were ":capitalize(name)+" was ")+
   _get_state(ix,name)+" of "+capitalize(domain)+".\n");
  if (ix == D_LORD)
   rc = ({int})MASTER->remove_domain_master (name, domain);
  else
   if (ix == D_VICE)
    rc = ({int})MASTER->remove_domain_vice (name, domain);
   else
    rc = ({int})MASTER->remove_domain_member (name, domain);
  if (!rc)
   return notify_fail("Funny, couldn't remove old entry. - Action aborted.\n");

#ifdef LOG
  log_file(LOG,"\
fdset: removing wizard entry "+ctime()+"\n\
> called by: "+geteuid(PL)+"\n\
> domain: "+domain+"\n\
> name: "+name+"\n");
#endif

  DOMAININFO->SetWizard(name, domain, D_NOMEMBER);
 }

 rc = 0;
 switch (stat)
 {
  case 0: rc = ({int})MASTER->add_domain_member (name, domain, IsProject(domain));
          if (rc) DOMAININFO->SetWizard (name, domain, D_MEMBER);

#ifdef LOG
if (rc)
  log_file(LOG,"\
fdset: new status of wizard "+ctime()+"\n\
> called by: "+geteuid(PL)+"\n\
> domain: "+domain+"\n\
> status: Member\n\
> name: "+name+"\n");
#endif
          break;
  case 1: rc = ({int})MASTER->add_domain_vice (name, domain, IsProject(domain));
          if (rc) DOMAININFO->SetWizard (name, domain, D_VICE);

#ifdef LOG
if (rc)
  log_file(LOG,"\
fdset: new status of wizard "+ctime()+"\n\
> called by: "+geteuid(PL)+"\n\
> domain: "+domain+"\n\
> status: Vice Lord/Lady\n\
> name: "+name+"\n");
#endif

          break;
  case 2: rc = ({int})MASTER->add_domain_master (name, domain, IsProject(domain));
          if (rc) DOMAININFO->SetWizard (name, domain, D_LORD);
#ifdef LOG
if (rc)
  log_file(LOG,"\
fdset: new status of wizard "+ctime()+"\n\
> called by: "+geteuid(PL)+"\n\
> domain: "+domain+"\n\
> status: Lord/Lady\n\
> name: "+name+"\n");
#endif
          break;
 }

 if (!rc) return notify_fail("Funny, couldn't perform your command.\n");
 else
 {
  write(isme?"You are ":capitalize(name)+" is ");
  write ("set as ");
  switch (stat)
  {
   case 0: write ("Member"); break;
   case 1: write ("Vice"); break;
   case 2:
  ob=find_living(name);
  if(!ob){
    filename=SAVEPATH+name[0..0]+"/"+name+".o";
    data=explode(read_file(filename),"\n");
    gender=to_int((regexp(data,"^aGender")+({""}))[0][8..]);
  }else{
   gender=(({int})ob->QueryGender());
  }
  if (gender != 2){
    write ("Lord");
  }else{
    write ("Lady");
  }
  break;
  }
  write (" of "+capitalize(domain)+".\n");

#if 0
  dirname = (IsProject(domain) ? PPATH : DPATH) + domain+"/"+name;
  if ((rc = file_size(dirname)) == -1)
   if (mkdir(dirname)) write ("Directory "+dirname+" created.\n");
   else write ("Creation of directory "+dirname+" failed.\n");
  else
   if (rc == -2)
    write ("Directory "+dirname+" already exists.\n");
   else write (dirname+" is a file.\n");
#endif

  check_dir(domain);
 }
 return 1;
}

/*-------------------------------------------------------------------------
** Command 'drm'
*/

int fdrm(string arg)
{
 string domain, dirname, name, euid;
 int    stat;  /* 0: member, 1:vice, 2:lord */
 int    ix = 0, isme, rc;
 mixed *dom, *ddata;

 if (!check_euid(1,environment(ME))) return 1;
 if (!arg || arg == "?")
  return notify_fail(
"drm [[domain|project] <dname>] <name>\n\
  Removes from domain <dname> (or the default domain) the wizard <name>\n\
  If necessary, the directory /["+DDIR","+PDIR+"]/<dname>/<name> will be removed.\n\
  The <name> 'me' defaults to yourself.\n\
  You must be in position to do that, of course.\n\
Syn.: dremove\n");

 if (!check_status()) return 0;

 if (!check_euid()) return 1;
 euid = geteuid(ME);
 arg = lower_case(arg);

 /* Parse arg */

 if      (sscanf (arg, "domain %s %s", domain, name) == 2);
 else if (sscanf (arg, "project %s %s", domain, name) == 2);
 else if (sscanf (arg, "%s %s"       , domain, name) == 2);
  else name = arg, domain = actdomain;
 if (name == "me") name = geteuid(PL), isme = 1;
 else isme = 0;

 /* Check validities */

 if (!domain) return notify_fail("No domain specified.\n");

 ddata = ({mixed *})DOMAININFO->QueryDomain (domain);
 if (!pointerp(ddata)) return notify_fail("No such domain '"+arg+"'.\n");

 if (!({mixed *})MASTER->find_userinfo(name = lower_case(name))
     || ({int})MASTER->query_user_level(name) < LVL_WIZARD)
  return notify_fail("There's no wizard '"+name+"'.\n");

 if (!dom = ({mixed *})DOMAININFO->QueryWizardData (name)
      || !pointerp(dom) || !sizeof(dom[0])) stat = D_NOMEMBER;
 else stat = member (dom[0], domain);
 stat = dom[1][stat];

 if (stat == D_NOMEMBER)
  return notify_fail((isme?"You are":capitalize(name)+" is")+
   " not involved in "+capitalize(domain)+"\n");

 if (   (stat == D_LORD && euid != ROOTID
                     && ({int})MASTER->query_user_level(euid) < LVL_WIZARD)
     || (stat == D_VICE && euid != ROOTID
                     && !({int})MASTER->domain_master(euid,domain)
                     && ({int})MASTER->query_user_level(euid) < LVL_WIZARD)
     || (stat == D_MEMBER && euid != ROOTID
                     && !({int})MASTER->domain_master(euid,domain)
                     && ({int})MASTER->query_user_level(euid) < LVL_WIZARD)
    )
  return notify_fail("You are not of high enough level to do that.\n");


  /* Do it */

 write((isme?"You were ":capitalize(name)+" was ")+
  capitalize(_get_state(ix,name))+" of "+capitalize(domain)+".\n");

 rc = 0;
 switch (stat)
 {
  case D_MEMBER: rc = ({int})MASTER->remove_domain_member (name, domain); break;
  case D_VICE  : rc = ({int})MASTER->remove_domain_vice   (name, domain); break;
  case D_LORD  : rc = ({int})MASTER->remove_domain_master (name, domain); break;
 }

#ifdef LOG
if (rc)
  log_file(LOG,"\
fdset: removing wizard entry "+ctime()+"\n\
> called by: "+geteuid(PL)+"\n\
> domain: "+domain+"\n\
> name: "+name+"\n");
#endif

 if (!rc) return notify_fail("Funny, couldn't perform your command.\n");
 else
 {
  DOMAININFO->SetWizard (name, domain, D_NOMEMBER);
  write ("Remove successful.\n");
  dirname = (IsProject(domain) ? PPATH : DPATH)+domain+"/"+name;
  if ((rc = file_size(dirname)) == -1)
   write ("No directory "+dirname+" needs to be removed.\n");
  else
   if (rc == -2)
    if (rmdir(dirname)) write ("Directory "+dirname+" removed.\n");
    else write ("Can't remove directory "+dirname+".\n");
   else write (dirname+" is a file.\n");

  check_dir(domain);
 }
 return 1;
}

/*-------------------------------------------------------------------------
** Command 'dcr'
*/

int fdcr (string arg)
{
 string dirname, euid, *args, tag;
 int rc, isproject;
 mixed *ddata;

 if (!check_euid(1,environment(ME))) return 1;
 if (!arg || arg == "?")
  return notify_fail(
"dcr <dname>\n"
"  Creates the domain <dname> and, if possible, its directory "+DPATH+"<dname>.\n"
"dcr project <pname>\n"
"  Creates the project <pname> and, if possible, its directory "+PPATH+"<pname>.\n"
"\n"
"  You must be in position to do that, of course.\n"
" Syn.: dcreate\n"
                         );

 if (!check_euid()) return 1;
 euid = geteuid(ME);
 if (euid != ROOTID && ({int})MASTER->query_user_level(euid) < LVL_WIZARD)
  return notify_fail("You are not of high enough level to do that.\n");

 args = explode(lower_case(arg), " ") - ({ "", 0 });
 if (args[0] != "project")
 {
   tag = "domain";
   isproject = 0;
   if (args[0] == "domain")
     args = args[1..];
 }
 else
 {
   tag = "project";
   isproject = 1;
   args= args[1..];
 }
 if (sizeof(args) != 1)
  return notify_fail("Illegal name '"+arg+"'.\n");
 arg = args[0];

 if (!check_status())
  return notify_fail(
   "Database isn't loaded up to now. Please wait and try again later.\n");

 ddata = ({mixed *})DOMAININFO->QueryDomain(arg);
 if (pointerp(ddata))
  write (capitalize(tag)+" '"+capitalize(arg)+"' already existing.\n");
 else
 {
  ddata = DD_EMPTY;
  write (capitalize(tag)+" '"+capitalize(arg)+"' created.\n");
 }
 dirname = (isproject ? PPATH : DPATH)+arg;
 if ((rc = file_size(dirname)) == -1)
  if (mkdir(dirname)) write ("Directory "+dirname+" created.\n");
  else write ("Creation of directory "+dirname+" failed.\n");
 else
  if (rc == -2)
   write ("Directory "+dirname+" already exists.\n");
  else write (dirname+" is a file.\n");
 ddata[DD_HASDIR] = (file_size(dirname) == -2);
 DOMAININFO->SetDomain(arg, ddata);
#ifdef LOG
  log_file(LOG,"\
fdcr: creation of new domain "+ctime()+"\n\
> called by: "+geteuid(PL)+"\n\
> domain: "+arg+"\n\
> data: "+(({string})"/lib/string"->mixed_to_string(ddata))+"\n");
#endif
 return 1;
}

/*-------------------------------------------------------------------------
** Command 'dext'
*/

int fdext (string arg)
{
 string dirname, euid, wname;
 int rc, i;
 mixed *ddata, *wdata;

 if (!check_euid(1,environment(ME))) return 1;
 if (!arg || arg == "?")
  return notify_fail(
"dext <dname>\n\
  Removes the domain <dname> and, if possible, its directory /["+DDIR","+PDIR+"]/<dname>.\n\
  You must be in position to do that, of course.\n");

 if (!check_euid()) return 1;
 euid = geteuid(ME);
 if (euid != ROOTID && ({int})MASTER->query_user_level(euid) < LVL_WIZARD)
  return notify_fail("You are not of high enough level to do that.\n");

 if (sizeof(explode(arg, " ")) != 1)
  return notify_fail("Illegal name '"+arg+"'.\n");

 if (!check_status())
  return notify_fail(
   "Database isn't loaded up to now. Please wait and try again later.\n");

 arg = lower_case(arg);

 ddata = ({mixed *})DOMAININFO->QueryDomain(arg)+({});
 if (!pointerp(ddata))
  write ("There's no domain '"+capitalize(arg)+"'.\n");
 else
 {
  dirname = (IsProject(arg) ? PPATH : DPATH) + arg;
  wdata = ddata[DD_WIZ];
  for (i = 0; i < sizeof(wdata[DW_NAME]); i++)
  {
   wname = wdata[DW_NAME][i];
   switch (wdata[DW_DSTAT][i])
   {
    case D_MEMBER: MASTER->remove_domain_member (wname, arg); break;
    case D_VICE  : MASTER->remove_domain_vice   (wname, arg); break;
    case D_LORD  : MASTER->remove_domain_master (wname, arg); break;
   }
   DOMAININFO->SetWizard(wname, arg, D_NOMEMBER);
#ifdef LOG
  log_file(LOG,"\
fdext: removing wizard entry "+ctime()+"\n\
> called by: "+geteuid(PL)+"\n\
> domain: "+arg+"\n\
> name: "+wname+"\n");
#endif
   rmdir (dirname+"/"+wname);
  }
  DOMAININFO->RemoveDomain (arg);
#ifdef LOG
  log_file(LOG,"\
fdext: removing domain entry "+ctime()+"\n\
> called by: "+geteuid(PL)+"\n\
> domain: "+arg+"\n");
#endif
  write (capitalize(Describe(arg))+" '"+capitalize(arg)
         +"' ("+i+" members) removed.\n");
 }

 if ((rc = file_size(dirname)) == -1)
  write ("No directory "+dirname+" needs to be removed.\n");
 else if (rc == -2)
  if (rmdir(dirname)) write ("Directories "+dirname+"[/*] removed.\n");
   else write ("Can't remove directories "+dirname+"[/*].\n");
 else write (dirname+" is a file.\n");
 return 1;
}

/*-------------------------------------------------------------------------
** Command 'dinfo'
*/

int fdinfo (string arg)
{
 string s;

 if (!check_euid(1,environment(ME))) return 1;
 if (arg == "?" || (arg && lower_case(arg) != "reload"))
  return notify_fail(
"dinfo [reload]\n\
  Either displays the status of the domaininfo, or tells it to reload\n\
  it's database.\n");

 if (arg && lower_case(arg) == "reload")
 {
  DOMAININFO->ReloadData();
  write ("Ok, the domaininfo reloads its database.\n");
 }
 else
 {
  write (" -- Domaininfo v."+({string})DOMAININFO->QueryVersion()+" --\n");
  write ("Database ");
  switch (({int})DOMAININFO->QueryState())
  {
   case DI_UNINITED: write ("not loaded"); break;
   case DI_INITING : write ("loading"); break;
   case DI_INITED  : write ("loaded"); break;
   default         : write ("unknown"); break;
  }
  s = ctime(({int})DOMAININFO->QueryLoadTime());
  s = s[8..9]+"-"+s[4..6]+"-"+s[20..23]+" "+s[11..18];
  write (" on "+s+"\n");
  write ("Loaded: "+({int})DOMAININFO->QueryDCount()+" Domains, ");
  write (""+({int})DOMAININFO->QueryWCount()+" Wizards.\n");
  s = ctime(({int})DOMAININFO->QueryChangeTime());
  s = s[8..9]+"-"+s[4..6]+"-"+s[20..23]+"  "+s[11..18];
  write ("Last change: "+s+"\n");
 }
 return 1;
}

/*--------------------------------------------------------------------------*/
varargs int QueryAC() {return 0;}
/*--------------------------------------------------------------------------*/

/*
* -----------------------------------------------------------------------------
* init()
* Control our environment and add commands
* -----------------------------------------------------------------------------
*/

void init()
{
 object env;
 thing::init();
 if (query_once_interactive(env = environment(this_object())))
  if (!IS_IMMORTAL(env))
   return ({void})this_object()->remove();
 if (!IS_IMMORTAL(this_player())) return;

 if (!defaulted && check_euid(1,environment(ME)))
 {
  actdomain = QueryDefDomain();
  defaulted = 1;
 }

 add_action ("fhelp"   , "help");
 add_action ("fddef"   , "ddef");
 add_action ("fdwhich" , "dwhich");
 add_action ("fdlist"  , "dlist");
 add_action ("fdset"   , "dset");
 add_action ("fdrm"    , "drm");
 add_action ("fdrm"    , "dremove");
 add_action ("fdcr"    , "dcr");
 add_action ("fdcr"    , "dcreate");
 add_action ("fdext"   , "dext");
 add_action ("fdinfo"  , "dinfo");
 add_action ("fdbugs"  , "dbugs");
}

/*************************************************************************/
