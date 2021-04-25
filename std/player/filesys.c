/* Filesys-Commands
**
** Implement basic programmer commands for handling files.
** Fileexpansion and currentdir are also handled here.
**
** The source is roughly divided into two parts.
** At the beginning are all the funs interfacing to the player.
** At the end all the funs doing the real work, since these are long and
** also meant for use from external objects.
** These latter ones also do all the checking and name-expanding.
** In detail there are:
**
**  exp_fname(): Expands file-aliases in a filename.
**  get_path() : Normalises an filename absolute.
**  is_file()  : Checks if a filename descripts a valid file.
**  is_dir()   : Checks if a filename descripts a valid dir.
**
**  type(): Types a file completely, only the head, or only the tail.
**  more(): Mores a file.
**  ls()  : Lists a directory or the size of a file.
**  rm()  : Removes a file.
**  cp()  : Copies, appends or moves a file.
**  cd()  : Changes the current directory.
**
**  asynstop(): Aborts any asynchronous operation (ls, type or copy).
**
** This module implements four properties:
**   CURRENTDIR : the actual current directory.
**   FALIASES   : a mapping of defined file-aliases.
**   PAGESIZE   : the no of lines/page for more().
**   (FACTIVE)   : the flag if any async operation is executing.
**
** The pagesize and the faliases are also saved.
**
**   31-Jul-92 [m] First implementation.
**   02-Aug-92 [m] Independent of /obj/lib now.
**   05-Aug-92 [m] Small bug in fed() fixed.
**   07-Aug-92 [m] faliases now reorder on startup. (before mapping change)
**   12-Sep-92 [m] lls is now synchronous and uses features of Amylaar's GD
**   03-Oct-92 [m] SECURE2() for rm and cp/join/mv.
**   05-Oct-92 [m] SECURE2() broke the asynchronous stuff - SECURE3() used.
**   13-Oct-92 [m] SEERs are allowed to use man.
**   14-Oct-92 [m] secure copy/move implemented.
**   19-Oct-92 [p] TI -> this_interactive()
**   21-Oct-92 [m] copy commands now need two arguments
**   31-Jan-92 [m] Learner now have full write access.
**   30-Aug-93 [m] cd() now uses WIZARDDIR instead of "players"
**   21-Sep-93 [m] 'pagesize' is now a normal player-command.
**   23-Sep-93 [m] more() doesn't print the filename for players.
**   30-Nov-93 [p] More docs, fman() updated.
**   06-Dec-93 [h] changed include statements for @242
**   15-Feb-94 [m] SEERhood removed.
**   28-Feb-94 [m] Improved security to check all previous_objects().
**   26-Jun-94 [m] Added special path '@' to cd.
**   22-Dec-94 [m] Fixed small bug in man command.
**                 Added regexp search to more().
**   04-Jan-95 [m] 'man' now lists multiple entries.
**   19-Jan-95 [m] 'mv' now creates destination directories if necessary.
**   11-May-95 [m] 'cd' displays a '.readme' file in target dir if existant.
**   02-Jan-96 [m] 'man' modified for property docs.
**   20-Jul-96 [m] Invisible wizards more'ing a file aren't announced to
**   03-Feb-97 [m] Same with ed.
**   15-Jan-98 [s] Call to new more command an flexible file selection
**
**  [m] Mateese
**  [p] Pepel
**  [h] Hyp
**  [s] Softbyte
*/

#include <secure/wizlevels.h>
#include <search.h>
#include <properties.h>

/*-------------------------------------------------------------------------*/

#define PL       this_player()
#define PL_NAME  ({string})this_player()->QueryName()
#define PL_RNAME ({string})this_player()->QueryRealName()
#define ME       this_object()
#define TI	 this_interactive()

#define USAGE(cond,txt) if(cond) { notify_fail (txt); return 0; }
#define SECURE(val) \
  if (!PL || PL != ME) return (val);
#define SECURE3(val) \
  if (!PL || PL != ME \
      || !check_previous()) return (val);
#define SECURE2(val) \
  if (!PL || PL != ME || TI != ME \
      || !check_previous()) return (val);

  /* for parse_fname() */
#define NAME  0
#define PATH  1
#define BASE  2
#define SIZE  3
#define PSIZE 4
#define BSIZE 5

  /* sizes of parse_fname() */
#define FILE_NA   -1
#define FILE_DIR  -2
#define FILE_WILD -3

public varargs mixed Query(string str); // std/base

/*-------------------------------------------------------------------------*/

  /* Prototypes of our locally used funs */

varargs int more (string fname, int quiet, int chunk);
int cp (int cmd, string src, string dst);
varargs int type (int cmd, string fname, int chunk);
int rm (string fname);
int ls (int cmd, string fname);
string cd (string newpath);
void set_falias (string alias, string exp);
string get_falias (string alias);
string exp_falias (string alias);
void rm_falias (string alias);
string exp_fname (string fname);
varargs string is_file (mixed thing, string suffix);
string is_dir (string name);
varargs string get_path (string fname, int noaliases, mixed usr);
int copyfile(string src, string dst);
void asynstop();
static void skip_to_end_of_more();
static void _do_cp_part();
static void _do_rm_part();
static void _do_ls_part();
static void even_more(string str);

/*-------------------------------------------------------------------------*/

nosave string currentdir;  /* Our currentdir */
int pagesize;              /* Lines per page - autoloading */
mapping faliases;          /* Our file-aliases */

nosave mixed *srcd;        /* Source-Description */
nosave mixed *sfile;       /* List of Sourcefile-Basenames */
nosave mixed *dstd;        /* Destination-Description */
nosave int factive;        /* TRUE if sth's still (allowed to) going on */
nosave int adone;          /* Generic counter */
nosave int atodo;          /* ..mumble.. */
nosave int acmd;           /* Command code */
nosave string asynfun;     /* Name of the asynchronous done fun */

/*-------------------------------------------------------------------------
** Setup
*/

void create () {
  faliases = ([]);
  factive = 0;
  pagesize = 16;
  asynfun = 0;
}

/*-------------------------------------------------------------------------
** Handle our properties CURRENTDIR, PAGESIZE, FALIASES.
** Additionally, the activity-state (FACTIVE) may be frobbed for emergency.
*/

string SetCurrentDir (string path) { return currentdir = path; }
string QueryCurrentDir ()          { return currentdir; }

