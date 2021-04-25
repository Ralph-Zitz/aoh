//---------------------------------------------------------------------------
// secure/simul_efun.c
//
// Global accessible functions, which behave just like efuns but are not
// implemented directly into the gamedriver.
//
// TODO: Make this a /secure/kernel/...
// TODO: Wizlist handling
// TODO: Handling of living and player names.
// TODO: command-Stackhandling, including notifz_fail(), command()
//       It should be possible to store the data in the global wizlist entry,
//       so that the player object themselves don't need to store them.
// TODO: guard set_wizlist_info(), get_wizlist_info() such that "0" data is
//       secured.
// TODO: query_boot_time(), query_wiz_tag(), find_clones(), resolve_file()
// TODO: Generalized access to the 0-wizlist-vars.
// TODO: input_to() das INPUT_IGNBANG abfangen!
//---------------------------------------------------------------------------

#include "/i/sys/config.h"
#include "/i/sys/dirent.h"
#include "/i/messages.h"
// TODO: don't include this one!

#pragma strict_types

#define THIS      efun::this_object()
#define TI        efun::this_interactive()
#define TP        efun::this_player()
#define PREV      efun::previous_object()

// Set the current object to be the previous object so we can do clean
// call_other()s.
#define FAKE_OBJECT set_this_object(previous_object())

// How much call_out()s are allowed at once?
#define CALL_OUT_WARN 1000  // limit for re-enabling
#define CALL_OUT_MAX  1200  // upper limit

// The max size of a log file.
#define MAX_LOG_SIZE 50000

//---------------------------------------------------------------------------

nomask int prevent_shadow (object shadow) { return 1; }

//========================================================================
//  Security efuns
//========================================================================

//---------------------------------------------------------------------------
public nomask string prot2domain (string prot)

// Return the name of the domain if the protection concerns domain matters.
//
// Argument:
//   prot: the protection to analyze
//
// Result:
//   The name of the concerned domain, or 0.

{
  string *tmp;

  tmp = explode(prot,"@");
  if (sizeof(tmp) > 1 && member(({LORDID, MEMBERID}), tmp[0]) != -1)
    return tmp[1];
  return 0;
}

//---------------------------------------------------------------------------
public nomask varargs string get_permission (object obj, string domain)

// Get the permission of an object.
//
// Arguments:
//   obj   : the object
//   domain: the name of the domain if the permission concerns domain topics
//
// Result:
//   The permission of the object.
//   Normally this is the euid of the object (BOTTOMID if obj is 0).
//   However, if the object is 'unguarded' = the euid starts with '+',
//   the object's uid prepended with a '+' is used as permission.
//   Alternatively, if the object is or was interactive, the permission
//   as returned by SECUREINFO->Permission() is returned.

{
  FAKE_OBJECT;
  return (string) MASTER->_get_perm(obj, domain);
}

//---------------------------------------------------------------------------
public nomask varargs string *get_protection (string file, mixed user)

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
  FAKE_OBJECT;
  return (string *) MASTER->_get_prot(file, user);
}

//---------------------------------------------------------------------------
public nomask int cmp_perm (string perm1, string perm2)

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
  FAKE_OBJECT;
  return (int) MASTER->_compare_perms(perm1, perm2);
}

//---------------------------------------------------------------------------
public nomask varargs string eff_perm (string domain, mixed skip)

// Evaluate the current effective permission.
//
// Argument:
//   domain: the domainname if domain topics are concerned
//   skip  : if integer, the 'n'th object to start the scan with
//           else the object itself.
//
// Result:
//   The evaluated permission.
//
// The function scans back through the call stack and evaluates the lowest
// common denomitor of all permissions.

{
  FAKE_OBJECT;
  return (string) MASTER->_effective_perm(domain, skip);
}

//---------------------------------------------------------------------------
public nomask varargs int check_permission (mixed prot, mixed skip)

// Check the given protection against the effective permissions.
//
// Arguments:
//   prot: the protection to check against.
//         If it is an object, the object's permissions is used.
//   skip: if integer, the 'n'th object to start the scan with
//         else the object itself.
//
// Result:
//   != 0 if the action is allowed.

{
  string eperm;

  if (objectp(prot))
    prot = (string) MASTER->_get_perm(prot);
  FAKE_OBJECT;
  eperm = (string) MASTER->_effective_perm(prot2domain(prot), skip);
  return (int) MASTER->_compare_perms(eperm, prot) >= 0;
}

