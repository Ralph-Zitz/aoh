/*--------------------------------------------------------------------------
 * /secure/master.c  --  The interface between gamedriver and mudlib.
 *
 * Copyright (C) 1992-1997 by Nightfall.
 * Copyright (C) 1997-1998 by One Step Beyond.
 * All Rights Reserved.
 * Permission granted for non-commercial use.
 *--------------------------------------------------------------------------
 * The master is the gateway between the gamedriver and the mudlib to perform
 * actions with mudlib specific effects.
 * Calls to the master by the gamedriver have an automatic catch() in effect.
 *
 * Note that the master is loaded first of all objects. Thus you shouldn't
 * inherit an other object, nor is the compiler able to search include files
 * (read: they must be specified with full path).
 *
 * Amylaar: actually, you can inherit, but the file will be loaded then before
 * the master, which isn't good for most files.
 *--------------------------------------------------------------------------
 * The OSB master performs all the functions expected from the master.
 * To handle the file access rights, it directly reads the secure-save files
 * of the users using restore_object().
 * Also, the user level handling has its home here, using save_object().
 * To speed up operation, the most recent information requested is held
 * in a cache.
 *
 * Every day, the master executes the function daily_jobs(), in which
 * serveral daemons are (re)started.
 *--------------------------------------------------------------------------
 * 05-Aug-97  [Mateese]
 *    Prettyprinted and commented it, using the master_skeleton.c provided
 *    with the driver as template.
 * 21-Mar-98  [Mateese]
 *    Replaced the old wizard level handling by OSB's user level handling.
 */
#pragma no_inherit
#pragma strict_types
#pragma pedantic
#pragma save_types
#if __VERSION__ > "3.5.0"
#pragma rtt_checks
#endif
#include "/sys/secure/config.h"
#include "/sys/secure/kernel.h"
#include "/sys/secure/userinfo.h"
#include "/sys/secure/wizlevels.h"
#include "/sys/driver/wizlist.h"
#include "/sys/driver/driver_hook.h"
#include "/sys/driver/erq.h"
#include "/sys/driver/rtlimits.h"
#include "/sys/driver/object_info.h"
#include "/sys/driver/interactive_info.h"
#include "/sys/driver/debug_message.h"
#include UDP_HEADER

//---------------------------------------------------------------------------
#define MASTER_ERQ_HANDLER

#undef ACCESS_DEBUG
#undef DEBUG_GET_PATH
#undef DEBUG_MASTER

#ifndef SIMUL_EFUN_FILE
#define SIMUL_EFUN_FILE       KERNEL
#endif
#ifndef SPARE_SIMUL_EFUN_FILE
#define SPARE_SIMUL_EFUN_FILE SPAREKERNEL
#endif

#ifndef REMOVER
#define REMOVER "/secure/remover"
#endif

#define NULLID "no/ne"

//---------------------------------------------------------------------------
// Mudwho stuff
#if 0
#ifdef __HOST_NAME__
#if __EFUN_DEFINED__(send_udp)
#if __HOST_NAME__ == "nova" || __HOST_NAME__ == "quest"
#define MUDWHO 1
#endif
#endif
#endif

#ifdef MUDWHO
#define MUDWHO_SERVER "134.2.62.161"
#define MUDWHOPORT 6888
#if __HOST_NAME__ == MUDHOST
#define MUDWHO_NAME MUDNAME
#else
#define MUDWHO_NAME MUDNAME
#endif
#define MUDWHO_PASSWORD mudwho_password /* stored in /secure/mudwho-password */
#define MUDWHO_REFRESH_TIME 250
/* QUOTE_PERCENT needs new explode() to work right on leading/trailing '%' */
#define QUOTE_PERCENT(s) (implode(explode((s), "%"), "%%"))
nosave private string mudwho_ping;
nosave private mapping mudwho_info = ([]);
nosave private closure send_mudwho_info;
nosave private string mudwho_password;
#endif /* MUDWHO */
#undef MUDWHO
#endif

//---------------------------------------------------------------------------
// Internal variables

nosave mapping nuserlist = ([]);
/* Some statistical data about the userlist cleanup */
nosave int _nr_cleanups, _nr_processed, _nr_unprocessed;

#ifdef __HOST_NAME__
nosave int HookSet;  // True: hooks are initialized.
#endif

// Informations about the last object trying to load a virtual object.
// This comes in handy when tracking down can't-load errors.
// The variables are used by compile_object(), runtime_error() and log_error().
nosave string last_filename;
nosave string last_loader;
nosave string last_env;
nosave int    last_lived;

// Information of log_error() about time and logfile of the last error;
// used to reduce the amount of output.
nosave string *last_log_file;
nosave int last_log_time;

// Information of heart_beat_error() about time and logfile of the last error;
// used to reduce the amount of output.
nosave string *last_heart_file;
nosave int last_heart_time;

// Information of runtime_error() about time and logfile of the last error;
// used to reduce the amount of output.
nosave string *last_runt_file;
nosave int last_runt_time;

// Mutex of unwind_cmd_stack() against reentrant calls.
nosave int _mutex_unwind = 0;

// Statistical data from _reset_fun().
nosave int nrresets;
nosave int timereset;
nosave int rmincost, rmaxcost, rsumcost;

// Info from valid_write() about the last save .o file, for error messages.
nosave string last_save_fname;

#ifdef MASTER_ERQ_HANDLER
// Erq queue for pending sends
mixed * erq_queue;
#endif

//---------------------------------------------------------------------------
// Variables from the secure-save files.

string name, password, *domains, shell;
int level, promotion_age;
mixed elpromo;

// A short reference to all functions...
//---------------------------------------------------------------------------
//     Auxiliary functions
//
// Macro log_file(file,str)
// Macro USERNAME(x)
// Macro CAP_NAME(x)
// Macro TP
// Macro TI
// Macro PREV
//
// Simul-Efun-Macro FIND_PLAYER(pl)
// Simul-Efun-Macro FIND_LIVING(pl)
// Simul-Efun-Macro DTELL(pl,msg)
//
// string backtrace()
//   Construct a readable backtrace of the call stack and return it.
//
// string *full_path_array (string path, string user)
//   Normalize and split a filename into its parts and return it as array.
//   Also take care of the shortcuts (']', '~', '&') and relative files.
//
// string *full_path (string path, string user)
//   Normalize a filename.
//   Also take care of the shortcuts (']', '~', '&') and relative files.
//
// void _info_chef (string what)
//   Print a debug message to a user.
//
// object load_obj (mixed obj)
//   Safely load an object.
//
// static void unwind_cmd_stack(object pl)
//   Unwind the command giver stack, thus performing all pending post-cmd-calls.
//
//---------------------------------------------------------------------------
//     Hooks
//
// static string _include_dirs_hook (string include_name, string current_file)
//   Return the full pathname of an include file.
//
// static void _move_hook_fun (object item, object dest)
//   Move object <item> into object <dest>.
//
// static mixed _load_uids_fun (mixed object_name, object prev)
//   Return the uids given to an object just loaded.
//
// static mixed _clone_uids_fun (object blueprint, string new_name, object prev)
//   Return the uids given to an object just cloned.
//
// static int _create_fun (int clone, object prev, object this)
//   Handle the creation of an object.
//
// static int _reset_fun (object prev, object this)
//   Handle the reset of an object.
//
// static mixed _modify_cmd_fun (string cmd, object player)
//   Preprocess a command given to a player.
//
// static string _notify_fail_fun (string cmd)
//   Handle the failure of a command processing.
//
//---------------------------------------------------------------------------
//     Initialisation
//
// void inaugurate_master (int arg)
//   Perform mudlib specific setup of the master.
//
// string get_master_uid ()
//   Return the string to be used as uid (and -euid) of a (re)loaded master.
//
// void flag (string arg)
//   Evaluate an argument given as option '-f' to the driver.
//
// string *epilog (int eflag)
//   Perform final actions before opening the game to players.
//
// void preload (string file)
//   Preload a given object.
//
// void external_master_reload ()
//   Called after a reload of the master on external request.
//
// void reactivate_destructed_master (int removed)
//   Reactivate a formerly destructed master.
//
// string|string * get_simul_efun ()
//   Load the simul_efun object(s) and return one or more paths of it.
//
//---------------------------------------------------------------------------
//     Cron section
//
// void daily_jobs (int just_loaded)
//   Do jobs once a day.
//
// void send_mudwho_info()
//   Update the mudwho server.
//
// void adjust_mudwho(object ob)
//   Adjust the mudwho server.
//
// static void wiz_decay()
//   Decay the wizlist info.
//
//---------------------------------------------------------------------------
//     Handling of player connections
//
// object connect ()
//   Handle the request for a new connection.
//
// void disconnect (object obj)
//   Handle the loss of an IP connection.
//
// void remove_player (object player)
//   Remove a player object from the game.
//
// void stale_erq (closure callback)
//   Notify the loss of the erq demon.
//
//---------------------------------------------------------------------------
//     Runtime Support
//
// object compile_object (string filename)
//   Compile an virtual object.
//
// string get_wiz_name (object|string file)
//   Return the author of a file.
//
// string printf_obj_name (object obj)  (3.2.6 and later)
// string object_name (object obj)      (3.2.5 and before)
//   Return a printable name for an object.
//
// mixed prepare_destruct (object obj)
//   Prepare the destruction of the given object.
//
// void quota_demon (void)
//   Handle quotas in times of memory shortage.
//
// void receive_udp (string host, string msg, int port)
//   Handle a received UDP message.
//
// void slow_shut_down (int minutes)
//   Schedule a shutdown for the near future.
//
// void notify_shutdown ()
//   Notify the master about an immediate shutdown.
//
//---------------------------------------------------------------------------
//     Error Handling
//
// string *error_logfile (string file)
//   Return appropriate names (for an error logfile).
//
// void dangling_lfun_closure ()
//   Handle a dangling lfun-closure.
//
// void log_error (string file, string err, int warn, int line)
//   Announce a compiler-time error.
//
// mixed heart_beat_error (object culprit, string err,
//                         string prg, string curobj, int line)
//   Announce an error in the heart_beat() function.
//
// void runtime_error (string err, string prg, string curobj, int line)
//   Announce a runtime error.
//
//---------------------------------------------------------------------------
//     Security and Permissions
//
// int privilege_violation (string op, mixed who, mixed arg, mixed arg2)
//   Validate the execution of a privileged operation.
//
// int query_allow_shadow (object victim)
//   Validate a shadowing.
//
// int query_player_level (string what)
//   Check if the player is of high enough level for several things.
//
// int valid_exec (string name, object ob, object obfrom)
//   Validate the rebinding of an IP connection by usage of efun exec().
//
// int valid_query_snoop (object obj)
//   Validate if the snoopers of an object may be revealed by usage of the
//   efun query_snoop().
//
// int valid_snoop (object snoopee, object snooper)
//   Validate the start/stop of a snoop.
//
//---------------------------------------------------------------------------
//     Userids and depending Security
//
// string get_bb_uid()
//   Return the string to be used as backbone-uid.
//
// int valid_seteuid (object obj, string neweuid)
//   Validate the change of an objects euid by efun seteuid().
//
// int|string valid_read  (string path, string euid, string fun, object caller)
// int|string valid_write (string path, string euid, string fun, object caller)
//   Validate a reading/writing file operation.
//
//---------------------------------------------------------------------------
//     ed() Support
//
// string make_path_absolute (string str)
//   Absolutize a relative filename given to the editor.
//
// int save_ed_setup (object who, int code)
//   Save individual settings of ed for a wizard.
//
// int retrieve_ed_setup (object who)
//   Retrieve individual settings of ed for a wizard.
//
// string get_ed_buffer_save_file_name (string file)
//   Return a filename for the ed buffer to be saved into.
//
//---------------------------------------------------------------------------
//     parse_command() Support
//
// string *parse_command_id_list ()
//   Return generic singular ids.
//
// string *parse_command_plural_id_list ()
//   Return generic plural ids.
//
// string *parse_command_adjectiv_id_list ()
//   Return generic adjective ids.
//
// string *parse_command_prepos_list ()
//   Return common prepositions.
//
// string parse_command_all_word()
//   Return the one(!) 'all' word.
//
//---------------------------------------------------------------------------
//     FTP Support
//
// string dtime(int wann)
//   Return the time <wann> in a Unix-ls-like format.
//
// string show_dir (string wo, string user)
//   Simulate a Unix-ls.
//
// void FtpAccess(string host, string message, int port)
//   Handle the interface to the NaseFTPD.
//
//---------------------------------------------------------------------------
//     ERQ Support (if MASTER_ERQ_HANDLER is defined)
//
// void erq_callback( int * data, int len, closure orig_cb )
//   Call the original callback if it is still valid.
//
// int erq_flush_queue()
//   Send all data in erq_queue to the erq till it does not accept any more.
//
// int erq_send( int req, mixed data, closure cb )
//   Send data to the erq. If the erq does not accept any more, buffer the
//   data in the erq_queue.
//
//---------------------------------------------------------------------------
//     Userinfo handling
//
// int _sizeof_userlist()
//   Return the number of entries in the Userinfo cache.
//
// mixed * _userlist_names()
//   Return the cached names in the Userinfo cache.
//
// mixed * _cleanup_data()
//   Return the statistical data about the cache cleanups.
//
// void _cleanup_uinfo()
//   Clean the Userinfo Cache, and handle idling users.
//
// mixed * find_userinfo (string user)
//   Load the userinfo for <user> and return it.
//
// void expunge_userinfo (string user)
//   Remove the cached userinformation for a given user from the cache.
//
// static void save_userinfo (mixed* userinfo)
//   Save the given userinformation.
//
// int CheckPasswd(string user, string pw)
//   Check a user/password combination.
//
// string delete_password (string user)
//   Delete the password of the given user.
//
// int update_password (string old,string new)
//   Change the password of a player.
//
//---------------------------------------------------------------------------
//     Wizardstatus and Wizardtype handling.
//
// int change_user_level (string user, int level)
//   Change the user level of a user.
//
// int query_user_level (mixed obj)
//   Return the user level of an object
//
// string create_wizard(string owner, mixed domain, object caller)
//   Promote a wizard and create his environment.
//
//---------------------------------------------------------------------------
//     Domain and Project membership handling.
//
// int domain_member (string user, string domain)
//   Test if a user is a member of a specific domain/project.
//
// int domain_master (string user, string domain)
//   Test if a user is lord of a specific domain/project.
//
// int domain_vice (string user, string domain)
//   Test if a user is vice lord of a specific domain/project.
//
// varargs int add_domain_member (string user, string dom, int isproject)
//   Add a new member to a domain or project.
//
// int remove_domain_member(string user, string dom)
//   Remove a member from a domain or project.
//
// varargs int add_domain_vice (string user,string dom, int isproject)
//   Add a new vicelord to a domain or project.
//
// int remove_domain_vice (string user,string dom)
//   Remove a vice lord from a domain or project.
//
// varargs int add_domain_master(string user,string dom, int isproject)
//   Add a new lord to a domain or project.
//
// int remove_domain_master(string user,string dom)
//   Remove a lord from a domain or project.
//
//---------------------------------------------------------------------------


//===========================================================================
//   Forward Declarations.
//===========================================================================

int query_user_level(mixed obj);
#if __VERSION__ > "3.5.0"
string get_wiz_name (object|string file);
#else
string get_wiz_name (mixed file);
#endif
string get_bb_uid();
mixed * find_userinfo(string user);
void FtpAccess(string host, string message, int port);
int domain_member(string user, string domain);
int domain_master(string user, string domain);
int domain_vice (string user, string domain);
int CheckPasswd(string user, string pw);
varargs int add_domain_member(string user,string dom, int isproject);
varargs int add_domain_master(string user,string dom, int isproject);
void erq_stop();

//===========================================================================
//  Auxiliary functions
//
// These functions do useful things for the other functions and are not
// meant to be called from outside.
//===========================================================================

//---------------------------------------------------------------------------
// Macros

#define log_file(file,str)  catch(write_file("/log/"+file, str))
#define USERNAME(x)         getuid(x)
#define CAP_NAME(x)         capitalize(USERNAME(x))
#define TP                  efun::this_player()
#define TI                  efun::this_interactive()
#define TO                  efun::this_object()
#define PREV                efun::previous_object()

//---------------------------------------------------------------------------
// Simul-Efuns do not exist at compile-time, so we resolve them
// at runtime.

#define FIND_PLAYER(pl)  funcall(symbol_function('find_player),pl)     /*'*/
#define FIND_LIVING(pl)  funcall(symbol_function('find_living),pl)     /*'*/
#define DTELL(msg)       funcall(symbol_function('_info_chef),msg)     /*'*/
#define SETEUID(ob)      funcall(symbol_function('seteuid),ob)         /*'*/
#define SHADOW(ob,flag)  funcall(symbol_function('shadow),ob,flag)     /*'*/
#define QOI(ob)          funcall(symbol_function('query_once_interactive),ob) /*'*/
#define QIDLE(ob)        funcall(symbol_function('query_idle),ob)      /*'*/
#define QSHADOWING(ob)   funcall(symbol_function('query_shadowing),ob) /*'*/
//---------------------------------------------------------------------------
string backtrace()

