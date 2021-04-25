/* News-Demon
**
** This is the News-Demon, administrating all News.
** It is based upon the News-Demon of MorgenGrauen (done by Jof), which
** I hotted it up to improve it's (already high) quality.
**
**   06-Jul-92 [j] The version I (m) used as basis.
**   05-Aug-92 [m] First works.
**   06-Aug-92 [m] Basic functionality done.
**   10-Aug-92 [m] Added GetNames()
**   11-Aug-92 [m] Added G_CNTMSG, G_LASTID, GetInterna()
**   12-Aug-92 [m] _delayed_save(): this could make the G_LASTID and G_ACTMSG
**                 info in grouplist inconsistent, but lessens the io-load.
**   17-Aug-92 [m] Illegal patterns to regexp() notified.
**   20-Aug-92 [m] Delayed save was nonfunctional
**   29-Aug-92 [m] StoreNote() now returns the set ID.
**   12-Sep-92 [m] Reusage of existing savefiles on group creation.
**   08-Dec-92 [m] M_LEVEL can now be an array.
**   22-Dec-92 [m] note logging fixed.
**   25-Jul-93 [p] /secure/login got rights to check the boards for initial
**                 message of unread notes.
**   13-Jan-94 [m] Group creators are set as owners by level.
**   15-Feb-94 [m] SEERhood removed.
**   14-May-94 [p] Added object call into CheckRights()
**   25-Jun-94 [m] AddRights(), RemoveRights() recognize levels < 0
**                 as 'unspecified'
**   13-May-95 [m] M_FLAGS added.
**   02-Jul-96 [m] RemoveNote() changed so that players can't remove other
**                 players' notes.
**   29-Jun-97 [m] StoreNote() can now be called by daemons as well.
**   23-Dec-97 [s] IPL==0,stringp check in CheckRights
**   21-Mar-98 [m] New level system.
**
**  [j] Jof, God of MorgenGrauen (chomsky.uni-muenster.de  128.176.121.10 4711)
**  [m] Mateese, Biontic Magics & Darkness
**  [p] Pumuckel
**  [s] Softbyte
**--------------------------------------------------------------------------
*/

#include <secure/wizlevels.h>
#include <news.h>

/*--------------------------------------------------------------------------*/

#define VERSION "1.16"

#define IPL this_interactive() /*this_player(1)*/
#define PO previous_object()
#define TO this_object()


  /* Convert a given groupname into its savefilename */
#define SAVEFILE(name) implode(explode(name,"."),"/")
#define LOGFILE(name) (implode(explode(name,"."),"/")+".log")

/*--------------------------------------------------------------------------*/

  /* This is the only non-static var and used for transfer of data
  ** from/to the savefiles.
  */
mapping nsaveload;

/* The internal lists */
nosave mapping ngroups;
nosave mapping ngrouplist;

nosave int last_flush; /* time() of the last cache-clear */
nosave int last_mid;   /* Our make_id()-Counter */
nosave int save_sched; /* Flag if a _delayed_save is under schedule */

void save_grouplist ();
varargs void save_group (string name, mixed *msgs);

/*--------------------------------------------------------------------------*/

void create() {
  if (sizeof(explode(object_name(this_object()),"#")) != 1)
    destruct(this_object());
  seteuid(getuid(this_object()));
  if (  file_size(NEWSPATH+"GroupList.o") > 0
     && restore_object(NEWSPATH+"GroupList")
     )
     ngrouplist = nsaveload;
  if (!ngroups || !mappingp(ngroups))
    ngroups = ([]);
  if (!ngrouplist || !mappingp(ngrouplist))
    ngrouplist = ([]);
  if (!member(ngrouplist, TRASH))
    ngrouplist[TRASH] = ({ TRASH, ({40}), ({1}), ({40}), ({40}), ({100}),
                          -40, M_NOLOG, 0, 0, "" });
  if (!member(ngroups, TRASH)) {
    ngroups[TRASH] = ({ ({}), 0 });
    save_group(TRASH, ({}));
  }
  if (file_size("/log/"+NEWSPATH) != -2) mkdir ("/log/"+NEWSPATH);

  call_out("_clear_groups",NEWS_TIME_TO_CLEAR);
}

int remove() {
  save_grouplist(); /* Grouplist can be inconsitent, but needn't to */
  destruct (this_object());
  return 1;
}

int query_prevent_shadow() { return 1; }

/*-------------------------------------------------------------------------
** string stime (int date)
**
** Like cdate() this converts a numeric date into a string, but in a more
** european manner: '15-Jun-1992 20:00:00'
*/

string stime (int date) {
  string cdate;
  cdate = ctime(date);
  return cdate[8..9]+"-"+cdate[4..6]+"-"+cdate[20..23]+" "+cdate[11..18];
}

/*--------------------------------------------------------------------------
** int make_id()
**
** Return a unique int for identification. It is guaranteed that this
** the int is not used again this age.
*/

int make_id() {
  if (time() > last_mid) return last_mid = time();
  return ++last_mid;
}