//---------------------------------------------------------------------------
public nomask varargs mixed unguarded ( closure code
                                      , mixed arg1, mixed arg2
                                      , mixed arg3, mixed arg4
                                      )

// Execute a closure unguarded (i.e. using the this_object()s full permissions).
//
// Arguments:
//   code   : the closure to execute.
//   arg1..4: optional arguments to pass to the closure.
//
// Result:
//   The result from the closure call.
//
// The code is executed within a catch(). If an error occurs, it is re-thrown.

{
  object prev;
  mixed rc, err;

  MASTER->_add_unguarded(prev = PREV);
  FAKE_OBJECT;
  err = catch(rc = funcall(code, arg1, arg2, arg3, arg4));
  MASTER->_sub_unguarded(prev);
  if (err)
    throw(err);
  return rc;
}

//========================================================================
//  New efuns
//========================================================================

//------------------------------------------------------------------------
public varargs string blueprint (mixed obj)

// Return the name of an object's blueprint.
//
// Argument:
//   obj: the object under inspection (default is the current object)
//
// Result:
//   The filename of the blueprint.

{
  int p;

  if (stringp(obj))
    return (p = member(obj, '#')) < 0 ? obj : obj[0..p-1];
  else if (objectp(obj) || obj = PREV)
    return (p = member(obj = file_name(obj), '#')) < 0 ? obj : obj[0..p-1];
  return 0;
}

#if 0
//------------------------------------------------------------------------
public varargs mixed call ( mixed obj, mixed fun
                          , mixed arg1, mixed arg2, mixed arg3, mixed arg4
                          , mixed arg5, mixed arg6, mixed arg7, mixed arg8
                          )

// Call a function in an object, but allow for default function.
//
// Arguments:
//   obj       : the object to call the function.
//   fun       : the name of the function to call.
//   arg1..arg8: the args to pass to the function.
//
// Result:
//   The result from the function call, or 0.
//
// As call_other(), the <fun>ction is called in <obj>ect. However, if
// the named function doesn't exist, <obj>::method() is called with
// <fun> as first argument, and the others following after. The result
// from that call is then returned as 'the' result.

{
  mixed rc;

  FAKE_OBJECT;
  if (call_resolved(&rc, obj, fun, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8))
    return rc;
  return call_other(obj, "method", fun, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
}
#endif

//------------------------------------------------------------------------
public varargs int clonep (object obj)

// Test if an object is a clone.
//
// Argument:
//   obj: the object to test (default is the current object)
//
// Result:
//   True for a clone (all interactives count as clones).

{
  return (obj || (obj = PREV))
         ? (query_once_interactive(obj) || member(file_name(obj), '#') > 0)
         : 0;
}

//------------------------------------------------------------------------
public int file_date (string file)

// Return the creation date of a file.
//
// Arguments:
//   file: The file under interest.
//
// Result:
//   The integer date of the file.

{
  mixed *data;

  return sizeof(data = get_dir(file, FILE_DATE)) ? data[0] : 0;
}

//------------------------------------------------------------------------
string *get_dir_ext (string path)

// Return identified contents of a directory, like get_dir().
//
// Argument:
//   path: The directorypath.
//
// Result:
//   An array of filenames, extended with some special indentification
//   characters:
//     a "/" will be appended to directories.
//     a "*" will be appended to files which have been loaded or inherited.

{
  string *strs, file;
  mixed *finfo;
  int i, s;

  finfo = get_dir(path, FILE_NAME | FILE_SIZE);
  if (finfo && (s = sizeof(finfo))) {
    strs = (string*)MASTER->_normalize(path, TP ? getuid(TP) : 0);
    path = sizeof(strs) ? "/" + implode(strs[0..<2], "/") + "/" : "/";
    strs = ({});
    while(s--, s-- > 0) {
      file = finfo[i++];
      switch(finfo[i++]) {
      case -2:
        file += "/";
        break;
      case -1:
        file += "@";
        break;
      default:
        if(find_object(path + file))
          file += "*";
      }
      strs += ({ file });
    }
  }
  return strs;
}

//------------------------------------------------------------------------
public object load (string file)

// Load an object.
//
// Argument:
//   file: the filename to load.
//
// Result:
//   The loaded blueprint.

{
  FAKE_OBJECT;
  call_other(file, "???");
  return find_object(file);
}

//------------------------------------------------------------------------
public varargs int netdeadp (object obj)

// Test if an object is netdead.
//
// Argument:
//   obj: the object to test (default is the current player).
//
// Result:
//   True if the object is netdead.

{
  return   (obj || (obj = TP))
         ? !interactive(obj) && query_once_interactive(obj)
         : 0;
}

//---------------------------------------------------------------------------
public varargs string normalize (string path, string user)

// Normalize a given filename.
//
// Arguments:
//   path: The filename to normalize.
//   user: The name to use with relative filenames (may be 0).
//
// Result:
//   The normalized filename.
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
  FAKE_OBJECT;
  return "/"+implode((string *)MASTER->_normalize(path,user), "/");
}

