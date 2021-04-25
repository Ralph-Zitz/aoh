/* recursive file commands:
 * tcp, tmv, trm, tbackup, tgrep and tdu
 * (c) 1993/09/20 (?) - 1994/03/23 by Taube
 *
 * Syntax:
 * tcp (source dest | [-r] (source|dir) {source|dir} destdir) [-o outfile]
 * tmv (source dest | [-r] (source|dir) {source|dir} destdir) [-o outfile]
 * trm [-r] (file|dir) {file|dir} [-o outfile]
 * tbackup (source dest | [-r] (source|dir) {source|dir} destdir) [-o outfile]
 * tgrep pattern [-r] (file|dir) {file|dir} [-o outfile]
 * tdu [file|dir]
 * tshow
 * tlist
 *
 * The filenames may be wildcarded in the usual way:
 *    '?' stands for a single character
 *    '*' stands for 0 or more characters
 *    the other characters are treated like patterns for regexp.
 * The pattern is treated as regexp.
 *
 * Comments:
 *    The commands are done asynchronously.
 *    New commands may be given while others are still pending.
 *    You have to clone this object by yourself.
 *    You must have a wizlevel >=20 to work with this object.
 *    Every overwritten or deleted file is saved into
 *    "/players/<yourname>/.trashcan/". These backups are automatically deleted
 *    14 days later, if you are logged in for more than 1 hour.
 *
 * Ideas:
 *    Option -i for query for each file
 *    Option -f for 'force'
 *    Option -p for preserve permissions/date/time
 *    A command to stop all current actions
 */

/*#define DEBUG*/

nosave variables inherit "/std/thing";
nosave variables inherit "/lib/string";   /* thanks, Mateese */

#include <config.h>
#include <moving.h>
#include <properties.h>
#include <secure/wizlevels.h>


#define ME this_object()
#define TP this_player()
#define TI this_interactive()


#define SECURE(val) if(!check_previous()) return(val);


#define MINWIZLEVEL LVL_WIZARD   /* minimum wizlevel for this tool */

#define BRINT        3600   /* interval for checking old backups */
#define RMINT     1209600   /* interval for removing old backups */

#define DELAY           1   /* wait n seconds before each call_out() */
#define COUNT          15   /* copy max. n files at a time */
#define MAXRMTRY      100   /* maximum tries to remove a dir */

#define FNLENGTH       12   /* maximum length of the shortened filename */
#define MAXNUM      10000   /* maximum number appended to then filename */

#define TRASHCAN    "/.trashcan"
#define TMPDIR      "/tmp/"


#define cp(src,des) (rm(des), catch(write_file(des, read_file(src))))
#define copy(arg) (pointerp(arg)?arg+({}):arg)


static int check_previous();
static int IsWildcarded(string file);
static string *DeWildcard(string file);
static varargs void NewAction(string func, string *files, mixed args, int magic);
static string *ExpandFileList(string *args);
static int IsFile(string file);
static string NewName(string file);
static string GetName(string file);
static void Write(mixed output, string text);
static int AddStore(string func, mixed args);
static int PutStore(int magic, mixed item);
static void IncStore(int magic);
static int GetStore(int magic);
static void DecStore(int magic);
static int CheckStore(int magic);
void CopyIt(string src, mixed args);
void RescueFile(string file);
void MoveIt(string src, mixed args);
void RemoveIt(string file, mixed *args);
void BackupIt(string src, mixed args);
void GrepIt(string file, mixed args);
void DiskUsageFile(string file, mixed *args);

object user;
string uid, save;

mixed *actions;
//mixed *stores;
mapping stores;