/*--------------------------------------------------------------------------
** void _clear_groups()
**
**  Check all groups in 'groups' if they had been touched often enough.
** If not, swap them out.
*/

void _clear_groups() {
  last_flush = time();
  ngroups = filter(ngroups, (: ngroups[$1][GD_TOUCHES] >= NEWS_MIN_TOUCHED :));
  ngroups = map(ngroups, (: 0 :));
  call_out("_clear_groups",NEWS_TIME_TO_CLEAR);
}

/*--------------------------------------------------------------------------
** void save_group (string name, void|mixed *msgs)
**
**  Write the messages of group <name> into it's savefile.
**  If <msgs> is given, it is take as the messages, else the msgs
**  are retrieved from groups.
*/

varargs void save_group (string name, mixed *msgs) {
  if (!msgs)
    msgs = ngroups[name][GD_MSGS];
  nsaveload = ngroups;
  save_object(NEWSPATH+SAVEFILE(name));
}

/*--------------------------------------------------------------------------
** void _delayed_save()
**
** Schedule a save_grouplist() to happen after NEWS_DSAVE_TIME minutes.
** This way the grouplist is not saved on every change of G_LASTID or
** G_MAXMSG. If the master crashes during the interval, the named entries
** will be invalid for not-yet-cached groups after reload. But since this
** info is used unchecked only by GetStatus() this inconsistency can be
** ignored.
*/

static void _delayed_save () {
  if (save_sched) return;
  save_sched = 1;
  call_out ("save_grouplist", NEWS_DSAVE_TIME);
}

/*--------------------------------------------------------------------------
** void save_grouplist()
**
**  Save the grouplist, remove any scheduled _delayed_saves
*/

void save_grouplist () {
  nsaveload = ngrouplist;
  save_object (NEWSPATH+"GroupList");
  if (save_sched) remove_call_out ("save_grouplist");
  save_sched = 0;
}

/*--------------------------------------------------------------------------
** int load_group (string name)
**
**  If necessary, load the messages of group <name> and insert them into
**  groups. Return the index where to find them in groups, else -1.
*/

static int load_group (string name) {
  int cnt;
  mixed *dsc;

  if (member(ngroups, name) != 0) {
    ngroups[name][GD_TOUCHES]++;
    return 1;
  }
  if (!member(ngrouplist, name))
    return -1;
  if (!restore_object(NEWSPATH+SAVEFILE(name)))
    return -1;
  ngroups[name] = nsaveload[name];
  cnt = sizeof(nsaveload[name][GD_MSGS]);
  dsc = ngrouplist[name];
  // TODO: Remove this compat code
  // Adapt the stored levels to the new level system.
  if (pointerp(dsc[G_OWNER]) && sizeof(dsc[G_OWNER]) && dsc[G_OWNER][0] > LVL_WIZARD)
    dsc[G_OWNER][0] = LVL_WIZARD;
  if (pointerp(dsc[G_READ]) && sizeof(dsc[G_READ]) && dsc[G_READ][0] > LVL_WIZARD)
    dsc[G_READ][0] = LVL_WIZARD;
  if (pointerp(dsc[G_WRITE]) && sizeof(dsc[G_WRITE]) && dsc[G_WRITE][0] > LVL_WIZARD)
    dsc[G_WRITE][0] = LVL_WIZARD;
  if (pointerp(dsc[G_DELETE]) && sizeof(dsc[G_DELETE]) && dsc[G_DELETE][0] > LVL_WIZARD)
    dsc[G_DELETE][0] = LVL_WIZARD;
  if (pointerp(dsc[G_DELETE]) && sizeof(dsc[G_DELETE]) && dsc[G_DELETE][0] > LVL_WIZARD)
    dsc[G_DELETE][0] = LVL_WIZARD;
  dsc[G_ACTMSG] = cnt;
  dsc[G_LASTID] = (cnt ? nsaveload[name][GD_MSGS][cnt-1][M_ID] : "");
  ngrouplist[name] = dsc;
  return 1;
}

/*--------------------------------------------------------------------------
** void _log_note (mixed *dsc, mixed *msg, int mode)
**
** Logs <msg> according to the definitions in the group descriptor <dsc>.
** <mode> is either M_LO_NEW or M_LOG_EXP, depending on the mind of the
** caller.
*/

void _log_note (mixed *dsc, mixed *msg, int mode) {
  object logger;
  string name;

  if (dsc[G_LOGMODE] == M_NOLOG || dsc[G_LOGMODE] != mode) return;
  logger = 0;
  if ((name = dsc[G_LOGGER]) && name != "" && !(logger = find_object(name))) {
    catch(call_other(name, "???"));
    logger = find_object(name);
  }
  if (logger) logger->log_note (msg, mode);
  else
    write_file ("/log/"+NEWSPATH+LOGFILE(dsc[G_NAME]),
"----------------------------------------------------------------------------\n"
+((mode==M_LOG_NEW)?"New ":"Expired ")+"ID: "+msg[M_ID]+"\n"
+"Groups: "+implode(msg[M_GROUPS],", ")+"\n"
+"Author: "+msg[M_WRITER]+" ["
  +(pointerp(msg[M_LEVEL]) ? (msg[M_LEVEL][0]+"/"+msg[M_LEVEL][1])
                           : msg[M_LEVEL]
   )
  +"]\t\t\t\t"+stime(msg[M_TIME])+"\n\n"
+" -- "+msg[M_TITLE]+" --\n"
+msg[M_TEXT]
               );
}