//----------------------------------------------------------------------------
public varargs string stime (int fmt, int date)

// Convert the given or current time into a string.
//
// Arguments:
//   fmt  : specifies which parts appear:
//            SD_WDAY  : day of the week
//            SD_DATE  : the date itself
//            SD_SDATE : the date itself, but w/o the century
//            SD_TIME  : the time
//          Default is (SD_WDAY | SD_DATE | SD_TIME) == SD_FULL
//   date : The numeric time as returned by time() to convert.
//          Default is time().
//
// Result:
//   The time as a string in a more european manner:
//   Example: 'Mon 15-Jun-1992 20:00:00'

{
  string cdate, rc;
  if (!fmt) fmt = SD_FULL;
  if (fmt & SD_DATE && fmt & SD_SDATE) fmt ^= SD_SDATE;
  if (date) cdate = ctime(date);
  else      cdate = ctime();
  rc = "";
  if (SD_WDAY & fmt) {
    rc = cdate[0..2];
    if (fmt & (SD_DATE | SD_TIME | SD_SDATE)) rc += " ";
  }
  if (SD_DATE & fmt) {
    rc = rc+cdate[8..9]+"-"+cdate[4..6]+"-"+cdate[20..23];
    if (fmt & SD_TIME) rc += " ";
  }
  if (SD_SDATE & fmt) {
    rc = rc+cdate[8..9]+"-"+cdate[4..6]+"-"+cdate[22..23];
    if (fmt & SD_TIME) rc += " ";
  }
  if (SD_TIME & fmt) rc = rc+cdate[11..18];
  return rc;
}

//===========================================================================
//  Redefined efuns
//===========================================================================

//---------------------------------------------------------------------------
static int CallOutStopped = 0;
static int CallOutCount = 0;
static int CountMessages = 0;
static string LastMessage;

varargs nomask void call_out (mixed what, int delay, mixed arg1, mixed arg2,
                              mixed arg3, mixed arg4, mixed arg5, mixed arg6,
                              mixed arg7, mixed arg8, mixed arg9, mixed arg10,
                              mixed arg11, mixed arg12, mixed arg13,
                              mixed arg14, mixed arg15)

// Setup a call to be executed after a given delay.
//
// Arguments:
//   what : function name or closure to call
//   delay: time in seconds to call the function after.
//          Minimal delay is 1 == next input round.
//   arg1..arg15: arguments to give to the call.
//
// This is a wrapper to the normal efun::call_out() which will reject
// additional callouts if already CALL_OUT_MAX callouts are active.
// These attempts will be logged in /log/adm/callout.
// If the number of callouts falls below CALL_OUT_MAX/4, new callouts
// are accepted again.
//
// Aim is to prevent the mud from Karniggel objects which hose the mud
// down with selfreplicating call_out()s.
//
// Written after an idea of Jof@Morgengrauen.