int SetFActive (int state) { return factive = state; }
int QueryFActive () { return factive; }

int SetPageSize (int size) { return pagesize = size; }
int QueryPageSize () { return pagesize; }

mapping SetFAliases (mapping aliases) {
  faliases = ([]);
  if (aliases && mappingp(aliases) && sizeof(aliases))
    faliases = aliases;
  return faliases;
}
mapping QueryFAliases () { return faliases; }

/*-------------------------------------------------------------------------
** Check the stack of previous objects to allow just direct calls from
** the player.
** Return 0 on failure.
*/

protected int check_previous() {
  int i;
  object o;
  for (i = 0; o = previous_object(i); i++)
    if (o != ME)
      return 0;
  return 1;
}

/*-------------------------------------------------------------------------
**  Stringformatting
*/

string mkstr (int len) {
  string rc, pattern;
  int actlen;

  if (len <= 0) return "";
  pattern = "          ";
  rc = pattern;
  actlen = sizeof(pattern);
  while (actlen < len) rc += rc, actlen *= 2;
  return rc[0..len-1];
}

string ladjust (string str, int len) {
  int actlen;

  if (!str || len <= 0) return 0;
  if ((actlen = sizeof(str)) >= len) return str;
  return str+mkstr(len-actlen);
}

string radjust (string str, int len) {
  int actlen;

  if (!str || len <= 0) return 0;
  if ((actlen = sizeof(str)) >= len) return str;
  return mkstr(len-actlen)+str;
}


/*-------------------------------------------------------------------------
** Add all commands we're implementing.
*/

void add_filesys_commands() {
  SECURE(0);
  if (IS_WIZARD(ME)) {
    add_action ("fmore", "more");
    add_action ("ftype", "type");
    add_action ("ftype", "cat");
    add_action ("ftype", "tail");
    add_action ("fls", "ls");
    add_action ("fls", "lls");
    add_action ("fls", "fchk");
    add_action ("fpwd", "pwd");
    add_action ("fcd", "cd");
    add_action ("falias", "falias");
    add_action ("faction", "faction");
  }
  if (IS_WIZARD(ME)) {
    add_action ("fcp", "cp");
    add_action ("fcp", "join");
    add_action ("fcp", "mv");
    add_action ("fcp", "scp");
    add_action ("fcp", "smv");
    add_action ("frm", "rm");
    add_action ("fed", "ed");
    add_action ("fmkdir", "mkdir");
    add_action ("frmdir", "rmdir");
  }

    /* cd to home directory, if existing */
  if (IS_WIZARD(ME))
    cd(0);
}

/*-------------------------------------------------------------------------
** NOT YET IMPLEMENTED.
*/

#ifdef EXECFILE
int _exec_file(string cmd, string str)
{ int err;

  if (file_size(cmd+".c") > 0) {

/* BIG kludge: we call a dummy function in the file to test */
/* if it is loadable. catch(ret = call_other...) kills the driver */
    err = catch(call_other(cmd,"dummy",str));
    if (err) {
      write("Error in executing "+cmd+":\n");
      write(err);
      return ok();
    }
    call_other(cmd,"execute",str);
    return ok();
  }
  return 0;
}

int exec_file(string cmd, string str)
{ string cmdpath;
  string *paths;
  int i;


  if (cmd[0]=='/' && _exec_file(cmd,str)) return 1;
  paths = ({ "/bin" });
  if (!paths) return 0;

  for (i=0;i<sizeof(paths); i++) {
    cmdpath = get_path(paths[i]+"/"+cmd);
    if (_exec_file(cmdpath,str)) return 1;
  }
  return 0;
}
#endif

/*-------------------------------------------------------------------------
** Command 'more'
*/

int fmore (string str) {
  int flag;
  int i;
  string fname;
  string *strs;

  USAGE (!str || str == "?", "more <file> [nohtml]\n"
     "Shows <file>. If file is not found it is tried to complete"
     " the filename reasonably.\n");

  flag=0;
  if (1==sscanf(str,"%s nohtml",fname))
  {
    str=fname;
    flag|=4;
  }

  if (Query(P_INVIS)) flag|=1;

  // str = get_path( str, 0, this_player() );
  str = get_path(str);
  fname=str;

  if (file_size(fname) <0) fname=str+".c";
  if (file_size(fname) <0) fname=str+".h";
  if (file_size(fname) <0) fname=str+".o";
  if (file_size(fname) <0)
  {
    strs=explode(str,"/");
    fname=implode(strs[0..<2],"/");
    str=strs[<1];
    strs=get_dir(fname+"/*");
    for (i=0;i<sizeof(strs);i++)
    {
      if (strstr(strs[i],str)==0)
      {
        if (file_size(fname+"/"+strs[i])>=0)
        {
          fname=fname+"/"+strs[i];
          break;
        }
      }
    }

    if (file_size(fname) <0)
    {
      notify_fail("File not found.\n");
      return 0;
    }
  }
  "/lib/more"->more(fname, flag, pagesize);
  return 1;
}

/*-------------------------------------------------------------------------
** Command 'type'/'cat'/'tail'
*/

int ftype (string str) {
  string verb;
  int cmd;  /* 0: Type, 1: Cat, 2: Tail */

  verb = query_verb();
  cmd = 0;
  if (verb == "cat") cmd = 1;
  if (verb == "tail") cmd = 2;

  if (!str || str == "?") {
    if (cmd == 0) notify_fail ("type <file>\n  Types the <file>.\n");
    if (cmd == 1) notify_fail ("cat <file>\n  Types the head of <file>.\n");
    if (cmd == 2) notify_fail ("tail <file>\n  Types the tail of <file>.\n");
    return 0;
  }
  type (cmd, str, pagesize);
  return 1;
}

/*-------------------------------------------------------------------------
** Command 'cp'/''//'join'/'mv'/''
**
**  Codes: 0: cp, 1: join, 2: move
*/