/*--------------------------------------------------------------------------
** void _trash_note (mixed *msg)
**
** Add note <msg> to the TRASH-group.
*/

void _trash_note (mixed *msg) {
  int max;
  mixed *msgs, *data;

    /* Make sure we get a real copy of msg */
  msg = msg + ({});

  if (load_group (TRASH) == -1) return; /* shouldn't happen */
  msgs = (data = ngroups[TRASH])[GD_MSGS];
  max = (max = ngrouplist[TRASH][G_MAXMSG]) > 0 ? max : -max;
  if (!max) return;
  if (sizeof(msgs) >= max) msgs = msgs[-max+1..-1];
  data[GD_MSGS] = msgs + ({ msg });
  ngroups[TRASH] = data;
  save_group (TRASH, data[GD_MSGS]);
  return;
}

/*--------------------------------------------------------------------------
** Check this_interactive() against some given rights and return result.
*/

int CheckRights (mixed *rights) {
  int i, len;
  object master, ob;
  if (IPL && (IS_WIZARD(IPL) || sizeof(explode(object_name(IPL),"secure/login")) > 1))
    return 1;
  if (!rights || !(master = find_object("/secure/master"))) return 0;
  if (IS_WIZARD(IPL) || rights[0] <= query_user_level(IPL))
    return 1;
  for (i = sizeof(rights)-1; i; i--) {
    if (!rights[i] || !stringp(rights[i]) ||
       !(len = sizeof(rights[i]))) continue;
    switch (rights[i][0]) {
    case  '$' : if (({int})master->domain_member(geteuid(IPL), rights[i][1..len]))
                  return 1;
    case  '+' : if (({int})master->domain_vice(geteuid(IPL), rights[i][1..len]))
                  return 1;
    case  '*' : if (({int})master->domain_master(geteuid(IPL), rights[i][1..len]))
                  return 1;
                break;
    case  '@' :
      ob = 0;
      if (rights[i][1..len] != "" && !(ob = find_object(rights[i][1..len])))
      {
  	    catch(call_other(rights[i][1..len], "???"));
  	    ob = find_object(rights[i][1..len]);
      }
      if (!ob)
        log_file (NEWSPATH+LOGFILE("MASTER"),
 "------------------------------------------------------------------------\n"
+ctime()+" CheckRights(): Failed to load file '"+ rights[i][1..len]+"'\n"
                 );
      else if (({int})ob->CheckRights(
         IPL?geteuid(IPL):(PO?geteuid(PO):(TO?geteuid(TO):0)) ))
        return 1;
      break;
    default   : if (geteuid(IPL) == rights[i]) return 1;
                break;
    }
  }
  return 0;
}

/*--------------------------------------------------------------------------
** int SetGroup (mixed *dsc)
**
** Set the description for the appropriate group <dsc>.
** Only owner and wizards may do this.
*/

int SetGroup (mixed *dsc) {
  if (!dsc || !pointerp(dsc) || sizeof(dsc) != G_SIZE)
    return NEWS_ILLPARM;
  if (!member(ngrouplist, dsc[G_NAME]))
    return NEWS_ILLGROUP;
  if (dsc[G_NAME] == TRASH) {
    if (!IS_WIZARD(IPL)) return NEWS_NORIGHT;
    dsc[G_OWNER] = ({ LVL_WIZARD });
    dsc[G_READ] = ({ LVL_ANCIENT });
    dsc[G_WRITE] = dsc[G_DELETE] = ({ LVL_WIZARD });
    dsc[G_SUBGROUP] = ({ LVL_WIZARD });
    dsc[G_LOGMODE] = M_NOLOG;
  }
//  else if (!CheckRights (ngrouplist[dsc[G_NAME]][GL_DSC][G_OWNER]))
  else if (!CheckRights (ngrouplist[dsc[G_NAME]][G_OWNER]))
    return NEWS_NORIGHT;
//  dsc[G_ACTMSG] = ngrouplist[dsc[G_NAME]][GL_DSC][G_ACTMSG];
//  dsc[G_LASTID] = ngrouplist[dsc[G_NAME]][GL_DSC][G_LASTID];
  dsc[G_ACTMSG] = ngrouplist[dsc[G_NAME]][G_ACTMSG];
  dsc[G_LASTID] = ngrouplist[dsc[G_NAME]][G_LASTID];
  ngrouplist[dsc[G_NAME]] = dsc;
  save_grouplist();
  return NEWS_OK;
}