{
  object tmp;
  int num;
  string message;

  if (CallOutStopped)
  {
    num = sizeof(call_out_info());
    if (num < CALL_OUT_MAX/4)
    {
      message = "NOTE: call_outs reactivated, "+num+" pending.\n";
      // TODO: Announce message to all wizards.
      write_file("/log/adm/callout", ctime()+" call_outs reactivated.\n");
      CallOutStopped = 0;
    }
  }
  if (CallOutStopped)
  {
    message=sprintf("%O %O %O %O %O %O %O %O %O %O %O %O %O %O\n"
                   ,previous_object(),what,delay,arg1,arg2,arg3,arg4,arg5
                   ,arg6,arg7,arg8,arg9,arg10,arg11,arg12,arg13,arg14,arg15);
    if (LastMessage == message)
      CountMessages++;
    else
    {
      if (CountMessages)
      {
        write_file("/log/adm/callout"
                  , sprintf("Last message repeated %d times.\n",CountMessages)
                  );
        CountMessages=0;
      }
      write_file("/log/adm/callout", message);
      LastMessage = message;
    }
    return;
  }

  CallOutCount++;
  if (!(CallOutCount % 50))
  {
    CallOutCount = 0;
    num = sizeof(call_out_info());

    if (num > CALL_OUT_MAX)
    {
      message = "ERROR: Max number of call_outs exceeded. call_outs stopped.\n";
      // TODO: Announce message to all wizards.
      write_file("/log/adm/callout", ctime()+" *** EMERGENCY BREAK: "
                                     +num+" call_outs ***\n");
      CallOutStopped = 1;
      return;
    }

    if (num > CALL_OUT_WARN)
    {
      message = sprintf( "WARNING: %d call_outs pending, upper limit is %d.\n"
                       , num, CALL_OUT_MAX);
      // TODO: Announce message to all wizards.
    }
  }
  tmp = this_object();
  FAKE_OBJECT;
  efun::call_out(what,delay,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9
                , arg10,arg11,arg12,arg13,arg14,arg15);
  set_this_object(tmp);
}

//---------------------------------------------------------------------------
public nomask varargs closure bind_lambda (closure code, mixed target)

// Bind a lambda-closure to a target.
//
// Arguments:
//   code  : the closure to bind.
//   target: The designated target (default is caller).
//
// Result:
//   The bound closure or 0.
//
// The attempt will fail when binding closures to objects of higher
// level or to root/admin objects.

{
  string cperm, prot;
  int domain;

  if (intp(target) && (target == 0 || target == 1))
    target = PREV;
  prot = ((string *)MASTER->_get_prot(target))[2];
  domain = member(({LORDID, MEMBERID}), explode(prot, "@")[0]) != -1;
  FAKE_OBJECT;
  cperm = (string)MASTER->_effective_perm(domain);
  if (   (int) MASTER->_compare_perms(prot, ADMINID) >= 0
      && (int) MASTER->_compare_perms(cperm, prot) >= 0
     )
    return efun::bind_lambda(code, target);
  raise_error("Illegal object to bind closure to.\n");
  return 0;
}

#if 0
//---------------------------------------------------------------------------
public object find_living (string name)

// Find a living by name.
//
// Argument:
//   name: The name of interest.
//
// Result:
//   The livingobject with this name, or 0.
//
// This function also checks the name against the id() of all players.

{
  object pl, *pls;

  if (pl = efun::find_living(name)) return pl;
  if (sizeof(pls = filter_objects(users(), "id", name)))
    return pls[0];
}

//---------------------------------------------------------------------------
public object find_player (string name)

// Find a player by name.
//
// Argument:
//   name: The name of interest.
//
// Result:
//   The playerobject with this name, or 0.
//
// This function also checks the name against the id() of all players.

{
  object pl, *pls;

  if (pl = efun::find_player(name)) return pl;
  if (sizeof(pls = filter_objects(users(), "id", name)))
    return pls[0];
}
#endif

//---------------------------------------------------------------------------
public int log_file (string file, string txt)

// Write some text into a logfile.
//
// Arguments:
//   file: The name of the logfile (will be prefixed by "/log/").
//   txt : The text to log.
//
// Result:
//   True on success.
//
// If the size of the logfile exceeds MAX_LOG_SIZE, a backup is mde
// and a new file is started.

{
  FAKE_OBJECT;
  file = "/"+implode((string *)MASTER->_normalize("/log/"+file), "/");
  if (file_size(file) > MAX_LOG_SIZE)
    catch(rename(file, file + ".old"));
  return write_file(file, txt);
}

//---------------------------------------------------------------------------
public nomask void remove_interactive (object victim)

// Remove a netconnection.
//
// Arguments:
//   victim: the object to remove the connection from.
//
// The call succeeds only if performed by an admin or arch object.
// Else an error is raised.

