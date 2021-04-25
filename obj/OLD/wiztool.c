/*
** Wizard-Tool
**
** This is a generic Wizard-Tool. It may be easily adapted to every
** Wizard, to her or his favourite commands, etc.
**
** Author: Thragor@Nightfall, 19dec96
** Update:
*/

#define CMD_EXEC 0
#define CMD_HELP 1
#define CMD_SIZE 2

#define SAVE_PROP 0
#define SAVE_HIDE 1
#define SAVE_SIZE 2

#define FLAG_TOGGLE "leading char"
#define TOGGLE_ON "on"
#define TOGGLE_OFF "off"

#define MAIN_FUNC "main"
#define PAGEWIDTH 75

#define SF(x) #'x //'
#define ME this_object()
#define TP this_player()
#define TI this_interactive()

#include <secure/wizlevels.h>

inherit "/std/thing";
inherit "/lib/number";
inherit "/lib/string";

public mapping Psaveprops; // Properties that will be saved in AutoObject
private static mapping Pcommands; // ([<command>:<exec-file>;<help-file>])

public status IsWizTool()
{
  return 1;
}

public mapping SetCommands(mapping m)
{
  return Pcommands = m;
}

public mapping QueryCommands()
{
  return (Pcommands||allocate_mapping(0,CMD_SIZE))+allocate_mapping(0,CMD_SIZE);
}

public void AddCmd(string cmd,string exec,string help)
{
  mapping m;
  m = QueryCommands();
  m[cmd,CMD_EXEC] = resolve_file(exec);
  m[cmd,CMD_HELP] = resolve_file(help);
  SetCommands(m);
}

public mapping SetSaveProperties(mapping m)
{
  return Psaveprops = m;
}

public mapping QuerySaveProperties()
{
  return (Psaveprops||allocate_mapping(0,SAVE_SIZE))+
    allocate_mapping(0,SAVE_SIZE);
}

public varargs void SetSave(string prop,mixed value,status hide)
{
  mapping m;
  m = QuerySaveProperties();
  m[prop,SAVE_PROP] = value;
  m[prop,SAVE_HIDE] = hide;
  SetSaveProperties(m);
}

public mixed QuerySave(string prop)
{
  mapping m;
  m = QuerySaveProperties();
  return m[prop,SAVE_PROP];
}

public void RemoveSave(string prop)
{
  mapping m;
  m = QuerySaveProperties();
  m_delete(m,prop);
  SetSaveProperties(m);
}

public void SetAutoObject(mixed value)
{
  if (!mappingp(value)) return;
  SetSaveProperties(value);
}

public mapping QueryAutoObject()
{
  return QuerySaveProperties();
}

public void create()
{
  (::create());
  SetShort("a generic wiztool");
  SetLong("This is a generic wiztool. It may be easily configured, so you'll\n"
	  "have your own sets of commands.\n"
	  "Just type 'help <id>' to get an overview about the general\n"
	  "commands, which are included into every wiztool.\n");
  SetIds(({"wiztool","tool"}));
  SetAds(({"generic"}));
  SetHelpMsg("The tool offers the following basic functions:\n"
	     "  [?]list          - list all available commands\n"
	     "  [?]help <cmd>    - help to a specific command\n"
	     "  <?>toggle        - switches between: Command has to start\n"
	     "                     with leading filename-character and may\n"
	     "                     start with leading filename-character\n"
	     "  [?]status        - lists all flags\n"
	     "  [?]destruct      - destroy the tool\n\n"
	     "[?] means, that the command may start with the first character\n"
	     "in the filename, <?> means, that it has to.\n"
	     "You may call every command in the list with the command-string\n"
	     "lead by the first character of the file-name of the tool.\n");
  SetSave(FLAG_TOGGLE,TOGGLE_OFF,0);
}

public void init()
{
  string *cmds;
  int i;
  
  (::init());
  i = sizeof(cmds = m_indices(QueryCommands()));
  while(i--)
    add_action("exec_cmd",cmds[i]);
  add_action("parse_cmd",explode(object_name(),"/")[<1][0..0],1);
}