/*--------------------------------------------------------------------------
** mixed *GetGroup (string name)
**
** Return the description of group <name>, else the approbiate errorcode.
*/

mixed GetGroup (string name) {
  if (!name) return NEWS_ILLPARM;
  if (!member(ngrouplist, name))
    return NEWS_ILLGROUP;
  return ngrouplist[name];
}

/*--------------------------------------------------------------------------
** int SetRights (string name, int which, mixed *data)
**
** Set the rights <which> (G_OWNER et al) of group <name> to <data>.
** Only owner and arches may do this.
** <data> is considered being valid and properly built.
*/

int SetRights (string name, int which, mixed *data) {
  mixed *dsc;

  if (   !name || !pointerp(data) || !intp(data[0])
      || member(({ G_OWNER, G_READ, G_WRITE, G_DELETE, G_SUBGROUP })
               , which
               ) == -1
     ) return NEWS_ILLPARM;
  if (!member(ngrouplist, name))
    return NEWS_ILLGROUP;
  dsc = ngrouplist[name];
  if (name == TRASH || !CheckRights (dsc[G_OWNER]))
    return NEWS_NORIGHT;
  dsc[which] = data;
  ngrouplist[name] = dsc;
  save_grouplist();
  return NEWS_OK;
}

/*--------------------------------------------------------------------------
** mixed GetRights (string name, int which)
**
** Return the rights <which> (G_OWNER et al) of group <name>, else
** the approbiate errorcode.
*/

mixed GetRights (string name, int which) {
  mixed *dsc;

  if (   !name
      || member(({ G_OWNER, G_READ, G_WRITE, G_DELETE, G_SUBGROUP })
               , which
               ) == -1
     ) return NEWS_ILLPARM;
  if (!member(ngrouplist, name))
    return NEWS_ILLGROUP;
  dsc = ngrouplist[name];
  return dsc[which];
}

/*--------------------------------------------------------------------------
** int AddRights (string name, int which, mixed data)
**
** Adds the contents of <data> to the rights <which> (G_OWNER et al)
** of group <name>.
** <data> may be an int, a string or an array of these. The ints are
** considered as new level-criteria and set. Strings are added if not
** already existing. So it's possible to have all three domain-criterias
** in one right, but this is useful.
** Only owner and arches may do this.
*/

int AddRights (string name, int which, mixed data) {
  int i, level;
  mixed rights, dsc;

  if (   !name
      || member(({ G_OWNER, G_READ, G_WRITE, G_DELETE, G_SUBGROUP })
               , which
               ) == -1
     ) return NEWS_ILLPARM;
  if (!member(ngrouplist, name))
    return NEWS_ILLGROUP;
  dsc = ngrouplist[name];
  if (name == TRASH || !CheckRights (dsc[G_OWNER]))
    return NEWS_NORIGHT;
  rights = dsc[which];
  if (rights[0] >= 0)
    level = rights[0];
  rights = rights[1..sizeof(rights)];
  if (!pointerp(data)) data = ({ data });
  for (i = 0; i < sizeof(data); i++) {
    if (intp(data[i])) { level = data[i]; continue; }
    if (!data[i] || !sizeof(data[i])) continue;
    if (member(rights, data[i]) == -1) rights += ({ data[i] });
  }
  dsc[which] = ({ level }) + rights;
  ngrouplist[name] = dsc;
  save_grouplist();
  return NEWS_OK;
}

/*--------------------------------------------------------------------------
** int RemoveRights (string name, int which, mixed data)
**
** Removes the contents of <data> from the rights <which> (G_OWNER et al)
** of group <name>.
** <data> may be an int, a string or an array of these. The ints are
** considered as new level-criteria and set. Strings are removed if
** already existing.
** Only owner and arches may do this.
*/

int RemoveRights (string name, int which, mixed data) {
  int i, level;
  mixed *rights, dsc;

  if (   !name
      || member(({ G_OWNER, G_READ, G_WRITE, G_DELETE, G_SUBGROUP })
               , which
               ) == -1
     ) return NEWS_ILLPARM;
  if (!member(ngrouplist, name))
    return NEWS_ILLGROUP;
  dsc = ngrouplist[name];
  if (name == TRASH || !CheckRights (dsc[G_OWNER]))
    return NEWS_NORIGHT;
  rights = dsc[which];
  if (rights[0] >= 0)
    level = rights[0];
  rights = rights[1..sizeof(rights)];
  if (!pointerp(data)) data = ({ data });
  for (i = 0; i < sizeof(data); i++) {
    if (intp(data[i])) { level = data[i]; continue; }
    if (!data[i] || !sizeof(data[i])) continue;
    if (member(rights, data[i]) != -1) rights -= ({ data[i] });
  }
  dsc[which] = ({ level }) + rights;
  ngrouplist[name] = dsc;
  save_grouplist();
  return NEWS_OK;
}