varargs void create()
{
  (::create());
  if(!clonep()) return;
  if(!geteuid()) {
    write("\
For security reasons this object may NOT be cloned\n\
from a wizard's directory. Try /obj or /obj/tool.\n");
    remove();
    if(ME) destruct(ME);
    return;
  }
  user=0;
  /* some ids... */
  SetIds(({"filecommands", "filecommand", "filecmds", "filecmd",
	     "file commands", "file command", "file cmds", "file cmd",
	     "fcommands", "fcommand", "fcmds", "fcmd",
	     "filetool", "ftool"}));
  SetAds(({"rec", "recursive"}));
  SetHelpMsg("Just examine the filecommands.\n");
  SetShort("recursive file commands");
  SetLong("This tool provides you with additional recursive file commands.\n\
Usage:\n\
   tcp (source dest | [-r] (source|dir) {source|dir} destdir)\n\
   tmv (source dest | [-r] (source|dir) {source|dir} destdir)\n\
   trm [-r] (file|dir) {file|dir}\n\
   tbackup (source dest | [-r] (source|dir) {source|dir} destdir)\n\
   tgrep pattern [-r] (file|dir) {file|dir}\n\
   tdu [file|dir]\n\
These commands may also be called with the option '-o outfile'\n\
to write the output to the file 'outfile'.\n\n\
The filenames may be wildcarded in the usual way:\n\
   '?' means a single character\n\
   '*' means 0 or more characters\n\
   The other characters are treated like patterns for regexp.\n\
The pattern (see 'tgrep') is treated as regexp.\n\n\
Every overwritten or deleted file is saved into\n\
\"/players/<yourname>/.trashcan/\".\n\
These backups are removed automatically after 24 hours.\n\n\
This tool works asynchronously in the background.\n\
Use 'tshow' and 'tlist' to list the current activities.\n\n\
\t(c) 13.02.1993 by Taube\n");
  SetNoDrop(1);
  Set(P_NOBUY,1);
  Set(P_NOSELL,1);
  Set(P_AUTOOBJECT,1);
  actions=({});
  //stores=({({}),({}),({}),({}),({})});
  stores = ([]);
}


varargs int move(mixed dest, int method, mixed extra)
{
  if(user)
    return ME_NO_MOVE;
  if(dest && living(dest)) {
    user=TP;
    if(!IS_WIZARD(user)) {
      write("Sorry! You are not allowed to use this tool.\n");
      remove();
      if(ME) destruct(ME);
      return ME_DESTRUCTED_SELF;
    }
    seteuid(uid=getuid());
    save=user->get_path("~/"+TRASHCAN);
#ifdef DEBUG
    write_file("/log/taube/filecmds", ctime(time())+": "+uid+"\n");
#endif
    if(file_size(save)!=-2) mkdir(save);
    if(sizeof(get_dir(save+"/*"))>2)
      write("Filetool: There are backup files in "+save+"\n");
    call_out("RemoveOldBackups", BRINT);
  }
  return ::move(dest, method, extra);
}


void init()
{
  (::init());
  if(TP!=user) return;
  add_action("CopyFile", "tcp");
  add_action("MoveFile", "tmv");
  add_action("RemoveFile", "trm");
  add_action("BackupFile", "tbackup");
  add_action("GrepFile", "tgrep");
  add_action("DiskUsage", "tdu");
  add_action("ShowActions", "tshow");
  add_action("ListActions", "tlist");
}



void RemoveOldBackups()
{
  string *files, file;
  int     index;

  /* if(!CheckPermissions()) return; */
  call_out("RemoveOldBackups", BRINT);
  files=get_dir(save+"/*");
  for(index=sizeof(files); index--;) {
    file=save+"/"+files[index];
    if(IsFile(file) && time()-file_date(file)>RMINT) {
      write("Removed backup file "+file+"\n");
      rm(file);
    }
  }
}



int CopyFile(string str)
{
  string *files, dest;
  int    recursive, arg;
  mixed  output;

#ifdef DEBUG
  write_file("/log/taube/filecmds", "CopyFile("+mixed_to_string(str)+")\n");
#endif
  SECURE(0)
  if(!str || str=="") {
    write
      ("Usage:\n"+
       "tcp (source dest | source {source} destdir | -r dir {dir} destdir)\n");
    return 1;
  }
  files=explode(str," ")-({""});
  if(recursive=member(files, "-r")!=-1)
    files-=({"-r"});
  output=0;
  while((arg=member(files, "-o"))!=-1) {
    output=files[arg+1];
    files=files[0..arg-1]+files[arg+2..];
  }
  files=ExpandFileList(files);
  if(sizeof(files)<2) {
    write("tcp needs at least two arguments.\n");
    return 1;
  }
  dest=files[<1];
  files=files[0..<2];
  if(file_size(dest)!=-2) {
    NewAction("CopySingle", files, ({dest, output}));
  } else if(recursive) {
    NewAction("CopyRec", files, ({dest, output}));
  } else {
    NewAction("CopyFiles", files, ({dest, output}));
  }
  return 1;
}