int fcp (string str) {
  int cmd;
  string verb, src, dst;

  SECURE2(0);
  verb = query_verb();
  cmd = member(({ "cp", "join", "mv", 0, "scp", 0, "smv"}), verb);

  if (str)
    str = implode(explode(str, " ") - ({ "" }), " ");

  if (!str || str == "?") {
    if (cmd == 0) str = "cp <src> <dst>\n  Copies";
    if (cmd == 1) str = "join <src> <dst>\n  Appends";
    if (cmd == 2) str = "mv <src> <dst>\n  Moves";
    if (cmd == 4) str = "scp <src> <dst>\n  Copies";
    if (cmd == 6) str = "smv <src> <dst>\n  Moves";
    str +=" the <src>file to <dst> (or the cwd).\n";
    if (cmd == 4 || cmd == 6)
      str += "  Any existing <dst> won't be overwritten.\n";
    else if (cmd != 1)
      str +="  Any existing <dst> will be overwritten.\n";
    str += "  <dst> may just designate the dest directory.\n"+
           "  The <src> may be wildcarded '*'.\n";
    notify_fail (str);
    return 0;
  }

  if (sscanf (str, "%s %s", src, dst) != 2) {
    notify_fail("No destination specified.\n");
    return 0;
  }
  cp (cmd, src, dst);
  return 1;
}

/*-------------------------------------------------------------------------
** Command 'rm'
*/

int frm (string str) {
  SECURE2(0);
  USAGE (!str || str == "?",
         "rm <file>\n  Removes the <file>(s).\n  Wildcards '*' are allowed.\n"
        );

  if (str)
    str = implode(explode(str, " ") - ({ "" }), " ");

  rm (str);
  return 1;
}

/*-------------------------------------------------------------------------
** Command 'ls'/'lls'/'fchk'
**   Codes:  0: ls, 1: lls, 2: fchk
*/

int fls (string str) {
  int cmd;
  string name;

  name = query_verb();
  cmd = 0;
  if (name == "lls") cmd = 1;
  if (name == "fchk") cmd = 2;

  if (str == "?") {
    if (cmd == 0)
      notify_fail (
             "ls [<fdir>]\n"
            +"  Lists info about the file/dir <fdir> (or the cwd).\n"
            +"  Directories are marked with a trailing '/'.\n"
            +"  Wildcards '*' are allowed.\n"
                  );
    if (cmd == 1)
      notify_fail (
             "lls [<fdir>]\n"
            +"  Lists a bit more about <fdir> (or the cwd).\n"
            +"  Wildcards '*' are allowed.\n"
                  );
    if (cmd == 2)
      notify_fail (
             "fchk [<fdir>]\n"
            +"  Prints size/type of file/dir <fdir> (or the cwd).\n"
            +"  Wildcarding is possible, chk works like lls then.\n"
                  );
    return 0;
  }

  if (str)
    str = implode(explode(str, " ") - ({ "" }), " ");

  ls (cmd, str);
  return 1;
}

/*-------------------------------------------------------------------------
** Command 'falias'
*/

int falias (string arg) {
  string alias, exp;

  USAGE (arg == "?",
 "falias <alias> <exp>\n"
+"  Sets the expansion for <alias> to <exp> after expanding <exp> itself\n"
+"  with all other file-aliases.\n"
+"falias <abbr>\n"
+"  Removes the file-alias <alias>.\n"
+"falias\n"
+"  Lists all defined file-aliases.\n"
        );

  if (arg && sscanf (arg, "%s %s", alias, exp) == 2) {
    if (sizeof(explode(alias, "/")) != 1) {
      write ("Illegal alias '"+alias+"'.\n");
      return 1;
    }
    if (sizeof(explode(exp, " ")) != 1) {
      write ("Illegal expansion '"+exp+"'.\n");
      return 1;
    }
    if (get_falias(alias)) rm_falias (alias);
    exp = exp_fname (exp);
    set_falias (alias, exp);
    write ("'"+alias+"' set to '"+exp+"'.\n");
  }
  else if (arg)
    if (!get_falias(arg)) write ("No such file-alias.\n");
    else {
      rm_falias(arg);
      write ("'"+arg+"' removed.\n");
    }
  else {
    write("There are "+ sizeof(faliases) + " aliases.\n");
    walk_mapping(faliases,
      (:
        write("  '"+ladjust($1+"'", 15)+ " = '"+$2+"'\n");
        return 0;
      :),
      faliases
    );
  }
  return 1;
}

/*-------------------------------------------------------------------------
** Command 'faction'
*/

int faction (string str) {
  string name;

  USAGE (str && str != "stop",
         "faction [stop]\n  Shows or stops any asynchronous operation.\n"
        );

  if (!str)
    if (!factive) write ("There is no command executing.\n");
    else {
      write ("There is a command in execution.\n");
      if (asynfun && sscanf (asynfun, "_do_%s_part", name) == 1)
        write ("It seems to a '"+name+"'-type command.\n");
    }
  else asynstop ();
  return 1;
}

/*-------------------------------------------------------------------------
** Command 'ed'
*/
string QueryName();

int fed (string arg) {
  object ob;

  USAGE (arg == "?",
         "ed [<file>|<object>]\n"
         "  Starts the editor and loads <file> resp. the source file\n"
         "  of <object> if given.\n"
        );

  if (!arg) {
    ed();
    return 1;
  }
  ob = ({object})ME->WSearchInvEnv(lower_case(arg));
  if (ob) {
    arg = blueprint(ob)+".c";
    write("Editing "+arg+" : "+(({string})ob->QueryShort()||"something")+"\n");
  }
  else
    arg = get_path(arg);
  if (is_dir (arg)) {
    notify_fail(arg+": Can't edit a directory!\n");
    return 0;
  }
  if (!Query(P_INVIS))
    say(QueryName()+" starts changing a bit of the world.\n");
  ed(arg);
  return 1;
}


/*-------------------------------------------------------------------------
** Command 'pwd'
*/

int fpwd() { write(QueryCurrentDir() + "\n");  return 1; }

/*-------------------------------------------------------------------------
** Command 'cd'
*/

int fcd (string str) {
  string name, olddir;

  USAGE (str == "?",
         "cd <newdir>\n  Changes the current directory to <newdir>.\n"
        +"cd @\n  Changes the current directory to the directory containing\n"
        +      "  the room you are in.\n"
        );
  olddir = QueryCurrentDir();
  name = cd(str);
  if (name != olddir && file_size(name+"/.readme") > 0)
    cat(name+"/.readme");
  return 1;
}

/*-------------------------------------------------------------------------
** Command 'mkdir'
*/