/*--------------------------------------------------------------------------
** int AddGroup (string name, void|mixed *owner, void|mixed *newdsc)
**
** Add a new Group <name> and set its <owner> if specified.
** If possible, inherit rights from parent-group and add this player
** as owner if it's not arch.
** Else set the rights as following:
**   Owner    : level LVL_WIZARD, the creating player, the owner
**   Read     : all
**   Write    : all wizards
**   Delete   : all wizards
**   SubGroup : level LVL_WIZARD
**   30 Messages, no saving of extraneous notes.
** If <newdsc> is specified, a call to SetGroup (*newdsc) is done
** after successful creation of the group.
** A new group may be added by Wizard, or any Owner/SubGroup of
** the lowest parentgroup.
*/

varargs int AddGroup (string name, mixed *owner, mixed *newdsc) {
  mixed *dsc;
  string savefile, parent;
  string *savefilea;
  int ix, i, savef;

  if (!name) return NEWS_ILLPARM;
  name = lower_case(name);
  if (newdsc && name != newdsc[G_NAME]) return NEWS_ILLPARM;
  if (name == TRASH || member(ngrouplist, name) != 0)
    return NEWS_ILLGROUP;

  savefilea = explode(name,".");
  savefile = implode(savefilea,"/");
  if (file_size(NEWSPATH+savefile+".o") >= 0) savef = 1;

    /* Find parentgroup */
  for (ix = sizeof(savefilea)-1; ix; ix--)
    if (member(ngrouplist, implode(savefilea[0..ix-1], "."))) {
      parent = implode(savefilea[0..ix-1], ".");
      break;
    }
  if (!ix) {
    if (!IS_WIZARD(IPL)) return -1; /* Not allowed */
    dsc = allocate(G_SIZE);
    dsc[G_OWNER]    = ({ LVL_WIZARD });
    dsc[G_READ]     = ({ 0 });
    dsc[G_WRITE]    = ({ LVL_ANCIENT });
    dsc[G_DELETE]   = ({ LVL_ANCIENT });
    dsc[G_SUBGROUP] = ({ LVL_WIZARD });
    dsc[G_MAXMSG]   = 30;
  }
  else {
    /* W/o this addition we would get just a pointer and thus
    ** smash the parent entry!!!
    */
    if (ngrouplist[parent][G_NAME] == TRASH)
      return NEWS_ILLPARM;
    dsc = ({}) + ngrouplist[parent];
    if (!CheckRights (dsc[G_OWNER]) && !CheckRights(dsc[G_SUBGROUP]))
      return -1;
  }
  dsc[G_NAME] = name;
  dsc[G_ACTMSG] = 0;
  dsc[G_LASTID] = "";

    /* Make the necessary dirs */
  for (i = ix-1; i < sizeof(savefilea)-1; i++) {
    mkdir(NEWSPATH+implode(savefilea[0..i],"/"));
    mkdir("log/"+NEWSPATH+implode(savefilea[0..i],"/"));
  }

  ngrouplist[name] = dsc;
  ngroups[name] = ({ ({}), 1 });

  if (savef && (catch (i=load_group(name)) || i < 0)) savef = 0;
  save_group (name);
  if (!IS_WIZARD(IPL) && !CheckRights(dsc[G_OWNER]))
    dsc[G_OWNER] += ({ geteuid(IPL) });
  if (owner)
    AddRights (name, G_OWNER, owner);
  if (newdsc)
    SetGroup (newdsc);
  else
    save_grouplist();
  return savef ? NEWS_ILLFILE : NEWS_OK;
}

/*--------------------------------------------------------------------------
** int RemoveGroup (string name)
**
** Remove group <name> and its files and dirs as far as possible.
** A group may be removed by an arch, any owner or any owner/subgroup
** of the lowest parentgroup.
*/

int RemoveGroup (string name) {
  int i, ix;
  string *gnames, savefile, parent;
  mixed dsc, *pdsc;

  if (!name) return NEWS_ILLPARM;
  if (name == TRASH) return NEWS_NORIGHT;
  if (!member(ngrouplist, name))
    return NEWS_ILLGROUP;
  dsc = ngrouplist[name];
  gnames = explode (name,".");

    /* Find parentgroup */
  for (ix = sizeof(gnames)-1; ix; ix--)
    if (member(ngrouplist, implode(gnames[0..ix-1], "."))) {
      parent = implode(gnames[0..ix-1], ".");
      break;
    }
    /* Check rights */
  if (!CheckRights (dsc[G_OWNER])) {
    if (!ix) return -1;
    pdsc = ngrouplist[parent];
    if (!CheckRights (pdsc[G_OWNER]) && !CheckRights(pdsc[G_SUBGROUP]))
      return -1;
  }

  savefile = SAVEFILE(name);

    /* Remove files */
  if (file_size (NEWSPATH+savefile+".o") >= 0)
    rm (NEWSPATH+savefile+".o");
  if (file_size ("/log/"+NEWSPATH+savefile) >= 0)
    rm (NEWSPATH+savefile);

    /* Remove directories */
  for (i = sizeof(gnames)-2; i >= ix-1 && i >= 0; i--) {
   rmdir(NEWSPATH+implode(gnames[0..i], "/"));
   rmdir("/log/"+NEWSPATH+implode(gnames[0..i], "/"));
  }

  m_delete(ngrouplist, name);
  m_delete(ngroups, name);
  save_grouplist();
  return NEWS_OK;
}