void CopySingle(mixed args)
{
  string *files;

#ifdef DEBUG
  write_file("/log/taube/filecmds", "CopySingle("+mixed_to_string(args)+")\n");
#endif
  SECURE(0)
  files=args[0];
  if(sizeof(files)!=1) {
    write("Cannot copy multiple files into one.\n");
    return;
  }
  CopyIt(files[0], args[1]);
}

void CopyRec(mixed args)
{
  SECURE(0)
  map(args[0], "CopyRec2", ME, args[1]);
}

void CopyRec2(string file, mixed args)
{
  SECURE(0)
  args=copy(args);
  args[0]+="/"+GetName(file);
  if(file_size(file)==-2) {
    NewAction("MkDir", ({args[0]}), ({args[1]}));
    NewAction("CopyRec", map(filter(get_dir(file+"/*"),
						"IsOrdinary", ME),
				   "AddPath", ME, file+"/"), args);
  } else {
    CopyIt(file, args);
  }
}

void CopyFiles(mixed args)
{
  SECURE(0)
  map(args[0], "CopyFiles2", ME, args[1]);
}

void CopyFiles2(string file, mixed args)
{
  SECURE(0)
  args=copy(args);
  if(file_size(file)==-2) {
    NewAction("CopyFiles", filter(map(get_dir(file+"/*"),
						  "AddPath", ME, file+"/"),
					"IsFile", ME), args);
  } else {
    args[0]+="/"+GetName(file);
    CopyIt(file, args);
  }
}

void CopyIt(string src, mixed args)
{
  string des;
  mixed  output;

#ifdef DEBUG
  write_file("/log/taube/filecmds", "CopyIt("+mixed_to_string(src)+","+
	     mixed_to_string(args)+")\n");
#endif
  SECURE(0)
  if(file_size(src)<0) {
    write("Source file "+src+" does not exist.\n");
    return;
  }
  des=args[0];
  output=args[1];
  RescueFile(des);
  Write(output, "Copied file "+src+" to "+des+"\n");
#ifdef DEBUG
  if(des[0..4]==TMPDIR)
#endif
    cp(src, des);
}



int MoveFile(string str)
{
  string *files, dest;
  int    recursive, arg;
  mixed  output;

  SECURE(0)
  if(!str || str=="") {
    write
      ("Usage:\n"+
       "tmv (source dest | source {source} destdir | -r dir {dir} destdir)\n");
    return 1;
  }
  files=explode(str," ")-({""});
  if(recursive=member(files, "-r")!=-1)
    files-=({"-r"});
  output=0;
  while((arg=member(files, "-o"))!=-1) {
    output=files[arg+1];
    files=files[0..arg-1]+files[arg+2..];
  }
  files=ExpandFileList(files);
  if(sizeof(files)<2) {
    write("tmv needs at least two arguments.\n");
    return 1;
  }
  dest=files[<1];
  files=files[0..<2];
  if(file_size(dest)!=-2) {
    NewAction("MoveSingle", files, ({dest, output}));
  } else if(recursive) {
    NewAction("MoveRec", files, ({dest, output}));
  } else {
    NewAction("MoveFiles", files, ({dest, output}));
  }
  return 1;
}

void MoveSingle(mixed args)
{
  string *files;

  SECURE(0)
  files=args[0];
  if(sizeof(files)!=1) {
    write("Cannot move multiple files into one.\n");
    return;
  }
/*
  if(file_size(files[0])<0) {
    write("Source file "+files[0]+" does not exist.\n");
    return;
  }
*/
  MoveIt(files[0], args[1]);
}

void MoveRec(mixed args)
{
  SECURE(0)
  map(args[0], "MoveRec2", ME, args[1]);
}

void MoveRec2(string file, mixed args)
{
  SECURE(0)
  args=copy(args);
  args[0]+="/"+GetName(file);
  if(file_size(file)==-2) {
    NewAction("MkDir", ({args[0]}), ({args[1]}));
    NewAction("MoveRec", map(filter(get_dir(file+"/*"),
						 "IsOrdinary", ME),
				    "AddPath", ME, file+"/"), args);
  } else {
    MoveIt(file, args);
  }
}

void MoveFiles(mixed args)
{
  SECURE(0)
  map(args[0], "MoveFiles2", ME, args[1]);
}

void MoveFiles2(string file, mixed args)
{
  SECURE(0)
  args=copy(args);
  if(file_size(file)==-2) {
    NewAction("MoveFiles", filter(map(get_dir(file+"/*"),
						  "AddPath", ME, file+"/"),
					"IsFile", ME), args);
  } else {
    args[0]+="/"+GetName(file);
    MoveIt(file, args);
  }
}

