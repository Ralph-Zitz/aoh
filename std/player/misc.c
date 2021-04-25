/* Miscellaneous Commands
**
** Commands implemented here:
**   bug  [<object>:] [<text>]
**   idea [<object>:] [<text>]
**   typo [<object>:] [<text>]
**   laud [<object>:] [<text>]
**   localcmd [<object>] [long]
**   creator <object>
**   whoiswho
**   goto [church|living|player|object|room|target] <target>
**   target <targetname> [[living|player|object|room|target] <target>]
**   target <targetname> remove
**   target <targetname>
**   target list
**   onestepbeyond
**   osb
**   version
**   dict <text>
**
**   28-Dec-95 [m] Extracted from /std/player, added 'laud' and [<object>:]
**                 capability.
**   31-Jan-96 [m] whoiswho implemented.
**   14-Feb-96 [m] goto extracted from /std/player.
**   11-Mar-96 [m] targets added.
**   31-Jan-97 [m] localcmd added.
**   28-Feb-97 [m] creator added.
**   29-May-97 [M] added smore for P_HELP_MSG part of 'help'
**   09-Jun-97 [m] moved 'time' here from objects.c and cleant it up.
**   19-Jun-97 [M] added fmuffle and fcolor
**   28-jun-97 [t] added dict
**   17-Aug-97 [m] added 'version'
**   25-Apr-98 [t] moved time to /std/player/bin/
**
**  [m] Mateese
**  [M] Mica
**  [t] Tune
*/

#include <secure/wizlevels.h>
#include <secure/domaininfo.h>
#include <driver/sent.h>
#include <driver/commands.h>
#include <config.h>
#include <magic.h>
#include <moving.h>
#include <search.h>
#include <properties.h>
#include <daemons.h>
#include <stdrooms.h>
#include <msgclass.h>

#define THIS this_object()
#define TP   this_player()
#define TI   this_interactive()
#define ENV  environment
#define SF(x) #'x //' Emacs-Hack

public varargs mixed Query(string name); // std/base
public varargs mixed Set(string name, mixed value ); // std/base
string exp_fname (string fname); // std/player/filesys
int more(string fname, int flag); // std/player/filesys
public string QueryCurrentDir(); // std/player/filesys
int QueryPageSize(); // std/player/filesys
public string QuerySP();         // std/living/body
public varargs int ReduceSP(int diff, int min);
                                 // std/living/body
public string QueryRealName();   // std/living/description
public varargs int move(mixed dest, int method, mixed extra);
                                 // std/living/moving

public int QueryNoBuffering();      // std/player/message
public int SetNoBuffering( int n ); // std/player/message
/*-------------------------------------------------------------------------*/

mapping aTargets; // List of targets

/*-------------------------------------------------------------------------*/

public mapping QueryTargets() { return aTargets; }
public mapping SetTargets(mapping m) { return aTargets = m; }

/*-------------------------------------------------------------------------
** Add all commands we're implementing.
*/

void add_misc_commands()
{
  add_action("fidea", "idea");
  add_action("ftypo", "typo");
  add_action("fbug", "bug");
  add_action("fpraise", "laud");
  add_action("fcreator", "creator");
  add_action("fhelp", "help");
  add_action("fgoto", "goto");
  add_action("ftarget", "target");
  add_action("fmudname", lower_case(MUDNAME));
#if defined(MUDSHORT) && MUDSHORT != MUDNAME
  add_action("fmudname", lower_case(MUDSHORT));
#endif
  add_action("fversion", "version");
  if (IS_IMMORTAL(THIS))
  {
    add_action("flocalcmd", "localcmd");
  }
}

/*-------------------------------------------------------------------------*/
// lines below (bug/typo/idea) changed by Thragor 03dec93

private varargs void smart_log (string type, string msg, object env, object obj)
// Write message <msg> of <type> ("BUGS", "HELP", "IDEA", "TYPO") into the
// logfiles for object <obj> (default is the current environment).
{
  string creat, lmsg, suff;
  string *names;

  if (!env) env = ENV(TP);
  if (env == obj) obj = 0;

  suff = (type=="HELP")?".help":"";

  lmsg = ctime()+" "+type+" from "+QueryRealName();
  if (obj)
    lmsg += " for "+object_name(obj);
  lmsg += " in "+object_name(env)+":\n"+msg+"\n";

  if (!obj) obj = env;
  names = explode(object_name(obj), "/");
  if (creat = ({string})MASTER->creator_file(obj))
    write_file("/log/rep/"+creat+suff, lmsg);
  if ((names[0] == "d" || names[0] == "p") && sizeof(names) > 1)
    write_file("/log/rep/"+names[1]+suff, lmsg);
  write_file("/log/rep/"+type, lmsg);
}

