/*---------------------------------------------------------------------------
/*  secure/master.c
**
** The master is the gateway between the gamedriver and the mudlib to perform
** actions with mudlib specific effects.
** Calls to the master by the gamedriver have an automatic catch() in effect.
**
** Note that the master is loaded first of all objects. Thus you shouldn't
** inherit an other object, nor is the compiler able to search include files
** (read: they must be specified with full path).
**
** Amylaar: actually, you can inherit, but the file will be loaded then before
** the master, which isn't good for most files.
**
** 03-Sep-93 [Mateese]
**   Derived from master-skeleton.c, with concepts from Nightfall.
** 02-Oct-93 [Mateese]
**   /log/adm added, /log/ERR renamed to /log/err.
** 21-Feb-94 [Mateese]
**   notify_shutdown(), argument to inaugurate_master() added.
**   First routines for new security handling.
** 27-Nov-94 [Mateese]
**   Adapted for 3.2.1
** 28-Nov-94 [Mateese]
**   Changed 'unguarded' storage method.
** 27-Apr-95 [Mateese]
**   The modify_cmd_hook() empties the unguarded-list.
**   Load errors are not logged in /log/err.
**---------------------------------------------------------------------------
** Hooks not yet used:
**   H_NO_IPC_SLOT (@55)
**   H_TELNET_NEG (@60)
**   H_NOECHO (@85)
**   H_ERQ_STOP (@85)
**---------------------------------------------------------------------------
** TODO: Mudwho support.
** TODO: Erq support.
** TODO: Intermud/Naseftp support.
** TODO: Wizlist support.
** TODO: last-loader in compile_object() merken und in log_error mit ausgeben.
**---------------------------------------------------------------------------
*/

#include "/i/sys/config.h"
#include "/i/sys/driverhooks.h"
#include "/i/sys/kernel.h"
#include "/i/sys/stdprops.h"

#define PREV     previous_object()
#define PREVN(n) previous_object(n)
#define ENV(o)   environment(o)
#define THIS     this_object()
#define TP       this_player()
#define TI       this_interactive()

#define log(file,str) write_file("/log/"+file, str)

string get_file_perm (mixed obj); // forward

private mapping perm_matrix;   // Permission comparison matrix
private string *wlist, *dlist;

private object *unguardlist; // Copy of backbone_wizinfo[WINFO_UNGUARDED]

private string last_save_fname;  // last name used in save_object()

//---------------------------------------------------------------------------
nomask static void init_matrices ()
{
  perm_matrix = ([ ROOTID:0
                 , ADMINID:0
                 , ARCHID:0
                 , LORDID:0
                 , MEMBERID:0
                 , WIZARDID:0
                 , PLAYERID:0
                 , PRIVATEID:0
                 , BOTTOMID:0
                ]);
  perm_matrix[ROOTID]  = ([ ROOTID:0
                          , ADMINID:1
                          , ARCHID:1
                          , LORDID:1
                          , MEMBERID:1
                          , WIZARDID:1
                          , PLAYERID:1
                          , PRIVATEID:1
                          , BOTTOMID:1
                         ]);
  perm_matrix[ADMINID] = ([ ROOTID:-1
                          , ADMINID:0
                          , ARCHID:1
                          , LORDID:1
                          , MEMBERID:1
                          , WIZARDID:1
                          , PLAYERID:1
                          , PRIVATEID:-2
                          , BOTTOMID:1
                         ]);
  perm_matrix[ARCHID]  = ([ ROOTID:-1
                          , ADMINID:-1
                          , ARCHID:0
                          , LORDID:0
                          , MEMBERID:1
                          , WIZARDID:1
                          , PLAYERID:1
                          , PRIVATEID:-2
                          , BOTTOMID:1
                         ]);
  perm_matrix[LORDID]  = ([ ROOTID:-1
                          , ADMINID:-1
                          , ARCHID:0
                          , LORDID:-2
                          , MEMBERID:-2
                          , WIZARDID:-2
                          , PLAYERID:-2
                          , PRIVATEID:-2
                          , BOTTOMID:1
                         ]);
  perm_matrix[MEMBERID] = ([ ROOTID:-1
                           , ADMINID:-1
                           , ARCHID:-1
                           , LORDID:-1
                           , MEMBERID:-2
                           , WIZARDID:-2
                           , PLAYERID:-2
                           , PRIVATEID:-2
                           , BOTTOMID:1
                          ]);
  perm_matrix[WIZARDID] = ([ ROOTID:-1
                           , ADMINID:-1
                           , ARCHID:-1
                           , LORDID:-2
                           , MEMBERID:-2
                           , WIZARDID:-2
                           , PLAYERID:1
                           , PRIVATEID:-2
                           , BOTTOMID:1
                          ]);
  perm_matrix[PLAYERID] = ([ ROOTID:-1
                           , ADMINID:-1
                           , ARCHID:-1
                           , LORDID:-2
                           , MEMBERID:-2
                           , WIZARDID:-1
                           , PLAYERID:0
                           , PRIVATEID:-2
                           , BOTTOMID:1
                          ]);
  perm_matrix[PRIVATEID] = ([ ROOTID:-1
                            , ADMINID:-1
                            , ARCHID:-1
                            , LORDID:-2
                            , MEMBERID:-2
                            , WIZARDID:-2
                            , PLAYERID:-2
                            , PRIVATEID:-2
                            , BOTTOMID:1
                           ]);
  perm_matrix[BOTTOMID] = ([ ROOTID:-1
                           , ADMINID:-1
                           , ARCHID:-1
                           , LORDID:-1
                           , MEMBERID:-1
                           , WIZARDID:-1
                           , PLAYERID:-1
                           , PRIVATEID:-2
                           , BOTTOMID:0
                          ]);
  wlist = ({ PLAYERID, WIZARDID, WIZARDID, ARCHID, ADMINID });
  dlist = ({ BOTTOMID, MEMBERID, LORDID });
}

//===========================================================================
//  Auxiliary functions.
//===========================================================================

//---------------------------------------------------------------------------
string * _normalize (string path, string user)

// Normalize a given filename.
//
// Arguments:
//   path: The filename to normalize.
//   user: The name to use with relative filenames (may be 0).
//         It may be an ID-permission, which is then filtered out.
//
// Result:
//   The normalized filename, split up by it's '/'.
//
// Besides the elimination of embedded '..' the function expands following
// constructs:
//   /...        is treated normally.
//   ~<name>/... is replaced by /w/<name>/...
//   ~/...       is replaced by /w/<user/... if <user> is defined
//   +<name>/... is replaced by /d/<name>/...
//   .(.)/...    is replaced by <file_name(PREV)::DirPart>/...
//   ...         is replaced by <TP.CurrentDir>/... if TP is <user>
//                           by <file_name(PREV)::DirPart>/... else.
//
// Credits: First written by Hyp@Nightfall.