int fmkdir (string str) {
  USAGE (!str || str == "?"
        , "mkdir <name>\n  Creates a new directory <name>.\n"
        );
  str = get_path (str);
  if (is_dir(str)) {
    notify_fail(str+"/ already exists.\n");
    return 0;
  }
  if (!mkdir(str)) {
    notify_fail("Couldn't create "+str+"/.\n");
    return 0;
  }
  write (str+"/ created.\n");
  return 1;
}

/*-------------------------------------------------------------------------
** Command 'rmdir'
*/

int frmdir (string str) {
  USAGE (!str || str == "?"
        , "rmdir <name>\n  Removes a directory <name>.\n"
        );
  str = get_path (str);
  if (!is_dir(str)) {
    notify_fail(str+"/doesn't exist.\n");
    return 0;
  }
  if (!rmdir(str)) {
    notify_fail("Couldn't remove "+str+"/.\n");
    return 0;
  }
  write (str+"/ removed.\n");
  return 1;
}

/*-------------------------------------------------------------------------
** string stime (int date)
**
** Like cdate() this converts a numeric date into a string, but in a more
** european manner: 'Mon 15-Jun-1992 20:00:00'
*/

string stime (int date) {
  string cdate, rc;
  cdate = ctime(date);
  rc = cdate[8..9]+"-"+cdate[4..6]+"-"+cdate[20..23]+" "+cdate[11..18];
  return rc;
}

/*-------------------------------------------------------------------------
** Handling of file-aliases.
**
** set_falias (alias, exp)
**   Set file-alias <alias> to expansion <exp>.
** get_falias (alias)
**   Return the expansion for <alias>, or 0 if no expansion is defined.
** exp_falias (alias)
**   Return the expansion for <alias>, or <alias> itself if no expansion
**   is defined.
** rm_falias (alias)
**   Removes the expansion for <alias>.
**
** exp_fname (fname)
**   Expands every falias found in fname. Faliases are separated by "/"
**   and not recursively expanded.
*/


void set_falias (string alias, string exp) {
  faliases[alias] = exp;
}

string get_falias (string alias) {
  if (!member(faliases, alias))
    return 0;
  return faliases[alias];
}

string exp_falias (string alias) {
  if (!faliases)
    faliases = ([]);
  if (member(faliases, alias) == 0)
    return alias;
  return faliases[alias];
}

void rm_falias (string alias) {
  m_delete(faliases, alias);
}

string exp_fname (string fname) {
  string headsl, tailsl, *xname;
  int i, len;

  if (!fname || !(len = sizeof(fname))) return fname;
  headsl = (fname[0] == '/') ? "/" : "";
  tailsl = (len > 1 && fname[len-1] == '/') ? "/" : "";
  xname = explode (fname, "/");
  if (pointerp(xname)) {
    for (i = 0; i < sizeof(xname); i++) xname[i] = exp_falias(xname[i]);
    return headsl+implode(xname, "/")+tailsl;
  }
  return headsl+tailsl;
}

/*-------------------------------------------------------------------------
** string basepath (string fname, void|int what)
**
**  Split a normalized filename (beginning with "/") into path (with
**  trailing "/") and basename. Return either the path (what == 0) or
**  the basename (what != 0).
*/

varargs string basepath (string fname, int what) {
  int i, last;
  last = sizeof(fname)-1;
  for (i = last; i >= 0; i--) {
    if (fname[i] == '/') {
      if (what == 0) return fname[0..i];
      else return fname[(i+1)..last];
    }
  }
  if (what == 0) return "/"; else return "";
}

/*-------------------------------------------------------------------------
** mixed *parse_fname (string src, void|object pl)
**
**  Expand <src> to a complete filename by calling pl->get_path() (if pl isn't
**  given, it defaults to this_player()) and return an array with following
**  information:
**
**    [NAME] (str): the complete pathname of srcname.
**    [PATH] (str): the pure path of [0] with trailing "/" (may be just "/") .
**    [BASE] (str): the basename of [0] (may be "").
**    [SIZE] (int): file_size([0]) or FILE_WILD if the basename contains
**                  wildcards ('*'). If it's FILE_NA, the full name is invalid.
**    [PSIZE](int): file_size([1]); if it's FILE_NA, the full name is invalid.
**    [BSIZE](int): file_size([2]) if given and not wildcarded, else undefined.
**
**  The special constants used for SIZE,PSIZE,BSIZE are:
**
**    FILE_NA   == -1 : File doesn't exists or is not accessible.
**    FILE_DIR  == -2 : File is a directory.
**    FILE_WILD == -3 : Filename contains wildcards ('*').
**
**  FILE_NA and FILE_DIR are the ones also returned by efun file_size().
*/

varargs mixed *parse_fname(string src, object pl) {
  mixed *ret;
  int size;
  if (!pl) pl = PL;
  ret = allocate(6);
  src = ({string})pl->get_path(src,0,pl);
  if (sscanf(src, "%~s*%~s")) {
/*if (sscanf(src, "%~s*%~s", foo, bar)) { */
      ret[PATH] = basepath(src,0);
    ret[BASE] = basepath(src,1);
    ret[NAME] = src;
    if (sscanf(ret[PATH], "%~s*%~s")) size = FILE_NA;
/*  if (sscanf(ret[PATH], "%~s*%~s", foo, bar)) size = FILE_NA; */
    else size = file_size(ret[PATH]);
    ret[PSIZE] = size;
    ret[SIZE] = (size != FILE_NA) ? FILE_WILD : FILE_NA;
  } else {
    size = file_size(src);
    if (size >= 0) {
      ret[NAME] = src;
      ret[PATH] = basepath(src, 0);
      ret[BASE] = basepath(src, 1);
      ret[PSIZE] = FILE_DIR;
      ret[BSIZE] = size;
      ret[SIZE] = size;
    } else {
      if (size == FILE_DIR) ret[PATH] = src+(src!="/" ? "/" : "");
      else ret[PATH] = src;
      ret[BASE] = "";
      ret[NAME] = ret[PATH];
      ret[PSIZE] = size;
      ret[SIZE] = size;
    }
  }
  return ret;
}