/*-------------------------------------------------------------------------*/
public int _doreport2 ( string input, string type, string ack,
			 string nak, object env, object obj)
  // function has to be public, because it gets called by
  // input_to.
{
  if (!input || input == "")
    {
      write(nak || "Report cancelled.\n");
      return 1;
    }
  smart_log(type, input, env, obj);
  write(ack ||
	"Your report will be saved and checked. Thanks for your help.\n");
  return 1;
}

/*-------------------------------------------------------------------------*/
private int _doreport (string str, string type, string ack, string nak)

{
  int i;
  object obj;
  string sobj;

  if (str && -1 != (i = member(str, ':')) && i > 1 && str[i-1] != ' ')
  {
    sobj = norm_id(str[0..i-1]);
    obj = present(sobj, ENV(THIS))
          || present(sobj, THIS)
          || search(THIS, sobj, SEARCH_ENV_INV|SM_REALLY|SM_OBJECT|SM_IGNLIGHT);
    if (!obj)
    {
      write("No object '"+sobj+"' around here.\n");
      return 1;
    }
    do i++; while (str[i] == ' ');
    str = str[i..];
  }
  if (!str || str == "")
  {
    write("Enter your report-line: ");
    input_to( "_doreport2", 0, type, ack, nak, obj);
    return 1;
  }
  _doreport2(str, type, ack, nak, ENV(THIS), obj);
  return 1;
}

/*-------------------------------------------------------------------------*/
public int fbug (string str)

{
  return _doreport(str, "BUGS"
        , "Your bug-report will be saved and checked. Thanks for your help.\n"
        , "Bug report cancelled.\n");
}

public int ftypo (string str)
{
  return _doreport(str, "TYPO"
         , "Thanks for reporting the mispselingl :-). Your report got saved.\n"
         , "Typo report cancelled.\n");
}

public int fidea (string str)
{
  return _doreport(str, "IDEA"
       , "Idea saved. Thanks for helping to make this game more fun to play.\n"
         , "Idea report cancelled.\n");
}

public int fpraise (string str)
{
  return _doreport(str, "PRAISE"
         , "Praise saved. Thank you very much.\n"
         , "Praise cancelled.\n");
}