{
  string *strs;
  int p;

#define PATH_ARRAY(x) (efun::explode(x, "/")-({"","."}))

  if (!path)
    path = "";

  switch (path[0]) {
  case '/':
    if (!path[1]) return ({});
    strs = PATH_ARRAY(path);
    break;
  case '+':
    if (!path[1]) return ({"d"});
    if (path[1] == '/' && user)
      strs = ({"d", user}) + PATH_ARRAY(path[1..]);
    else
      strs = ({"d"}) + PATH_ARRAY(path[1..]);
    break;
  case '~':
    if (user && !path[1])
      return ({"w", user});
    if (user && path[1] == '/')
      strs = ({"w",user}) + PATH_ARRAY(path[2..]);
    else
      strs = ({"w"}) + PATH_ARRAY(path[1..]);
    break;
  case '.':
    strs = PATH_ARRAY(file_name(PREV))[0..<2] + PATH_ARRAY(path);
    break;
  default:
    if ( user && user[0] != '!' && TP && geteuid(TP) == user )
      strs = PATH_ARRAY(TP->Query(SP_CurrentDir)+"/"+path);
    else
      strs = PATH_ARRAY(file_name(PREV))[0..<2] + PATH_ARRAY(path);
  }
  while ((p = member(strs, "..")) != -1)
    strs = strs[0..p-2] + strs[p+1..sizeof(strs)];
  return strs;

#undef PATH_ARRAY
}

//---------------------------------------------------------------------------
varargs string _stime (int date)

// Convert the given or current time into a string.
//
// Arguments:
//   date : The numeric time as returned by time() to convert.
//          Default is time().
//
// Result:
//   The time as a string in a more european manner:
//   Example: 'Mon 15-Jun-1992 20:00:00'

{
  string cdate, rc;

  cdate = ctime(date||time());
  return  cdate[0..2]+" "
         +cdate[8..9]+"-"+cdate[4..6]+"-"+cdate[20..23]+" "
         +cdate[11..18];
}

//---------------------------------------------------------------------------
varargs string dump_current (int full, object ob, string txt)

// Dump the current call state into one string.
//
// Arguments:
//   full : If 0, just the previous object is dumped, else all previous objects.
//   ob   : An optional object to go first, may be 0.
//   txt  : The string to prefix <ob>.
//
// Result:
//   A printable string of the current call state.
//   All dumped entries (except for ob) are indented by 3 spaces.
//
// If this function is called by the master itself, that call is not dumped.
// It is used for logs when detecting security violations.

{
  int start, i;
  string rc;

  start = PREV == THIS ? 1 : 0;

  rc = "";
  if (ob) {
    rc = txt;
    if (living(ob) && query_once_interactive(ob)) {
      rc += capitalize(geteuid(ob)+" "+wizlevel(ob));
      if (!interactive(ob)) rc += " netdead.\n";
      else if (query_idle(ob)) rc += " idle "+query_idle(ob)+" sec.\n";
      else rc += " active.\n";
    }
  }
  rc += "   PrevObj: ";
  if (!PREVN(start)) rc += "?\n";
  else
    for (i = start; PREVN(i); i++) {
      if (i != start) rc += "            ";
      rc += file_name(PREVN(i))+" ["+getuid(PREVN(i))+"]\n";
      if (!full) break;
    }
  rc += "   Player : ";
  if (!TP) rc += "?\n";
  else     rc += file_name(TP)+" ["+geteuid(TP)+"]\n";
  rc += "   Interactive: ";
  if (!TI) rc += "?\n";
  else     rc += file_name(TI)+" ["+geteuid(TI)+"]\n";
  return rc;
}

//===========================================================================
//  Hook functions.
//===========================================================================

//---------------------------------------------------------------------------
static void move_hook (object item, object dest)

// Move object <item> into object <dest>.
// This hook also takes care of calling init() in all concerned objects.
// TODO: Allow moves into void.

{
  object *others;
  int i;
  efun308(item, dest);
  if (living(item))
  {
    efun::set_this_player(item);
    dest->init();
    if (environment(item) != dest)
      return;
  }
  others = all_inventory(dest);
  others[member(others, item)] = 0;
  for (i = 0; i < sizeof(others); i++)
    if (living(others[i]))
    {
      efun::set_this_player(others[i]);
      item->init();
    }
  if (living(item))
  {
    efun::set_this_player(item);
    filter_objects(others, "init");
  }
  if (living(dest) && item)
  {
    efun::set_this_player(dest);
    item->init();
  }
}

//---------------------------------------------------------------------------
static int create_hook (int what, object prev, object this)

// Object <this> has just been cloned/loaded/inherited by <prev>, now call the
// appropriate create: either create_clone()/create_obj() or create().
// Return the SP_ResetInterval setting to determine the time of the
// first reset().