/*--------------------------------------------------------------------------
** mixed StoreNote (string title, string text, mixed dest, [object ipl])
**
** Stores <text> under <title> into the approbiate group <dest>.
** <dest> may be one string or an array of strings, then the message
** will be crossposted.
** This can be done by arches, owner and 'writers'.
** If <ipl> is given, it is used the the 'storer' of the note, else
** the interactive user.
**
** Special returns:
**   an array of the used M_ID and the groupnames which failed, each
**   with reason.
*/

varargs mixed StoreNote (string title, string text, mixed dest, object ipl) {
  int i, j;
  string *rejected;
  mixed *dsc, *msgs, *msg, *data;

  if (!ipl) ipl = IPL;
  if (!title || !text || !dest) return NEWS_ILLPARM;
  if (!pointerp(dest)) dest = ({ dest });

  msg = allocate (M_SIZE);
  msg[M_TITLE] = title;
  msg[M_TEXT] = text;
  msg[M_GROUPS] = dest;
  msg[M_WRITER] = capitalize(geteuid(ipl));
  if (!(msg[M_LEVEL] = query_user_level(ipl)))
    msg[M_LEVEL] = ({ ({int})ipl->QueryLevel()
                    , ({int})RACEMASTER->XPtoLevel(({int})ipl->QueryXP())
                   });
  msg[M_ID] = MUDNAME ":"+make_id();
  msg[M_TIME] = time();
  msg[M_FLAGS] = 0;

  rejected = ({ });
  for (i = 0; i < sizeof(dest); i++) {
    if (load_group (dest[i]) == -1) {
      rejected += ({ "No such group: "+dest[i] });
      continue;
    }
    dsc = ngrouplist[dest[i]];
    if (!CheckRights (dsc[G_OWNER]) && !CheckRights(dsc[G_WRITE])) {
      rejected += ({ "No permission: "+dest[i] });
      continue;
    }
    _log_note (dsc, msg, M_LOG_NEW);
    msgs = (data = ngroups[dest[i]])[GD_MSGS];
    if (dsc[G_MAXMSG] < 0)
    {
      j = 0;
      while (j < sizeof(msgs) && sizeof(msgs) >= -dsc[G_MAXMSG]) {
        if (msgs[j][M_FLAGS] && -1 != member(msgs[j][M_FLAGS], MF_LOCKED))
        {
          j++;
        }
        else
        {
          _log_note (dsc, msgs[j], M_LOG_EXP);
          _trash_note(msgs[j]);
          if (j)
            msgs = msgs[0..j-1]+msgs[j+1..];
          else
            msgs = msgs[1..sizeof(msgs)];
        }
      }
      if (sizeof(msgs) >= -dsc[G_MAXMSG])
      {
        rejected += ({ "Group full: "+dest[i] });
        continue;
      }
    }
    if (dsc[G_MAXMSG] > 0 && sizeof(msgs) >= dsc[G_MAXMSG]) {
      rejected += ({ "Group full: "+dest[i] });
      continue;
    }

    data[GD_MSGS] += ({ msg });
    ngroups[dest[i]] = data;
    save_group (dest[i], data[GD_MSGS]);
    dsc[G_ACTMSG] = sizeof(msgs)+1;
    dsc[G_LASTID] = msg[M_ID];
    ngrouplist[dest[i]] = dsc;
    save_grouplist();
  }
  if (sizeof(dest) && sizeof(rejected) != sizeof(dest))
    _delayed_save();
  if (sizeof(rejected)) return ({ msg[M_ID] }) + rejected;
  return ({ msg[M_ID] });
}

/*--------------------------------------------------------------------------
** int RemoveNote (string name, string|int nr)
**
** Removes the <nr>th message from group <name>.
** If <nr> is a string, the message with ID [MUDNAME':']<nr> will be removed.
** This can be done by arches, owner and the writer;
** 'deleters' may remove it if their wiz_level is higher than the writers'.
*/