/*-------------------------------------------------------------------------
** string *get_flist (mixed *fdsc, void|int mode) {
**
**  get_flist() takes the return of parse_fname() and returns an array
**  of the basenames of matching files (which may also be empty if no files
**  are found).
**  If parse_fname() got an illegal file-designation, get_flist() returns 0.
**  If <mode> is < 0, the simul_efun get_dir_ext() is used instead
**  of get_dir(), so that the filetype is automagically added.
**  Else <mode> is used as 'mode' parameter to get_dir().
**  Note that get_flist("/dir/") lists the full dir,
**  whereas get_flist("/dir/*") may stumble over one file "/dir/*".
*/

varargs mixed get_flist (mixed *fdsc, int mode) {
  mixed flist;
  string dir, pat;
  int i, msk;
  if (fdsc[SIZE] == FILE_NA) return 0;
  if (fdsc[SIZE] >= 0) return ({ fdsc[BASE] });
  dir = fdsc[PATH]; pat = fdsc[BASE];
  if (!mode) mode = 1;
  if (fdsc[SIZE] == FILE_DIR) { /* Make sure, that complete dir is listed */
    dir = fdsc[PATH]; pat = "*";
    while (   (flist = (mode<0?get_dir_ext(dir+pat):get_dir(dir+pat,mode)))
           && sizeof(flist) == 1 && flist[0] == pat
          )
      pat += "*";
    /* ... since some folks create files with name '*', '**' etc. */
  } else flist = (mode < 0 ? get_dir_ext(dir+pat) : get_dir (dir+pat,mode));
  if (!flist) flist = ({ });
    /* get_dir with masks sometimes returns on element too much.
    ** Amylaar is going to fix it.
    */
  if (mode > 0) {
    msk = !!(mode & 1) + !!(mode & 2) + !!(mode & 4) + !!(mode & 8);
    if (msk != 1) {
      if (i = (sizeof(flist) % msk))
	      flist = flist[0..sizeof(flist)-(1+i)];
    }
    else if (sizeof(flist) > 0 && !flist[sizeof(flist)-1])
      flist = flist[0..sizeof(flist)-2];
  }
  return flist;
}

/*-------------------------------------------------------------------------
** string f_name (mixed arg, void|int pointc)
**
**   Find the real filename (opt. with trailing '.c') of an object.
**   arg may be the object itself, or already it's object_name().
**   The name is not checked for validity, though.
*/

varargs string f_name (mixed arg, int pointc) {
  string name, foo;
  if (objectp(arg)) name = object_name(arg); else name = arg;
  if (sscanf(name, "%s#%~s", foo)) return name = foo;
  if (pointc && !sscanf(name, "%s.c", foo)) name += ".c";
  return name;
}

/*-------------------------------------------------------------------------
** Normalize a filepath.
** Before normalization, file-aliases are expanded unless noaliases
** is non-zero.
** The usr-argument is transformed and passed to the master as
** the 'user'arg.
** usr ==      0: user = this_player()'s name, thus using it's cwd all the time
** usr ==     ob: user = ob's name, thus using it's cwd if it is using us.
** usr == string: user = string, thus using <strings> cwd if it is using us.
** usr ==    int: user = 0, using noone's cwd at all (int != 0)
*/

varargs string get_path (string fname, int noaliases, mixed usr) {
  string name;
  if (intp(usr))
    if (!usr) name = PL_RNAME;
    else name = 0;
  else if (stringp(usr)) name = usr;
  else if (objectp(usr)) name = ({string})usr->QueryRealName();
  else name = 0;
  if (!noaliases) fname = exp_fname (fname);
  return ({string})MASTER->full_path (fname, name);
}

/*-------------------------------------------------------------------------
** Check if <thing> (with <suffix> appended if given) is a valid file.
** If yes, return it's full path, else 0.
** If <thing> is an object, it's object_name() (w/o ".") will be used, else
** <thing> has to be a string which may contain file-aliases.
*/

varargs string is_file (mixed thing, string suffix) {
  string newpath;

  if (objectp(thing)) newpath = f_name (thing, 0);
  else newpath = exp_fname (thing);
  if (!suffix) suffix = "";
  if (newpath != "" && file_size (newpath+suffix) < 0)  return 0;
  return get_path (newpath, 1);
}

varargs string find_file (mixed thing, string suffix) {
  return is_file(thing, suffix);
}

/*-------------------------------------------------------------------------
** Check if <name> is a valid directory.
** If yes, return it's full path, else 0.
** File-aliases are expanded.
*/

string is_dir (string name) {
  string newpath;

  newpath = exp_fname(name);
  if (newpath != "" && file_size(newpath) != -2)  return 0;
  return get_path (newpath, 1);
}

string find_dir (string name) { return is_dir (name); }

/*-------------------------------------------------------------------------
** more (string fname [, int quiet [,int chunk]])
**
** Mores the file <fname> to the player in chunks of <chunk> lines
** (or pagesize).
** If <quiet> is true, no message about the player is given.
** Returns success.
*/

nosave string more_file;
nosave int more_line;
nosave object more_pl;
nosave int more_chunk;
nosave int mquiet;
nosave string more_regexp;

void more_prompt (int m_line, int m_chunk) {
  write("=== More: Lines "+m_line+".."+(m_line+m_chunk-1)
       +" [CR,u,f,l,q,/<regexp>,<line>,?] ");
}