void MoveIt(string src, mixed args)
{
  string des;
  mixed  output;

  SECURE(0)
  if(file_size(src)<0) {
    write("Source file "+src+" does not exist.\n");
    return;
  }
  des=args[0];
  output=args[1];
  RescueFile(des);
  Write(output, "Moved file "+src+" to "+des+"\n");
#ifdef DEBUG
  if(des[0..4]==TMPDIR)
#endif
    if(rename(src, des))
      cp(src, des);
}



int RemoveFile(string str)
{
  string *files;
  int    recursive, arg;
  mixed  output;

  SECURE(0)
  if(!str || str=="") {
    write
      ("Usage:\n"+
       "trm (file {file} | -r dir {dir})\n");
    return 1;
  }
  files=explode(str," ")-({""});
  if(recursive=member(files, "-r")!=-1)
    files-=({"-r"});
  output=0;
  while((arg=member(files, "-o"))!=-1) {
    output=files[arg+1];
    files=files[0..arg-1]+files[arg+2..];
  }
  files=ExpandFileList(files);
  if(sizeof(files)<1) {
    write("tmv needs at least one argument.\n");
    return 1;
  }
  if(recursive) {
    NewAction("RemoveRec", files, ({output}));
  } else {
    NewAction("RemoveFiles", files, ({output}));
  }
  return 1;
}

void RemoveRec(mixed args)
{
  SECURE(0)
  map(args[0], "RemoveRec2", ME, args[1]);
}

void RemoveRec2(string file, mixed args)
{
  SECURE(0)
  args=copy(args);
  if(file_size(file)==-2) {
    NewAction("RemoveRec", map(filter(get_dir(file+"/*"),
						 "IsOrdinary", ME),
				    "AddPath", ME, file+"/"), args);
    NewAction("RmDir", ({file}), args);
  } else {
    RemoveIt(file, args);
  }
}

void RemoveFiles(mixed args)
{
  SECURE(0)
  map(args[0], "RemoveFiles2", ME, args[1]);
}

void RemoveFiles2(string file, mixed args)
{
  SECURE(0)
  args=copy(args);
  if(file_size(file)==-2) {
    NewAction("RemoveFiles", filter(map(get_dir(file+"/*"),
						 "AddPath", ME, file+"/"),
				       "IsFile", ME), args);
  } else {
    RemoveIt(file, args);
  }
}

void RemoveIt(string file, mixed *args)
{
  mixed output;

  SECURE(0)
  if(file_size(file)<0) {
    write("File "+file+" does not exist.\n");
    return;
  }
  output=args[0];
  RescueFile(file);
  Write(output, "Removed file "+file+"\n");
#ifdef DEBUG
  if(file[0..4]==TMPDIR)
#endif
    rm(file);
}



int BackupFile(string str)
{
  string *files, dest;
  int    recursive, arg;
  mixed  output;

  SECURE(0)
  if(!str || str=="") {
    write
      ("Usage:\n"+
       "tbackup (source dest | source {source} destdir | -r dir {dir} destdir)\n");
    return 1;
  }
  files=explode(str," ")-({""});
  if(recursive=member(files, "-r")!=-1)
    files-=({"-r"});
  output=0;
  while((arg=member(files, "-o"))!=-1) {
    output=files[arg+1];
    files=files[0..arg-1]+files[arg+2..];
  }
  files=ExpandFileList(files);
  if(sizeof(files)<2) {
    write("tbackup needs at least two arguments.\n");
    return 1;
  }
  dest=files[<1];
  files=files[0..<2];
  if(file_size(dest)!=-2) {
    NewAction("BackupSingle", files, ({dest, output}));
  } else if(recursive) {
    NewAction("BackupRec", files, ({dest, output}));
  } else {
    NewAction("BackupFiles", files, ({dest, output}));
  }
  return 1;
}

void BackupSingle(mixed args)
{
  string *files;

  SECURE(0)
  files=args[0];
  if(sizeof(files)!=1) {
    write("Cannot backup multiple files into one.\n");
    return;
  }
/*
  if(file_size(files[0])<0) {
    write("Source file "+files[0]+" does not exist.\n");
    return;
  }
*/
  BackupIt(files[0], args[1]);
}