{
  string cperm, prot;

  if (!victim || !interactive(victim))
    return;

  prot = ((string *)MASTER->_get_prot(victim))[2];
  FAKE_OBJECT;
  cperm = (string)MASTER->_effective_perm(0);
  if ((int) MASTER->_compare_perms(prot, ARCHID) >= 0 )
    efun::remove_interactive(victim);
  else
    raise_error("Illegal attempt to remove_interactive()\n");
}

//---------------------------------------------------------------------------
public nomask object query_snoop (object snoopee)

// Check if someone is snooped.
//
// Argument:
//   snoopee: The player being checked.
//
// Result:
//   The snooper, or 0.
//
// A snoop can be detected if the snooper is of same or lower level than the
// querying player.

{
  object snooper;

  if (!snoopee)
    return 0;
  snooper = efun::query_snoop(snoopee);
  if (!snooper)
    return 0;
  if (check_permission(get_permission(snooper)))
    return snooper;
  return 0;
}

//---------------------------------------------------------------------------
private static object _what_snoopee (object try, object snooper)

// Check if 'snooper' is snooping 'try'. Return 'try' on success, else 0.
// Auxiliary function to snoop().
{
  if (efun::query_snoop(try) == snooper)
    return try;
  else
    return 0;
}


public nomask int snoop (object me, object you)

// Snoop someone.
//
// Arguments:
//   me: The snopee.
//   you: The snooper, or 0 to stop a snoop.
//
// Result:
//   True on success.
//
// Snoops on someone of same or higher level are notified to the snoopee.
// Snoops by non-arches are logged in /log/adm/snoop.
// TODO: Special snoop free rooms.