{
  string fun;
  closure cl;

  // Using symbol_function() here is tempting, but leads to problems with
  // the this_object()/previous_object() relation on the actual call. *sigh*
  fun = ({"create_clone", "create_obj", "create_inh"})[what];
  if (!function_exists(fun, this))
    fun = "create";

  if (prev)
  {
    // The call to fun must be in from the prev object, so bind the call-lambda
    // to it.
    cl = bind_lambda(unbound_lambda(0, ({ #'call_other, this, fun })), prev);
    funcall(cl);
  }
  else
    call_other(this, fun);

  if (!this)  // don't stumble over self-destructing objects
    return 0;

  return (int)this->Query(SP_ResetInterval);
}

//---------------------------------------------------------------------------
static int reset_hook (object prev, object this)

{
  closure cl;

  if (prev)
  {
    cl = bind_lambda(unbound_lambda(0, ({#'call_other, this, "reset", 0 })), prev);
    funcall(cl);
  }
  else
    this->reset(0);

  if (!this)  // don't stumble over self-destructing objects
    return 0;

  this->Set(SP_LastReset, time());
  return (int)this->Query(SP_ResetInterval);
}

//---------------------------------------------------------------------------
static mixed modify_cmd_hook (string cmd, object player)

{
  mixed *info;

  // Empty the unguardlist in case a runtime error caused the
  // leftover of old data.
  // TODO: Empty the unguardlist just if it's not a nested command.
  //   See TubMud-player for how to find out.
  info = get_extra_wizinfo(0);
  info[WINFO_UNGUARDED] = ({});

  // TODO: Move the following all into the player.
  // TODO: player->PushCmdData();
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
static string _notify_fail_fun (string cmd)

{
  // TODO:
  //   mixed * rc;
  //  rc = ((mapping)this_player()->PopCmdData())["NotifyFail"];
  //  if (pointerp(rc))
  //    return funcall(rc[0]);
  return "What?\n";
}

//===========================================================================
//  Initialisation
//
// These functions are called after (re)loading the master to establish the
// most basic operation parameters.
//
// The initialisation of LPMud on startup follows this path:
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
void reset ()

// Do whatever is needed with every reset.

{
  write("----- "+_stime()+"  Master reset -----\n");
  garbage_collection();
}

//---------------------------------------------------------------------------
void inaugurate_master (int arg)

// Perform mudlib specific setup of the master.
//
// Argument:
//   arg: 0 if the mud just started.
//        1 if the master is a reactivated destructed one, which lost all
//             variables.
//        2 if the master is reactivated destructed one.
//        3 if the master was just reloaded.
//
// This function has at least to set up the driverhooks to use. Also, any
// mudwho or wizlist handling has to be initialized here.
//
// Besides that, do whatever you feel you need to do,
// e.g. set_auto_include_string(), or give the master a decent euid.

{
  mixed * info;

  if (!(info = get_extra_wizinfo(0)))
    set_extra_wizinfo(0, info = allocate(WINFO_SIZE));
  if (!pointerp(info[WINFO_UNGUARDED]))
    unguardlist = info[WINFO_UNGUARDED] = ({});

  if (!arg)
    info[WINFO_BOOTTIME] = time();

  if (arg == 1 || arg == 2)
    init_matrices();

  set_auto_include_string("#pragma combine_strings\n"
                          "#pragme verbose_errors\n"
                          "#include \"/i/sys/config.h\"\n"
                         );
  seteuid(ROOTID);
  set_driver_hook( H_MOVE_OBJECT0
  , unbound_lambda( ({'item, 'dest})
    , ({#',
       , ({#'?, ({#'!=, 'item, ({#'this_object})})
              , ({#'raise_error, "Illegal to move other object than this_object()\n"})
         })
       , ({#'move_hook, 'item, 'dest })
      })
    )
  );
  set_driver_hook( H_LOAD_UIDS
  , unbound_lambda( ({'object_name}), ({#'get_file_perm, object_name }) )
  );
  set_driver_hook( H_CLONE_UIDS
  , unbound_lambda( ({'blueprint, 'new_name}), ({#'get_file_perm, new_name }) )
  );
  set_driver_hook(H_CREATE_CLONE,
    unbound_lambda( 0,
      ({#'create_hook, 0, ({#'previous_object}), ({#'this_object}) })
                  ) );
  set_driver_hook(H_CREATE_OB,
    unbound_lambda( 0,
      ({#'create_hook, 0, ({#'previous_object}), ({#'this_object}) })
                  ) );
  set_driver_hook(H_CREATE_SUPER,
    unbound_lambda( 0,
      ({#'create_hook, 0, ({#'previous_object}), ({#'this_object}) })
                  ) );
  set_driver_hook(H_RESET,
    unbound_lambda( 0,
      ({#'reset_hook, ({#'previous_object}), ({#'this_object}) })
                  ) );
#if defined(H_CLEAN_UP)
  set_driver_hook(H_CLEAN_UP, "clean_up");
#endif
#if defined(H_MODIFY_COMMAND)
  set_driver_hook(H_MODIFY_COMMAND,
    unbound_lambda( ({ 'command_string, 'player})
                  , ({ #'modify_cmd_hook 'command_string, 'player })
                  ) );
#endif
#if defined(H_MODIFY_COMMAND_FNAME)
    set_driver_hook(H_MODIFY_COMMAND_FNAME, "modify_command");
#endif
#if defined(H_NOTIFY_FAIL)
  set_driver_hook(H_NOTIFY_FAIL,
    unbound_lambda( ({ 'command_string })
                  , ({ #'notify_fail_hook, 'command_string })
                  ) );
#endif
#if defined(H_INCLUDE_DIRS)
  set_driver_hook(H_INCLUDE_DIRS, ({ "/i/sys/", /i/", "/g/i/" }) );
#endif
  // TODO: Call daily_jobs() to (re)start various servers and to
  //       perform daily (an dalso initial) cleanup jobs.
  // Formula for the call_out-delay till next day, 2am:
  //   delay = (24+2)*60-((60*to_int(ctime()[11..12]))+to_int(ctime()[14..15])))*60;
  //   if (delay > 24*60) delay -= 24*60;
  //   call_out("daily_jobs", delay, 0);
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
  if (    sscanf(arg, "call %s %s", obj, fun) == 2
       || sscanf(arg, "call %s %s %s", obj, fun, rest) == 3)
  {
    write(obj+"->"+fun+"(\""+rest+"\") = ");
    write(call_other(obj, fun, rest));
    write("\n");
    return;
  }
  write("master: Unknown flag "+arg+"\n");
}

//---------------------------------------------------------------------------
static string * _file_array (string file)
{
  string data;
  data = read_file(file);
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
//   An array of strings, which designate the objects to be
//   preloaded with preload().
//   Any other result is interpreted as 'no object to preload'.
//   The resulting strings will be passed one at the time as
//   arguments to preload().
//
// The reboot of the game is logged in /log/adm/game.

{
  string *files, *tmp;

  log("adm/game", _stime()+"  Game started using "+__VERSION__
           +" on "+__HOST_NAME__+".\n");
  write("----- "+_stime()+"  Game started using "+__VERSION__
        +" on "+__HOST_NAME__+" -----\n");

  if (eflag) {
    write("  Epilog skipped (eflag = "+eflag+")\n");
    return 0;
  }

  tmp = _file_array("/adm/preload_file");
  write("  admin = "+sizeof(tmp)+" lines.\n");
  files = tmp;

  tmp = _file_array("/g/preload_file");
  write("  rsc   = "+sizeof(tmp)+" lines.\n");
  files += tmp;

  tmp = _file_array("/d/preload_file");
  write("  domain= "+sizeof(tmp)+" lines.\n");
  files += tmp;

  return files;
}

//---------------------------------------------------------------------------
void preload (string file)

// Preload a given object.
//
// Arguments:
//   file: The filename of the object to preload, as returned by epilog().
//
// You can of course do anything else with the passed strings - preloading
// is just the traditional task.

{

#define TIME(val) (""+(val/60)+":"+(val%60))

  int t1;
  string err;
  string name;

  if (   !file || !file[0] || file[0] == ';' || file[0] == '#'
      || file_size(file+".c") < 0)
    return;

  write("Preloading: "+file+"...");

  t1 = time();
  err = catch(call_other(file, "??"));
  if (err != 0) write(" Error "+err+"\n");
  else          write(" "+TIME(time()-t1)+"\n");

#undef TIME
}

//---------------------------------------------------------------------------
// void external_master_reload ()

// Reload the master on external request.
//
// If the gamedriver destruct and reloads the master on external request
// via SIGUSR1, it does this by a call to this function.
// However, it will be called after inaugurate_master() of course.
// If you plan to do additional magic here, you're welcome.

//---------------------------------------------------------------------------
void reactivate_destructed_master (int removed)

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

{
  init_matrices();
}

//---------------------------------------------------------------------------
string get_simul_efun ()

// Load the simul_efun object and return one or more paths of it.
//
// Result:
//   Either a single string with the file_name() of the simul_efun object,
//   or an array of strings which has to start with that file_name().
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
  if (!catch(call_other(SIMUL_EFUN, "???")))
    return SIMUL_EFUN;
  write("Failed to load "+SIMUL_EFUN+"\n");

  if (!catch(call_other(SPARE_SIMUL_EFUN, "???")))
    return SPARE_SIMUL_EFUN;
  write("Failed to load "+SPARE_SIMUL_EFUN+"\n");

  shutdown();
  return 0;
}


//===========================================================================
//  Handling of player connections
//
// See also valid_exec().
//===========================================================================

//---------------------------------------------------------------------------
object connect ()

// Handle the request for a new connection.
//
// Result:
//   An login object the requested connection should be bound to.
//
// Note that the connection is not bound yet!
//
// The gamedriver will call the lfun 'logon()' in the login object after
// binding the connection to it. That lfun has to return !=0 to succeed.

{ object ob;
  string err;

  err = catch(ob = clone_object(LOGIN));
  if (err) {
    write ("Error cloning "+LOGIN+"\n");
    write (err+"\n");
  }
  return ob;
}

//---------------------------------------------------------------------------
void remove_player (object player)

// Remove a player object from the game.
//
// Argument:
//   player: The player object to be removed.
//
// This function is called by the gamedriver to expel remaining players
// from the game on shutdown in a polite way.
// If this functions fails to quit/destruct the player, it will be
// destructed the hard way by the gamedriver.
//
// Note: This function must not generate runtime errors.

{
  if (_compare_perms(_effective_perm(0), ADMINID) < 0)
  {
    raise_error("Naughty, naughty...\n");
    return;
  }
  if (!objectp(player))
    return;
  catch(player->quit());
  if (player)
    catch(destruct(player));
}

//---------------------------------------------------------------------------
void disconnect (object obj)

// Handle the loss of an IP connection.
//
// Argument:
//   obj: The formerly interactive object (player).
//
// This called by the gamedriver to handle the removal of an IP connection,
// either because the connection is already lost ('Netdeath') or due to
// calls to exec() or remove_interactive().
// The connection will be unbound upon return from this call.

{
  // TODO: if (getuid(obj) != ROOTID) Announce netdeath of obj.
  if (obj->Query(SP_GuestNo))  // Netdead guests aren't tolerated
    remove_player(obj);
  else
    obj->Netdead(1);  // Inform the object that it's netdead now.
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

{
  // Try to reattach standard erq.
  // TODO: Prevent multiple calls for one loss.
  attach_erq_demon("", 0);
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

{
  object room, vmaster, obj;
  if (getuid(PREV) == ROOTID)
    return 0;
  vmaster = ((object)PREV->Get(SP_VMaster)) || PREV;
  room = (object)vmaster->compile_object(filename);
  if (!room && (obj = environment(PREV)))
  {
    vmaster = ((object)obj->Get(SP_VMaster)) || obj;
    room = (object)vmaster->compile_object(filename);
  }
  return room;
}

//---------------------------------------------------------------------------
string get_wiz_name (mixed file)

// Return the author of a file.
//
// Arguments:
//   file: The name of the file in question.
//
// Result:
//   The name of the file's author (or 0 if there is none).
//
// This function is called for maintenance of the wiz-list, to assign errors
// to the right wizard.
//
// We do it this way:
// - Files in /w/<wizname>/* return <wizname>
// - Files in /d/<domain>/<wizname> return <wizname>.
// - Other files in /d/<domain>/* return <domain>.
// - All other files return 0.

{
  string *strs;
  int s;

  if (objectp(file))
    strs = _normalize(file_name(file), 0);  // shouldn't happen
  else if (stringp(file))
    strs = _normalize(file, 0);
  else return 0;

  s = sizeof(strs);
  if (s < 2) return 0;

  switch (strs[0]) {
  case "d":
    if (s>2 && strs[2] != "base" && strs[2] != "secure")
      return strs[2];
    return strs[1];
  case "w":
    return s > 2 ? strs[1] : 0;
  }
  return 0;
}

//---------------------------------------------------------------------------
// string object_name (object obj)

// Return a printable name for an object.
//
// Arguments:
//   obj: The object which name is of interest.
//
// Result:
//   A string with the objects name, or 0.
//
// This function is called by sprintf() to print a meaningful name
// in addition to the normal file_name().
// If this functions returns a string, the object will be printed
// as "<filename> (<obj_name>)".


//---------------------------------------------------------------------------
void destruct_environment (object obj)

// Remove an object from its environment which is destructed.
//
// Arguments:
//   obj: The object whose enviroment is under destruction.
//
// When an object is destructed, this function is called with every
// item in that object. We get the chance to save players !

{
  if (!interactive(obj)) return;
  tell_object(obj, "Suddenly your environment dissolves before your eyes...\n");
  // TODO: if (catch(obj->move_player("is transferred", "/std/void")) && obj) destruct ob;
  // But check for the case that the /std/void itself s destructed
}

//---------------------------------------------------------------------------
void move_or_destruct (object what, object to)

// Rescue an object from an destructed environment.
//
// Arguments:
//   what: The object to be rescued.
//   to  : The destination of the object to be rescued.
//
// An error in this function can be very nasty. Note that unlimited recursion
// is likely to cause errors when environments are deeply nested

{
  int rc;
  object oenv;

  oenv = ENV(what);
  // TODO: Adapt this to the actual move() argument values.
  if (   catch(rc = (int) what->move(to, 1)) || rc != 1
      || (what && ENV(what) == oenv))  // rescue failed
    destruct(what);
}

//---------------------------------------------------------------------------
mixed prepare_destruct (object obj)

// Prepare the destruction of the given object.
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
  object super, item;

  if (!objectp(obj))
    return 0;

  // TODO if (interactive(obj) && obhj != login-object)
  //   Announce that a player leaves the world.
  // You might also want to check if the player has been legally leaving
  // the world, or illegally being destructed by some nasty coward.

  if (!query_shadowing(obj))
    for (item = shadow(obj, 0); item; item = super)
    {
      super = shadow(item, 0);
      funcall(bind_lambda(#'unshadow, item)); /* avoid deep recursion */
      destruct(item);
    }

  super = environment(obj);
  if (!super) {
    while (item = first_inventory(obj)) {
      destruct_environment(item);        // Give it opportunity to die gracefully
      if (item && environment(item) == obj)  // Make sure it's gone.
        destruct(item);
    }
  }
  else
    while (first_inventory(obj))
      move_or_destruct(first_inventory(obj), super);  // Try to rescue the thing.

  return 0;
}

//---------------------------------------------------------------------------
// void quota_demon (void)

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

//---------------------------------------------------------------------------
void receive_imp (string host, string msg, int port)

// Handle a received IMP message.
//
// Arguments:
//   host: Name of the host the message comes from.
//   msg : The received message.
//   port: The port number from which the message was sent.
//
// This function is called for every message received on the IMP port.

{
  log("adm/impmsgs", "Host: "+host+": '"+msg+"'\n");
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
//
// It is possible that the driver may reallocate some memory after the
// function has been called, and then run again into a low memory situation,
// calling this function again.
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
  object armageddon;
  string err;

  // TODO: if (TI) log (TI); else log(GAMEDRIVER);
  //       log(slow_shut called);
  // TODO: Clone only if armageddon doesn't exist or has less minutes to
  //       go yet.
  err = catch(armageddon = clone_object(ARMAGEDDON));
  if (err) {
    log ("game", _stime()+"  Can't load Armageddon: "+err);
    write ("Can't load Armageddon: "+err);
    // TODO: Announce GD's wish to shut down.
    return;
  }

  filter(users()
              , lambda( ({'u})
                      , ({#'&&, ({#'environment, 'u})
                              , ({#'!=, 'u, ({#'this_player})})
                              , ({#'tell_object, 'u
                                 , "The Gamedriver shouts: Damn! I ran out of memory!\n"})
                        })
              ));

  armageddon->Shutdown (minutes, "Memory exhausted.");
  // If Armageddon isn't where he is, move him away.
}

//---------------------------------------------------------------------------
// void notify_shutdown (void)

// Notify the master about an immediate shutdown.
//
// If the gamedriver shuts down, this is the last function called prior to
// severage of all Internet connections.
// The function has the opportunity to perform any cleanup operation, like
// informing the mudwho server that the mud is down, or updating the WIZLIST.

{
  filter(users(), #'tell_object
              , "Gamedriver shouts: "+MUDNAME+" shutting down immediately.\n");
}

//===========================================================================
//  Security and Permissions
//
// See also valid_read() and valid_write().
//===========================================================================

//---------------------------------------------------------------------------
void _add_unguarded (object obj)

// Add the given object to the list of unguarded objects.
// The function can't be called from other than root objects.
// Notice that one object may appear several times in the unguardlist.

{
  mixed *info;

  if (getuid(PREV) == ROOTID)
    unguardlist += ({ obj });
  unguardlist -= ({ 0 });
  info = get_extra_wizinfo(0);
  info[WINFO_UNGUARDED] = unguardlist;
}

//---------------------------------------------------------------------------
void _sub_unguarded (object obj)

// Remove the given object from the list of unguarded objects.
// The function can't be called from other than root objects.
// Notice that one object may appear several times in the unguardlist.

{
  int i;
  mixed *info;

  if (getuid(PREV) == ROOTID && -1 != (i = member(unguardlist, obj)))
    unguardlist[i..i] = ({});
  unguardlist -= ({ 0 });
  info = get_extra_wizinfo(0);
  info[WINFO_UNGUARDED] = unguardlist;
}

//---------------------------------------------------------------------------
string _domain_prot (string prot)

// Return the name of the domain if the protection concerns domain matters.
//
// Argument:
//   prot: the protection to analyze
//
// Result:
//   The name of the concerned domain, or 0.

{
  string *tmp;

  tmp = explode(prot,":");
  if (sizeof(tmp) > 1 && member(({LORDID, MEMBERID}), tmp[0]) != -1)
    return tmp[1];
  return 0;
}

//---------------------------------------------------------------------------
string _get_perm (object obj, string domain)

// Get the permission of an object.
//
// Arguments:
//   obj   : the object
//   domain: the domain name if the permission concerns domain topics.
//
// Result:
//   The permission of the object.
//   Normally this is the euid of the object (BOTTOMID if obj is 0).
//   However, if the object is 'unguarded', the object's uid is used
//   as permission.
//   Alternatively, if the object is or was interactive, the permission
//   as returned by SECUREINFO->Permission() is returned.

{
  string operm;
  int wlevel;

  if (!obj)
    operm = BOTTOMID;
  else if (-1 != member(unguardlist, obj))
    operm = getuid(obj);
  else if (query_once_interactive(obj))
    operm = (string)SECUREINFO->Permission(geteuid(obj), domain);
  else
    operm = geteuid(obj);

  return operm;
}

//---------------------------------------------------------------------------
static string *_get_prot_normal (string *path)

// Get the protections for a file.
//
// Arguments:
//   path : the normalized exploded filename to analyze.
//
// Result:
//   An array with the protections of the file:
//   [0]: the read protection
//   [1]: the write protection
//   [2]: the basic object permissions

{
  string * rc;

  rc = ({ BOTTOMID, BOTTOMID, BOTTOMID });

  if (!sizeof(path))
    return ({ BOTTOMID, ARCHID, BOTTOMID });

  switch (path[0])
  {
  case "adm":
    rc = ({ BOTTOMID, ADMINID, ROOTID });
    break;
  case "std":
    rc = ({ BOTTOMID, ADMINID, ADMINID });
    break;
  case "bin":
    rc = ({ BOTTOMID, ADMINID, ROOTID });
    break;
  case "d":
    if (sizeof(path) > 1)
    {
      rc = ({ BOTTOMID, LORDID+":"+path[1], LORDID+":"+path[1] });
      if (sizeof(path) > 2)
        if (path[2] == "base")
          rc = ({ BOTTOMID, MEMBERID+":"+path[1], LORDID+":"+path[1] });
        else
          rc = ({ BOTTOMID, MEMBERID+":"+path[1]+":"+path[2], LORDID+":"+path[1] });
    }
    else
      rc = ({ BOTTOMID, ARCHID, BOTTOMID });
    break;
  case "doc":
    rc = ({ BOTTOMID, ARCHID, BOTTOMID });
    break;
  case "g":
    rc = ({ BOTTOMID, ARCHID, ARCHID });
    break;
  case "i":
    if (sizeof(path) > 1 && path[1] == "sys")
      rc = ({ BOTTOMID, ADMINID, BOTTOMID });
    else
      rc = ({ BOTTOMID, ARCHID, BOTTOMID });
    break;
  case "log":
    if (sizeof(path) > 1 && path[1] == "adm")
      rc = ({ BOTTOMID, ARCHID, BOTTOMID });
    else
      rc = ({ BOTTOMID, BOTTOMID, BOTTOMID });
    break;
  case "save":
    rc = ({ BOTTOMID, ARCHID, BOTTOMID });
    if (sizeof(path) >= 2 && path[1] == "mail")
    {
      if (sizeof(path) == 4 && path[2][<2..<1] == ".o")
        rc = ({ PRIVATEID+":"+path[2][0..<3], ADMINID, BOTTOMID });
      else
        rc = ({ BOTTOMID, ADMINID, BOTTOMID });
    }
    break;
  case "secure":
    if (sizeof(path) > 1 && path[1] == "save")
      if (sizeof(path) > 3 && path[3][<2..<1] == ".o")
        rc = ({ WIZARDID+":"+path[2][0..<3], ADMINID, BOTTOMID });
      else
        rc = ({ BOTTOMID, ADMINID, BOTTOMID });
    else
      rc = ({ BOTTOMID, ROOTID, ROOTID });
    break;
  case "std":
    rc = ({ BOTTOMID, ADMINID, ADMINID });
    break;
  case "w":
    if (sizeof(path) > 1)
    {
      string name;
      name = WIZARDID+":"+path[1];
      if (sizeof(path) > 2 && path[2] == "secure")
        rc = ({ name, name, name });
      else
        rc = ({ BOTTOMID, name, name });
    }
    else
      rc = ({ BOTTOMID, ARCHID, BOTTOMID });
    break;
  }
  return rc;
}

//---------------------------------------------------------------------------
string *_get_prot (string file, mixed user)

// Get the protections for a file.
//
// Arguments:
//   file : the filename to analyze
//   user: The name to use with relative filenames (may be 0).
//         It may be an ID-permission, which is then filtered out.
//
// Result:
//   An array with the protections of the file:
//   [0]: the read protection
//   [1]: the write protection
//   [2]: the basic object permissions

{
  return _get_prot_normal(_normalize(file, user));
}

//---------------------------------------------------------------------------
string get_file_perm (mixed obj)

// Return the basic permissions for an object based upon its filename.
//
// Arguments:
//   obj: The object, given by pointer or by file_name().
//
// Result:
//   A string holding the permission string, or 0.
//
// This function is called for newly created objects.
// If the function returns 0, the object can't be loaded and is destructed
// again.

{
  string uid;
  int s;

  if (objectp(obj))
    uid = _get_prot(file_name(obj), 0)[2];
  else if (stringp(obj))
    uid = _get_prot(obj, 0)[2];
  else
    return 0;

  if (uid == BOTTOMID)
    return 0;

  return uid;
}

//---------------------------------------------------------------------------
int _compare_perms (string perm1, string perm2)

// Compare two permissions/protections.
//
// Arguments:
//   perm1 ,
//   perm2 : the two permissions to compare.
//
// Result:
//   = -2: the two perms are not compareable
//   = -1: perm1 < perm2
//   =  0: perm1 = perm2
//   =  1: perm1 > perm2

{
  string name1, name2, *tmp;
  string wname1, wname2;
  int rc;

  if (perm1[0] != '!' || perm2[0] != '!')
    return -2;

  tmp = explode(perm1, ":");
  perm1 = tmp[0];
  if (sizeof(tmp) > 1)
    name1 = tmp[1];
  else
    name1 = "";
  if (sizeof(tmp) > 2)
    wname1 = tmp[2];
  else
    wname1 = "";

  tmp = explode(perm2, ":");
  perm2 = tmp[0];
  if (sizeof(tmp) > 1)
    name2 = tmp[1];
  else
    name2 = "";
  if (sizeof(tmp) > 2)
    wname2 = tmp[2];
  else
    wname2 = "";

  rc = perm_matrix[perm1][perm2];
  if (rc != -2)
    return rc;

  // So far, all '-2' relations need a match in the names.
  if (name1 != name2)
    return -2;

  if (perm1 == PRIVATEID || perm2 == PRIVATEID)
    return 0;

  if (perm1 == LORDID && perm2 == MEMBERID)
    return 1;

  // Interdomain comparisons need a match in the second name as well.
  if (perm1 == MEMBERID && perm2 == MEMBERID)
    return wname1 == wname2 ? 0 : -1;

  return rc;
}

//---------------------------------------------------------------------------
varargs string _effective_perm (string domain, mixed skip)

// Evaluate the current effective permission.
//
// Argument:
//   domain: the domain name if the permission concerns domain topics
//   skip  : if integer, the 'n'th object to start the scan with
//           else the object itself.
//
// Result:
//   The evaluated permission.
//
// The function scans back through the call stack and evaluates the lowest
// common denomitor of all permissions.

{
  string cperm, perm;
  object prev;
  int i, rc, unguarded;

  if (!PREV || !PREVN(0))
    return ROOTID;

  if (skip && intp(skip))
    skip = PREVN(skip);
  else if (!skip)
  {
    perm = _get_perm(PREV, domain);
    unguarded = (-1 != member(unguardlist, PREV));
  }

  for (i = 0, prev = PREVN(i); prev && !unguarded; prev = PREVN(++i))
  {
    if (skip) {
      if (prev == skip) {
        perm = _get_perm(prev, domain);
        unguarded = (-1 != member(unguardlist, prev));
        skip = 0;
      }
      continue;
    }
    cperm = _get_perm(prev, domain);
    unguarded = (-1 != member(unguardlist, prev));
    rc = _compare_perms(perm, cperm);
    if (rc == 1)
      perm = cperm;
  }
  return perm;
}

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
//   input_to          : Object <who> issues an 'ignore-bang'-input_to() for
//                       commandgiver <arg3>; the exakt flags are <arg4>.
//   nomask simul_efun : Attempt to get an efun <arg> via efun:: when it
//                       is shadowed by a 'nomask'-type simul_efun.
//   rename_object     : Current object <who> renames object <arg>
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

{
  string perm;

  switch (op) {
    case "call_out_info":
    case "wiz_list_info":
      return 1;
    case "erq":
      if (how == ERQ_RLOOKUP)
        return 1;
      /* FALL THROUGH */
    case "attach_erq_demon":
    case "input_to":
    case "get_extra_wizinfo":
    case "set_extra_wizinfo":
    case "send_udp":
    case "bind_lambda":
    case "shadow_add_action":
    case "nomask simul_efun":
    case "set_driver_hook":
    case "set_this_object":
    case "set_auto_include_string":
    case "set_extra_wizinfo_size":
    case "symbol_variable":
      return _compare_perms(_effective_perm(0), ADMINID) >= 0;
    default:
      log("adm/privilege",
            "Op: "+op
          +" who: "+to_string(who)
          +" arg: "+to_string(arg)+"\n"
          +dump_current(0, 0, "  ")
         );
    }
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
// The function should deny shadowing on all root objects, else it might
// query the victim for clearance.

{
  if (_compare_perms(_get_perm(victim,0), ADMINID) >= 0)
    return 0;
  return !(int)victim->prevent_shadow(PREV);
}

int prevent_shadow (object shadow)
{
  return 1;
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
//                     command 'showsmallnewmalloced'?
//   "wizard"        : Is the player considered at least a "minimal" wizard?
//   "error messages": Is the player allowed to get run time error messages?
// TODO: are these all still in use?

{
  if (!TP)
    return 0;
  switch(what) {
  case "inspect memory":
  case "trace":
    return isArch(TP);
  case "error messages":
  case "wizard":
    return wizardp(TP);
  }
  return 0;
}

//---------------------------------------------------------------------------
int valid_exec (string name)

// Validate the rebinding of an IP connection by usage of efun exec().
//
// Arguments:
//    name: The name of the _program_ attempting to rebind the connection.
//          This is not the file_name() of the object, and has no leading
//          slash.
//
// Result:
//   Return a non-zero number to allow the action, any other value
//   to disallow it.
//
// It is a good idea to give just root programs (or just the login object)
// the clearance for this function, and then filtering the requests
// by simul_efun.c

{
  return get_file_perm(name) == ROOTID;
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
// It is a good idea to give just root objects the clearance for this
// function, and then filtering the requests by simul_efun.c

{
  return getuid(PREV) == ROOTID;
}

//---------------------------------------------------------------------------
int valid_snoop (object snoopee, object snooper)

// Validate the start/stop of a snoop.
//
// Arguments:
//   previous_object(): The asking object
//   snoopee          : The victim of the snoop.
//   snooper          : The wannabe snooper, or 0 when stopping a snoop.
//
// Result:
//   Return a non-zero number to allow the action,
//   any other value to disallow it.
//
// It is a good idea to give just root objects the clearance for this
// function, and then filtering the requests by simul_efun.c

{
  return getuid(PREV) == ROOTID;
}

//===========================================================================
//  Userids and depending Security
//
// micro abuses the userids to implement a completely different security
// system. We just have to take care that no object gets a '0' euid.
//===========================================================================

//---------------------------------------------------------------------------
string get_bb_uid()  // euids

// Return the string to be used as backbone-uid.
// It is just used by process_string() only if no this_object() is present.

{
  return BOTTOMID;
}

//---------------------------------------------------------------------------
int valid_seteuid (object obj, string neweuid) // euids

// Validate the change of an objects euid by efun seteuid().
//
// Arguments:
//   obj    : The object requesting the new euid.
//   neweuid: The new euid requested.
//
// Result:
//   Return 1 to allow the change, any other value to disallow it.
//
// Rootobjects can do anything.
// euid changes are allowed between BOTTOMID and uid.

{
  string operm;

  if (getuid(obj) == ROOTID)
    return 1;

  if (!stringp(neweuid))
    return 0;

  if (   strlen(neweuid) > 1 && neweuid[0] != '!'
      && query_once_interactive(obj)
     )
    operm = (string)SECUREINFO->Permission(neweuid);
  else
    operm = neweuid;

  if (_compare_perms(getuid(obj), operm) < 0)
    return 0;

  return 1;
}

//---------------------------------------------------------------------------
string valid_read (string path, string euid, string fun, object caller)

// Validate a reading file operation.
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
//   Or return 1 to indicate that the path can be used unchanged.
//
// The path finally to be used must not contain spaces or '..'s .
//
// valid_read() is called for these operations:
//   ed_start        (when reading a file)
//   file_size
//   get_dir
//   print_file
//   read_bytes
//   read_file
//   restore_object
//   tail

{
  string *strs, prot;

  switch (fun)
  {
  case "restore_object":
    euid = 0;
    break;
  case "ed_start":
    if (!path || !path[0])  // Read errorfile as default.
      path = "/w/"+(euid||"-")+"/.err";
    break;
  }

  strs = _normalize(path, euid);
  path = "/"+implode(strs, "/");
  prot = _get_prot_normal(strs)[0];

  if (_compare_perms(_effective_perm(_domain_prot(prot)), prot) >= 0)
    return path;

  return 0;
}

//---------------------------------------------------------------------------
string valid_write (string path, string euid, string fun, object caller)

// Validate a writing file operation.
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
//
// The path finally to be used must not contain spaces or '..'s .
//
// valid_write() is called for these operations:
//   cindent
//   ed_start     (when writing a file)
//   do_rename    (twice for each the old and new name)
//   mkdir
//   save_object
//   remove_file
//   rmdir
//   write_bytes
//   write_file

{
  string *strs, prot;

  switch (fun) {
  case "save_object":
    if (!path)
      return 0;
    euid = 0;
    break;
  case "ed_start":
    if (!path || !path[0])
      path = "/w/"+(euid||"-")+"/.err";
    break;
  }

  strs = _normalize(path, euid);
  path = "/"+implode(strs, "/");
  if ("save_object" == fun)
    last_save_fname = path;
  prot = _get_prot_normal(strs)[1];

  if (_compare_perms(_effective_perm(_domain_prot(prot)), prot) >= 0)
    return path;

  return 0;
}

//===========================================================================
//  Error Handling
//
//===========================================================================

//---------------------------------------------------------------------------
varargs string *error_logfile (mixed file, int loaderr)

// Return one or more approbiate geteuid(s) for an error logfile.
//
// Arguments:
//   file   : The name of file to contain the error.
//            It may be the object itself so that its file_name() is used.
//   loaderr: TRUE if the error is just a load error.
//            Load errors are not logged for the standard objects
//            as these happen too often.
//
// Result:
//   An array of the filegeteuid(s) where the error should be logged.
//
// The resulting filename is constructed using the cuid derived from
// the given filename:
//   no cuid        : Result = /log/err/generic
//   cuid = <wizard>: Result = /w/<wizard>/.err
//   cuid = <domain>: Result = /d/<domain>/.err and /d/<domain>/<wizard>/.err
//   cuid = <else>  : Result = /log/err/<else>

{
  string *lfile;
  string cuid, wuid;

  cuid = get_wiz_name(file);
  if (cuid && file_size("/w/"+cuid) == -2)
    lfile = ({ "/w/"+cuid+"/.err" });
  else if (cuid && file_size("/d/"+cuid) == -2)
  {
    lfile = ({ "/d/"+cuid+"/.err" });
    wuid = _normalize(file, cuid)[2];
    if (file_size("/w/"+wuid) == -2)
      lfile += ({ "/w/"+wuid+"/.err" });
  }
  else {
    cuid = get_file_perm(file) || BOTTOMID;
    switch (cuid) {
      case ROOTID  : cuid = loaderr ? "" : "root"; break;
      case BOTTOMID: cuid = loaderr _ "" : "bottom"; break;
      default      : cuid = cuid[1..];
    }
    lfile = cuid != "" ? ({ "/log/err/"+cuid }) : ({});
  }
  return lfile;
}

//---------------------------------------------------------------------------
void dangling_lfun_closure ()

// Handle a dangling lfun-closure.
//
// This is called when the gamedriver executes a closure using a vanished lfun.
// The proper handling is to raise a runtime error.
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
static string *last_log_file;
static int     last_log_time;

void log_error (string file, string err)

// Announce a compile-time error.
//
// Arguments:
//   file: The name of file containing the error (it needn't be an object
//         file!).
//   err : The error message.
//
// Whenever the LPC compiler detects an error, this function is called.
// It should at least log the error in one or more files, and also announce
// to the active player if it is an wizard.

{
  string *lfile;

#if __EFUN_DEFINED__(expand_define)
  if (code = expand_define("__FILE__"))
    err += "Compiling "+code+", Line "+expand_define("__LINE__")+"\n";
#endif

  if (wizardp(TI))
    write (err);

  lfile = error_logfile(file);
  if (time() > last_log_time+59 || lfile[0] != last_log_file[0])
  {
    last_log_time = time();
    last_log_file = lfile;
    err = " -- "+ctime()+" --\n"+err;
  }
  catch(map(lfile, #'write_file, err));
  return;
}

//---------------------------------------------------------------------------
static string *last_runt_file;
static int     last_runt_time;

void runtime_error (string err, string prg, string curobj, int line)

// Announce a runtime error.
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
// Note that <prg> denotes actually execute program (which might be inherited)
// where <curobj> is just the offending object.

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

  message = "Obj: "+curobj+", File: "+prg+", Line: "+line;
#if __EFUN_DEFINED__(expand_define)
  if (code = expand_define("__FILE__"))
    message += "\nCompiling "+code+", Line "+expand_define("__LINE__");
#endif
  file = error_logfile(curobj, err == "Error loading object");

  if ((theobj = find_object(curobj))
   && stringp(code = (string)theobj->Query(SP_Creator))
     )
  {
    file += error_logfile(code);
    message += "\nCreated by "+code;
  }
  if (time() > last_runt_time+59 || file[0] != last_runt_file[0])
  {
    last_runt_time = time();
    last_runt_file = file;
    map(file, #'write_file, " -- "+ctime()+" --\n"+message+"\n  "+err);
  }
  else
    map(file, #'write_file, message+"\n  "+err);

  if (TI)
  {
    if (!catch(flag = wizardp(TI)) && flag) {
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
      write("Your sensitive mind notices a wrongness in the fabric of space.\n");
      // TODO: Announce the error to wizards
    }
  }
  // TODO: Unwind CommandData stack.
}

//---------------------------------------------------------------------------
static string *last_heart_file;
static int     last_heart_time;

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
//
// Note that <prg> denotes actually execute program (which might be inherited)
// where <curobj> is just the offending object.


{
  object causer;
  string *lfile;
  string message;
  string code;
  string fname;
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
      message += " in "+file_name(causer);
    message += ", File: "+prg+", Line: "+line;
    lfile = error_logfile(curobj);
    if (time() > last_heart_time+59 || lfile[0] != last_heart_file[0])
    {
      last_heart_time = time();
      last_heart_file = lfile;
      map(lfile, #'write_file, " -- "+ctime()+" --\n"+message+"\n  "+err);
    }
    else
      map(lfile, #'write_file, message+"\n  "+err);
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
    if (!catch(flag = wizardp(culprit)) && flag) {
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
        // TODO: Announce error to wizards ("hb tuerned off")
        CWRITE("A wrongness in the fabric of space stops your heart!\n");
      }
      rc = 0;
    }
    else {
      if (causer != THIS) { // In case its in the microcode
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
          // TODO: Announce error to wizards ("offender can't bestructed, hb turned off")
          CWRITE("A wrongness in the fabric of space stops your heart!\n");
        }
        else {
          // TODO: Announce error and recovery to wizards
          CWRITE("Shortly you notice a wrongness in the fabric of space.\n");
        }
      }
    }
  }
  else { // culprit not interactive
    if (culprit)
      fname = file_name(culprit);
    else
      fname = "<zombie>";
    map(error_logfile(culprit), #'write_file
              ,file_name(culprit)+": Error in heartbeat\n"+message
               +"\n  "+err);
    // TODO: Announce error in hb of <fname> to wizards
    if (objectp(causer) && interactive(causer)) { // Don't remove players
      // TODO: Announce 'Offending object can't be destroyed. Heartbeat
      //       of <culprit> turned off.' to wizards
      rc = 0;
    }
    else if(causer) // Remove the offending object
      if (causer != THIS) {
        catch(causer->remove());
        if (causer) catch(destruct(causer));
      }
      if (causer || !culprit)
        rc = 0;  // Don't restart heart if causer is still there, or if
                 // culprit was the causer and got therefore removed.
      // TODO: Announce to wizards if causer could or couldn't be destructed
    }
  }

  // TODO: Unwind CommandData Stack

  return rc;

#undef CWRITE
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
  return "/"+implode(_normalize(str, geteuid(TP)), "/");
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
// This function saves the given integer into the file /w/<wizard/.edrc.

{ string file;

  if (!intp(code)) return 0;
  file = "/w/"+lower_case(geteuid(who))+"/.edrc";
  catch(rm(file));
  catch(write_file(file, to_string(code)));
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
// This functions reads the .edrc-file written by save_ed_setup().

{ string file;
  int code;

  file = "/w/"+geteuid(who)+"/.edrc";
  if (file_size(file) <= 0) return 0;
  sscanf(read_file(file), "%d", code);
  return code;
}


//---------------------------------------------------------------------------
string get_ed_buffer_save_file_name (string file)

// Return a filename for the ed buffer to be save into.
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
//
// The result is constructed out of the given filename as
//  '/w/<wizard>/.dead_ed/<file>'

{
  return "/w/"+geteuid(TP)+"/.dead_ed/"+(efun::explode(file, "/")[<1]);
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
// The whole parsing has a preferrence for the english language, so the
// the code for parsing english constructs is given as well.
//===========================================================================

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

/****************************************************************************/