void BackupRec(mixed args)
{
  SECURE(0)
  map(args[0], "BackupRec2", ME, args[1]);
}

void BackupRec2(string file, mixed args)
{
  SECURE(0)
  if(file_size(file)==-2) {
    NewAction("BackupRec",
	      map(filter(get_dir(file+"/*"), "IsOrdinary", ME),
			"AddPath", ME, file+"/"), args);
  } else {
    args=copy(args);
    args[0]=NewName(args[0]+"/"+GetName(file));
    BackupIt(file, args);
  }
}

void BackupFiles(mixed args)
{
  SECURE(0)
  map(args[0], "BackupFiles2", ME, args[1]);
}

void BackupFiles2(string file, mixed args)
{
  SECURE(0)
  if(file_size(file)==-2) {
    NewAction("BackupFiles",
	      filter(map(get_dir(file+"/*"),
				     "AddPath", ME, file+"/"),
			   "IsFile", ME), args);
  } else {
    args=copy(args);
    args[0]=NewName(args[0]+"/"+GetName(file));
    BackupIt(file, args);
  }
}

void BackupIt(string src, mixed args)
{
  string des;
  mixed  output;

  SECURE(0)
  if(file_size(src)<0) {
    write("Source file "+src+" does not exist.\n");
    return;
  }
  des=args[0];
  output=args[1];
  Write(output, "Backuped file "+src+" to "+des+"\n");
#ifdef DEBUG
  if(des[0..4]==TMPDIR)
#endif
    cp(src, des);
}



int GrepFile(string str)
{
  string *files, pattern;
  mixed  output;
  int    recursive, arg;

  SECURE(0)
  if(!str || str=="") {
    write
      ("Usage:\n"+
       "tgrep pattern (file {file} | -r dir {dir})\n");
    return 1;
  }
  files=explode(str," ")-({""});
  if(sizeof(files)<1) {
    write("tgrep needs a pattern.\n");
    return 1;
  }
  pattern=files[0];
  files=files[1..];
  if(recursive=member(files, "-r")!=-1)
    files-=({"-r"});
  output=0;
  while((arg=member(files, "-o"))!=-1) {
    output=files[arg+1];
    files=files[0..arg-1]+files[arg+2..];
  }
  files=ExpandFileList(files);
  if(sizeof(files)<1) {
    write("tgrep needs at least one argument.\n");
    return 1;
  }
  if(recursive) {
    NewAction("GrepRec", files, ({pattern, output}));
  } else {
    NewAction("GrepFiles", files, ({pattern, output}));
  }
  return 1;
}

void GrepRec(mixed args)
{
  SECURE(0)
  map(args[0], "GrepRec2", ME, args[1]);
}

void GrepRec2(string file, mixed args)
{
  SECURE(0)
  args=copy(args);
  if(file_size(file)==-2) {
    NewAction("GrepRec",
	      map(filter(get_dir(file+"/*"), "IsOrdinary", ME),
			"AddPath", ME, file+"/"), args);
  } else {
    GrepIt(file, args);
  }
}

void GrepFiles(mixed args)
{
  SECURE(0)
  map(args[0], "GrepFiles2", ME, args[1]);
}

void GrepFiles2(string file, mixed args)
{
  SECURE(0)
  args=copy(args);
  if(file_size(file)==-2) {
    NewAction("GrepFiles",
	      filter(map(get_dir(file+"/*"),
				     "AddPath", ME, file+"/"),
			   "IsFile", ME), args);
  } else {
    GrepIt(file, args);
  }
}

void GrepIt(string file, mixed args)
{
  string pattern, text, *lines, *found;
  int    n, line, last;
  mixed  output;

  SECURE(0)
  pattern=args[0];
  output=args[1];
  text="";
  catch(text=read_file(file));
  if(!text || text=="") return;
  catch(found=regexp(lines=explode(text,"\n"), pattern));
  last=0;
  if(sizeof(found)) {
    Write(output, "\n*** "+file+" ***\n");
    for(n=0; n<sizeof(found); n++) {
      line=member(lines, found[n])+1;
      lines=lines[line..];
      last+=line;
      Write(output, last+":"+found[n]+"\n");
    }
  }
}