{
  int rc;
  object *former;

  rc = 0;
  if (!me || !TI)
    return 0;

  former = filter(users(), #'_what_snoopee, me);
  if (you) { // Start a snoop from me on you
    if (me != you && you != TI && !efun::query_snoop(you)) {
      rc = efun::snoop(me, you);
    }
  }
    // else stop a snoop on me
  else if (me == TI || check_permission(get_permission(me,0)))
    rc = efun::snoop(me);

  // Notify former snoopee if wizard, make the log entry.
  if (   sizeof(former) && former[0] && rc)
  {
    if (   wizardp(former[0])
        && (!check_permission(former[0]) || wizlevel(me) == LEARNER_LVL)
       )
      tell_object(former[0], "*** "+capitalize(username(me))+" stopped snooping you.\n");

    if (wizlevel(me) < ARCH_LVL)
      log_file("adm/snoop"
              , sprintf(stime(SD_DATE|SD_TIME)+" %O on %O in %O *OFF*\n"
                             , me, former[0], environment(former[0]) || "<void>")
              );
  }

  // Notify new victim if wizard, make the log entry.
  if (   you && rc)
  {
    if (   wizardp(you)
        && (!check_permission(you) || wizlevel(me) == LEARNER_LVL)
       )
      tell_object(you, "*** "+capitalize(username(me))+" snoops you!\n");
    if (rc && you && wizlevel(me) < ARCH_LVL)
      log_file("adm/snoop"
              , sprintf(stime(SD_DATE|SD_TIME)+" %O on %O in %O\n"
                       , me, you, environment(you) || "<void>")
              );
  }

  return rc;
}

//---------------------------------------------------------------------------
public nomask void efun308 (object item, object dest)

// Set the new environment for an object.

{
  if (!PREV || (obj != PREV && !check_permission(ADMINID)))
  {
    raise_error("Illegal attempt to use efun308().\n");
    return;
  }
  FAKE_OBJECT;
  efun::efun308(item, dest);
}

//---------------------------------------------------------------------------
public nomask void set_this_player (object obj)

// Change this_player().

{
  if (!PREV || (obj != PREV && !check_permission(ADMINID)))
  {
    raise_error("Illegal attempt to change this_player().\n");
    return;
  }
  FAKE_OBJECT;
  efun::set_this_player(obj);
}

//---------------------------------------------------------------------------
public nomask int set_is_wizard(object obj, int what)

// Set or query the 'is_wizard' flag.

{
  if (what < 0)
    return efun::set_is_wizard(obj, -1);
  if (!check_permission(ADMINID))
  {
    raise_error("Illegal call to set_is_wizard()\n");
    return efun::set_is_wizard(obj, -1);
  }
  return efun::set_is_wizard(obj, what);
}

//---------------------------------------------------------------------------
public nomask void shutdown()

// Shut down the game, but log it.

{
  if (TP && PREV) {
    log_file("adm/game", stime() + " shutdown()\n" +
             " previous:    " + file_name(PREV) + " [" + geteuid(PREV) + "]\n" +
             " player:      " + file_name(TP)  + " [" + geteuid(TP)  + "]\n" +
             " interactive: " + (TI ? geteuid(TI) : "(none)") + "\n");
    efun::shutdown();
  }
}

//---------------------------------------------------------------------------
public void save_object (string file)

// Save the variables into the file <file>.o.
//
// Arguments:
//   file  : the filename to save to (without ".o" suffix)
//
// This is a safe guard against careless calls to save_object(), since the
// properties need extra handling.

{
  if (PREV)
    PREV->save(file);
}

//---------------------------------------------------------------------------
public int restore_object (string file)

// Restore the variables from the file <file>.o.
//
// Arguments:
//   file  : the filename to read from (without ".o" suffix)
//
// Result:
//   True if the restore was successful.
//
// This is a safe guard against careless calls to restore_object(), since the
// properties need extra handling.

{
 if (!PREV)
   return 0;
 return (int)PREV->restore(file);
}

//===========================================================================
// Efuns for Light support
//===========================================================================

//---------------------------------------------------------------------------
public varargs int see (mixed can, string cant)

// Display a visual to the current player.
//
// Arguments
//   can  : the string to display if the player can see
//          or an array of what should be ({ can, cant }).
//   cant : the string to display if the player can't see
//
// Result:
//   < 0: the player can't see because of darkness.
//   = 0: the player can see.
//   > 0: the player can't see because of bright.
//
// This is the counterpart to write().

{
  if (TP) {
    FAKE_OBJECT;
    if (pointerp(can))
      TP->catch_msg(({MSG_SEE})+can, THIS);
    else
      TP->catch_msg(({MSG_SEE, can, cant}), THIS);
    return TP->CantSee();
  }
}

//---------------------------------------------------------------------------
public varargs object * show(string can, mixed ce, mixed excl)

// Display a visual to the environment of the current player.
//
// Arguments
//   can  : the string to display if the players can see
//   ce   : the string to display if the players can't see
//          or an array of objects to exclude
//   excl : an array of objects to exclude
//
// Result:
//   An array of those objects which did see the 'can' message.
//
// This is the counterpart to say().

{
  object env;
  FAKE_OBJECT;
  if (!(env = environment(TP||THIS)))
    return ({});
  if (objectp(ce) || pointerp(ce))
    return (object *)env->display(can, objectp(ce) ? ({ TP, ce }) : ce+({ TP }));
  if (pointerp(excl))
    return (object *)env->display(({can, ce}), excl+({ TP }));
  if (pointerp(can))
    return (object *)env->display(can, ({ TP }));
  return (object *) env->display(({can, ce}), ({ TP }));
}

//---------------------------------------------------------------------------
public varargs int show_object(object obj, mixed can, string cant)

// Display a visual to a specific object.
//
// Arguments
//   can  : the string to display if the object can see
//   cant : the string to display if the object can't see
//
// Result:
//   < 0: the object can't see because of darkness.
//   = 0: the object can see.
//   > 0: the object can't see because of bright.
//
// This is the counterpart to tell_object().

{
  if (!obj)
    return 0;
  FAKE_OBJECT;
  if (pointerp(can))
    obj->catch_msg(({MSG_SEE})+can, THIS);
  else
    obj->catch_msg(({MSG_SEE, can, cant}), THIS);
  return obj->CantSee();
}

//---------------------------------------------------------------------------
public varargs void show_room(mixed room, string can, mixed ce, mixed excl)

// Display a visual to all objects in a room.
//
// Arguments
//   room : the room to display in.
//   can  : the string to display if the objects can see
//   ce   : the string to display if the objects can't see
//          or an array of objects to exclude
//   excl : an array of objects to exclude
//
// Result:
//   An array of those objects which did see the 'can' message.
//
// This is the counterpart to tell_room().

{
  if (!room)
    return ({});
  FAKE_OBJECT;
  if(pointerp(ce))
    return (object *) room->display(can, ce);
  if(pointerp(excl))
    return (object *) room->display(({can, ce}), excl);
  if (pointerp(can))
    return (object *) room->display(can);
  return (object *) room->display(({can, ce}));
}

//************************************************************************