int RemoveNote (string name, mixed nr) {
  int i, j;
  string lastid;
  mixed *dsc, *data, *msgs;

  if (!name) return NEWS_ILLPARM;
  if (load_group(name) == -1) return NEWS_ILLGROUP;
  msgs = (data = ngroups[name])[GD_MSGS];
  if (intp(nr)) {
    if (nr <= 0) return NEWS_ILLPARM;
    if (nr > sizeof(msgs)) return NEWS_ILLNOTE;
  }
  else if (stringp(nr)) {
    if (sizeof(explode(nr,":")) != 2) nr = MUDNAME ":"+nr;
    for (i = 0; i < sizeof(msgs); i++)
      if (nr == msgs[i][M_ID]) {
        nr = i+1;
        break;
      }
    if (stringp(nr)) return NEWS_ILLNOTE;
  }
  else return NEWS_ILLPARM;

  i = (-1 != member(msgs[nr-1][M_FLAGS]||({}), MF_LOCKED));
  dsc = ngrouplist[name];
  j = msgs[nr-1][M_WRITER] != capitalize(geteuid(IPL));
  if (   !CheckRights (dsc[G_OWNER])
      && j
      && i || (   (!CheckRights (dsc[G_DELETE]) && j)
               && ( !intp(msgs[nr-1][M_LEVEL]) || IS_WIZARD(IPL))
	      )
     ) return NEWS_NORIGHT;
  _trash_note (msgs[nr-1]);
  data[GD_MSGS] = (msgs = msgs[0..nr-2]+msgs[nr..sizeof(msgs)]);
  ngroups[name] = data;
  save_group (name, msgs);
  lastid = (sizeof(msgs) ? msgs[sizeof(msgs)-1][M_ID] : "");
  dsc[G_ACTMSG] = sizeof(msgs);
  dsc[G_LASTID] = lastid;
  ngrouplist[name] = dsc;
  _delayed_save();
  return NEWS_OK;
}

/*--------------------------------------------------------------------------
** int AddFlag (string name, string|int nr, string flag)
**
** Add <flag> to M_FLAGS of message <nr> in group <name>.
** If <nr> is a string, the message with ID [MUDNAME':']<nr> will be removed.
*/

int AddFlag (string name, mixed nr, string flag) {
  int i;
  mixed *dsc, *data, *msgs;

  if (!name) return NEWS_ILLPARM;
  if (load_group(name) == -1) return NEWS_ILLGROUP;
  msgs = (data = ngroups[data])[GD_MSGS];
  if (intp(nr)) {
    if (nr <= 0) return NEWS_ILLPARM;
    if (nr > sizeof(msgs)) return NEWS_ILLNOTE;
  }
  else if (stringp(nr)) {
    if (sizeof(explode(nr,":")) != 2) nr = MUDNAME ":"+nr;
    for (i = 0; i < sizeof(msgs); i++)
      if (nr == msgs[i][M_ID]) {
        nr = i+1;
        break;
      }
    if (stringp(nr)) return NEWS_ILLNOTE;
  }
  else return NEWS_ILLPARM;

  switch(flag)
  {
    case MF_LOCKED:
      dsc = ngrouplist[name];
      if (!CheckRights (dsc[G_OWNER]))
        return NEWS_NORIGHT;
      if (-1 == member(msgs[nr-1][M_FLAGS]||({}), MF_LOCKED))
        msgs[nr-1][M_FLAGS] = (msgs[nr-1][M_FLAGS]||({})) + ({ MF_LOCKED });
      break;
    default:
      return NEWS_ILLPARM;
  }
  save_group(name);
  return NEWS_OK;
}

/*--------------------------------------------------------------------------
** int RemoveFlag (string name, string|int nr, string flag)
**
** Remove <flag> from M_FLAGS of message <nr> in group <name>.
** If <nr> is a string, the message with ID [MUDNAME':']<nr> will be removed.
*/

int RemoveFlag (string name, mixed nr, string flag) {
  int i;
  mixed *dsc, *msgs;

  if (!name) return NEWS_ILLPARM;
  if (load_group(name) == -1) return NEWS_ILLGROUP;
  msgs = ngroups[name][GD_MSGS];
  if (intp(nr)) {
    if (nr <= 0) return NEWS_ILLPARM;
    if (nr > sizeof(msgs)) return NEWS_ILLNOTE;
  }
  else if (stringp(nr)) {
    if (sizeof(explode(nr,":")) != 2) nr = MUDNAME ":"+nr;
    for (i = 0; i < sizeof(msgs); i++)
      if (nr == msgs[i][M_ID]) {
        nr = i+1;
        break;
      }
    if (stringp(nr)) return NEWS_ILLNOTE;
  }
  else return NEWS_ILLPARM;

  switch(flag)
  {
  case MF_LOCKED:
    dsc = ngrouplist[name];
    if (!CheckRights (dsc[G_OWNER]))
      return NEWS_NORIGHT;
    msgs[nr-1][M_FLAGS] -= ({ MF_LOCKED });
    break;
  default:
    return NEWS_ILLPARM;
  }
  save_group(name);
  return NEWS_OK;
}

/*--------------------------------------------------------------------------
** mixed GetHeaders (string name, void|int nr)
**
** Retrieves the header of the <nr>th message from group <name>,
** or of all if <nr> <= 0.
** This can be done by arches, owner and 'readers'.
*/