int DiskUsage(string str)
{
  string *files;
  int    arg, magic;
  mixed  output;

  SECURE(0)
  if(!str || str=="") {
    write
      ("Usage:\n"+
       "tdu [file|dir] [-o outfile]\n");
    return 1;
  }
  files=explode(str," ")-({""});
  output=0;
  while((arg=member(files, "-o"))!=-1) {
    output=files[arg+1];
    files=files[0..arg-1]+files[arg+2..];
  }
  files=ExpandFileList(files);
  if(sizeof(files)<1) {
    write("tdu needs at least one argument.\n");
    return 1;
  }
  magic=AddStore("DiskUsageWrite", ({output, "Total Bytes"}));
  PutStore(magic, 0);
  NewAction("DiskUsageRec", files, ({output, magic}), magic);
  return 1;
}

void DiskUsageRec(mixed args)
{
  SECURE(0)
  map(args[0], "DiskUsageRec2", ME, args[1]);
}

void DiskUsageRec2(string file, mixed args)
{
  int magic, oldmagic;

  SECURE(0)
  args=copy(args);
  if(file_size(file)==-2) {
    oldmagic=args[1];
    IncStore(oldmagic);
    magic=AddStore("DiskUsageDir", ({args[0], file, oldmagic}));
    PutStore(magic, 0);
    args[1]=magic;
    NewAction("DiskUsageRec",
	      map(filter(get_dir(file+"/*"), "IsOrdinary", ME),
			"AddPath", ME, file+"/"),
	      args, magic);
  } else {
    DiskUsageFile(file, args);
  }
}

void DiskUsageFile(string file, mixed *args)
{
  SECURE(0)
  PutStore(args[1], GetStore(args[1])+file_size(file));
}

void DiskUsageDir(int item, mixed args)
{
  SECURE(0)
  Write(args[0], item+": "+(string)(args[1])+"\n");
  PutStore(args[2], GetStore(args[2])+item);
  DecStore(args[2]);
  CheckStore(args[2]);
}

void DiskUsageWrite(int item, mixed args)
{
  SECURE(0)
  Write(args[0], item+": "+(string)(args[1])+"\n");
}

int ShowActions(string str)
{
  SECURE(0)
  if(str && str!="") {
    notify_fail("tshow does not like any arguments.\n");
    return 0;
  }
  write("Filetool: Currently there are "+sizeof(actions)+" active actions.\n");
  return 1;
}

int ListActions(string str)
{
  int n;
  SECURE(0)
  if(str && str!="") {
    notify_fail("tlist does not like any arguments.\n");
    return 0;
  }
  write("Filetool: Currently there are "+sizeof(actions)+" active actions:\n");
  for(n=0; n<sizeof(actions); write(mixed_to_string(actions[n++])+"\n"));
  return 1;
}

void RescueFile(string file)
{
  SECURE(0)
  if(file_size(file)>=0) {
    cp(file, NewName(save+"/"+GetName(file)));
  }
}

void MkDir(mixed args)
{
  string dir;
  mixed  output;

#ifdef DEBUG
  write_file("/log/taube/filecmds", "MkDir("+mixed_to_string(args)+")\n");
#endif
  SECURE(0)
  dir=args[0][0];
  output=args[1][0];
  Write(output, "Created directory "+dir+"\n");
  mkdir(dir);
}

void RmDir(mixed args)
{
  string dir;
  mixed  output;

#ifdef DEBUG
  write_file("/log/taube/filecmds", "RmDir("+mixed_to_string(args)+")\n");
#endif
  SECURE(0)
  dir=args[0][0];
  output=args[1][0];
  if(sizeof(args[1])<2)
    args[1]+=({0});
  else
    args[1][1]++;
  if(sizeof(get_dir(dir+"/*"))>2) {
    if(args[1][1]<MAXRMTRY)
      NewAction("RmDir", args[0], args[1]);
    else
      write("Could not remove directory "+dir+"\n");
  } else {
    Write(output, "Removed directory "+dir+"\n");
    rmdir(dir);
  }
}

static string *ExpandFileList(string *args)
{
  string *files, file;
  int    index;

  files=({});
  /* do NOT change the order of the files! */
  for(index=0; index<sizeof(args); index++) {
    file=(string)user->get_path(args[index]);
    if(IsWildcarded(file))
      files+=DeWildcard(file);
    else
      files+=({file});
  }
  return files;
}



static int IsWildcarded(string file)
{
  return (member(file, '*')!=-1 || member(file, '?')!=-1 ||
	  member(file, '[')!=-1 || member(file, ']')!=-1);
}