/*-------------------------------------------------------------------------*/
public int fcreator(string arg)
{
  object   obj;
  string   dname, wname, s_buffer;
  mixed  * data;
  string * path, * names, * lords, * vices, * _visible;
  int      j;

  if (!arg)
    arg = "here";
  if (arg)
    {
      if ("room" == norm_id(arg))
	arg = "here";
      obj =    present(arg, ENV(THIS))
	    || present(arg, THIS)
	    || search(THIS, arg, SEARCH_ENV_INV|SM_REALLY|
		                 SM_OBJECT|SM_IGNLIGHT);
      if (!obj)
	{
	  write("No object '"+arg+"' around here.\n");
	  return 1;
	}
    }

  arg = ({string})obj->QueryName() || ({string})obj->Short() || ({string})obj->IntShort() || arg;
  arg = ( (s_buffer = strip_article(arg))!=arg
	  ? "The "+s_buffer
	  : capitalize(arg)
        );

  path = explode(blueprint(obj)[1..], "/");
  if (-1 == member(({ PDIR, WDIR, DDIR }), path[0]))
    {
      write(arg+" is a standard object.\n");
      arg = ({string})obj->QueryCreator();
      path = explode((arg || "")[1..], "/");
      if (-1 == member(({ PDIR, WDIR, DDIR }), path[0]))
	{
	  write("\nNo more information is available.\n");
	  return 1;
	}
      write("It was created by an object from ");
    }
  else
    write(arg+" originates from ");

  switch(path[0])
    {
    case WDIR:
      wname = path[1];
      write(capitalize(add_gen(wname))+" collection.\n");
      obj = find_player(wname);
      if (obj && interactive(obj))
	{
	  if (!({int})obj->Query(P_INVIS))
	    write("\n"+capitalize(({string})obj->QueryPronoun())+" is online.\n");
	  else if (IS_IMMORTAL(THIS))
	    write("\n"+capitalize(({string})obj->QueryPronoun())+
		  " is invisibly online.\n");
	}
      return 1;
    case DDIR:
      dname = path[1];
      wname = path[2];
      if (!query_user_level(wname))
	{
	  wname = "";
	  write(capitalize(dname)+".\n\n");
	}
      else
	write(capitalize(dname)+", thought out by "+capitalize(wname)+".\n\n");
      break;
    case PDIR:
      dname = path[1];
      wname = "";
      write("the "+capitalize(dname)+" project.\n\n");
      break;
    }

  switch(({int})DOMAININFO->QueryState())
    {
    case DI_UNINITED:
      write("Sorry, no further information available at the moment.\n");
      DOMAININFO->check_state();
      return 1;
    case DI_INITING:
      write("  (the following information is incomplete at the moment)\n");
      break;
    }

  names = 0;
  lords = 0;
  _visible = 0;
  data = ({mixed *})DOMAININFO->QueryDomain(dname);
  if (   pointerp(data)
      && pointerp(data[DD_WIZ])
      && sizeof(data[DD_WIZ][DW_NAME])
     )
    {
      data = data[DD_WIZ];
      names = ({});
      lords = ({});
      vices = ({});
      _visible = ({ });
      for (j = 0; j < sizeof(data[DW_NAME]); j++)
	{
	  switch(data[DW_DSTAT][j])
	    {
	    case D_LORD:
	      lords += ({ capitalize(data[DW_NAME][j]) });
	      break;
	    case D_VICE:
	      vices += ({ capitalize(data[DW_NAME][j]) });
	      break;
	    default:
	      names += ({ capitalize(data[DW_NAME][j]) });
	      break;
	    }
	  obj = find_player(data[DW_NAME][j]);
	  if (   obj
	      && interactive(obj)
	      && (IS_IMMORTAL(THIS) || !({int})obj->Query(P_INVIS))
             )
	    _visible += ({ capitalize(data[DW_NAME][j]) });
	}
      if (sizeof(vices))
	vices = sort_array(vices, SF(>));
      if (sizeof(names))
	names = sort_array(names, SF(>));
      if (sizeof(_visible))
	_visible = sort_array(_visible, SF(>));
      if (sizeof(lords))
	vices = sort_array(lords, SF(>)) + vices;
      vices = vices + names;

      if (sizeof(vices))
	write(implode_list(vices, "Wizards of "+capitalize(dname)+" are "));
      else
	write(capitalize(dname)+" has no Wizards.\n");

      if (sizeof(_visible))
      {
	if (IS_IMMORTAL(THIS)) // otherwise check not necessary
	  for (j = sizeof(_visible); j--; )
	    {
	      obj = find_player(lower_case(_visible[j]));
	      if (({int})obj->Query(P_INVIS))
		      _visible[j] = "("+_visible[j]+")";
	    }
	write(implode_list(_visible, "Online are: "));
      }
      else
	write("No wizard of "+capitalize(dname)+" is online at the moment.\n");
    }
  else
    write("Sorry, no further information available at the moment.\n");

  return 1;
}

/*-------------------------------------------------------------------------*/
static object * find_target (string arg, int is_trans, int is_locate)

/* Find the target for a goto/in-command (is_trans is FALSE) or
 * a trans command (is_trans is TRUE).
 * is_locate has to be set if the target is just to be located (for adding
 * to the aTargets mapping).
 * Result is the ({ object, it's environment }) or 0.
 * If the object can't be found, a notify_fail() has been set.
 *
 * Syntaxes:
 *             <target>
 *   liv[ing]  <name>
 *   pl[ayer]  <name>
 *   obj[ect]  <object spec>|<object filename>
 *   room      <object spec>|<object filename>
 *   target    <name>
 */