// Construct a readable backtrace of the call stack and return it.

{
  int i;
  string str = "";
  object ob;

  if (query_verb()) str += "  Command '"+query_verb()+"'.\n";
  else str += "\n";
  for (i=0; ob = previous_object(i); i++) {
    if (!i) str += "   PrevObj: \t";
    else    str += "            \t";
    if (!ob) { str += "?\n"; break; }
    else {
      str += object_name(ob)+" ["+getuid(ob)+"]";
      if (!QOI(ob))
        str += "\n";
      else if (!interactive(ob))
        str += " netdead.\n";
      else if (QIDLE(ob))
        str += " idle "+(QIDLE(ob))+" sec.\n";
      else
        str += " interactive.\n";
    }
  }
  str += "   Player: \t";
  if (!this_player()) str += "?\n";
  else str += object_name(this_player())+" ["+getuid(this_player())+"]\n";
  str += "   Interactive: ";
  if (!this_interactive()) str += "?\n";
  else str += object_name(this_interactive())+" ["+getuid(this_interactive())
              +"]\n";
  return str;
}


//---------------------------------------------------------------------------
string *full_path_array (string path, string user)

// Normalize and split a filename into its parts and return it as array.
// Also take care of the shortcuts (']', '~', '&') and relative files.
//
// Argument:
//   path: Filename to work on
//   user: the current user (for relative paths)
//
// Result:
//   The <path> absolutized and split into its parts by the "/".

{
  string *strs;
  int p;

#define PATH_ARRAY(x) (efun::explode(x, "/")-({"","."}))

  if(!path || path == "")
    path=" ";
  switch(path[0]) {
  case '/':
    if(sizeof(path) == 1) return ({});
    strs=PATH_ARRAY(path);
    break;
  case '+':
    if(user) {
      if(!path[1]) return ({DDIR});
      if(path[1]=='/') {
        strs=({DDIR,user})+PATH_ARRAY(path[2..]);
        break;
      }
    }
    else if (path[1] == '/')
      log_file("MASTERLOG", "full_path_array('"+path+"', 0)\n");
    strs=({DDIR})+PATH_ARRAY(path[1..]);
    break;
  case '&':
    if(user) {
      if(!path[1]) return ({PDIR});
      if(path[1]=='/') {
        strs=({PDIR,user})+PATH_ARRAY(path[2..]);
        break;
      }
    }
    else if (path[1] == '/')
      log_file("MASTERLOG", "full_path_array('"+path+"', 0)\n");
    strs=({PDIR})+PATH_ARRAY(path[1..]);
    break;
  case '~':
    if(user) {
      if(!path[1]) return ({WDIR,user});
      if(path[1]=='/') {
        strs=({WDIR,user})+PATH_ARRAY(path[2..]);
        break;
      }
    }
    else if (path[1] == '/')
      log_file("MASTERLOG", "full_path_array('"+path+"', 0)\n");
    strs=({WDIR})+PATH_ARRAY(path[1..]);
    break;
  default:
    if(path == " ") path = "";
    if(user && TP && USERNAME(TP) == user)
      strs=PATH_ARRAY(({string})TP->QueryCurrentDir()+"/"+path);
    else
      strs=PATH_ARRAY(path);
  }
  while((p=member(strs, "..")) != -1)
    strs = strs[0..p-2]+strs[p+1..sizeof(strs)-1];
  return strs;

#undef PATH_ARRAY
}

//---------------------------------------------------------------------------
string full_path (string path, string user)

// Normalize a filename.
// This takes care of the shortcuts (']', '~', '&') and relative files.
//
// Argument:
//   path: Filename to work on
//   user: the current user (for relative paths)
//
// Result:
//   The <path> absolutized.

{
  return "/"+implode(full_path_array(path, user), "/");
}


//---------------------------------------------------------------------------
void _info_chef (string what)

// Print a debug message to a user.
//
// Argument:
//   what: the message to print.
//
// If Nostradamus is online, he is told the string <what>.

{
  string rc;
  object chef;
  rc = "[[secure/master, "+what+"]]:\n   this: "
       +(TO?object_name(TO):"0")
       +", uid '"+getuid(TO)
       +"', euid '"+geteuid(TO)
       +"', prev: "+(PREV?object_name(PREV):"0")
       +".\n";
  if (chef = FIND_PLAYER("nostradamus"))
    tell_object(chef, rc);
}

//---------------------------------------------------------------------------
object load_obj (mixed obj)

// Safely load an object.
//
// Argument:
//   obj: the filename of the object to load.
//
// Result:
//   The loaded object.
//
// The function takes care that the object is loadable and gets no euid
// if it is not a root object. This is to prevent security breaches.

{
  string cf;
  if (objectp(obj)) return obj;
  if (!stringp(obj)) return 0;
  if (!find_object(obj)) {
    cf = get_wiz_name(obj);
    if (!cf)
      return 0;
    if (cf != ROOTID)
      SETEUID(NULLID);
    else
      SETEUID(ROOTID);
    catch(call_other(obj, "???"));
    SETEUID(ROOTID);
  }
  return find_object(obj);
}


//---------------------------------------------------------------------------
static void unwind_cmd_stack(object pl)

// Unwind the command giver stack, thus performing all pending post-cmd-calls.
// Auxiliarey function to heart_beat_error() and runtime_error().
//
// Argument:
//   pl : The command_giver to start with.
//
// The function will attempt to unwind the command giver stack, and then
// walk through the caller chain.

{
  object prev_pl;
  int i;

  if (_mutex_unwind) return;
  _mutex_unwind++;

  while (pl)
  {
    prev_pl = ({object})pl->QueryCmdData("PreviousPlayer");
    catch(pl->PopCmdData(1));
    pl = prev_pl;
  }
  for (i = 0; pl = previous_object(i); i++)
    if (living(pl))
      catch(pl->EmptyCmdDataStack(1));

  _mutex_unwind--;
}


//===========================================================================
//  Hooks
//
// These functions are set as driver hooks to perform low-level functions
// for the mud.
//===========================================================================

//---------------------------------------------------------------------------
static string _include_dirs_hook (string include_name, string current_file)

// Return the full pathname of an include file.
//
// Argument:
//   include_name: the name given in the #include <...> directive.
//   current_file: the filename of the file compiled.
//
// Result:
//   The full pathname of the include file.
//   0 if no such file exists.
//
// The function looks in /sys for /sys/<include_name>.
// If that file doesn't exist, and <current_file> is from a domain or project,
// the function also checks /p/<project>/sys/<include_name> (analog for
// domains).

{
  string name, part;
  int pos;

  name = "sys/"+include_name;
  if (file_size("/"+name) >= 0)
    return name;
  if ("d/" == current_file[0..1] || "p/" == current_file[0..1])
  {
    pos = member(current_file[2..], '/');
    if (pos > 0)
    {
      name = current_file[0..pos+2]+"sys/"+include_name;
      if (file_size("/"+name) >= 0)
        return name;
    }
  }
  return 0;
}


//---------------------------------------------------------------------------
static void _move_hook_fun (object item, object dest)

// Move object <item> into object <dest>.
//
// Argument:
//   item: the object to be moved.
//   dest: the destination for the object.
//
// The function performs all the checks for a valid move (item exists,
// destination exists, destination is not current environment, etc).
// In addition, it implements the init() protocol needed for add_action()
// to work.

{
  object *others;
  int i;

  efun::set_environment(item, dest);
  if (living(item)) {
    efun::set_this_player(item);
    dest->init();
    if (!item)
      raise_error(sprintf("%O->init() destructed moved item\n", dest));
    if (environment(item) != dest)
      return;
  }
  others = all_inventory(dest) - ({item});
  for (i = 0; i < sizeof(others); i++)
  {
    if (living(others[i])) {
      efun::set_this_player(others[i]);
      item->init();
    }
    if (!item)
      raise_error(sprintf("item->init() for %O (#%d) destructed moved item\n", others[i], i));
  }
  if (living(item)) {
    efun::set_this_player(item);
    filter_objects(others, "init");
  }
  if (living(dest) && item) {
    efun::set_this_player(dest);
    item->init();
  }
}

//---------------------------------------------------------------------------
static mixed _load_uids_fun (mixed object_name, object prev)

// Return the uids given to an object just loaded.
//
// Argument:
//   object_name: name of the object loaded
//   prev       : loading object.
//
// Result:
//   The uids to give. See /doc/concepts/native for a full explanation.

{
  string creator_name;

  creator_name = get_wiz_name(object_name);
  if (prev && creator_name == getuid(prev))
    return geteuid(prev);
  if (prev && creator_name == get_bb_uid())
    return geteuid(prev);
  return ({ creator_name, 1 });
}

//---------------------------------------------------------------------------
static mixed _clone_uids_fun (object blueprint, string new_name, object prev)

// Return the uids given to an object just cloned.
//
// Argument:
//   blueprint: the blueprint cloned.
//   new_name : name of the object cloned
//   prev     : cloning object.
//
// Result:
//   The uids to give. See /doc/concepts/native for a full explanation.

{
  string creator_name;

  creator_name = get_wiz_name(new_name);
  if (prev && getuid(prev) == creator_name)
    return geteuid(prev);
  if (prev && get_bb_uid() == creator_name)
    return geteuid(prev);
  return ({ creator_name, 1 });
}

//---------------------------------------------------------------------------
static int _create_fun (int clone, object prev, object this)

// Handle the creation of an object.
//
// Argument:
//   clone: true if the object was cloned.
//   prev:  object causing the creation
//   this:  created object.
//
// Result:
//   The reset interval to use for the object, 0 for the default interval.
//   This will be the value of P_RESET_INTERVAL of the object.
//
// The function tries to call create_clone() resp. create_obj() in the
// created object first. If that function does not exist, create() is called.
// If the object does not selfdestruct, it's property P_LASTRESET is set
// to the current time and the function returns the value of P_RESET_INTERVAL.