static string *DeWildcard(string file)
{
  string *files, path, name, mask;
  int     pos, index;

  pos=sizeof(file);
  while(file[--pos]!='/');
  path=file[0..pos];
  name=file[pos+1..];
  files=get_dir(path+"*");
  files-=({"."});
  files-=({".."});
  mask="^";
  for(pos=0; pos<sizeof(name); pos++) {
    switch(name[pos]) {
    case '?':
      mask+="."; break;
    case '*':
      mask+=".*"; break;
    case '.': case '$':
      mask+="\\";
    default:
      mask+=name[pos..pos];
    }
  }
  mask+="$";
  files=map(regexp(files, mask), "AddPath", ME, path);
  return files;
}


static string GetName(string file)
{
  int pos;

  pos=sizeof(file);
  while(file[--pos]!='/');
  return file[pos+1..];
}


static string NewName(string file)
{
  string name, new;
  int    pos;

  pos=sizeof(file);
  while(file[--pos]!='/');
  name=file[pos+1..];
  name=file[0..pos]+name[0..FNLENGTH];
  if(name[<1]!='.') name+=".";
  do {
    new=name+random(MAXNUM);
  } while(file_size(new)!=-1);
  return new;
}


static int IsFile(string file)
{
  return file_size(file)>=0;
}


static int IsOrdinary(string file)
{
  return file!="." && file!="..";
}


static string AddPath(string file, mixed path)
{
  return ((string)path)+file;
}



static void Write(mixed output, string text)
{
  if(output) catch(write_file(output, text));
  catch(write(text));
}



static varargs void NewAction(string func, string *files, mixed args, int magic)
{
  int pos;
#ifdef DEBUG
  write_file("/log/taube/filecmds", "NewAction("+mixed_to_string(func)+","+mixed_to_string(files)+","+mixed_to_string(args)+")\n");
#endif
  actions+=({({func, files, args, magic})});
  IncStore(magic);
  if(find_call_out("DoAction")==-1) {
    write("Filetool: starting:\n");
    call_out("DoAction", 0);
  }
}


void DoAction()
{
  string func, *files;
  mixed  args;
  int    cnt, magic, dec;

#ifdef DEBUG
  write_file("/log/taube/filecmds", "DoAction()\n");
#endif
  SECURE(0)
  call_out("DoAction", DELAY);
  cnt=COUNT;
  while(cnt>0 && sizeof(actions)) {
#ifdef DEBUG
    write_file("/log/taube/filecmds", "sizeof(actions)="+sizeof(actions)+"\n");
#endif
    func=actions[<1][0];
    files=actions[<1][1];
    args=actions[<1][2];
    magic=actions[<1][3];
    if(sizeof(files)>cnt) {
      actions[<1][1]=files[cnt..];
      files=files[0..cnt-1];
      dec=0;
    } else {
      actions=actions[0..<2];
      dec=1;
    }
    cnt-=sizeof(files);
    call_other(ME, func, ({files, args}));
    if(dec) {
      DecStore(magic);
      CheckStore(magic);
    }
  }
  if(!sizeof(actions)) {
    remove_call_out("DoAction");
    write("Filetool: ready.\n> ");
  }
}



static int AddStore(string func, mixed args)
{
  int magic;

  do {
    magic=random(999999999)+1;
  } while(member(stores, magic));
  stores += ([ magic: 0; 0; func; args ]);
  return magic;
}


static void RemoveStore(int magic)
{
  m_delete(stores, magic);
}

static int CheckStore(int magic)
{
  if (member(stores, magic) && stores[magic,1]) {
    call_other(this_object(), stores[magic,2], stores[magic,0], stores[magic,3]);
    RemoveStore(magic);
    return 1;
  }
  return 0;
}

static void IncStore(int magic)
{
  if (magic && member(stores, magic))
    stores[magic,1]++;
}

static void DecStore(int magic)
{
  if (magic && member(stores, magic))
    stores[magic,1]--;
}

static int GetStore(int magic)
{
  if (magic && member(stores, magic))
    return stores[magic,0];
  else
    return 0;
}

static int PutStore(int magic, mixed item)
{
  if (magic && member(stores, magic))
    return stores[magic,0] = item;
  else
    return 0;
}


static int check_previous()
{
  int n;
  object ob;

  for(n=0; ob=previous_object(n); n++) {
    if(getuid(ob)!=getuid()) return 0;
  }
  return 1;   /* everything is ok */
}