{
  string spec, *tmp, name;
  object obj, env;
  int try, flag;
  mixed rc;

#define TRY_LIVING   (1<<0)
#define TRY_PLAYER   (1<<1)
#define TRY_OBJ_SPEC (1<<2)
#define TRY_OBJ_NAME (1<<3)
#define TRY_ROOM     (1<<4)
#define TRY_TARGET   (1<<5)
#define TRY_ALL      (0xFF)

  if (!IS_IMMORTAL(THIS))
  {
    if (!is_locate && QuerySP() < TELEPORT_COST)
    {
      notify_fail("Too low on power.\n", NOTIFY_NOT_VALID);
      return 0;
    }
    try = TRY_LIVING|TRY_PLAYER|TRY_OBJ_SPEC|TRY_TARGET;
  }
  else
    try = TRY_ALL;

  if (is_trans)
    try = try & ~(TRY_ROOM);

  tmp = explode(arg, " ") - ({ "", 0 });
  if (!sizeof(tmp))
  {
    if (is_trans)
      notify_fail("Trans whom?\n", NOTIFY_NOT_VALID);
    else
      notify_fail(capitalize(query_verb())+" where?\n", NOTIFY_NOT_VALID);
    return 0;
  }

  flag = 0;
  if (sizeof(tmp) > 1)
  {
    flag = 1;
    switch(lower_case(tmp[0]))
    {
      case "living": case "liv":
        try = try & TRY_LIVING;
        break;
      case "player": case "pl":
        try = try & TRY_PLAYER;
        break;
      case "obj": case "object":
        try = try & (TRY_OBJ_SPEC|TRY_OBJ_NAME);
        break;
      case "room":
        if (is_trans)
          try = try & (TRY_ROOM);
        else
          flag = 0;
        break;
      case "target":
        try = try & TRY_TARGET;
        break;
      default:
        flag = 0;
        break;
    }
  }
  spec = implode(tmp[(flag ? 1 : 0)..], " ");

  obj = env = 0;

  // Try to find the object by living name.
  if (try & TRY_LIVING)
  {
    obj = find_living(lower_case(spec));
  }
  // Try to find the object by player name.
  if (!obj && (try & TRY_PLAYER))
  {
    obj = find_player(lower_case(spec));
  }
  // Try to find the object by object specification.
  if (!obj && (try & TRY_OBJ_SPEC))
  {
    obj = is_trans ? search_env_inv(spec) : search_env(spec);
  }
  // Try to find the object by target specification.
  if (!obj && (try & TRY_TARGET) && sizeof(aTargets))
  {
    name = aTargets[lower_case(spec)];
    if (stringp(name))
      obj = find_object(name);
    if (!obj)
    {
      rc = catch(call_other(name, "???"));
      if (IS_IMMORTAL(THIS) && rc)
        notify_fail("Can't load "+arg+": "+rc);
      else
        obj = find_object(name);
    }
    if (!is_trans && obj && !living(obj))
      env = obj;
  }
  // Try to find the object by filename.
  if (!obj && (try & (TRY_OBJ_NAME|TRY_ROOM)))
  {
    name = exp_fname(spec);
    if (-1 == member("~+&/", name[0]))
      name = QueryCurrentDir()+"/"+name;
    name = resolve_file(name);
    obj = find_object(name);
    if (!obj && ".c" == name[<2..])
    {
      name = name[0..<3];
      obj = find_object(name);
    }
    if (!obj && -1 == member(name, '#'))
    {
      rc = catch(call_other(name, "???"));
      if (IS_IMMORTAL(THIS) && rc)
        notify_fail("Can't load "+arg+": "+rc);
      else
        obj = find_object(name);
    }
    if (!is_trans && obj)
      if (!living(obj) || TRY_ROOM == (try & (TRY_OBJ_NAME|TRY_ROOM)))
        env = obj;
  }

  // Evaluate the finding. If env is valid here, it is the room to go into.
  if (!obj)
  {
    if (!is_locate)
      notify_fail("Can't find "+arg+" to "+(is_trans ? "trans" : "go to")
                 +".\n", NOTIFY_NOT_VALID);
    else
      notify_fail("Can't locate "+arg+".\n", NOTIFY_NOT_VALID);
    return 0;
  }
  if (!env)
    env = environment(obj);

  return ({ obj, env });

#undef TRY_LIVING
#undef TRY_PLAYER
#undef TRY_OBJ_SPEC
#undef TRY_OBJ_NAME
#undef TRY_ROOM
#undef TRY_ALL
}