{
  closure cl;
  string fun;

  // Using symbol_function() here is tempting, but leads to problems with
  // the this_object()/previous_object() relation on the actual call. *sigh*
  switch (clone) {
    case 0: fun = "create_obj"; break;
    case 1: fun = "create_clone"; break;
    case 2: fun = "create_lwobject"; break;
    default: fun = "create";
  }
//  fun = clone ? "create_clone" : "create_obj";
  if (!function_exists(fun, this))
    fun = "create";

  if (prev)
  {
    // The call to fun must be in from the prev object, so bind the call-lambda
    // to it.
    cl = bind_lambda(unbound_lambda(0, ({ #'call_other, this, fun })), prev); /*'*/
    funcall(cl);
  }
  else
    call_other(this, fun);

  if (!this)  // don't stumble over self-destructing objects
    return 0;

  this->Set(SP_LAST_RESET, time());
  return ({int})this->Query(SP_RESET_INTERVAL);
}

//---------------------------------------------------------------------------
static int _reset_fun (object prev, object this)

// Handle the reset of an object.
//
// Argument:
//   prev:  object causing the reset
//   this:  reset object.
//
// Result:
//   The new reset interval to use for the object, 0 for the default interval.
//   This will be the value of P_RESET_INTERVAL of the object.
//
// The function mainly calls reset() in the given object, and does some
// statistical bookkeeping (results are written to /log/PROFILE).
// If the object does not selfdestruct, it's property P_LASTRESET is set
// to the current time and the function returns the value of P_RESET_INTERVAL.

{
  closure cl;
  int maxcost;

//debug_message(sprintf("DEBUG: %s reset: prev %O, this %O\n", ctime(), prev, this));
  // TODO: Extend the profiling onto create() and call_out() and whatever
  // TODO: Introduce a hb in master, taking care of the flushing.
  if (time() != timereset)
  {
    if (timereset && 0)
    {
      log_file("PROFILE", sprintf("%s: RESET-STAT: %d objs %d ticks: %d..%d..%d\n"
				 , ctime(timereset), nrresets, rsumcost
				 , rmincost, rsumcost / (nrresets || 1), rmaxcost
				 ));
    }
    timereset = time();
    rmincost = 1<<30;
    rmaxcost = 0;
    rsumcost = 0;
    nrresets = 0;
  }

  maxcost = get_eval_cost();
  if (prev && this)
  {
    cl = bind_lambda(unbound_lambda(0, ({#'call_other, this, "reset", 0 })), prev); /*'*/
    funcall(cl);
  }
  else if (this)
    this->reset(0);

  if (!this)  // don't stumble over self-destructing objects
    return 0;

  maxcost = maxcost - get_eval_cost();
  if (maxcost > 5000 && 0)
    log_file("PROFILE", sprintf("%s: RESET %6d %O\n", ctime(), maxcost, this));

  if (maxcost > rmaxcost)
    rmaxcost = maxcost;
  if (maxcost < rmincost)
    rmincost = maxcost;
  rsumcost += maxcost;
  nrresets++;

  this->Set(SP_LAST_RESET, time());
  return ({int})this->Query(SP_RESET_INTERVAL);
}

//---------------------------------------------------------------------------
static mixed _modify_cmd_fun (string cmd, object player)

// Preprocess a command given to a player.
//
// Argument:
//   cmd   : the command to process
//   player: the player to whom the command is applied.
//
// Result:
//   The result determines with what command string the gamedriver will
//   continue the command-process (i.e. add_action lookup).
//     A string with the 'real' command if <cmd> is an abbreviation or other.
//     0 to use the given <cmd> as command.
//     1 to stop the command processing.
//
// Besides the usual 'n'->'north' expansion, the function mainly helps
// implementing the various pre/post command facilities OSB offers (Command-
// lifetime variables, message buffers).

{
  if (caller_stack_depth() < 2)
  {
    // We need one stacked command level to be able to run the
    // registered command functions.
    // Damn! :-(
    if (player)
      set_this_object(player);
    if (player)
      player->command_me(cmd);
    // player might got destructed in the command_me(), e.g. by cmd == 'quit'
    if (player)
      player->FlushReceive();
/*
    if (player && QOI(player))
      player->PrintPrompt();
*/
    return 1;
  }
  player->PushCmdData();

  if ( ({int})player->QueryEditing() ) {
    if ( ! ({int})player->query_no_buffering() ) {
      player->set_no_buffering( 1 );
      /* we need an array for the cmd-fun, just the string does not work */
      player->RegisterCmdFun( ({ player, "set_no_buffering", 0 }) );
    }
  }

  switch (cmd) {
    case "e"  : return "east";
    case "w"  : return "west";
    case "n"  : return "north";
    case "s"  : return "south";
    case "ne" : return "northeast";
    case "nw" : return "northwest";
    case "se" : return "southeast";
    case "sw" : return "southwest";
    case "d"  : return "down";
    case "u"  : return "up";
  }
  return cmd;
}

//---------------------------------------------------------------------------
static string _notify_fail_fun (string cmd, object command_giver)

// Handle the failure of a command processing.
//
// Argument:
//   cmd: the command which couldn't be matched.
//
// Result:
//   The error string to print to the player.
//
// The function redirects the call to querying the "NotifyFail" Commandvariable
// in the player, implementing proper stacked commands.
// This also is part of the implementation of the post-command facilities.

{
  mixed rc;
  rc = (({mapping})this_player()->PopCmdData() || ([]))["NotifyFail"];
  if (pointerp(rc))
  {
    rc = funcall(rc[0]);
    if (!stringp(rc))
        raise_error(sprintf("Invalid notify_fail message '%O' for %O\n", rc, this_player()));
    return sprintf("%-78=s", rc);
  }
  return "What?\n";
}

protected void _telnet_negot_fun( int action, int option, int * opts, object prev )
{
  if (!prev)
	  return;
  prev->got_telnet(action, option, opts);
}

protected void _no_echo_fun (int flag, object user)
{
  if (!user)
  	return;
  user->set_noecho( flag );
}

//===========================================================================
//  Initialisation
//
// These functions are called after (re)loading the master to establish the
// most basic operation parameters.
//
// The initialisation of LPMud on startup follows this schedule:
//   - The gamedriver evaluates the commandline options and initializes
//     itself.
//   - The master is loaded, but since the driverhooks are not set yet,
//     no standard initialisation lfun is called.
//   - get_master_uid() is called. If the result is valid, it becomes the
//     masters uid and euid.
//   - inaugurate_master() is called.
//   - flag() is called for each given '-f' commandline option.
//   - get_simul_efun() is called.
//   - the WIZLIST is read in.
//   - epilog() is called. If it returns an array of strings, they are given
//     one at a time as argument to preload().
//     Traditionally, these strings are the filenames of the objects to
//     preload, which preload() then does.
//   - The gamedriver sets up the IP communication and enters the backend
//     loop.
//
// If the master is reloaded during the game, this actions are taken:
//   - The master is loaded, and its initialisation lfun is called according
//     to the settings of the driverhooks (if set).
//   - Any auto-include string and all driverhooks are cleared.
//   - get_master_uid() is called. If the result is valid, it becomes the
//     masters uid and euid.
//   - inaugurate_master() is called.
//
// If the master was destructed, but couldn't be reloaded, the old
// master object could be reactivated. In that case:
//   - reactivate_destructed_master() is called.
//   - inaugurate_master() is called.
//===========================================================================

//---------------------------------------------------------------------------
// Initialization of the master object.
//
// As the lfuns which are called to initialize objects after a load are
// defined through driver hooks, and these hooks are cleared prior to
// a master (re)load, the first function called is inaugurate_master().
// Anyway it's not very sensible to do anything earlier as the master is
// not recognized as such at that time, and so a number of (important) things
// would not work.
//
// Which lfun is called during runtime to reset the master is also depending
// on the driverhook settings. Arbitrary actions may be done on a reset.

//---------------------------------------------------------------------------
void inaugurate_master (int arg)

// Perform mudlib specific setup of the master.
//
// Argument:
//   arg: 0 if the mud just started.
//        1 if the master is reactivated destructed one.
//        2 if the master is a reactivated destructed one, which lost all
//             variables.
//        3 if the master was just reloaded.
//
// This function is called whenever the master becomes fully operational
// after (re)loading (it is now recognized as _the_ master).
// This doesn't imply that the game is up and running.
//
// The function sets up the driver hooks, on necessity initializes the
// mudwho, and sets up the WizInfo data.
// In addition, it schedules a call to daily_info() to be performed as soon
// as the call_out handling is active.

{

  if (!arg) // Mud started - one-time inits
  {
    mixed *info;
    int   i;
    string abc;

    set_extra_wizinfo(0, info = allocate(BACKBONE_WIZINFO_SIZE));
#ifdef TIME_DELTA
    info[WI_BOOTTIME] = time()+TIME_DELTA;
#else
    info[WI_BOOTTIME] = time();
#endif
    // Test the existance of several directories which are usually
    // missing from distributions, and recreate them if necessary.
#define NEWDIR(x) if (file_size(x) == -1) mkdir(x)
    NEWDIR("/log");
    NEWDIR("/log/ERR");
    NEWDIR("/save");
    NEWDIR(SAVEPATH);
    NEWDIR(DAEMONSAVEPATH);
    NEWDIR(SECURESAVEPATH);
    NEWDIR(WPATH);
    abc = "abcdefghijklmnopqrstuvwxyz";
    for (i = 0; i < sizeof(abc); i++)
    {
      NEWDIR(SAVEPATH+abc[i..i]);
      NEWDIR(MAILPATH+abc[i..i]);
      NEWDIR(SECURESAVEPATH+abc[i..i]);
    }
#undef NEWDIR
  }

  if (!nuserlist)
    nuserlist = ([]);
#if 0
#ifdef MUDWHO
  MUDWHO_PASSWORD = read_file(WPATH "/ze/secure/mudwho-password", 3, 1)[0..<2];
  send_mudwho_info =
    lambda(({'key, 'info}), ({#'send_udp, MUDWHO_SERVER, MUDWHOPORT, 'info})); /*'*/
  if (!arg) {
    send_udp(MUDWHO_SERVER, MUDWHOPORT,
             sprintf("U\t%.20s\t%.20s\t%.20s\t%:010d\t0\t%.25s",
                     MUDWHO_NAME, MUDWHO_PASSWORD, MUDWHO_NAME,time()
                    , __VERSION__[1..]
                    )
            );
    get_extra_wizinfo(0)[WI_MUDWHO] = mudwho_info;
  } else {
    mudwho_info = get_extra_wizinfo(0)[WI_MUDWHO];
  }
  mudwho_ping = sprintf("M\t%.20s\t%.20s\t%.20s\t%%:010d\t0\t%.25s",
                        QUOTE_PERCENT(MUDWHO_NAME),
                        QUOTE_PERCENT(MUDWHO_PASSWORD),
                        QUOTE_PERCENT(MUDWHO_NAME),
                        QUOTE_PERCENT(__VERSION__[1..])
                       );
  if (find_call_out("send_mudwho_info") < 0)
    call_out("send_mudwho_info", MUDWHO_REFRESH_TIME);
#endif /* MUDWHO */
#endif
  // Wizlist simulation
  if (find_call_out("wiz_decay") < 0)
    call_out("wiz_decay", 3600);

  if (!HookSet)
  {
#if __VERSION__ > "3.5.2"
    set_driver_hook(H_FILE_ENCODING, "UTF-8");
    configure_interactive(0, IC_ENCODING, "UTF-8");
#endif
    set_driver_hook(H_MOVE_OBJECT0,
      unbound_lambda( ({'item, 'dest}), ({#',,                                /*'*/
        ({#'?, ({#'!=, 'item, ({#'this_object})}),                            /*'*/
          ({#'raise_error,                                                    /*'*/
            "Illegal to move other object than this_object()\n"}) }),
        ({#'_move_hook_fun, 'item, 'dest }) })                                /*'*/
      )
    );
    set_driver_hook(H_LOAD_UIDS,
      unbound_lambda( ({'object_name}),                                       /*'*/
        ({ #'_load_uids_fun, 'object_name, ({#'previous_object}) })           /*'*/
      )
    );
    set_driver_hook(H_LWOBJECT_UIDS,
      unbound_lambda( ({'object_name}),                                       /*'*/
        ({ #'_load_uids_fun, 'object_name, ({#'previous_object}) })           /*'*/
      )
    );
    set_driver_hook(H_CLONE_UIDS,
      unbound_lambda( ({ 'blueprint, 'new_name}),                             /*'*/
        ({ #'_clone_uids_fun, 'blueprint, 'new_name, ({#'previous_object}) }) /*'*/
      )
    );
    set_driver_hook(H_CREATE_SUPER, "create_inh");
    // It needs unbound lambdas here to be able to determine this_object() and
    // previous_object() at the time of the call. Unfortunately the lambdas are
    // bound to the newly created object which makes some things more difficult.
    set_driver_hook(H_CREATE_OB,
      unbound_lambda( 0,
        ({#'_create_fun, 0, ({#'previous_object}), ({#'this_object}) })       /*'*/
      )
    );
    set_driver_hook(H_CREATE_CLONE,
      unbound_lambda( 0,
        ({#'_create_fun, 1, ({#'previous_object}), ({#'this_object}) })       /*'*/
      )
    );
    set_driver_hook(H_CREATE_LWOBJECT,
      unbound_lambda( 0,
        ({#'_create_fun, 2, ({#'previous_object}), ({#'this_object}) })       /*'*/
      )
    );
    set_driver_hook(H_RESET,
      unbound_lambda( 0,
        ({#'_reset_fun, ({#'previous_object}), ({#'this_object}) })           /*'*/
      )
    );
    set_driver_hook(H_CLEAN_UP,     "clean_up");
    set_driver_hook(H_MODIFY_COMMAND,
      unbound_lambda( ({ 'command_string, 'player }),                         /*'*/
        ({ #'_modify_cmd_fun, 'command_string, 'player })                     /*'*/
      )
    );
    set_driver_hook(H_MODIFY_COMMAND_FNAME, "modify_command");
    set_driver_hook(H_NOTIFY_FAIL,
      unbound_lambda( ({ 'command_string, 'command_giver }),                  /*'*/
        ({ #'_notify_fail_fun, 'command_string, 'command_giver })             /*'*/
      )
    );
    set_driver_hook(H_PRINT_PROMPT, "print_prompt");
    set_driver_hook(H_DEFAULT_PROMPT, "> ");
    set_driver_hook(H_TELNET_NEG,
      unbound_lambda( ({ 'action, 'option, 'opts }),                          /*'*/
	      ({ #'_telnet_negot_fun, 'action, 'option, 'opts, ({#'this_player}) }) /*'*/
      )
    );
    set_driver_hook(H_NOECHO,
      unbound_lambda( ({ 'flag, 'user }),                                     /*'*/
	      ({#'_no_echo_fun, 'flag, 'user })                                     /*'*/
      )
    );
#if 0
    set_driver_hook(H_SEND_NOTIFY_FAIL,"receive_notify_fail");
#endif
    set_driver_hook(H_ERQ_STOP, #'erq_stop);                                  /*'*/
#if 0
      unbound_lambda( 0,
	      ({#'call_other, ({#'this_object }), "stale_erq" })                    /*'*/
      )
    );
#endif
    set_driver_hook(H_INCLUDE_DIRS, #'_include_dirs_hook);                    /*'*/
  }
  set_driver_hook(H_AUTO_INCLUDE, AUTO_INCLUDE_STRING);
  HookSet=1;

  if (find_call_out("_cleanup_uinfo") < 0)
    call_out ("_cleanup_uinfo", 3600);
  if (find_call_out("daily_jobs") < 0)
    call_out("daily_jobs", 1, 1);
}

//---------------------------------------------------------------------------
string get_master_uid ()

// Return the string to be used as uid (and -euid) of a (re)loaded master.
// Under !native, the function may also return a non-zero number.
// In that case, the uid is set to 0, as is the euid.

{
  return ROOTID;
}

//---------------------------------------------------------------------------
void flag (string arg)

// Evaluate an argument given as option '-f' to the driver.
//
// Arguments:
//   arg: The argument string from the option text '-f<arg>'.
//        If several '-f' options are given, this function
//        will be called sequentially with all given arguments.
//
// This function can be used to pass the master commands via arguments to
// the driver. This is useful when building a new mudlib from scratch.
// It is called only when the game is started.
//
// The code given implements these commands:
//   '-fcall <ob> <fun> <arg>': call function <fun> in object <ob> with
//                              argument <arg>.
//   '-fshutdown': shutdown the game immediately.
// Thus, starting the game as 'parse "-fcall foo bar Yow!" -fshutdown' would
// first do foo->bar("Yow!") and then shutdown the game.

{
  string obj, fun, rest;

  if (arg == "shutdown")
  {
    shutdown();
    return;
  }
  if (sscanf(arg, "call %s %s %s", obj, fun, rest) >= 2)
  {
    write(obj+"->"+fun+"(\""+rest+"\") = ");
    write(({string})call_other(obj, fun, rest));
    write("\n");
    return;
  }
  write("master: Unknown flag "+arg+"\n");
}

//---------------------------------------------------------------------------
static string *explode_files(string file)

// Aux functionfor epilog().
// Return the string argument <file> exploded by \n, or an empty string.

{
  string data;

  data=read_file(file);
  if (!(data && data[0]))
    return ({});
  return efun::explode(data,"\n");
}

//---------------------------------------------------------------------------
string *epilog (int eflag)

// Perform final actions before opening the game to players.
//
// Arguments:
//   eflag: This is the number of '-e' options given to the parser.
//          Normally it is just 0 or 1.
//
// Result:
//   An array of strings, which traditionally designate the objects to be
//   preloaded with preload().
//   Any other result is interpreted as 'no object to preload'.
//   The resulting strings will be passed one at the time as
//   arguments to preload().
//
// The function reads the files /p/preload_file, /d/preload_file and
// /std/preload_file and returns the content as list of files to be loaded.

{
  string *files, *tmp;

  SETEUID(ROOTID);

#ifdef __HOST_NAME__
  log_file("GAME_LOG", ctime(time())+" Game started using "+__VERSION__
                       +" on "+__HOST_NAME__+".\n");
#else
  log_file("GAME_LOG", ctime(time())+" Game started using "+__VERSION__
                       +" on "+query_host_name()+".\n");
#endif
  if (eflag) {
    write("skip epilog because of eflag (ignored)\n");
    return 0;
  }
  tmp=explode_files(PPATH "preload_file");
  write(" project= #"+sizeof(tmp)+"\n");
  files=tmp;
  tmp=explode_files(DPATH "preload_file");
  write(" domain= #"+sizeof(tmp)+"\n");
  files+=tmp;
  tmp=explode_files("/std/preload_file");
  write(" std   = #"+sizeof(tmp)+"\n");
  files+=tmp;

  return files;
}

//---------------------------------------------------------------------------
void preload (string file)

// Preload a given object.
//
// Arguments:
//   file: The filename of the object to preload, as returned by epilog().
//
// It is task of the epilog()/preload() pair to ensure the validity of
// the given strings (e.g. filtering out comments and blank lines).
// For preload itself a 'call_other(file, "???")' is sufficient, but it
// should be guarded by a catch() to avoid premature blockings.
// Also it is wise to change the master's euid from master_uid to something
// less privileged for the time of the preload.
//
// You can of course do anything else with the passed strings - preloading
// is just the traditional task.

{
  int t1;
  string err;
  string name;

#define TString(val) (""+(val/60)+"."+(""+(100+val%60))[1..10])
  if (!file || (file && sizeof(file) < 1))
    return;
  if(!file[0] || file[0] == ';' || file_size(file+".c") < 0)
    return;

  write("Preloading: " + file + "...");

  name=get_wiz_name(file);
  if (!name) {
    write("no creator found for "+file+"\n");
    return;
  }
  if (member(({ BACKBONEID, "mail", "news", "lib" }), name) != -1) {
    write (file+" not allowed for preload.\n");
    return;
  }
  find_userinfo(name); /* Prevents endless recusion in valid_read() */
  if (SETEUID(name)==0) {
    write("cannot set euid to "+name+"\n");
    return;
  }
  t1 = time();
  err = catch(load_obj(file));
  if (err != 0) {
    write("\nGot error " + err + " when loading " + file + "\n");
  } else {
    t1 = time() - t1;
    write("(" + TString(t1) + ")\n");
  }
  SETEUID(ROOTID);
}

//---------------------------------------------------------------------------
void external_master_reload ()

// Master was reloaded on external request by SIGUSR1.
//
// If the gamedriver destruct and reloads the master on external request
// via SIGUSR1, it does this by a call to this function.
// It will be called after inaugurate_master() of course.
// If you plan to do additional magic here, you're welcome.

{
  return;
}

//---------------------------------------------------------------------------
// void reactivate_destructed_master (int removed)

// Reactivate a formerly destructed master.
//
// Arguments:
//   removed: True if the master was already on the list of destructed
//            objects.
//
// This function is called in an formerly destructed master since a new master
// couldn't be loaded.
// This function has to reinitialize all variables at least to continue
// operation.

//---------------------------------------------------------------------------
mixed get_simul_efun ()

// Load the simul_efun object(s) and return one or more paths of it.
//
// Result:
//   Either a single string with the object_name() of the simul_efun object,
//   or an array of strings which has to start with that object_name().
//   Return 0 if this feature isn't wanted.
//
// Note that the object(s) must be loaded by this function!
//
// When you return an array of strings, the first string is taken as path
// to the simul_efun object, and all other paths are used for backup
// simul_efun objects to call simul_efuns that are not present in the
// main simul_efun object. This allows to remove simul_efuns at runtime
// without getting errors from old compiled programs that still use the
// obsolete simul_efuns. A side use of this mechanism is to provide
// a 'spare' simul_efun object in case the normal one fails to load.
//
// If the game depends on the simul_efun object, and none could be loaded,
// an immediate shutdown should occur.

{
  if(!catch(call_other(SIMUL_EFUN_FILE, "???")))
    return SIMUL_EFUN_FILE;
  efun::write("Failed to load " + SIMUL_EFUN_FILE + "\n");
  if(!catch(call_other(SPARE_SIMUL_EFUN_FILE, "???")))
    return SPARE_SIMUL_EFUN_FILE;
  efun::write("Failed to load " + SPARE_SIMUL_EFUN_FILE + "\n");
  efun::shutdown();
  return 0;
}


//===========================================================================
//  Cron section
//
// The section contains the functions performing system services at
// predefined times, most notably daily_jobs().
//
// See also _cleanup_uinfo() and inaugurate_master().
//===========================================================================

//---------------------------------------------------------------------------
static void _start_dragon(string name, int arg)

// Start a dragon object.
//
// Argument:
//   name: Name of the dragon to start.
//   arg : Argument to pass to the dragon.
//
// The function loads object <name> and then calls the lfun start_dragon(arg)
// in it.

{
  object obj;
  if (obj = load_obj(name))
    obj->start_dragon(arg);
}


//---------------------------------------------------------------------------
void daily_jobs (int just_loaded)

// Do jobs once a day.
//
// Argument:
//   just_loaded: true if the master was just loaded
//
// The function starts several dragons and schedules a call_out() so that
// it is called every day at about 3am.

{
  int delay;

  /* Remove pending call_outs and schedule the new one */
  while (remove_call_out("daily_jobs") != -1)
    /* skip */;
  delay = 30*60-((60*to_int(ctime()[11..12]))+to_int(ctime()[14..15]));
  if (delay > 24*60) delay -= 24*60;
  /* if (delay > 12*60) delay -= 12*60; */
  call_out("daily_jobs", delay*60, 0);

  /* --- Start dragons --- */
  call_out ("_start_dragon", 600, REMOVER);
  call_out ("_start_dragon", 500, "/p/daemons/toplist_d");

  if (!just_loaded) {
    garbage_collection();
    // Either the simple garbage_collection(), or the call to secure/mudclean
    // call_out("_start_dragon", 1, "/secure/mudclean", 7);
  }

  // Though these are life-time jobs, they might need to be restarted.
  call_out ("_start_dragon", 1, "/secure/loadlogger");
  call_out ("_start_dragon", 10, "/secure/domaininfo");
}

//---------------------------------------------------------------------------
#if 0
#ifdef MUDWHO
void send_mudwho_info()

// Update the mudwho server.

{
    send_udp(MUDWHO_SERVER, MUDWHOPORT, sprintf(mudwho_ping, time()));
    walk_mapping(mudwho_info, send_mudwho_info);
    call_out("send_mudwho_info", MUDWHO_REFRESH_TIME);
}


void adjust_mudwho(object ob)

// Adjust the mudwho server.

{
    if (ob && interactive(ob)) {
        if (mudwho_info[ob][<5..] == "login") {
            mudwho_info[ob][<5..] = ((string)ob->QueryRealName())[0..24];
            send_udp(MUDWHO_SERVER, MUDWHOPORT, mudwho_info[ob]);
        }
    }
}
#endif /* MUDWHO */
#endif

//---------------------------------------------------------------------------
static void wiz_decay()

// Decay the wizlist info.

{
    mixed *wl;
    int i;

    wl = wizlist_info();
    for (i=sizeof(wl); i--; ) {
        set_extra_wizinfo(wl[i][WL_NAME], wl[i][WL_EXTRA] * 99 / 100);
    }
    call_out("wiz_decay", 3600);
}


//===========================================================================
//  Handling of player connections
//
// See also valid_exec().
//===========================================================================

//---------------------------------------------------------------------------
object connect ()

// Handle the request for a new connection by cloning and returning
// an instance of /secure/login.c
//
// Result:
//   An login object the requested connection should be bound to.
//
// Note that the connection is not bound yet!
//
// The gamedriver will call the lfun 'logon()' in the login object after
// binding the connection to it. That lfun has to return !=0 to succeed.

{
  string err;
  object ob;
  object blueprint;

  blueprint = find_object(LOGIN);
  if (blueprint && environment(blueprint) != 0)
    destruct(blueprint);
  err = catch(ob = clone_object(LOGIN));
  if (err) {
    write("Error cloning " LOGIN ".c\n");
    write(err+"\n");
    shutdown();
  }
#if 0
#ifdef MUDWHO
  mudwho_info[ob] =
    sprintf("A\t%.20s\t%.20s\t%.20s\t%.20s\t%:010d\t0\tlogin",
      MUDWHO_NAME, MUDWHO_PASSWORD, MUDWHO_NAME,
      explode(object_name(ob), "#")[<1] + "@"+MUDWHO_NAME, time()
    )
  ;
#endif /* MUDWHO */
#endif
  return ob;
}


//---------------------------------------------------------------------------
void disconnect (object obj)

// Handle the loss of an IP connection.
//
// Argument:
//   obj: The (formerly) interactive object (player).
//
// This called by the gamedriver to handle the removal of an IP connection,
// either because the connection is already lost ('netdeath') or due to
// calls to exec() or remove_interactive().
// The connection will be unbound upon return from this call.
//
// The function will simply announce the netdeath of the object (if it is
// a true user and no daemon).

{
  if (getuid(obj) != ROOTID && !({int})obj->IsServiceDemon() )
  {
    if (IS_WIZARD(obj) && ({int})obj->QueryInvis())
      catch(load_obj(CHANNEL_D)->SendEmote("general", capitalize(getuid(obj)),"is netdead",1));
    else
      catch(load_obj(CHANNEL_D)->SendEmote("news", capitalize(getuid(obj)),"is netdead",1));
    catch(log_file("ENTER", ctime()[4..15]+" netdead \t"
                            +getuid(TO)+"\n"));
  }
#if 0
#ifdef MUDWHO
  if (mudwho_info[obj])
    send_udp(MUDWHO_SERVER, MUDWHOPORT,
      "Z\t" + implode(explode(mudwho_info[obj], "\t")[1..4],"\t"));
#endif
#endif
}


//---------------------------------------------------------------------------
void remove_player (object player)

// Remove a player object from the game.
//
// Argument:
//   player: The player object to be removed.
//
// This function is called by the gamedriver to expell remaining players
// from the game on shutdown in a polite way.
// If this functions fails to quit/destruct the player, it will be
// destructed the hard way by the gamedriver.
//
// Note: This function must not cause runtime errors.

{
  if (PREV && to_string(PREV)[0..7] != SECUREPATH) {
    raise_error("Naughty naughty!\n");
    return;
  }
  if (!objectp(player))
    return;
  catch(player->cmd_quit());
  if (player)
    destruct(player);
}

void erq_stop() {
#ifdef __ERQ_MAX_SEND__ /* ERQ is optional */
  attach_erq_demon("", 0);
#endif
}

//---------------------------------------------------------------------------
void stale_erq (closure callback)

// Notify the loss of the erq demon.
//
// Argument:
//   callback: the callback closure set for an erq request.
//
// If the erq connection dies prematurely, the driver will call this lfun for
// every pending request with set callback. This function should notify the
// originating object that the answer will never arrive.
//
// In our case, we simply reattach the default erq.

{
  //attach_erq_demon("", 0);
  funcall(callback, -1, -1);
}


//===========================================================================
//  Runtime Support
//
// Various functions used to implement advanced runtime features.
//===========================================================================

//---------------------------------------------------------------------------
object compile_object (string filename)

// Compile an virtual object.
//
// Arguments:
//   previous_object(): The object requesting the virtual object.
//   filename         : The requested filename for the virtual object.
//
// Result:
//   The object to serve as the requested virtual object, or 0.
//
// This function is called if the compiler can't find the filename for an
// object to compile. The master has now the opportunity to return an other
// which will then serve as if it was compiled from <filename>.
// If the master returns 0, the usual 'Could not load'-error will occur.
//
// The function will try several possible VMasters in sequence, calling
// compile_object(<filename>) in each of them, until on of them returns
// an object. The objects tried in order (if existing) are:
//  1. previous_object()->Query(SP_VMASTER) || previous_object()
//  2. environment(previous_object())->Query(SP_VMASTER)
//     || environment(previous_object())
//  3. <path_of_filename>/vmaster.c
//  4  <path_of_filename>.c
//    In cases 3. and 4., only the basename part of <filename> is passed
//    to compile_object().
// TODO: LP-Scripting support?
//
// compile_object() will put information about the loading object into
// the last_* variables. These will be used by log_error() and runtime_error()
// in case of can't-load errors, because then compile_object() _will_ be
// called.

{
  object obj, room;
  mixed vmaster;
  string filepath;

  last_filename = filename;
  last_loader = to_string(PREV);
  last_env = environment(PREV) ? to_string(environment(PREV)) : 0;
  last_lived = living(PREV);
  if (filename[0] != '/') filename = "/"+filename;
  /* This security check is probably incomplete */
  if ("/std" == filename[0..3]
   || "/obj" == filename[0..3]
   || "/lib" == filename[0..3]
   || "/room" == filename[0..4]
   || SECUREPATH == filename[0..7]
   || "/mail" == filename[0..4]
   || "/news" == filename[0..4]
     )
    return 0;
  // This test was to protect admins from malicious wizards.
  // As OSB has no admins anymore, it is no longer necessary.
  // I keep it for now as comment in case it has other side effects.
  // if (getuid(PREV) != get_wiz_name(filename)
  //  && LVL_WIZARD >= query_user_level(get_wiz_name(filename))
  //    )
  //   return 0;
  vmaster = ({mixed})(PREV->Query(SP_VMASTER))|| PREV;
  room = ({object})vmaster->compile_object(filename);
  if (!room && (obj = environment(PREV)))
  {
    vmaster = ({mixed})(obj->Query(SP_VMASTER)) || obj;
    room = ({object})vmaster->compile_object(filename);
  }
  if (!room)
  {
    filepath = implode(explode(filename,"/")[0..<2],"/");
    vmaster = filepath+"/vmaster";
    if (0 <= file_size(vmaster+".c"))
      room = ({object})vmaster->compile_object(explode(filename,"/")[<1]);
    if (!room && 0 <= file_size(filepath+".c"))
      room = ({object})filepath->compile_object(explode(filename,"/")[<1]);
  }
  if (room)
    room->SetIsVirtual(1);
  return room;
}


//---------------------------------------------------------------------------
#if __VERSION__ > "3.5.0"
string get_wiz_name (object|string file)
#else
string get_wiz_name (mixed file)
#endif
// Return the author of a file.
//
// Arguments:
//   file: The name of the file in question.
//
// Result:
//   The name of the file's author (or 0 if there is none).
//
// This function is called for maintenance of the wiz-list, to score errors
// to the right wizard.
//
// The files and returned cuids are as following:
//   /players/ROOTID/*        -> 0
//   /players/<wizname>/*     -> <wizname>
//   /d/<domain>/*            -> <domain>.
//   /p/<project>/*           -> <project>.
//   /secure/*                -> ROOTID
//   /obj/*, /room/*, /std/*  -> BACKBONEID
//   /news/*                  -> "news"
//   /lib/*                   -> "lib"
//   /mail/*                  -> "mail"
//   /neu/secure/*            -> 0
//   /neu/*                   -> As if /neu/ is not part of the name.
//   Other                    -> 0

{
  string *strs;
  int s;

  if(objectp(file))
    strs=full_path_array(object_name(file), 0);
  else if(stringp(file))
    strs=full_path_array(file, 0);
  else return 0;

  s=sizeof(strs);
  if (s<2) return 0;
  if (strs[0] == "neu" && strs[1] != SECUREDIR) strs = strs[1..];

  switch(strs[0]) {
  case PDIR: return strs[1];       // we tested for s >= 2 already S.P.
  case DDIR: return strs[1];       // we tested for s >= 2 already S.P.
  case WDIR:
    return (s>2 && strs[1] != "ze") ? strs[1] : 0;
  case SECUREDIR:
    return ROOTID;
      /* The following uids are illegal for preload(). New uids here
      ** therefore need to go into preload() as well.
      */
  case "std":
  case "room":
  case "obj":
    return BACKBONEID;
  case "mail":
    return "mail";
  case "news":
    return "news";
  case "lib":
    return "lib";
  }
  return 0;
}


//---------------------------------------------------------------------------
// #if __VERSION__ < "3.2.6"
// string object_name (object obj)
// #else
// string printf_obj_name (object obj)
// #endif

// Return a printable name for an object.
//
// Arguments:
//   obj: The object which name is of interest.
//
// Result:
//   A string with the objects name, or 0.
//
// This function is called by sprintf() to print a meaningful name
// in addition to the normal object_name().
// If this functions returns a string, the object will be printed
// as "<filename> (<obj_name>)".


//---------------------------------------------------------------------------
void move_or_destruct(object what, object to)

// Auxiliary function for prepare_destruct().
// It tries to move <what> into <to> or into <what>s environment.
// If both fail, <what> is destructed.

{
  int rc;
  object oenv;
  oenv = environment(what);
  if ((catch(rc = ({int})what->move(to, 1)) || rc != 1 /* ME_OK Pepel */
       || (what && environment(what) == oenv))
      && what
     )
    destruct(what);
}

//---------------------------------------------------------------------------
void destruct_environment_of (object ob)

// Auxiliary function for prepare_destruct(), called when the destructed
// object (which is environment of <ob>) has no environment itself.
// The function tries to move interactive users into the /std/void resp.
// /room/netdeath, everything else is left where it is.

{
  string target;
  object env;
  target = VOID;
  if (!QOI(ob)) return;
  if (!interactive(ob) && to_string(environment(ob)) != NETDEATH)
    target = NETDEATH;
  else
    tell_object(ob,
"Everything you see is disolved. Luckily, you are transported somewhere...\n");
  env = environment(ob);
  /* The move is done first as an M_GO, then as M_NOCHECK */
  if (catch(ob->move(target, 2, "is transferred")) && ob)
    destruct(ob);
  if (!ob)
    return;
  if (env == environment(ob)
   && catch(ob->move(target, 1, "is transferred")) && ob
     )
    destruct(ob);
}

//---------------------------------------------------------------------------
mixed prepare_destruct (object obj)

// Prepare the destruction of the given object.
// Besides some announcements and logging for interactive <obj>s, the function
// tries to move all contents of <obj> into its environment before destructing
// it. If there is no environment, interactive users are moved into /std/void
// resp. /room/netdeath, everything else is destructed.
//
// Argument:
//   obj : The object to destruct.
//
// Result:
//   Return 0 if the object is ready for destruction, any other value
//   will abort the attempt.
//   If a string is returned, an error with the string as message will
//   be issued.
//
// The gamedriver calls this function whenever an object shall be destructed.
// It expects, that this function cleans the inventory of the object, or
// the destruct will fail.
// Furthermore, the function could notify the former inventory objects that
// their holder is under destruction (useful to move players out of rooms which
// are updated); and it could announce mudwide the destruction(quitting) of
// players.

{
  object super;
  mixed *errors;
  int i;
  string str, name, rname;
  object sh, next;

  if (!objectp(obj))
    return 0;
  catch(rname = ({string})obj->QueryRealName());
  catch(name = ({string})obj->QueryName());
#if 0
#ifdef MUDWHO
  if (mudwho_info[obj])
    send_udp(MUDWHO_SERVER, MUDWHOPORT,
      "Z\t" + implode(explode(mudwho_info[obj], "\t")[1..4],"\t"));
#endif
#endif
  if (getuid(obj) != "gopher" && rname != "<logon>"
      && explode(object_name(obj),"#")[0] != LOGIN) {
    if (interactive(obj)) {
      if (IS_WIZARD(obj) && ({int})obj->QueryInvis())
        catch(load_obj(CHANNEL_D)->SendEmote("general",
                               capitalize(rname||("["+getuid(obj)+"]")),
                               "leaves the world", 1));
      else
        catch(load_obj(CHANNEL_D)->SendEmote("news",
                               capitalize(rname||("["+getuid(obj)+"]")),
                               "leaves the world", 1));
    }
    catch(load_obj(CHANNEL_D)->UnregisterUser(obj));

/* To find out about the strange destructions of wizards... */
if (!catch(i = ({int})obj->QueryIsPlayer()) && i
    && (    obj != TP || obj != TI
         || TP != previous_object(caller_stack_depth()-1)
         || TP != TI
         || TI != previous_object(caller_stack_depth()-1)
       )
    && (   !previous_object(1)
        || (   object_name(previous_object(1))[0..7] != SECUREPATH
            && object_name(previous_object(1)) != "/"+capitalize(getuid(previous_object(1)))
           )
       )
   ) {
  str = (rname) || ("NPC '"+name+"'");
  str = ctime(time())+" "+capitalize(str)
        +(str != getuid(obj) ? (" ["+getuid(obj)+"] "):" ")
        +query_user_level(obj);
  if (!interactive(obj))
    str += " netdead.";
  else if (QIDLE(obj))
    str += " idle "+QIDLE(obj)+" sec.";
  else
    str += " active.";
  str += backtrace();
  if (file_size("/log/DESTRUCT") >= 50000)
    catch(rename("/log/DESTRUCT", "/log/DESTRUCT.old"));
  log_file("DESTRUCT", str);
}
  } /* if (is gopher) */

  /* Destruct all shadows of obj. This was in the driver formerly. */
  if (!QSHADOWING(obj))
    for (sh = SHADOW(obj, 0); sh; sh = next)
    {
      next = SHADOW(sh, 0);
      funcall(bind_lambda(#'unshadow, sh)); /* Avoid deep recursion */ /*'*/
      destruct(sh);
    }

  super = environment(obj);
  if (!super) {
    object item;

    while ( item = first_inventory(obj) ) {
      destruct_environment_of(item);
      if (item && environment(item) == obj) destruct(item);
    }
  } else {
    while ( first_inventory(obj) )
      move_or_destruct(first_inventory(obj), super);
  }
  return 0; /* success */
}


//---------------------------------------------------------------------------
void quota_demon ()

// Handle quotas in times of memory shortage.
//
// This function is called during the final phase of a garbage collection if
// the reserved user area couldn't be reallocated. This function (or a called
// demon) has now the opportunity to remove some (still active) objects from
// the game. If this does not free enough memory to reallocate the user
// reserve, slow_shut_down() will be called to start Armageddon.
//
// Note: Up to now, the wizlist lacks various informations needed to detect
//   the memory-hungriest wizards.

{
  log_file("GAME_LOG", ctime()+" quota_demon() called.\n");
}

//---------------------------------------------------------------------------
void receive_udp (string host, bytes buf, int port)

// Handle a received UDP message.
//
// Arguments:
//   host: Name of the host the message comes from.
//   msg : The received message.
//   port: the port number from which the message was sent.
//
// This function is called for every message received on the IMP port.
// The message is identified by her first characters and dispatched
// appropriately:
//   - "nftpd\t": if FTPD_IP is defined, its a NaseFTPD request.
//   - everything else is considered an Intermud request.

{
  string msg;

  if (catch(msg = to_text(buf, "UTF-8"); reserve 1000))
    msg = to_text(buf, "ASCII");
  INETD->receive_udp(host, msg, port);
}


//---------------------------------------------------------------------------
void slow_shut_down (int minutes)

// Schedule a shutdown for the near future.
//
// Argument:
//   minutes: The desired time in minutes till the shutdown:
//             6, if just the user reserve has been put into use;
//             1, if the (smaller) system or even the master reserve
//                has been put into use as well.
//
// The gamedriver calls this function when it runs low on memory.
// At this time, it has freed its reserve, but since it won't last long,
// the games needs to be shut down. Don't take the 'minutes' as granted
// remaining uptime, just deduce the urgency of the shutdown from it.
// The delay is to give the players the opportunity to finish quests,
// sell their stuff, etc.
// It is possible that the driver may reallocate some memory after the
// function has been called, and then run again into a low memory situation,
// calling this function again.
//
// This function loads an 'Armageddon' object and tells it what to do.
// It is the Armageddon object then which performs the shutdown.
//
// Technical:
//   The memory handling of the gamedriver includes three reserved areas:
//   user, system and master. All three are there to insure that the game
//   shuts down gracefully when the memory runs out: the user area to give
//   the players time to quit normally, the others to enable emergency-logouts
//   when the user reserve is used up as well.
//   The areas are allocated at start of the gamedriver, and released when
//   no more memory could be obtained from the host. In such a case, one
//   of the remaining areas is freed (so the game can continue a short
//   while) and a garbagecollection is initiated.
//   If the garbagecollection recycles enough memory (either true garbage
//   or by the aid of the quota_demon) to reallocate the areas, all is
//   fine, else the game is shut down by a call to this function.

{
  int i;
  object armageddon_ob;
  object where_ob;
  string err;

  find_userinfo("armageddon"); /* Prevents endless recusion in valid_read() */
  SETEUID("armageddon");

  if (this_interactive())
    log_file("GAME_LOG",ctime()+" "+geteuid(this_interactive()));
  else
    log_file("GAME_LOG",ctime()+" Gamedriver");
  log_file("GAME_LOG"," slow_shut_down("+minutes+")\n");

  armageddon_ob = find_object(SHUTDEMON);
  if (   !armageddon_ob
      || !({int})armageddon_ob->query_progress()
      || getuid(armageddon_ob) != "armageddon"
      || minutes < ({int})armageddon_ob->query_time_left() / 60
      || ({int})armageddon_ob->query_time_left() <= 0
     ) {
    err = catch(call_other(SHUTDEMON, "???")
	       , armageddon_ob = find_object(SHUTDEMON));
    SETEUID(ROOTID);
    if (err)
          return;
    filter(users(), lambda(({'u}),({#'&&,                                          /*'*/
      ({#'environment, 'u}),                                                       /*'*/
      ({#'!=, 'u, ({#'this_player})}),                                             /*'*/
      ({#'tell_object, 'u, "The Gamedriver shouts: Damn! I ran out of memory!\n"}) /*'*/
    })));
    armageddon_ob->set_reason("Game is out of memory.");
    armageddon_ob->shut(minutes);
  }
  else {
    log_file("GAME_LOG", "  Armageddon already active\n");
  }
}

//---------------------------------------------------------------------------
void notify_shutdown ()

// Notify the master about an immediate shutdown.
//
// If the gamedriver shuts down, this is the last function called before
// the mud shuts down the udp connections and the accepting socket for new
// players.
// The function has the opportunity to perform any cleanup operation, like
// informing the mudwho server that the mud is down. This can not be done
// when remove_player() is called because the udp connectivity is already
// gone then.

{
  // Anti-spoof check.
  if (PREV && PREV != TO)
      return;

  filter(users(), #'tell_object,                                        /*'*/
    "Game driver shouts: LPmud shutting down immediately.\n");
  /* Save the WIZLIST file */
  rm("/WIZLIST");
  catch(write_file(
    "/WIZLIST",
    implode(
      map(wizlist_info(),
        lambda(({'a}),                                                  /*'*/
          ({#'sprintf, "%s %d %d\n",                                    /*'*/
            ({#'[, 'a, WL_NAME}),                                       /*'*/
            ({#'[, 'a, WL_COMMANDS}),                                   /*'*/
            ({#'[, 'a, WL_EXTRA})                                       /*'*/
          })
        )
      ), ""
    )
  ));
#if 0
#ifdef MUDWHO
  send_udp(MUDWHO_SERVER, MUDWHOPORT,
    sprintf("D\t%.20s\t%.20s\t%.20s",
      MUDWHO_NAME, MUDWHO_PASSWORD, MUDWHO_NAME
    )
  );
#endif /* MUDWHO */
#endif
}


//===========================================================================
//  Error Handling
//
//===========================================================================

//---------------------------------------------------------------------------
string *error_logfile (string file)

// Return approbiate names( for an error logfile.
//
// Arguments:
//   file   : The name of file containing the error (it needn't be an object
//            file!).
// Result:
//   The filenames where the error should be logged.

{
  string *lfile;
  string cuid, puid;

  cuid = get_wiz_name(file);
  if (!cuid || cuid == ROOTID)
    lfile = ({ "/log/ERR/generic" });
  else if (file_size(WPATH+cuid) == -2)
    lfile = ({ WPATH+cuid+"/.err" });
  else if (file_size(DPATH+cuid) == -2) {
    lfile = ({ DPATH+cuid+"/.err" });
    puid = explode(file, "/")[2];
    if (file_size(WPATH+puid) == -2)
      lfile += ({ WPATH+puid+"/.err" });
  }
  else if (file_size(DPATH+cuid) == -2) {
    lfile = ({ PPATH+cuid+"/.err" });
    puid = explode(file, "/")[2];
    if (file_size(WPATH+puid) == -2)
      lfile += ({ WPATH+puid+"/.err" });
  }
  else {
    if (cuid == ROOTID) cuid = "root";
    lfile = ({ "/log/ERR/"+cuid });
  }
  return lfile;
}

//---------------------------------------------------------------------------
void dangling_lfun_closure ()

// Handle a dangling lfun-closure.
//
// This is called when the gamedriver executes a closure using a vanished lfun.
// A proper handling is to raise a runtime error.
//
// Technical:
//   Upon replacing programs (see efun replace_program()), any existing
//   lambda closures of the object are adjusted to the new environment.
//   If a closure uses a lfun which vanished in the replacement process,
//   the reference to this lfun is replaced by a reference to this function.
//   The error will then occur when the execution of the adjusted lambda
//   reaches the point of the lfun reference.
//   There are two reasons for the delayed handling. First is that the
//   program replacement and with it the closure adjustment happens at
//   the end of a backend cycle, outside of any execution thread: noone
//   would see the error at this time.
//   Second, smart closures might know/recognize the program replacement
//   and skip the call to the vanished lfun.

{
  raise_error("dangling lfun closure\n");
}

//---------------------------------------------------------------------------
void log_error (string file, string err, int warn, int line)

// Announce a compiler-time error.
//
// Arguments:
//   file: The name of file containing the error (it needn't be an object
//         file!).
//   err : The error message.
//
// Whenever the LPC compiler detects an error, this function is called.
// It should at least log the error in a file, and also announce it
// to the active player if it is an wizard.
//
// In case of cant-load errors, the function uses the current content
// of the last_* variables (set by compile_object()) to create the error
// message.
// It also updates/checks the last_log_* variables to reduce the amount
// of redundant output if one file repeatedly throws an error.

{
  string *lfile;
  string code;

#if __EFUN_DEFINED__(expand_define) /*!defined(__VERSION__) || __VERSION__ > "03.02.1@94"*/
  if (!catch(code = expand_define("__FILE__")) && code)
    err += sprintf("Compiling %s, Line %d\n", code, line);
    //err += sprintf("Compiling %s, Line %s\n", code, expand_define("__LINE__"));
    //err += "Compiling "+code+", Line "+expand_define("__LINE__")+"\n";
#endif
  if (last_loader)
    err += last_loader+" tried to load "+last_filename+"\n";
  last_loader = 0;
  last_filename = 0;
  if (IS_IMMORTAL(TI))
    write (err);
  lfile = error_logfile(file);
  if (time() > last_log_time+59 || lfile[0] != last_log_file[0])
  {
    last_log_time = time();
    last_log_file = lfile;
    err = " -- "+ctime()+
          " -- "+((warn == 1) ? "Warning --\n" : "Error --\n")+err;
  }
  if (catch(map(lfile, #'write_file /*'*/, err)))
    write ("Could not log error!\n");
}


//---------------------------------------------------------------------------
mixed heart_beat_error (object culprit, string err,
                        string prg, string curobj, int line)

// Announce an error in the heart_beat() function.
//
// Arguments:
//   culprit: The object which lost the heart_beat.
//   err    : The error message.
//   prg    : The executed program (might be 0).
//   curobj : The object causing the error (might be 0).
//   line   : The line number where the error occured (might be 0).
//
// Result:
//   Return anything != 0 to restart the heart_beat in culprit.
//
// This function has to announce an error in the heart_beat() function
// of culprit.
// At time of call, the heart_beat has been turned off.
// A player should at least get a "You have no heartbeat!" message, a more
// advanced handling would destruct the offending object and allow the
// heartbeat to restart.
//
// Note that <prg> denotes the program actually executed (which might be
// inherited one) whereas <curobj> is just the offending object.
//
// The function undergoes a lot of effort to make usable error messages.
// In general, if an object causes an error in a heart-beat, it is
// destructed and the heartbeat of the victim is continued. If the object
// can't be destructed, the heartbeat remains disabled.
// Interactive users are never destructed here.
//
// The function updates/checks the last_heart_* variables to reduce the amount
// of redundant output if one file repeatedly throws an error.


{
  object causer;
  string *file;
  string message;
  string code;
  int flag, rc;

#define CWRITE(s) tell_object(culprit, s)

  if (err == "Illegal use of save_object()\n" && last_save_fname)
  {
    err = err[0..<2] + " to '"+last_save_fname+"'\n";
    last_save_fname = 0;
  }

  if (prg) {
    message = "Obj: "+curobj;
    if ((causer = find_object(curobj)) && (causer = environment(causer)))
      message += " in "+object_name(causer);
    message += ", File: "+prg+", Line: "+line;
    file = error_logfile(curobj);
    if (time() > last_heart_time+59 || file[0] != last_heart_file[0])
    {
      last_heart_time = time();
      last_heart_file = file;
      catch(map(file, #'write_file, " -- "+ctime()+" --\n"+message+"\n  "+err)); /*'*/
    }
    else
      catch(map(file, #'write_file, message+"\n  "+err));                        /*'*/
    causer = find_object(curobj);
  }
  else {
    message = "Offending object not locatable (maybe destructed).\n";
    causer = 0;
  }

  rc = 1;
  if (objectp(culprit) && interactive(culprit))
  {
    flag = 0;
    if (!catch(flag = IS_IMMORTAL(culprit)) && flag) {
      CWRITE(
        "---------------------------------------------------------------\n");
      CWRITE("Error in your heartbeat!\n");
      CWRITE(message+"\n");
      if (prg && !catch(code=read_file("/"+prg, line, 1)) && code)
        CWRITE("\n"+code+"\n");
      CWRITE("Error: "+err);
      CWRITE(
        "---------------------------------------------------------------\n");
    }
    if (!causer || causer == culprit) {
      if (flag)
        CWRITE("Can't turn your heartbeat on again!\n");
      else {
        catch(load_obj(CHANNEL_D)->SendTell("error", "MASTER", message,1));
        catch(load_obj(CHANNEL_D)->SendTell("error", "MASTER",
          "Heartbeat of "+capitalize(getuid(culprit))+" turned off.\n",1));
        CWRITE("A wrongness in the fabric of space stops your heart!\n");
      }
      rc = 0;
    }
    else {
      if (causer != TO) { // In case its in the microcode
        catch(causer->remove());
        if (causer) catch(destruct(causer));
      }
      if (causer) rc = 0;
      if (flag) {
        if (causer)
          CWRITE("Failed to destruct offending object.\n"
                +"Can't turn your heartbeat on again!\n");
        else
          CWRITE("Offending object destructed.\n");
      }
      else {
        if (causer) {
          catch(load_obj(CHANNEL_D)->SendTell("error", "MASTER",
            "Offending object can't be destructed.\n",1));
          catch(load_obj(CHANNEL_D)->SendTell("error", "MASTER",
            "Heartbeat of "+capitalize(getuid(culprit))+" turned off.\n",1));
          CWRITE("A wrongness in the fabric of space stops your heart!\n");
        }
        else {
        catch(load_obj(CHANNEL_D)->SendTell("error", "MASTER",
          "Heartbeat of "+capitalize(getuid(culprit))+" continues.\n",1));
          CWRITE("Shortly you notice a wrongness in the fabric of space.\n");
        }
      }
    }
  }
  else { // culprit not interactive
    string fname;
    if (culprit)
      fname = object_name(culprit);
    else
      fname = "<zombie>";
    catch(map(error_logfile(fname), #'write_file
             ,fname+": Error in heartbeat\n"+message
              +"\n  "+err));
    catch(load_obj(CHANNEL_D)->SendTell("error", "MASTER",
      "Error in heartbeat of "+fname+"\n",1));
    catch(load_obj(CHANNEL_D)->SendTell("error", "MASTER", message,1));
    if (objectp(causer) && interactive(causer)) { // Don't remove players
      catch(load_obj(CHANNEL_D)->SendTell("error", "MASTER",
        "Offending object can't be destructed.\n",1));
      catch(load_obj(CHANNEL_D)->SendTell("error",
        "Heartbeat of "+capitalize(getuid(culprit))+" turned off.\n",1));
      rc = 0;
    }
    else if (causer) { // Remove the offending object.
      if (causer != TO) {
        catch(causer->remove());
        if (causer) catch(destruct(causer));
      }
      if (causer || !culprit)
        rc = 0;  // Don't restart heart if causer is still there, or if
                 // culprit was the causer and therefore got removed.
      if (causer)
        catch(load_obj(CHANNEL_D)->SendTell("error",
          "Offending object can't be destructed.\n",1));
    }
  }

  unwind_cmd_stack(TP);

  return rc;

#undef CWRITE
}


//---------------------------------------------------------------------------
static void handle_runtime_error (string err, string prg, string curobj, int line)

// Announce a runtime error.
//
// The call of this function is a three-step call: runtime_error() calls
// call_runtime_error() with no eval limits, thus costing the caller
// only 10 ticks. call_runtime_error() calls handle_runtime_error() with
// an eval limit of 200000, thus putting a safety brake on the error handler.
// The indirection is necessary to avoid putting the error handling cost
// on the caller.
//
// Arguments:
//   err    : The error message.
//   prg    : The executed program.
//   curobj : The object causing the error.
//   line   : The line number where the error occured.
//
// This function has to announce a runtime error to the active player.
// If it is a wizard, it might give him the full error message together
// with the source line; if it is a player, it should issue a decent
// message ("Your sensitive mind notices a wrongness in the fabric of space")
// and could also announce the error to the wizards online.
//
// Note that <prg> denotes the program actually executed (which might be
// inherited one) whereas <curobj> is just the offending object.
//
// In case of cant-load errors, the function uses the current content
// of the last_* variables (set by compile_object()) to create the error
// message.
// The function updates/checks the last_runt_* variables to reduce the amount
// of redundant output if one file repeatedly throws an error.

{
  string *file;
  string message;
  string code;
  int flag;
  object theobj;

  if (err == "Illegal use of save_object()\n" && last_save_fname)
  {
    err = err[0..<2] + " to '"+last_save_fname+"'\n";
    last_save_fname = 0;
  }
  message = "Obj: "+curobj+", File: "+prg+", Line: "+line+":";
#if __EFUN_DEFINED__(expand_define)
  if (code = expand_define("__FILE__"))
    message += sprintf("\nCompiling %s, Line %s", code, expand_define("__LINE__"));
#endif
  if (last_loader)
  {
    message += "\n"+last_loader;
    if (last_lived && last_env)
      message += " in "+last_env;
    message += " tried to load "+last_filename;
  }
  last_loader = 0;
  last_filename = 0;
  file = error_logfile(curobj);
  if (!stringp(curobj) || !(theobj = find_object(curobj)))
  {
    theobj = 0;
    code = 0;
    message += sprintf("\nCreator can't be found, curobj is %O.", curobj);
  }
  else if (stringp(code = ({string})theobj->QueryCreator()))
  {
    file += error_logfile(code);
    message += "\nCreated by "+code;
  }
  if (time() > last_runt_time+59 || file[0] != last_runt_file[0])
    {
    last_runt_time = time();
    last_runt_file = file;
    catch(map(file, #'write_file, " -- "+ctime()+" --\n"+message+"\n  "+err)); /*'*/
  }
  else
    catch(map(file, #'write_file, message+"\n  "+err));                        /*'*/

  if (TI)
  {
    if (!catch(flag = IS_IMMORTAL(TI)) && flag) {
      write(
        "---------------------------------------------------------------\n");
      write(message+"\n");
      if (prg && !catch(code=read_file("/"+prg, line, 1)) && code)
        write("\n"+code+"\n");
      write("Error: "+err);
      write(
        "---------------------------------------------------------------\n");
    }
    else {
      write(
        "Your sensitive mind notices a wrongness in the fabric of space.\n");
      if(code = catch(load_obj(CHANNEL_D)->SendTell("error", "MASTER",message+" "+err,1)))
        debug_message(sprintf("DEBUG: handle_runtime_error channel error: %O\n", file));
    }
  }

  unwind_cmd_stack(TP);
  debug_message(sprintf("DEBUG: handle_runtime_error done: %d ticks\n", get_eval_cost()));
}

static void call_runtime_error (string err, string prg, string curobj, int line)
{
  limited(#'handle_runtime_error /*'*/, ({ 200000 }), err, prg, curobj, line);
}

void runtime_error (string err, string prg, string curobj, int line)
{
  limited(#'call_runtime_error /*'*/, ({ LIMIT_UNLIMITED }), err, prg, curobj, line);
}

//===========================================================================
//  Security and Permissions
//
// Most of these functions guard critical efuns. A good approach to deal
// with them is to redefine the efuns by simul_efuns (which can then avoid
// trouble prematurely) and give root objects only the permission to
// execute the real efuns.
//
// See also get_wiz_name(), valid_read() and valid_write().
//===========================================================================

//---------------------------------------------------------------------------
int privilege_violation (string op, mixed who, mixed arg, mixed arg2)

// Validate the execution of a privileged operation.
//
// Arguments:
//   op   : the requested operation (see below)
//   who  : the object requesting the operation (filename or object pointer)
//   arg  : additional argument, depending on <op>.
//   arg2 : additional argument, depending on <op>.
//
// Result:
//     >0: The caller is allowed for this operation.
//      0: The caller was probably misleaded; try to fix the error
//   else: A real privilege violation; handle it as error.
//
// Privileged operations are:
//   attach_erq_demon  : Attach the erq demon to object <arg> with flag <arg2>.
//   bind_lambda       : Bind a lambda-closure to object <arg>.
//   call_out_info     : Return an array with all call_out informations.
//   erq               : A the request <arg4> is to be send to the
//                       erq-demon by the object <who>.
//   input_to          : Object <who> issues an 'ignore-bang'-input_to() for
//                       commandgiver <arg3>; the exakt flags are <arg4>.
//   nomask simul_efun : Attempt to get an efun <arg> via efun:: when it
//                       is shadowed by a 'nomask'-type simul_efun.
//   rename_object     : The current object <who> renames object <arg>
//                       to name <arg2>.
//   send_udp          : Send UDP-data to host <arg>.
//   set_auto_include_string : Set the string automatically included by
//                       the compiler.
//   get_extra_wizinfo : Get the additional wiz-list info for wizard <arg>.
//   set_extra_wizinfo : Set the additional wiz-list info for wizard <arg>.
//   set_extra_wizinfo_size : Set the size of the additional wizard info
//                       in the wiz-list to <arg>.
//   set_driver_hook   : Set hook <arg> to <arg2>.
//   set_this_object   : Set this_object() to <arg>.
//   shadow_add_action : Add an action to function <arg> from a shadow.
//   symbol_variable   : Attempt to create symbol of a hidden variable
//                       of object <arg> with with index <arg2> in the
//                       objects variable table.
//   wizlist_info      : Return an array with all wiz-list information.
//
// call_out_info can return the arguments to functions and lambda closures
// to be called by call_out(); you should consider that read access to
// closures, mappings and pointers means write access and/or other privileges.
// wizlist_info() will return an array which holds, among others, the extra
// wizlist field. While a toplevel array, if found, will be copied, this does
// not apply to nested arrays or to any mappings. You might also have some
// sensitive closures there.
// send_udp() should be watched as it could be abused to mess up the IMP.
// The xxx_extra_wizinfo operations are necessary for a proper wizlist and
// should therefore be restricted to admins.
// All other operations are potential sources for direct security breaches -
// any use of them should be scrutinized closely.
//
// The policies implemented here are this:
//   send_udp             : free for /secure objects and the inetd.
//   call_out_info        : free for everyobject
//   erq:                 : free for everyobject
//   other known functions: free for /secure objects
//   unknown functions    : disabled
// Rejected attempts are logged in /log/PRIVILEGE_VIOLATION.

{
  // debug_message(sprintf("priv (%O, %O, %O, %O)\n", op, who, arg, arg2));
  string bp;
  int p;

  switch(op) {
    case "bind_lambda":
      // 'who' is the object to which the lambda is bound to.
      if ((stringp(arg) && arg[0..6] == SECUREDIR "/")
       || (objectp(arg) && getuid(arg) == ROOTID))
        return 1;
    case "send_imp":
    case "send_udp":
      /* mudlink stuff */
      if ((stringp(who) && who[0..6] == SECUREDIR "/")
       || (objectp(who) && getuid(who) == ROOTID))
        return 1;

      if ( objectp( who ) )
	      bp = object_name( who );
      else if ( stringp( who ) )
	      bp = who;
      else
	      break; /* who is of wrong type */

      if ( ( p = member(bp, '#' ) ) != -1 )
	      bp = bp[0..p-1];

      if ( bp == INETD )
	      return 1; /* INETD is allowed to use send_udp */

      break;
    case "call_out_info":
      return 1;
    case "erq":
//      if (arg2 == ERQ_RLOOKUP)
      return 1;
      /* FALL THROUGH */
    case "attach_erq_demon":
    case "nomask simul_efun":
    case "set_this_object":
    case "set_auto_include_string":
    case "wizlist_info":
    case "get_extra_wizinfo":
    case "set_extra_wizinfo":
    case "set_extra_wizinfo_size":
    case "rename_object":
    case "input_to":
    case "shutdown":
    case "configure_interactive":
    case "configure_object":
    case "configure_driver":
    case "enable_telnet":
    case "garbage_collection":
    case "limited":
    case "mysql":
    case "pgsql":
    case "net_connect":
    case "set_driver_hook":
    case "set_limits":
    case "set_max_commands":
    case "shadow_add_action":
    case "sqlite_pragma":
    case "symbol_variable":
    case "variable_list":
      if ((stringp(who) && who[0..6] == SECUREDIR "/")
       || (objectp(who) && getuid(who) == ROOTID))
        return 1;
      if (objectp(who) && IS_WIZARD(who))
        return 1;
      /* FALL THROUGH */
    default:
      break;
    }
  log_file("PRIVILEGE_VIOLATION",
           ctime()+"  op: " + op +
           " who: " + to_string(who) +
           " arg: " + to_string(arg) + "\n"
           +backtrace());
  return -1;
}

//---------------------------------------------------------------------------
int query_allow_shadow (object victim)

// Validate a shadowing.
//
// Arguments:
//   previous_object(): the wannabe shadow
//   victim           : the object to be shadowed.
//
// Result:
//   Return 0 to disallow the shadowing, any other value to allow it.
//   Destructing the shadow or the victim is another way of disallowing.
//
// The function should denies shadowing of all root objects, else it calls
// the victims lfun 'prevent_shadow(shadower)' for clearance.

{
  if(getuid(victim) == ROOTID)
    return 0;
  return !({int})victim->prevent_shadow(PREV);
}


//---------------------------------------------------------------------------
int query_player_level (string what)

// Check if the player is of high enough level for several things.
//
// Argument:
//   what: The 'thing' type (see below).
//
// Result:
//   Return 0 to disallow, any other value to allow it.
//
// Types asked for so far are:
//   "trace"         : Is the player is allowed to use tracing?
//   "inspect memory": Is the player allowed to issue the special
//                     command 'showsmallnewmalloced' ?
// Old, now unused types are:
//   "error messages": Is the player is allowed to see error messages?
//   "wizard"        : Is the player considered at least a "minimal" wizard ?

{
  if(!TP)
    return 0;
  switch (what)
  {
    case "wizard":
      return IS_WIZARD(TP);
    case "trace":
      return IS_WIZARD(TP);
    case "error messages":
      return IS_WIZARD(TP);
    case "inspect memory":
      return IS_WIZARD(TP);
  }
  return 0;
}

#if __VERSION__ > "3.5.2"
int valid_trace(string what, int|string arg)
{
  return 1;
}
#else
#endif

//---------------------------------------------------------------------------
int valid_exec (string name, object ob, object obfrom)

// Validate the rebinding of an IP connection by usage of efun exec().
//
// Arguments:
//    name  : The name of the _program_ attempting to rebind the connection.
//            This is not the object_name() of the object, and has no leading
//            slash.
//    ob    : The object to receive the connection.
//    obfrom: The object giving the connection away.
//
// Result:
//   Return a non-zero number to allow the action,
//   any other value to disallow it.
//
// Only /secure/master and /secure/login are allowed to use exec().

{
  switch("/"+name)
  {
    case LOGIN ".c":
    case MASTER ".c":
    if (!interactive(ob))
    {
      mapping tsfrom = ({mapping})obfrom->transfer_ts();
#if 0
#ifdef MUDWHO
      if (interactive(obfrom)) {
        mudwho_info[ob] = mudwho_info[obfrom];
        efun::m_delete(mudwho_info, obfrom);
        call_out("adjust_mudwho", 0, ob);
      }
#endif /* MUDWHO */
#endif
      if (tsfrom)
        ob->transfer_ts(tsfrom);
	  ob->SetTerminal();
      return 1;
    }
    else {
      mapping tsto = ({mapping})ob->transfer_ts();
      mapping tsfrom = ({mapping})obfrom->transfer_ts();
      if (tsfrom)
        ob->transfer_ts(tsfrom);
      if (tsto)
        obfrom->transfer_ts(tsto);
	  ob->SetTerminal();
      return 1;
    }
  }
  return 0;
}


//---------------------------------------------------------------------------
int valid_query_snoop (object obj)

// Validate if the snoopers of an object may be revealed by usage of the
// efun query_snoop().
//
// Arguments:
//   previous_object(): the asking object.
//   obj              : the object which snoopers are to be revealed.
//
// Result:
//   Return a non-zero number to allow the action,
//   any other value to disallow it.
//
// Only root (/secure) objects are allowed to do this; this way the whole
// handling for the wizards is delegated

{
  return getuid(PREV) == ROOTID;
}


//---------------------------------------------------------------------------
int valid_snoop (object snoopee, object snooper)

// Validate the start/stop of a snoop.
//
// Arguments:
//   snoopee: The victim of the snoop.
//   snooper: The wannabe snooper, or 0 when stopping a snoop.
//
// Result:
//   Return a non-zero number to allow the action,
//   any other value to disallow it.
//
// Only root (/secure) objects are allowed to do this; this way the whole
// handling for the wizards is delegated

{
  return getuid(PREV) == ROOTID;
}


//===========================================================================
//  Userids and depending Security
//
// For each object in the mud exists a string attribute which determines the
// objects rights in security-sensitive matters. In compat muds this attribute
// is called the "creator" of the object, in !compat muds the object's "userid"
// ("uid" for short).
//
// "Effective Userids" are an extension of this system, to allow the easier
// implementation of mudlib security by diffentiating between an objects
// theoretical permissions (uid) and its current permissions (euid) (some
// experts think that this attempt has failed (Heya Macbeth!)).
//
// Userids are always available, effective userids only if the driver was
// compiled to run in euids mode (note that native mode without euids is not
// possible).
// The driver just implements the setting/querying of the (e)uids -- it is
// task of the mudlib to give out the right (e)uid to the right object, and
// to check them where necessary.
//
// The main use for (e)uids is for determination of file access rights, but
// you can of course use the (e)uids for other identification purposes as well.
//===========================================================================

//---------------------------------------------------------------------------
string get_bb_uid()

// Return the string to be used as backbone-uid.
// It is just used by process_string() only if no this_object() is present.

{
  return BACKBONEID;
}

//---------------------------------------------------------------------------
int valid_seteuid (object obj, string neweuid)

// Validate the change of an objects euid by efun seteuid().
//
// Arguments:
//   obj    : The object requesting the new euid.
//   neweuid: The new euid requested.
//
// Result:
//   Return 1 to allow the change, any other value to disallow it.
//
// The change is allowed under these circumstances:
//   - <ob> is a root object, or has a matching uid
//   - the cuid of <ob> matches the <neweuid>.

{
  if (getuid(obj) == ROOTID) return 1;
  if (getuid(obj) == neweuid) return 1;
  if (get_wiz_name(object_name(obj)) == neweuid) return 1;
  return 0;
}

//---------------------------------------------------------------------------
mixed valid_read  (string path, string euid, string fun, object caller)

// Validate a reading/writing file operation.
//
// Arguments:
//   path   : The (possibly partial) filename given to the operation.
//   euid   : !euids: 0;
//             euids: the euid of the caller (might be 0).
//   fun    : The name of the operation requested (see below).
//   caller : The calling object.
//
// Result:
//   The full pathname of the file to operate on, or 0 if the action is not
//   allowed.
//   You can also return 1 to indicate that the path can be used unchanged.
//
// The path finally to be used must not contain spaces or '..'s .
//
// These are the central functions establishing the various file access
// rights.
//
// valid_read() is called for these operations:
//   copy_file
//   ed_start        (when reading a file)
//   file_size
//   get_dir
//   print_file
//   read_bytes
//   read_file
//   restore_object
//   tail
//
// TODO: Make me nice and document me.

{
  string *strs;
  string ro_suffix;
  int s;

  if(!euid) euid="-";
/*    _info_chef("v_read("+path+","+euid+")"); */

  ro_suffix = ".o";
  switch(fun) {
  case "restore_object":
    strs=full_path_array("/"+path, 0);
    ro_suffix = "";
    break;
  case "ed_start":
    if(path && path[0])
      strs=full_path_array(path, euid);
    else
      strs=({WDIR,euid,".err"});
    break;
  default:
    strs=full_path_array(path, euid);
  }
  path=implode(strs, "/");

  if (!sizeof(strs) || euid == ROOTID || euid == "voted")
    return path;
  /*
  write_file("/log/master_debug","path="+path+" euid="+euid+" func="+fun+" obj="+to_string(obj)+"\n");
  */
  if(strs[0] == "mail" && sizeof(strs) > 2) {
    if(euid == "mail" || (sizeof(strs) >= 3 && euid == strs[2])) return path;
    return 0;
  }

  switch(member(strs, SECUREDIR)) {
  case 0:
    if (sizeof(strs) < 2 || strs[1] != "save")
      return path;
    if (   sizeof(strs) < 4
        || (   strs[3] != euid+ro_suffix
            && (!TI || "/"+(explode(object_name(TI),"#")[0]) != LOGIN)
            && euid != "gopher" // Don't ask :-(
           )
       )
      return 0;
    return path;
  case 2:
    if(strs[0] == WDIR && strs[1] != euid)
      return 0;
    return path;
  }

  return path;
}


//---------------------------------------------------------------------------
mixed valid_write (string path, string euid, string fun, object caller)

// Validate a reading/writing file operation.
//
// Arguments:
//   path   : The (possibly partial) filename given to the operation.
//   euid   : !euids: 0;
//             euids: the euid of the caller (might be 0).
//   fun    : The name of the operation requested (see below).
//   caller : The calling object.
//
// Result:
//   The full pathname of the file to operate on, or 0 if the action is not
//   allowed.
//   You can also return 1 to indicate that the path can be used unchanged.
//
// The path finally to be used must not contain spaces or '..'s .
//
// These are the central functions establishing the various file access
// rights.
//
// valid_write() is called for these operations:
//   copy_file
//   ed_start     (when writing a file)
//   mkdir
//   save_object
//   remove_file
//   rename_from, rename_to
//   rmdir
//   write_bytes
//   write_file
//
// TODO: Make me nice and document me.

{
  string *strs, *pstrs;
  int s, lvl;

  if (!euid) return 0;
  /* _info_chef("v_write("+path+","+euid+","+fun+","+to_string(caller)+")"); */

  switch (fun) {
  case "save_object":
    if (!path) return 0;
    strs = full_path_array("/"+path, 0);
    last_save_fname = implode(strs, "/");
    break;
  case "ed_start":
    if (path && path[0]) strs = full_path_array(path, euid);
    else strs = ({WDIR,euid,".err"});
#if 0
    if (!IS_WIZARD(euid) && sizeof(strs) >= 2 && strs[0] == "log"
        && member("ABCDEFGHIJKLMNOPQRSTUVWXYZ", strs[<1][0]) != -1
       ) return 0;
#endif
    break;
  case "mkdir":
  case "rmdir":
  case "remove_file":
    strs=full_path_array(path, euid);
#if 0
    if (!IS_WIZARD(euid) && sizeof(strs) >= 2 && strs[0] == "log"
        && member("ABCDEFGHIJKLMNOPQRSTUVWXYZ", strs[<1][0]) != -1
       ) return 0;
#endif
  default:
    strs=full_path_array(path, euid);
  }

  path=implode(strs, "/");

  if (euid == ROOTID || euid == "voted")
    return path;

  lvl = query_user_level(euid);
  s=sizeof(strs);
  if (s>1) {
    switch(strs[0]) {
    case SECUREDIR: return 0;

    case PDIR:
    case DDIR:
      if (lvl >= LVL_WIZARD)
        break;
      if (s>2 && strs[1] == euid)
        break;
      return 0;
    case "doc":
    case "neu":
    case "std":
    case "lib":
      if (lvl >= LVL_WIZARD)
        break;
      return 0;

    case "ftp":
    case "log":
    case "open":
    case "etc":
    case "tmp": break;

    case "sys":
      if (s > 1 && (strs[1] == "secure" || strs[1] == "driver"))
         return 0;
      if (lvl < LVL_WIZARD)
         return 0;
      break;
    case "mail":
    case "news":
      if (euid == strs[0])
        break;
      return 0;
    case "save":
      if (lvl  >= LVL_WIZARD && TI && getuid(TI) == euid) break;
      if (strs[1] == WDIR && (strs[2] != euid[0..0] || strs[3] != euid))
        return 0;
      if (strs[1] == WDIR || strs[1] == "daemons")
	break;
      if (strs[1] == "obj")
	break;
      return 0;
    default:
      if (lvl >= LVL_WIZARD) break;
      if (s>2 && strs[0] == WDIR && strs[1] == euid)
        break;
      return 0;
    }
    return path;
  }
  if (s == 1 && strs[0] == "COPYRIGHT") return 1;
  return lvl >= LVL_WIZARD && TI && getuid(TI) == euid;
}


//===========================================================================
//  ed() Support
//
//===========================================================================

//---------------------------------------------------------------------------
string make_path_absolute (string str)

// Absolutize a relative filename given to the editor.
//
// Argument:
//   str : The relative filename (without leading slash).
//
// Result:
//   The full pathname of the file to use.
//   Any non-string result will act as 'bad file name'.

{
  return full_path(str, USERNAME(TP));
}


//---------------------------------------------------------------------------
int save_ed_setup (object who, int code)

// Save individual settings of ed for a wizard.
//
// Arguments:
//   who : The wizard using the editor.
//   code: The encoded options to be saved.
//
// Result:
//   Return 0 on failure, any other value for success.
//
// This function saves the given integer in a safe place in the realm of
// the given wizard, i.e. /players/<name>/.edrc.
//
// Be aware of possible security breaches: under !compat, a write_file()
// should be surrounded by a temporary setting of the masters euid to
// that of the wizard.

{
  string file;

  if (!intp(code))
    return 0;
  file = WPATH + lower_case(USERNAME(who)) + "/.edrc";
  catch(rm(file));
  SETEUID(USERNAME(who));
  catch(write_file(file, code + ""));
  SETEUID(ROOTID);
  return 1;
}


//---------------------------------------------------------------------------
int retrieve_ed_setup (object who)

// Retrieve individual settings of ed for a wizard.
//
// Arguments:
//   who : The wizard using the editor.
//
// Result:
//   The encoded options retrieved (0 if there are none).
//
// The function returns the integer saved by save_ed_setup() for the given
// wizard from. /players/<name>/.edrc.

{
  string file;
  int code;

  file = WPATH + lower_case(USERNAME(who)) + "/.edrc";
  if (file_size(file) <= 0)
    return 0;
  sscanf(read_file(file), "%d", code);
  return code;
}


//---------------------------------------------------------------------------
string get_ed_buffer_save_file_name (string file)

// Return a filename for the ed buffer to be saved into.
//
// Arguments:
//   this_player(): The wizard using the editor.
//   file         : The name of the file currently in the buffer.
//
// Result:
//   The name of the file to save the buffer into, or 0.
//
// This function is called whenever a wizard is destructed/goes netdeath
// while editing. Using this function, his editing is not done in vain.
// The files are stored in /players/<name>/.dead_ed_files/*.

{
  string *file_ar;

  file_ar=efun::explode(file, "/");
  file=file_ar[sizeof(file_ar)-1];
  return WPATH+USERNAME(TP)+"/.dead_ed_files/"+file;
}


//===========================================================================
//  parse_command() Support  (!compat, SUPPLY_PARSE_COMMAND defined)
//
// LPMud has a builtin support for parsing complex commands.
// It does this by requestion several types of ids from the objects.
// The same queried functions are also in the master to provide decent
// defaults, especially for generic ids like 'all the blue ones'.
//
// Each of the functions has to return an array of strings (with the exception
// of parse_command_all_word), each string being one of the ids for that type
// of id.
//
// The whole parsing has a preference for the english language, so the
// the code for parsing english constructs is given as well.
//===========================================================================

// TODO: Update me from Lima

//---------------------------------------------------------------------------
string *parse_command_id_list ()

// Return generic singular ids.

{
  return ({ "one", "thing" });
}


//---------------------------------------------------------------------------
string *parse_command_plural_id_list ()

// Return generic plural ids.

{
  return ({ "ones", "things", "them" });
}


//---------------------------------------------------------------------------
string *parse_command_adjectiv_id_list ()

// Return generic adjective ids.
// If there are none (like here), return some junk which is likely never
// typed.

{
  return ({ "iffish" });
}


//---------------------------------------------------------------------------
string *parse_command_prepos_list ()

// Return common prepositions.

{
    return ({ "in", "on", "under", "behind", "beside" });
}


//---------------------------------------------------------------------------
string parse_command_all_word()

// Return the one(!) 'all' word.

{
  return "all";
}


//===========================================================================
//  FTP support
//
//===========================================================================

//---------------------------------------------------------------------------
string dtime(int wann)

// Return the time <wann> in a Unix-ls-like format.

{
  string monat, uhrzeit, wotag;
  int tag, jahr;

  sscanf(ctime(wann),"%s %s %d %s %d",wotag, monat, tag, uhrzeit, jahr);
  return wotag+(tag<=9 ? ",  " : ", ")+tag+". "+monat+" "+jahr+", "+uhrzeit;
}


//---------------------------------------------------------------------------
string show_dir (string wo, string user)

// Simulate a Unix-ls.
//
// Arguments:
//   wo  : name of the directory to list
//   user: name of the user requesting the list.
//
// Result:
//   The list of files (or an error message).
//
// TODO: This 'ls' should understand the most common flags, like -l, -R, -F.

{
  string ret;
  mixed *dir;
  int isdir,r,w,x;
  string tmp;
  object pl;
  int tim;
  string date,group,fname,*farr;

  if (!user) user=geteuid(TI);
  if (!wo && pl=FIND_PLAYER(user))
    wo=({string})pl->QueryCurrentDir();
  tim=time();
  isdir=(file_size(wo)==-2);
  ret="";
  for (dir=get_dir(isdir?wo+"/*":wo,7);sizeof(dir)>2;dir=dir[3..])
  {
    if (member(({".","..",".ftp"}), dir[0])==-1)
    {
      if (dir[0][<2..<1]!=".c")
        x='-';
      else
        x=(find_object(wo+"/"+dir[0]))?'x':'-';
      w=(valid_write(wo+"/"+dir[0],user,"write_file",0))?'w':'-';
      r=(valid_read(wo+"/"+dir[0],user,"read_file",0))?'r':'-';
      date=ctime(dir[2]);
      if ((tim-dir[2])<31536000)
        date=date[4..15];
      else
        date=date[4..10]+ctime()[19..];
      isdir=(dir[1]==-2);
      tmp=get_wiz_name(wo+"/"+dir[0]);
      if (tmp==ROOTID)
        tmp="root";
      else
        if (tmp==BACKBONEID)
          tmp="daemon";
        else
          if (!tmp)
            tmp="none";
      fname=wo+"/"+dir[0];
      farr=explode(fname,"/")-({"."});
      switch (farr[0])
      {
        case WDIR:
        group="wizard";
        break;
        case DDIR:
        if (sizeof(farr)>1)
          group=farr[1];
        else
          group="sys";
        break;
        group="news";
        break;
        case "mail":
        group="mail";
        break;
        case "open":
        group="public";
        break;
        case "p":
        group="project";
        break;
        default:
        group="sys";
      }
      ret+=sprintf("%c%c%c%cr--r-- 1 %-11s %-8s %8s %s %s%s\n",isdir?'d':'-',r,w,x,
                   tmp,group,
                   isdir?"0":sprintf("%d",dir[1]), date,dir[0],isdir?"":"");
    }
  }
  if (ret=="")
    ret="total 0\n";
  return ret;
}


//---------------------------------------------------------------------------
void FtpAccess(string host, string message, int port)

// Handle the interface to the NaseFTPD.

{
  string *comp;
  string reply, head;
  string *dir;
  int i;

#if __EFUN_DEFINED__(send_udp)
  comp = explode(message,"\t");
#define FTP_ID   0
#define FTP_SEQ  1
#define FTP_TAG  2
#define FTP_CMD  3
#define FTP_ARG1 4
#define FTP_ARG2 5
#define FTP_ARG3 6

  if (sizeof(comp) <= FTP_CMD || lower_case(comp[FTP_TAG]) != "req")
  {
    log_file("IMP_MSGS","Host: "+host+":"+port+" - '"+message+"'\n");
    return;
  }

  reply = "INVALID";
  head = sprintf("%s\t%s\tRPLY\t%s\t", comp[FTP_ID], comp[FTP_SEQ], comp[FTP_CMD]);

  switch (lower_case(comp[FTP_CMD]))
  {
    case "ping":
      if (sizeof(comp) <= FTP_CMD)
        break;
      reply = "OK";
      break;

    case "user":
      if (sizeof(comp) <= FTP_ARG1)
        break;
      reply = WPATH+lower_case(comp[FTP_ARG1]);
      break;

    case "pass":
      if (sizeof(comp) <= FTP_ARG2)
        break;
      comp[FTP_ARG1] = lower_case(comp[FTP_ARG1]);
      if (   CheckPasswd(comp[FTP_ARG1],comp[FTP_ARG2])
          && ("guest" == comp[FTP_ARG1] || IS_IMMORTAL(comp[FTP_ARG1]))
         )
        reply = "OK";
      else
        reply = "FAIL";
      break;

    case "resv":
      if (sizeof(comp) <= FTP_ARG3)
        break;
      if (   comp[FTP_ARG2] == ""
          || -1 == member("+~/", comp[FTP_ARG2][0]))
        comp[FTP_ARG2] = comp[FTP_ARG3]+"/"+comp[FTP_ARG2];
      reply = full_path(comp[FTP_ARG2], lower_case(comp[FTP_ARG1]));
      break;

    case "read":
      if (sizeof(comp) <= FTP_ARG2)
        break;
      comp[FTP_ARG1] = lower_case(comp[FTP_ARG1]);
      if (   (IS_IMMORTAL(comp[FTP_ARG1]) || comp[FTP_ARG2][0..4]=="/open")
          && comp[FTP_ARG2][0] == '/'
          && valid_read(comp[FTP_ARG2], comp[FTP_ARG1], "read_file", 0)
         )
        reply = "OK";
      else
        reply = "FAIL";
      break;

    case "writ":
      if (sizeof(comp) <= FTP_ARG2)
        break;
      if (   comp[FTP_ARG2][0] == '/'
          && valid_write(comp[FTP_ARG2], lower_case(comp[FTP_ARG1]), "write_file", 0)
         )
        reply = "OK";
      else
        reply = "FAIL";
      break;

    case "list":
      log_file("IMP_MSGS","Host: "+host+":"+port+" - '"+message+"'\n");
      if (sizeof(comp) <= FTP_ARG2)
        break;
      dir = explode(show_dir(comp[FTP_ARG2], comp[FTP_ARG1]),"\n");
      // Note: dir[<1] is "" and can be ignored.
      for (i = 0; i < sizeof(dir)-1; i += 20)
      {
	int j;
	j = i+20;
	if (j >= sizeof(dir)-1)
	  j = sizeof(dir)-2;
	reply = "LINE\t"+implode(dir[i..j], "\r\n")+"\r\n";
#if __VERSION__ > "3.5.2"
        send_udp(host, port, to_bytes(head+reply, "UTF-8"));
#else
        send_udp(host, port, head+reply);
#endif
      }
      reply = "OK";
      break;

    default:
      log_file("IMP_MSGS","Host: "+host+":"+port+" - '"+message+"'\n");
      break;
  }
#if __VERSION__ > "3.5.2"
  send_udp(host, port, to_bytes(head+reply, "UTF-8"));
#else
  send_udp(host, port, head+reply);
#endif
#endif
}


//===========================================================================
//  ERQ support
//
// The idea is to pass all send_erqs from socket objects through a central
// master callback, which calls the socket's callback only if the socket
// still exists. As a side effect we can limit the number of sockets.
//===========================================================================

#ifdef MASTER_ERQ_HANDLER

//---------------------------------------------------------------------------
void erq_callback( int|int * data, int len, closure orig_cb )

// Call the original callback if it is still valid.

{
  /* check is the orignal callback closure is still valid and bound
   * to a not destructed object. If it is, call it, if not, discard.
   */
  if ( ( closurep( orig_cb ) ) && ( to_object( orig_cb ) ) )
    funcall( orig_cb, data, len );
}


//---------------------------------------------------------------------------
int erq_flush_queue()

// Send all data in erq_queue to the erq till it does not accept any more.

{
  int res, i;

  /* try to send all entries of the queue
   */
  for ( i = 0; i < sizeof( erq_queue ); i++ ) {
    /* if the callback is invalid, use no callback but send data
     */
    if ( ( ! closurep( erq_queue[i][2] ) ) ||
         ( ! to_object( erq_queue[i][2] ) ) )
      erq_queue[i][2] = 0;

    /* send request and pass it through erq_callback with the original
     * callback closure as third parameter
     */
    res = efun::send_erq( erq_queue[i][0], erq_queue[i][1],
                          lambda( ({ 'd /*'*/, 'l /*'*/ }),
                                  ({ #'erq_callback /*'*/,
                                        'd /*'*/,
                                        'l /*'*/,
                                        erq_queue[i][2] }) ) );

    /* request was not succesful, adjust queue and call out for another
     * flush try
     */
    if (!res) {
      erq_queue = erq_queue[i..];
      call_out( #'erq_flush_queue /*'*/, 1 );
      return 0;
    }
  }

  /* queue complety sent, free mem
   */
  erq_queue = 0;

  /* and return status
   */
  return res;
}


//---------------------------------------------------------------------------
int erq_send( int req, mixed data, closure cb )

// Send data to the erq. If the erq does not accept any more, buffer the
// data in the erq_queue.

{
  int res;

  /* already entries in queue, append
   */
  if ( erq_queue) {
    erq_queue += ({ ({ req, data, cb }) });
    return 0;
  }

  /* if the callback is invalid, use no callback but send data
   */
  if ( ( ! closurep( cb ) ) || ( ! to_object( cb ) ) )
    cb = 0;

  /* send request and pass it through erq_callback with the original
   * callback closure as third parameter
   */
  res = efun::send_erq( req, data,
                        lambda( ({ 'd /*'*/, 'l /*'*/ }),
                                ({ #'erq_callback /*'*/,
                                      'd /*'*/,
                                      'l /*'*/,
                                      cb }) ) );

  /* erq does not accept data currently, create queue
   */
  if ( ! res ) {
    erq_queue = ({ ({ req, data, cb }) });
    call_out( #'erq_flush_queue /*'*/, 1 );
  }

  /* and return the status
   */
  return res;
}

#endif /* ifdef MASTER_ERQ_HANDLER */

//===========================================================================
//  Userinfo handling.
//
// The 'Userinfo' of a user is the data contained in the secure/save file,
// most notably the wizard level and the password.
// As this data is queried very often, the master implements a cache for
// the most recently queried data.
//===========================================================================

//---------------------------------------------------------------------------
int _sizeof_userlist()

// Return the number of entries in the Userinfo cache.

{
  return sizeof(nuserlist);
}

mapping _query_nuserlist()
{
  return nuserlist;
}

//---------------------------------------------------------------------------
mixed * _userlist_names()

// Return the cached names in the Userinfo cache.

{
  return m_indices(nuserlist);
}


//---------------------------------------------------------------------------
mixed * _cleanup_data()

// Return the statistical data about the cache cleanups.
//
// Result is an array of ints:
//   ({ Number of cleanups
//    , Number of entries processed
//    , Number of entries unprocessed
//    , Percentage of processed entries in relation to total number of entries
//   })

{
  return ({ _nr_cleanups, _nr_processed, _nr_unprocessed
          , (_nr_processed * 100) / ((_nr_processed+_nr_unprocessed)||1)
         });
}


//---------------------------------------------------------------------------
void _cleanup_uinfo()

// Clean the Userinfo Cache, and handle idling users.
//
// The functions removes all entries from the cache which hasn't been used
// for USER_TOUCH seconds. It schedules a recall for the next hour.
//
// As this function is called every hour, it also handles the automatic
// logout of idle players.
// TODO: The auto-idleout should go into a separate function?

{
  int i;
  mixed *u;
  mixed *ulist; // Copy of userlist in case of a evaluation too long

  while (remove_call_out ("_cleanup_uinfo") >= 0);
  call_out ("_cleanup_uinfo", 3600);
  _nr_cleanups++;

  u = map(users(), #'getuid);                                      /*'*/
  i = sizeof(nuserlist);
  ulist = m_values(nuserlist);

  _nr_unprocessed += i + sizeof(u);
  while (i-- > 0 && get_eval_cost() > 10000) {
    _nr_processed++;
    _nr_unprocessed--;
    if ( (sizeof(ulist[i]) > USER_TOUCH
          && (time()-ulist[i][USER_TOUCH] <= 3600))
        || (-1 != member(u, ulist[i][USER_NAME])))
        continue;
    m_delete(nuserlist, ulist[i][USER_NAME]);
  }
  /* Remove idle players */
  u = users();
  for (i = 0; i < sizeof(u); i++)
  {
    _nr_processed++;
    _nr_unprocessed--;
    if (u[i] && interactive(u[i])
             && QIDLE(u[i]) >= (sizeof(u) > 35 ? 3600 : 18000)
             && object_name(u[i]) != "/Mudlink")
      {
        tell_object( u[i]
                   , "Sorry, but you have been idle for too long time.\n"
                    +"Please relogin to continue mudding.\n\n");
        efun::remove_interactive(u[i]);
      }
  }
}


//---------------------------------------------------------------------------
mixed * find_userinfo (string user)

// Load the userinfo for <user> and return it.
//
// Argument:
//   user: the username to load the info for.
//
// Result:
//   An array with the userinformation (the data from his secure/save file)
//   or 0 if the user does not exist.
//
// The function adds caching to the mere reading of the secure/save file,
// thus speeding up the access to often requested names.

{
  mixed *userentry;
  string old_euid;

  if (!stringp(user))
    return 0;

  if (!member(nuserlist, user)) {
    old_euid = geteuid(TO);
    SETEUID(ROOTID);
    if (member(({ ROOTID, BACKBONEID, NULLID }), user) != -1
      || !restore_object(SECURESAVEPATH+user[0..0]+"/"+user))
      nuserlist += ([ user: ({ user, 0, -1, ({}), 0, 0, 0, time() }) ]);
    else {
      if (level >= 10) {
        level = LVL_WIZARD;
        save_object(SECURESAVEPATH+user[0..0]+"/"+user);
      }
      nuserlist += ([ user: ({ user, password, level, domains, shell, promotion_age,
                      elpromo, time() }) ]);
    }
    SETEUID(old_euid);
  }
  return nuserlist[user];
}


//---------------------------------------------------------------------------
void expunge_userinfo (string user)

// Remove the cached userinformation for a given user from the cache.
//
// Argument:
//   user: the name to flush the cached userinfo fo
//
// The function is used to remove the cached userinformation if it
// changed in the savefile (e.g. by manual edits).

{
  m_delete(nuserlist, user);
}


//---------------------------------------------------------------------------
static void save_userinfo (mixed* userinfo)

// Save the given userinformation.
//
// Argument:
//   userinfo: the userinfo to save in the secure/save file.
//
// Private function used by /secure/master.c internally.

{
  name = userinfo[USER_NAME];
  level = userinfo[USER_LEVEL];
  domains = userinfo[USER_DOMAIN];
  shell = userinfo[USER_OBJECT];
  password = userinfo[USER_PASSWORD];
  promotion_age = userinfo[USER_PROMOTION];
  elpromo = userinfo[USER_ELPROMO];
  save_object(SECURESAVEPATH+name[0..0]+"/"+name);
}

//---------------------------------------------------------------------------
int CheckPasswd(string user, string pw)

// Check a user/password combination.
//
// Argument:
//   user: the user to check the password for.
//   pw  : the given password.
//
// Result:
//   0 if the password does not match the one stored for the user,
//   non-0 if the password is correct.

{
  mixed *userentry;
  string cryptpw;

  if (!(userentry = find_userinfo(user))) return 0;
  cryptpw = userentry[USER_PASSWORD];
  if (cryptpw != "" && crypt(pw, cryptpw) != cryptpw) return 0;
  return 1;
}


//---------------------------------------------------------------------------
string delete_password (string user)

// Delete the password of the given user.
//
// Argument:
//   user: the name to delete the password for.
//
// Result:
//   "Ok." on success, else an error message.
//
// The function is to be used by wizards to remove the password of players
// or wizards, mostly if they forgot it.
// After the call to this function, the user can login without being asked
// for a password, so be careful!
// Any successful remove is logged in /log/PASSWORD.

{
  mixed *userentry;
  int i;

  if (!TI || !IS_WIZARD(TI))
    return "Error: No privilege.";
  if (!(userentry = find_userinfo(user)) || userentry[USER_NAME] != user)
    return "Error: Invalid username.";
  if (!member(nuserlist, user))
    return "Error: Invalid username.";
  nuserlist[user][USER_PASSWORD] = "";
  save_userinfo(nuserlist[user]);
  log_file("PASSWORD", ctime()+" "+to_string(TI)+" removed pwd of "+user+"\n");
  return "Ok.";
}


//---------------------------------------------------------------------------
int update_password (string old,string new)

// Change the password of a player.
//
// Argument:
//   previous_object(): the user changing his password
//   old              : the old password
//   new              : the new password
//
// Result:
//   0 if the action is forbidden (e.g. user gave wrong old password),
//   1 if the change succeeded.
//
// The function is slightly different from other changes of secure/savefile
// info, because it is called directly by the user.
// To prevent abuse, the caller must give the old password, and it must
// match the uid of the caller.

{
  mixed *userentry;
  string user;

  user = getuid(PREV);
  if (!(userentry = find_userinfo(user)))
    return 0;
  password = userentry[USER_PASSWORD];
  if (password != "" && crypt(old,password) != password)
    return 0;
  userentry[USER_PASSWORD] = crypt(new,0);
  nuserlist[user] = userentry;
  save_userinfo(userentry);
  return 1;
}


//===========================================================================
//  Wizardstatus and Wizardtype handling.
//
// This functions control and check the promotion/demotion of wizards.
//===========================================================================

//---------------------------------------------------------------------------
int change_user_level (mixed user, int level)

// Change the user level of a user.
//
// Argument:
//   user  : the name or object of the user to change the level for.
//   level : the new user level.
//
// Result:
//   0 if the attempt was denied,
//   non-0 if the type was changed.
//
// To prevent abuse, the function does some checks.
// Every change is logged in /log/USERLEVELS.

{
  mixed *userentry;
  int wlev, ilev;
  string name;
  object player;

  if (!user)
    return 0;
  if (!TI) {
    log_file ("USERLEVELS", ctime(time())+" "+getuid(player)
              +" tried to change by object from "+getuid(PREV)
              +" to "+level+"\n"
              );
    return 0;
  }
  if (TI != TP)
    return 0;

  name = stringp(user) ? user : USERNAME(user);

  if (!(userentry = find_userinfo(name)))
    return 0;

  wlev = userentry[USER_LEVEL];
  ilev = query_user_level(TI);

  // Don't allow non-wizards to wiz somebody else.
  if (level >= LVL_WIZARD && ilev < LVL_WIZARD)
    return 0;

  log_file ("USERLEVELS", ctime(time())+" "+geteuid(PREV)
                         +": update "+name+" from "+userentry[USER_LEVEL]
                         +" to "+level+"\n"
           );
  if (level >= LVL_WIZARD && userentry[USER_LEVEL] < LVL_WIZARD)
    userentry[USER_ELPROMO] = getuid(this_interactive());
  else
    userentry[USER_ELPROMO] = 0;
  userentry[USER_LEVEL] = level;
  nuserlist[user] = userentry;
  save_userinfo(userentry);
  return 1;
}


//---------------------------------------------------------------------------
int query_user_level (mixed obj)

// Return the user level of an object
//
// Argument:
//   obj: the name of a user, or an object, to query the level of.
//
// Result:
//   The found level.
//
// When passed objects, the function returns the user level of the associated
// euid.

{
  mixed *userentry;

  if (objectp(obj))
    obj = USERNAME(obj);
  if (obj && (userentry = find_userinfo(obj)))
    return userentry[USER_LEVEL];
  return 0;
}


//---------------------------------------------------------------------------
string create_wizard (string owner, mixed domain, object caller)

// Promote a wizard and create his environment.
//
// Arguments:
//   owner : The name of the player to promote to wizard.
//   domain: The domain the wizard shall be added to. 0 for freelancers.
//   caller: Tool/Object responsible for the promotion, defaults to
//           previous_object().
//
// Result:
//   0 on an illegal attempt,
//   the name of the created workroom on success.
//
// The function creates /players/<owner> and /players/<owner>/workroom.c,
// and cares for all the level setting.

{
  string def_workroom;
  string workroom, wizdir, dname;
  object player;

  player = FIND_PLAYER(owner);
  if (!player) return 0;
  if (!caller) caller = PREV;
  if (!change_user_level(player, LVL_WIZARD)) {
    tell_object(player, "That is an illegal attempt!\n");
    log_file ("USERLEVELS", ctime(time())+" create_wizard() failed for "
              +getuid(player)+"\n");
    return 0;
  }

  wizdir = WPATH + owner;
  workroom = WPATH + owner + "/workroom";
  if (file_size(wizdir) == -1) {
    mkdir(wizdir);
    tell_object(player, "You now have a home directory: "+wizdir+"\n");
  }
  def_workroom = ("#define NAME \"" + capitalize(owner) + "\"\n"+
                  read_file("/room/def_workroom.c"));
  if (file_size(workroom) > 0) {
    tell_object(player, "You already have a workroom!\n");
  }
  else if (write_file(workroom, def_workroom)) {
    tell_object(player, "You now have a workroom: " + workroom + "\n"+
                        "It'll be your new home.\n");
    player->SetHome(workroom);
  }
  else
    tell_object(player, "Failed to make a workroom for you!\n");

  if (stringp(domain) && add_domain_member (owner, domain))
    tell_object (player, "You are now member of the "+capitalize(domain)
                         +" domain.\n");

  return workroom;
}


//===========================================================================
//  Domain and Project membership handling.
//
// This functions promote/demote members of domains and projects.
//===========================================================================

//---------------------------------------------------------------------------
int domain_member (string user, string domain)

// Test if a user is a member of a specific domain/project.
//
// Argument:
//   user  : the user to test
//   domain: the domain/project to test the membership for.
//
// Result:
//   0: user is not member of domain
//   1: user is member of domain.

{
  mixed *userentry;
  string *domains;
  int i;

  if (!(userentry = find_userinfo(user)))
    return 0;
  if (!pointerp(userentry[USER_DOMAIN]))
    return 0;
  if (member(userentry[USER_DOMAIN],domain) != -1)
    return 1;
  if (member(userentry[USER_DOMAIN],"*"+domain) != -1)
    return 1;
  if (member(userentry[USER_DOMAIN],"+"+domain) != -1)
    return 1;
  return 0;
}


//---------------------------------------------------------------------------
int domain_master (string user, string domain)

// Test if a user is lord of a specific domain/project.
//
// Argument:
//   user  : the user to test
//   domain: the domain/project to test the lordship for.
//
// Result:
//   0: user is not lord of domain
//   1: user is lord of domain.

{
  return domain_member(user,"*"+domain);
}


//---------------------------------------------------------------------------
int domain_vice (string user, string domain)

// Test if a user is vice lord of a specific domain/project.
//
// Argument:
//   user  : the user to test
//   domain: the domain/project to test the viceship for.
//
// Result:
//   0: user is not vice lord of domain
//   1: user is vice lord of domain.

{
  return domain_member(user,"+"+domain);
}


//---------------------------------------------------------------------------
varargs int add_domain_member (string user, string dom, int isproject)

// Add a new member to a domain or project.
//
// Argument:
//   user     : the user to add
//   dom      : the domain/project to add the user to
//   isproject: if given and non-0, <domain> is a project.
//
// Result:
//   0: user could not be added to the domain/project.
//   1: user is now member of the domain/project.
//
// The add can be done by wizards.

{
  mixed *userentry;
  string domainlord;

  /* Check permission to do that */
  domainlord = geteuid(PREV);
  if (LVL_WIZARD > query_user_level(domainlord))
    return 0;
  if (!(userentry = find_userinfo(user)))
    return 0;
  if (userentry[USER_DOMAIN] == 0) {
    userentry[USER_DOMAIN] = ({dom});
  }
  if (member(userentry[USER_DOMAIN], dom) == -1) {
    userentry[USER_DOMAIN] += ({dom});
  }
  nuserlist[user] = userentry;
  save_userinfo(userentry);
  return 1;
}


//---------------------------------------------------------------------------
int remove_domain_member(string user, string dom)

// Remove a member from a domain or project.
//
// Argument:
//   user     : the user to remove
//   dom      : the domain/project to remove the user from
//
// Result:
//   0: user could not be removed from the domain/project.
//   1: user is no longer member of the domain/project.
//
// The remove can be done by wizards.

{
  mixed *userentry;
  string domainlord;

  /* Check permission to do that */
  domainlord = geteuid(PREV);
  if (LVL_WIZARD > query_user_level(domainlord))
    return 0;
  if (!(userentry = find_userinfo(user)))
    return 0;
  if (userentry[USER_DOMAIN] == 0) {
    return 0;
  }
  if (member(userentry[USER_DOMAIN], dom) != -1) {
    userentry[USER_DOMAIN] -= ({dom});
  }
  else {
    return 0;
  }
  nuserlist[user] = userentry;
  save_userinfo(userentry);
  return 1;
}


//---------------------------------------------------------------------------
varargs int add_domain_vice (string user,string dom, int isproject)

// Add a new vicelord to a domain or project.
//
// Argument:
//   user     : the user to add
//   dom      : the domain/project to add the user to as vice lord
//   isproject: if given and non-0, <domain> is a project.
//
// Result:
//   0: user could not be promoted in domain/project.
//   1: user is now vice lord of the domain/project.
//
// The add can be done by wizards.

{
  mixed *userentry;
  string domainlord;

  /* Check permission to do that */
  domainlord = geteuid(PREV);
  if (LVL_WIZARD < query_user_level(domainlord))
    return 0;
  if (!(userentry = find_userinfo(user)))
    return 0;
  if (userentry[USER_DOMAIN] == 0) {
    userentry[USER_DOMAIN] = ({"+"+dom});
  }
  if (member(userentry[USER_DOMAIN],"+"+dom) == -1) {
    userentry[USER_DOMAIN] += ({"+"+dom});
  }
  nuserlist[user] = userentry;
  save_userinfo(userentry);
  return 1;
}


//---------------------------------------------------------------------------
int remove_domain_vice (string user,string dom)

// Remove a vice lord from a domain or project.
//
// Argument:
//   user: the vice lord to remove
//   dom : the domain/project to remove the vice lord from
//
// Result:
//   0: user could not be demoted
//   1: user is no longer vice lord of the domain/project.
//
// The remove can be done by wizards.

{
  mixed *userentry;
  string domainlord;

  /* Check permission to do that */
  domainlord = geteuid(PREV);
  if (LVL_WIZARD < query_user_level(domainlord))
    return 0;
  if (!(userentry = find_userinfo(user)))
    return 0;
  if (userentry[USER_DOMAIN] == 0) {
    return 0;
  }
  if (member(userentry[USER_DOMAIN],"+"+dom) != -1) {
    userentry[USER_DOMAIN] -= ({"+"+dom});
  }
  else {
    return 0;
  }
  nuserlist[user] = userentry;
  save_userinfo(userentry);
  return 1;
}


//---------------------------------------------------------------------------
varargs int add_domain_master(string user,string dom, int isproject)

// Add a new lord to a domain or project.
//
// Argument:
//   user     : the user to add
//   dom      : the domain/project to add the user to as lord
//   isproject: if given and non-0, <domain> is a project.
//
// Result:
//   0: user could not be promoted in domain/project.
//   1: user is now lord of the domain/project.
//
// The add can be done by wizards.

{
  mixed *userentry;
  string domainlord;

  /* Check permission to do that */
  domainlord = geteuid(PREV);
  if (LVL_WIZARD < query_user_level(domainlord))
    return 0;
  if (!(userentry = find_userinfo(user)))
    return 0;
  if (userentry[USER_DOMAIN] == 0) {
    userentry[USER_DOMAIN] = ({"*"+dom});
  }
  if (member(userentry[USER_DOMAIN], "*"+dom) == -1) {
    userentry[USER_DOMAIN] += ({"*"+dom});
  }
  nuserlist[user] = userentry;
  save_userinfo(userentry);
  return 1;
}


//---------------------------------------------------------------------------
int remove_domain_master(string user,string dom)

// Remove a lord from a domain or project.
//
// Argument:
//   user: the lord to remove
//   dom : the domain/project to remove the lord from
//
// Result:
//   0: user could not be demoted
//   1: user is no longer lord of the domain/project.
//
// The remove can be done by wizards.

{
  mixed *userentry;
  string domainlord;

  /* Check permission to do that */
  domainlord = geteuid(PREV);
  if (LVL_WIZARD < query_user_level(domainlord))
    return 0;
  if (!(userentry = find_userinfo(user)))
    return 0;
  if (userentry[USER_DOMAIN] == 0) {
    return 0;
  }
  if (member(userentry[USER_DOMAIN], "*"+dom) != -1) {
    userentry[USER_DOMAIN] -= ({"*"+dom});
  }
  else {
    return 0;
  }
  nuserlist[user] = userentry;
  save_userinfo(userentry);
  return 1;
}

/****************************************************************************/