mixed GetHeaders (string name, int nr) {
  int i;
  mixed *dsc, *rc, *msgs;

  if (!name) return NEWS_ILLPARM;
  if (load_group (name) == -1) return NEWS_ILLGROUP;
  msgs = ngroups[name][GD_MSGS];
  dsc = ngrouplist[name];
  if (!CheckRights (dsc[G_OWNER]) && !CheckRights (dsc[G_READ]))
    return NEWS_NORIGHT;
  if (nr > sizeof (msgs)) return NEWS_ILLNOTE;
  if (nr > 0) return msgs[nr-1][0..M_TEXT-1];
  rc = ({});
  for (i = 0; i < sizeof(msgs); i++)
    rc += ({ msgs[i][0..M_TEXT-1] });
  return rc;
}

/*--------------------------------------------------------------------------
** mixed GetNote (string name, void|int|string nr)
**
** Retrieves the <nr>th message from group <name>, or all if <nr> <= 0.
** If <nr> is a string, the note with ID [MUDNAME':']<nr> is retrieved.
** This can be done by arches, owner and 'readers'.
*/

mixed GetNote (string name, mixed nr) {
  int i;
  mixed *dsc, *msgs;

  if (!name) return NEWS_ILLPARM;
  if (load_group (name) == -1) return NEWS_ILLGROUP;
  msgs = ngroups[name][GD_MSGS];
  if (intp(nr)) {
    if (nr > sizeof(msgs)) return NEWS_ILLNOTE;
  }
  else if (stringp(nr)) {
    if (sizeof(explode(nr,":")) != 2) nr = MUDNAME ":"+nr;
    for (i = 0; i < sizeof(msgs); i++)
      if (nr == msgs[i][M_ID]) {
        nr = i+1;
        break;
      }
    if (stringp(nr)) return NEWS_ILLNOTE;
  }
  else return NEWS_ILLPARM;

  dsc = ngrouplist[name];
  if (!CheckRights (dsc[G_OWNER]) && !CheckRights (dsc[G_READ]))
    return NEWS_NORIGHT;
  if (nr > 0) return msgs[nr-1];
  return msgs;
}

/*--------------------------------------------------------------------------
** mixed GetStatus (void|string name)
**
** Returns an array off all group descriptors (or just of group <name>),
** each expanded by two additional entries 'int usage'.
**   usage < 0 : Group not loaded.
**         >= 0: Group loaded and <usage> times accessed since last clear.
*/

mapping GetNGroups() { return ngroups; }
mapping GetNGroupList() { return ngrouplist; }

varargs mixed GetStatus (string name) {
  mixed *rc;
  int i;

  if (name) {
    if (!member(ngrouplist, name))
      return NEWS_ILLPARM;
    rc = ngrouplist[name];
    rc = ({ rc });
  }
  else rc = map(m_indices(ngrouplist), (: ({ $1 }) :));
  for (i = 0; i < sizeof(rc); i++) {
    if (member(ngroups, rc[i][G_NAME])) {
      rc[i] += ({ ngroups[rc[i][G_NAME]][GD_TOUCHES] });
    }
    else rc[i] += ({ -1 });
  }
  return rc;
}

/*----------------------------------------------------------------------
** int string_compare (str1, str2)
**
** Return if str1 is lexicographically smaller than str2.
*/

int string_compare (string str1, string str2) {
  return str1 >= str2;
}

/*--------------------------------------------------------------------------
** string *GetNames (void|string *pattern)
**
** Returns an array of groupnames: either of all groups, or of the
** groups which name matches one of the regular patterns in <pattern>.
** <pattern> may be a single pattern or an array.
*/

varargs string *GetNames (mixed pattern) {
  string *rc, *match, *common;
  int i;

  if (!pattern) rc = m_indices(ngrouplist) + ({});
  else {
    rc = ({});
    if (!pointerp(pattern)) pattern = ({ pattern });
    for (i = 0; i < sizeof(pattern); i++) {
      match = regexp (m_indices(ngrouplist), pattern[i]);
      if (!match)  {
        write ("Faulty pattern '"+pattern[i]+"'.\n");
        match = ({});
      }
      common = rc & match;
      rc = (rc - common) + match;
    }
  }
  if (sizeof(rc)) return sort_array (rc, "string_compare");
  return rc;
}

/*--------------------------------------------------------------------------
** mixed *GetInterna()
**
** Returns an array of information about the demon.
*/

mixed *GetInterna() {
  mixed *rc;
  rc = allocate(NEWS_ISIZE);
  rc[NEWS_IVERSION]   = VERSION;
  rc[NEWS_ICNTGRPS]   = sizeof(ngrouplist);
  rc[NEWS_ICNTCACHE]  = sizeof(ngroups);
  rc[NEWS_ITIMEINT]   = NEWS_TIME_TO_CLEAR;
  rc[NEWS_IMINTOUCH]  = NEWS_MIN_TOUCHED;
  rc[NEWS_ILASTFLUSH] = last_flush;
  rc[NEWS_IDSAVEINT]  = NEWS_DSAVE_TIME;
  rc[NEWS_IDSAVEPEND] = find_call_out ("save_grouplist");
  return rc;
}

/****************************************************************************/