/*-------------------------------------------------------------------------*/
public varargs int fgoto (string arg, int by_teleport)

/* Syntaxes:
 *   goto           <target>
 *   goto liv[ing]  <name>
 *   goto pl[ayer]  <name>
 *   goto obj[ect]  <object spec>|<object filename>
 *   goto room      <object spec>|<object filename>
 *   goto target    <name>
 */

{
  object * target, env;
  int flag;

  if (!arg)
  {
    notify_fail("Goto where?\n", NOTIFY_NOT_VALID);
    return 0;
  }

  if (!IS_IMMORTAL(THIS) && !by_teleport)
  {
    if (lower_case(arg) != "church" || LSTATE_GHOST != Query(P_LSTATE))
      return 0;
    flag = move(STARTROOM, M_TELEPORT);
    if (ME_OK != flag)
      write("Ops, you can't go there.\n");
    return 1;
  }

  target = find_target(arg, 0, 0);
  if (!target)
    return 0;
  env = target[1];

  if (!env)
    write("Uh-oh, "+arg+" floats in the void.\n");
  else if (env == environment(THIS))
    write("You are already there.\n");
  else
  {
    if (!IS_IMMORTAL(THIS))
      ReduceSP(TELEPORT_COST, -1);
    flag = move(env, M_TELEPORT);
    if (ME_OK != flag)
    {
      if (!IS_IMMORTAL(THIS))
        write("You can't go there.\n");
      else
        write("The move failed with code "+flag+".\n");
    }
  }
  return 1;
}

// teleport() seems to be called by magic objects.
// Verify this.

public int teleport(string arg) {
  log_file("COMPAT", sprintf("%O: %O->teleport(%O)\n"
                            , THIS, previous_object(), arg));
  return fgoto(arg, 1);
}

/*-------------------------------------------------------------------------*/
public varargs int ftarget (string arg)

/* Syntaxes:
 *   target <targetname> [living|player|object|room|target] <target>
 *   target <targetname> remove
 *   target <targetname>
 *   target list
 */

{
  string * args;
  string   tname;
  object * target;
  int      i;

  if (!IS_IMMORTAL(THIS) || !arg)
    return 0;

  args = explode(arg, " ") - ({ 0, "" });
  if (sizeof(args) < 1)
    return 0;

  if (sizeof(args) == 1 && lower_case(args[0]) == "list")
  {
    if (!sizeof(aTargets))
      write("No targets specified.\n");
    else
    {
      tname = "";
      args = sort_array(m_indices(aTargets), SF(>));
      for (i = 0; i < sizeof(args); i++)
        tname += sprintf("%-15.15'.'s %s\n", args[i], aTargets[args[i]]);
      write(tname);
    }
    return 1;
  }

  if (sizeof(args) < 2)
  {
    if (!sizeof(aTargets))
      write("No targets specified.\n");
    else
    {
      tname = aTargets[args[0]];
      if (!tname)
        write(sprintf("'%s' is no target.\n", args[0]));
      else
        write(sprintf("%-15.15'.'s %s\n", args[0], tname));
    }
    return 1;
  }

  tname = args[0];

  if (sizeof(args) == 2 && args[1] == "remove")
  {
    if (!sizeof(aTargets) || !aTargets[tname])
      write(sprintf("No target '%s' there to remove.\n", tname));
    else
    {
      aTargets = m_delete(aTargets, tname);
      write(sprintf("Target '%s' removed.\n", tname));
    }
    return 1;
  }

  target = find_target(implode(args[1..], " "), 1, 1);
  if (!target)
    return 0;
  if (!sizeof(aTargets))
    aTargets = ([ tname : to_string(target[0]) ]);
  else
    aTargets[tname] = to_string(target[0]);

  write(sprintf("Target '%s' set to '%s'.\n", tname, to_string(target[0])));
  return 1;
}


/*-------------------------------------------------------------------------*/
public varargs int fhelp (string arg)

/* Syntaxes:
 *   help [command|cmd|topic] <topic>
 *   help <object>
 */