protected int print_commands()
{
  string *cmds,txt;
  int columns,maxsize,space,i,j,distance;
  
  cmds = sort_array(m_indices(QueryCommands()),SF(>));
  maxsize = max(map(cmds,SF(strlen)));
  columns = PAGEWIDTH/(maxsize+2);
  space = PAGEWIDTH/columns-2;
  distance = sizeof(cmds)/columns+1;
  
  txt = "List of commands:\n\n";
  for(i=0;i<distance;i++)
    {
      for(j=0;j<columns;j++)
	if (i+j*distance < sizeof(cmds))
	  txt+=ladjust(cmds[i+j*distance],space,".")+"  ";
      txt = txt[0..<3];
      txt+="\n";
    }
  smore(txt,TI->QueryPageSize());
  return 1;
}

protected int print_status()
{
  string *cmds,txt;
  int columns,maxsize,space,i,j,distance;
  mapping m;
  
  m = QuerySaveProperties();
  cmds = sort_array(m_indices(m),SF(>));
  maxsize = max(map(cmds,SF(strlen)));
  
  txt = "List of settings:\n\n";
  for(i=0;i<sizeof(cmds);i++)
    if (!m[cmds[i],SAVE_HIDE])
      {
	txt+=ladjust(cmds[i]+": ",maxsize+1,".")+" ";
	if (stringp(m[cmds[i],SAVE_PROP])) txt+=m[cmds[i],SAVE_PROP];
	else txt+=mixed_to_string(m[cmds[i],SAVE_PROP]);
	txt+="\n";
    }
  smore(txt,TI->QueryPageSize());
  return 1;
}

protected int print_help(string arg)
{
  string txt;
  if (!arg)
    {
      smore(QueryHelpMsg(),TI->QueryPageSize());
      return 1;
    }
  switch(lower_case(arg))
    {
    case "toggle":
      txt=
	"Command: toggle\n"
	"Description:\n"
	"  If you use a command of this tool, you may either enter the bare\n"
	"  command or the command lead by a certain letter (evaluated from\n"
	"  the first letter of the filename).\n"
	"  This is the standard-behaviour. But you may 'toggle' this\n"
	"  behaviour. If you toggled it, you HAVE TO enter the commands with\n"
	"  the leading letter.\n";
      break;
    case "list":
      txt=
	"Command: list\n"
	"Description:\n"
	"  Lists all available commands (except the hardcoded-ones).\n";
      break;
    case "status":
      txt=
	"Command: status\n"
	"Description:\n"
	"  With status you may see all properties which are saved in your\n"
	"  playersavefile (but only those, which shall be visible.)\n";
      break;
    case "destruct":
      txt=
	"Command: destruct\n"
	"Description:\n"
	"  Destroys your tool.\n";
      break;
    case "help":
      txt=
	"Command: help <command>\n"
	"Description:\n"
	"  Gives informations about a certain command.\n";
      break;
    default:
      if (member(QueryCommands(),lower_case(arg)))
	txt = read_file(QueryCommands()[lower_case(arg)]);
      else
	txt = "Command not available.\n";
    }
  smore(txt,TI->QueryPageSize());
  return 1;
}

protected secure_check()
{
  if (   !TI
      || !IS_IMMORTAL(TI)
      || environment()!=TI
     )
    return 0;
  return 1;
}

public varargs int exec_cmd(string arg,string cmd)
{
  object ob;
  mapping cmds;
  if (!secure_check()) return 0;
  if (   !cmd
      && 
         (   QuerySave(FLAG_TOGGLE)==TOGGLE_ON
	  || query_verb()=="toggle"
         )
     )
    return 0;
  cmd = cmd||query_verb();
  switch(cmd)
    {
    case "toggle":
      if (QuerySave(FLAG_TOGGLE)==TOGGLE_ON) SetSave(FLAG_TOGGLE,TOGGLE_OFF);
      else SetSave(FLAG_TOGGLE,TOGGLE_ON);
      write(FLAG_TOGGLE+": "+QuerySave(FLAG_TOGGLE)+"\n");
      return 1;
    case "list":
      return print_commands();
    case "status":
      return print_status();
    case "help":
      return print_help(arg);
    case "destruct":
      remove();
      if (ME) destruct(ME);
      return 1;
    default:
      cmds = QueryCommands();
      if (!member(cmds,cmd)) return 0;
      return call_other(cmds[cmd,CMD_EXEC],MAIN_FUNC,arg);
    }
}
  
public int parse_cmd(string arg)
{
  string cmd,*cmds;
  cmd = query_verb()[1..];
  cmds = m_indices(QueryCommands());
  if (member(({"toggle","list","destruct","status","help"})+cmds,cmd)==-1)
    return 0;
  return exec_cmd(arg,cmd);
}