varargs int more (string fname, int quiet, int chunk) {
  int nline;
  if (!chunk) chunk = pagesize;
  if (!fname) fname = ".";
  fname = get_path(fname, 0, PL);
  if (file_size(fname) < 0) {
    write (fname+" isn't a file.\n"); return 0;
  }
  if (IS_IMMORTAL(PL))
    write ("--- "+fname+" ("+file_size(fname)+" Bytes) ---\n");
  if (!read_file(fname, chunk+1, 1)) { /* Short files may be cat'ed */
    cat (fname, 1, chunk);
    return 1;
  }
  mquiet = quiet;
  more_regexp = 0;
  more_pl = PL;
  more_file = fname; more_line = 1; more_chunk = chunk;
  if ((nline = cat(more_file, more_line, more_chunk)) == 0) {
    write("No such file\n");
    return 0;
  }
  if (!quiet)
    say (PL_NAME+" looks at a completely different reality.\n");
  more_prompt(more_line, nline);
  input_to(#'even_more /*'*/);
  return 1;
}

static void even_more(string str) {
  int nline, display;
  string foo;
  display = 0;
  if (str == "a") display = 1; /* do nothing but redisplay */
  else if (str == "" || str == "d")
    more_line += more_chunk, display = 1;
  else if (str == "q") { write("== Ok.\n"); return; }
  else if (str == "u" || str == "b")
    more_line -= more_chunk, display = 1;
  else if (str == "f")
    more_line = 1, display = 1;
  else if (str == "l") {
    write ("..wait..\n"); skip_to_end_of_more(); return;
  }
  else if (str[0] == '/') {
    if (sizeof(str) < 2 && sizeof(more_regexp || "") < 1)
      write("No previous regular expression.\n");
    else
    {
      if (sizeof(str) > 1)
        more_regexp = str[2..];
      if (!regexp(({"dummy"}), more_regexp)) {
         write("*** Bad regular expression.\n");
        more_regexp = 0;
      }
      else {
        nline = more_line+more_chunk;
        write("..searching..\n");
        while (   (foo = read_file(more_file, nline++, 1))
               && !sizeof(regexp(({foo}), more_regexp)));
        if (foo) {
          more_line = nline-1;
          display=1;
        }
        else
          write("*** Pattern not found.\n");
      }
    }
  }
  else if (str == "?")
    write ( "Accepted commands are:\n"
          + "  [CR], d  : next page\n"
          + "  u   , b  : last page\n"
          + "  f        : go to the first page\n"
          + "  l        : go to the last page\n"
          + "  <nr>     : go to line <nr>\n"
          + "  /<regexp>: scan forward until line matching <regexp>\n"
          + "  /        : repeats last /<regexp> command\n"
          + "  a        : redisplay current page\n"
          + "  ?        : this help\n"
          + "  q        : quits more.\n"
          + "Each page is by "+more_chunk+" lines long.\n"
          );
  else if (sscanf(str, "%d", nline) == 1) more_line = nline, display = 1;
  if (more_line < 1) more_line = 1;
  if (display && (nline = cat(more_file, more_line, more_chunk)) == 0) {
    more_file = 0;
    write("=== EOF\n");
    if (!mquiet)
      say (({string})more_pl->QueryName()+"'s view focuses again on this reality.\n");
    return;
  }
  more_prompt (more_line, nline);
  input_to(#'even_more /*'*/);
  return;
}


static void skip_to_end_of_more() {
  string foo;
  int step, nline;

  step = 0;
  while ((foo = read_file(more_file, more_line, more_chunk)) && foo != "") {
    more_line += more_chunk; step++;
    if (step >= 50) {
      tell_object (more_pl, ".. Line "+more_line+" ..\n");
      call_out(#'skip_to_end_of_more /*'*/, 0);
      return;
    }
  }
  more_line -= more_chunk;
  if (more_line < 1) more_line = 1;
  if ((nline = cat(more_file, more_line, more_chunk)) == 0) {
    more_file = 0;
    write("=== EOF\n");
    if (!mquiet)
      say (({string})more_pl->QueryName()+"'s view focuses again on this reality.\n");
    return;
  }
  more_prompt (more_line, nline);
  input_to(#'even_more /*'*/);
}

/*-------------------------------------------------------------------------
** type (int cmd, string fname [, int chunk])
**
** Types <fname> out. If <chunk> isn't defined, it is assumed to be
** pagesize. Returns success.
**   cmd = 0: types the whole file in chunks of <chunk> lines.
**   cmd = 1: types the first lines of the file ("cat")
**   cmd = 2: types the last lines of the file ("tail")
*/

varargs int type (int cmd, string fname, int chunk) {
  int mline;
  if (!chunk) chunk = pagesize;
  if (cmd < 0 || cmd > 2) {
    write ("Illegal parameter 'cmd "+cmd+"' for filelib::type().\n");
    return 0;
  }
  if (!fname) fname = ".";
  fname = get_path(fname, 0, PL);
  if (file_size(fname) < 0) {
    write (fname+" isn't a file.\n"); return 0;
  }
  write ("--- "+fname+" ("+file_size(fname)+" Bytes) ---\n");
  if (cmd == 0) {
    mline = 2;
    if (!cat(fname, 1, 1)) { write("No such file\n"); return 0; }
    while (cat(fname, mline, chunk)) mline+=chunk;
  }
  if (cmd == 1) {
    if (!cat(fname)) { write("No such file\n"); return 0; }
  }
  if (cmd == 2) {
    if (!tail(fname)) { write("No such file\n"); return 0; }
  }
  return 1;
}

/*-------------------------------------------------------------------------
** cp (int cmd, string src, string dst)
**
** Copies the file(s)/dir <src> to the file/dir <dst>.
**   cmd = 0, 4: Simple copy (4 is non-overwriting)
**   cmd = 1   : Appends <src> to <dst> if necessary
**   cmd = 2, 6: Moves <src> to <dst> (6 is non-overwriting)
** Both names may be empty, then the cwd is used.
** The <src> may be wildcarded.
** Note that Bit 2 (0x4) is the 'no overwrite'-flag.
**
** Adapted the move-command to utilize rename(). This usage is possibly
** not optimized, but as a late addition it wasn't rarely better possible.
*/

int cp (int cmd, string src, string dst) {
  if (factive) {
    write( "The previous action hasn't ended yet.\n");
    return 0;
  }
  if (member(({ 0, 1, 2, 4, 6}), cmd) == -1) {
    write ("Illegal parameter 'cmd "+cmd+"' for cp().\n");
    return 0;
  }
  if (!src) {
    write ("No source specified.\n"); return 0;
  }
  if (!dst) dst = ".";
  acmd = cmd;
  srcd = parse_fname(src, PL);
  if (srcd[SIZE] == FILE_NA) {
    write ("Src: Illegal path/name '"+srcd[NAME]+"'\n");
    return 0;
  }
  dstd = parse_fname(dst, PL);
  if (dstd[SIZE] == FILE_WILD) {
    write ("Dst: Illegal path/name '"+dstd[NAME]+"'\n");
    return 0;
  }

  if (acmd != 1) {
    if (srcd[SIZE] < 0) {
      if (dstd[SIZE] >= 0) {
        write ("Can't write multiple files to a single file.\n");
        return 0;
      }
      if (dstd[SIZE] == FILE_NA) {
        if (srcd[SIZE] == FILE_DIR && cmd == 2) {
          if (rename(srcd[NAME], dstd[NAME])) {
            write("Can't move '"+srcd[NAME]+"' to '"+dstd[NAME]+"/'\n");
            return 0;
          }
          write("Moved '"+srcd[NAME]+"' to '"+dstd[NAME]+"/'\n");
          return 1;
        }
        if (!mkdir(dstd[NAME])) {
          write("Dst: Can't dir '"+dstd[NAME]+"'\n");
          return 0;
        }
        write("Dst: Created dir '"+dstd[NAME]+"'\n");
        dstd[SIZE] = FILE_DIR;
        dstd[PATH] += dstd[BASE]+"/";
        dstd[BASE] = "";
        dstd[NAME] += "/";
      }
      else if (dstd[SIZE] == FILE_DIR && srcd[SIZE] == FILE_DIR
               && (cmd & 0x4)) {
        write("Can't remove directory '"+srcd[NAME]+"'\n");
        return 0;
      }

      if (dstd[SIZE] == FILE_DIR && srcd[PATH] == dstd[PATH]) {
        write ("Can't write files onto themselves.\n");
        return 0;
      }
    } else {
      if (dstd[SIZE] >= 0 && srcd[NAME] == dstd[NAME]) {
        write ("Can't write file onto itself.\n");
        return 0;
      }
    }
  }

  sfile = get_flist(srcd, 0);
  atodo = sizeof(sfile);
  if (!atodo && srcd[SIZE] == FILE_WILD) {
    write (srcd[NAME]+": No match.\n");
    return 0;
  }
  if (atodo <= 2 && srcd[SIZE] == FILE_DIR) { /* don't count . and .. */
    write ("Directory "+srcd[NAME]+" is empty.\n");
    return 0;
  }

  factive = 1; adone = 0; asynfun = "_do_cp_part";
  if (atodo <=10) _do_cp_part();
  else call_out("_do_cp_part", 0);
  return 1;
}


  /* Copy one bunch of files */
static void _do_cp_part() {
  string src, dst;
  int rc, i, part, cmd, norm;

  SECURE3(0);

  norm = acmd & 4;
  cmd = acmd & 3;
  part = atodo - adone;
  if (part > 10) part = 10;
  for (i = adone; i < adone + part; i++) {
    if (sfile[i] == "." || sfile[i] == "..") continue;
    src = srcd[PATH]+sfile[i];
    if (dstd[SIZE] == FILE_DIR) dst = dstd[PATH]+sfile[i];
    else dst = dstd[NAME];

    rc = file_size(src);
    if (rc < 0) {
      if (rc == FILE_DIR) tell_object (PL,"Directory "+src+"/ not copied.\n");
      else tell_object (PL,"Src: "+src+" not accessible.\n");
      continue;
    }

    rc = file_size(dst);
    if (rc == FILE_DIR) {
      tell_object (PL,"Dst: Can't write "+src+" as directory "+dst+"/\n");
      continue;
    }

    if (cmd != 1) {
      if (src == dst) {
        tell_object (PL,"Can't write "+src+" on to itself.\n");
        continue;
      }
      if (rc >= 0) {
        if (norm) {
	  tell_object (PL,"Dst: File "+dst+" already exists -- skipped.\n");
	  continue;
        }
        else if (!efun::rm(dst) && cmd != 2) {
	  tell_object (PL,"Dst: Can't remove old "+dst+"\n");
	  continue;
	}
      }
    }

/* to be removed in native mode: */
    if (!copyfile(src, dst)) {
      tell_object (PL,"Can't write "+dst+"\n");
      continue;
    }
/* end of remove */

#if 0
    if (cmd != 2) {
      if (!copyfile(src, dst)) {
        tell_object (PL,"Can't write "+dst+"\n");
        continue;
      }
    } else {
      if (rename(src, dst)) {
        tell_object (PL,"Can't move "+src+" to "+dst+"\n");
        continue;
      }
    }
#endif
    if (cmd == 1) tell_object (PL,"Joined ");
    else if (cmd == 2) tell_object (PL,"Moved ");
    else tell_object (PL,"Copied ");
    tell_object (PL, src+" to "+dst+"\n");


/* to be removed in native mode: */
    if (cmd == 2 && !efun::rm(src))
      tell_object (PL,"Src: Can't remove old "+src+"\n");
/* end of remove */
  }
  adone += part;
  if (adone >= atodo) {
    if (atodo > 10) tell_object (PL, "> ");
    factive = 0;
  } else if (factive) call_out ("_do_cp_part", 0);
}

  /* Copy one file */
int copyfile (string src, string dst) {
  int fpos, todo, chunk;
#if __VERSION__ > "3.5.2"
  bytes data;
#else
  string data;
#endif

  todo = file_size(src);
  fpos = 0;
  while (todo > 0) {
    chunk = todo;
    if (chunk > 10000) chunk = 10000;
    data = read_bytes (src, fpos, chunk);
    todo -= chunk;
    fpos += chunk;
#if __VERSION__ > "3.5.2"
    if (!write_file (dst, to_text(data, "UTF-8"))) return 0;
#else
    if (!write_file (dst, data)) return 0;
#endif
  }
  return 1;
}

/*-------------------------------------------------------------------------
** rm (string fname)
**
** Removes the file <fname>, wildcards are allowed.
*/

int rm (string fname) {
  if (factive) {
    write( "The previous action hasn't ended yet.\n");
    return 0;
  }
  if (!fname || fname == "") { write ("No file specified.\n"); return 0; }
  srcd = parse_fname(fname, PL);
  if (srcd[SIZE] == FILE_NA) {
    write ("Illegal path/name '"+srcd[NAME]+"'\n");
    return 0;
  }
  sfile = get_flist(srcd, 0);
  atodo = sizeof(sfile);
  if (!atodo && srcd[SIZE] == FILE_WILD) {
    write (srcd[NAME]+": No match.\n");
    return 0;
  }
  if (atodo <= 2 && srcd[SIZE] == FILE_DIR) { /* don't count . and .. */
    write ("Directory "+srcd[NAME]+" is empty.\n");
    return 0;
  }

  factive = 1; adone = 0; asynfun = "_do_rm_part";
  if (atodo > 10) call_out ("_do_rm_part", 0); else _do_rm_part();
  return 1;
}

  /* Remove one bunch of files */
static void _do_rm_part() {
  string src;
  int rc, i, part;

  SECURE3(0);
  part = atodo - adone;
  if (part > 10) part = 10;
  for (i = adone; i < adone + part; i++) {
    if (sfile[i] == "." || sfile[i] == "..") continue;
    src = srcd[PATH]+sfile[i];
    rc = file_size(src);
    if (rc < 0) {
      if (rc == FILE_DIR)
        tell_object (PL,"Directory "+src+"/ not removed.\n");
      else tell_object (PL,src+" not accessible.\n");
      continue;
    }

    if (!efun::rm(src)) tell_object (PL,"Can't remove "+src+"\n");
    else tell_object(PL, "Removed "+src+"\n");
  }
  adone += part;
  if (adone >= atodo) {
    if (atodo > 10) tell_object (PL, "> ");
    factive = 0;
  } else if (factive) call_out ("_do_rm_part", 0);
}

/*-------------------------------------------------------------------------
** ls (int cmd, string fname)
**
** Lists information about file/dir <fname>.
**   cmd = 0: Lists directory and flags subdirs and loaded objects.
**   cmd = 1: Lists directory with filelengths, subdir-flags and date.
**   cmd = 2: Prints size of file/directory.
** If <fname> is empty, the the cwd is assumed.
** Wildcards are allowed.
*/

int ls (int cmd, string fname) {
  int date, mode;

/*
  if (factive) {
    write( "The previous action hasn't ended yet.\n");
    return 0;
  }
*/
  if (cmd < 0 || cmd > 2) {
    write ("Illegal parameter 'cmd "+cmd+"' to ls().\n");
    return 0;
  }
  if (!fname) fname = ".";
  acmd = cmd;
  srcd = parse_fname(fname, PL);
  if (srcd[SIZE] == FILE_NA) {
    write ("Illegal path/name '"+srcd[NAME]+"'\n");
    return 0;
  }
  if (srcd[SIZE] >= 0) {
    write (srcd[NAME]+" : "+srcd[SIZE]+" Bytes   ");
    if ((date = file_date(srcd[NAME])) >= 0)
      write (stime(date));
    else write ("???");
    write ("\n");
    return 1;
  }

  switch (cmd) {
    case 0: mode = -1; break;
    case 1: mode = 7; break;
    case 2: mode = 0; break;
  }
  sfile = get_flist(srcd, mode);

  if (srcd[SIZE] == FILE_DIR && acmd == 2) {
    write (srcd[NAME]+" : ");
    if (cmd != 1) write (""+sizeof(sfile));
    else write (""+(sizeof(sfile)/3));
    write (" files   ");
    if ((date = file_date(srcd[NAME])) >= 0)
      write (stime(date));
    else write ("???");
    write ("\n");
    return 1;
  }

  /* if acmd is 'chk', [NAME] must be wildcarded here, so lets do a 'lls' */
  if (acmd == 2) {
    sfile = get_flist(srcd, 7);
    acmd = 1;
  }

  atodo = sizeof(sfile);
  if (!atodo) {
    if (srcd[SIZE] == FILE_WILD) write (srcd[NAME]+": No match.\n");
    else write ("Directory "+srcd[NAME]+" is empty.\n");
    return 0;
  }

/*
  factive = 1; adone = 0; asynfun = "_do_ls_part";
*/
  adone = 0;
    /* We use get_dir_ext() now, so only lls needs to be asynchronously */
  _do_ls_part();
  return 1;
}

  /* The complete lls can now be done in one execution, since Amylaar's GD
  ** get_dir() gives date and length of the files as well.
  ** Nevertheless, this is a nice separation of the argument parsing and
  ** the real ls'ing.
  ** Here acmd is either 0 (ls) or 1 (lls).
  */
static void _do_ls_part() {
  string line, name, foo;
  int i, last, size, part, date;

  if (!adone) {
    if (acmd != 0) tell_object (PL, "\n");
    tell_object (PL, "--- "+srcd[NAME]+" (");
    if (acmd != 1) tell_object (PL, ""+sizeof(sfile));
    else tell_object (PL, ""+(sizeof(sfile)/3));
    if (srcd[SIZE] == FILE_WILD) tell_object (PL, " matches");
    else tell_object (PL, " entries");
    tell_object (PL, ") ---\n");
  }

  line = "";
  part = atodo - adone;
/*  ahalf = (atodo+1)/2;
  now obsolete...
  if (acmd != 2 && part > 20) part = 20;
*/
  for (i = adone; i < adone+part; i++) {
    if (acmd == 0) {
      name = sfile[i];
      last = (i%4==3);
      name = name[0..17];
      name = ladjust(name, 18)+(last?"\n":" ");
    }
    if (acmd == 1) {
      last = 1;
      name = sfile[i];
      size = sfile[++i];
      if (size < 0) { foo = "       -"; name += "/"; }
      else foo = radjust(size+"",8);
      if ((date = sfile[++i]) >= 0)
        foo = stime(date) + foo;
      else foo = "- "+foo;
      name = radjust(foo,30)+" "+name+"\n";
    }
    line += name;
    if (last) { tell_object(PL, line); line = ""; }
  }
  if (!last) tell_object (PL, line+"\n");
  adone += part;
/*
  if (adone >= atodo) {
    factive = 0;
    if (acmd != 2) tell_object (PL, "> ");
  } else if (factive) call_out ("_do_ls_part", 0);
*/
}

/*-------------------------------------------------------------------------
** cd (path)
**
** Changes the currentdir to <path>, if that's a directory, and returns
** the new currentdir.
** If newdir is zero, currentdir is set to the home dir if existing.
*/

string cd (string path) {
  string newpath;

  if (!path) path = WPATH+ ({string})ME->QueryRealName();
  if (path == "@") {
    if (!environment()) {
      write("No environment.\n");
      return currentdir;
    }
    path = implode(explode(object_name(environment()), "/")[0..<2], "/");
  }
  newpath = is_dir (path = get_path(path));

  if (!newpath)
    if (is_file(path)) write(path+": Is a file.\n");
    else write (path+": No such directory.\n");
  else {
    write(newpath+"\n");
    currentdir = newpath;
  }
  return currentdir;
}

/*-------------------------------------------------------------------------
** asynstop ()
**
** Stops the current asynchronous activity.
*/

void asynstop() {
  if (factive) {
    factive = 0;
    if (asynfun) remove_call_out (asynfun);
    asynfun = 0;
    write ("=== Action stopped ===\n");
  }
}

/*************************************************************************/