{
  string fname;
  object obj;
  string * args;
  string sobj;
  string tmp;

  notify_fail("Sorry, no help available. Try 'help' alone.\n");

  if (arg && -1 != member(arg, '/'))
    return 0;

  fname = 0;

  if (!arg)
    fname = "/doc/help";
  else
  {
    args = norm_id(arg, 1);
    if (sizeof(args) > 1
     && -1 != member( ({ "command", "topic", "cmd" }), args[0])
       )
      fname = "/doc/helpdir/"+args[1];
    else
    {
      sobj = implode(args, " ");
      obj = present(sobj, ENV(THIS))
            || present(sobj, THIS)
            || search(THIS, sobj, SEARCH_ENV_INV|SM_REALLY|SM_OBJECT|SM_IGNLIGHT);
      if (!obj && "room" == sobj)
	      obj = ENV(TP);

      if (!obj)
        fname = "/doc/helpdir/"+args[0];
    }
  }

   // TODO: 'mail' can be both object and keyword. Resolve this.
  if (!obj)
  {
    if (!fname || file_size(fname) <= 0)
    {
      if (arg)
	      fname = ({string})KEYWORD_D->QueryKeyword(args[0]);
    }
    if (!fname || file_size(fname) <= 0)
      return 0;

    tmp = read_file( fname, 0, 0 );
    "/lib/string"->smore( tmp );
  }
  else
  {
    mixed msg;
    msg = funcall(({mixed})obj->Query(P_HELP_MSG));
    if (!stringp(msg))
    {
      write("There is nothing special to know about that.\n");
      smart_log("HELP", "No helpmsg available for '"+arg+"'.", environment(obj), obj);
    }
    else if (msg != "")
      "/lib/string"->smore(msg); /* was write(msg) [Mica] */
    else
      write("There is nothing special about it.\n");
  }
  return 1;
}

/*-------------------------------------------------------------------------*/
public varargs int flocalcmd (string arg)

/* Syntaxes:
 *   localcmd [<object>] [long]
 */

{
  object obj;
  string   sobj, text;
  string * args;
  mixed * data;
  int long, i;

  obj = 0;
  long = 0;

  if (arg)
  {
    args = norm_id(arg, 1);

    long = member(args, "long");
    if (-1 != long)
    {
      args[long..long] = ({ });
      long = 1;
    }
    else
      long = 0;

    sobj = implode(args, " ");
    if (sizeof(sobj))
    {
      obj = present(sobj, ENV(THIS))
            || present(sobj, THIS)
	    || find_player(sobj)
            || find_living(sobj)
            || find_object(sobj)
            || search(THIS, sobj, SEARCH_ENV_INV|SM_REALLY|SM_OBJECT|SM_IGNLIGHT)
            ;
      if (!obj)
      {
        notify_fail("Can't find object '"+sobj+"'.\n");
	return 0;
      }
    }
  }

  if (!obj)
    obj = this_player();

  if (!long)
  {
    text = sprintf("Local commands:\n%-"+({int})obj->QueryPageWidth()+"#s\n",
                   implode(query_actions(obj), "\n"));
    "/lib/string"->smore(text,QueryPageSize());
  }
  else
  {
    data = query_actions(obj, QA_FUNCTION|QA_OBJECT|QA_TYPE|QA_VERB);
    text = sprintf("Local commands for %O:\n", obj);
    for (i = 0; i < sizeof(data); i += 4)
      text += sprintf("  %-10O : %c %O->%O\n", data[i]
		     , data[i+1] & SENT_SHORT_VERB ? '*' : ' '
		     , data[i+2], data[i+3]);
    if (sizeof(data) > QueryPageSize())
      "/lib/string"->smore(text,QueryPageSize());
    else
      efun::write(text);
  }

  return 1;
}

/*-------------------------------------------------------------------------*/
public int fmudname(string arg)

{
  if (arg)
    return 0;
  write("At your service!\n");
  return 1;
}

/*-------------------------------------------------------------------------*/
public int fversion (string arg)

{
  string s;

  if (arg)
  {
    notify_fail("'version' takes no arguments.\n");
    return 0;
  }

  if (-1 == member(__VERSION__, '@'))
    s = "LDMud " __VERSION__;
  else
    s = "LPMud " __VERSION__ " (Amylaar)";
  msg_write(CMSG_GENERIC,
"This is " MUDNAME ", version "+({string})LIBVERSION_D->MudlibVersion()+".\n"
"Powered by "+s+"\n"
	   );
  return 1;
}

/*************************************************************************/
