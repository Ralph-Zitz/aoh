/*
** The book of errors
** Author: Thragor@Nightfall, 03jun96
** Update:
*/

#include "ebook/ebook.h"
#include <properties.h>

inherit EBOOK("errhandle");
inherit EBOOK("menu-compile");
inherit EBOOK("menu-runtime");
inherit EBOOK("menu-overview");
inherit "/std/thing";

private static status Popen;

public status SetOpen(status i)
{
  return Popen = i;
}

public status QueryOpen()
{
  return Popen;
}

public int cmd_eread(string str)
{
  string *letters;
  int i;
  if (!TP) return 0;
  if (!IS_IMMORTAL(TP))
    {
      write("Wow! You have a powerful wizard-tool! Now you can get control\n"
	    "about all errors... Did you read that? Bad argument 1 to\n"
	    "tell_object(). And there: Variable obj not declared!\n"
	    "This world of errors is really interesting.\n");
      return 1;
    }
  if (!str||str=="?")
    {
      write("ERead will read a .err-file at a given location and print\n"
	    "it's contents in a very own format to you. You may also get\n"
	    "help about the errors printed, if they are known to the\n"
	    "database. If not, Thragor get's automatically informed about\n"
	    "the missing hint.\n"
	    "The syntax is:\n"
	    "  eread /d/shadows\n"
	    "  -> Read /d/shadows/.err\n"
	    "  eread /d/shadows/.err.old\n"
	    "  -> Read /d/shadows/.err.old\n");
      return 1;
    }
  letters = explode(str,"");
  if (member(explode(str,"")," ")!=-1)
    return notify_fail("No spaces allowed in path-names.\n",
		       NOTIFY_NOT_VALID),0;
  str = TP->get_path(str);
  if (file_size(str)==-2)
    str+="/.err";
  if (file_size(str)<1)
    return notify_fail("No such file: "+str+"\n",
		       NOTIFY_NOT_VALID),0;
  write("You open the Book of Errors.\n");
  show(capitalize(TP->QueryName())+" opens the Book of Errors.\n");
  read_data(str);
  SetOpen(1);
  print_dir();
  return 1;
}

public int cmd_unresolved(string str)
{
  if (str)
    {
      write("'unresolved' is used to list and edit errors which have been\n"
	    "questioned at the error-database but no description was present.\n");
      return 1;
    }
  if (!TP||!IS_WIZARD(TP))
    return notify_fail("Only Vice Lords upwards may edit the database.\n",
		       NOTIFY_NOT_VALID),0;
  ERRORBASE->edit_unresolved();
  return 1;
}

public int cmd_errors(string str)
{
  if (str)
    {
      write("'errors' is used to list and edit errors which are stored in the\n"
	    "error-database.\n");
      return 1;
    }
  if (!TP||!IS_WIZARD(TP))
    return notify_fail("Only Vice Lords upwards may edit the database.\n",
		       NOTIFY_NOT_VALID),0;
  ERRORBASE->edit_errors();
  return 1;
}

public void init()
{
  (::init());
  add_action("cmd_eread","eread");
  add_action("cmd_unresolved","unresolved");
  add_action("cmd_errors","errors");
}

public string QueryExtralook()
{
  object env;
  if (!env = environment()) return 0;
  if (!query_once_interactive(env)) return 0;
  return capitalize(env->QueryPronoun())+" "+
    (QueryOpen()?"is reading in the Book of Errors":"carries the Book of Errors")+
    ".\n";
}

public status QueryAutoObject()
{
  if (!environment()) return 0;
  return IS_IMMORTAL(environment());
}

public void create()
{
  (::create());
  SetShort("Book of Errors");
  SetLong("The Book of Errors may read error-files, reduce the size by deleting\n"
	  "doubled reports and if the error-database is complete it will give\n"
	  "detailed help how to fix the errors which occured. To ease editing you\n"
	  "can directly call the editor from the Book of Errors.\n"
	  "To get more informations how to use this book, type 'eread ?'.\n"
	  "Vice Lords upwards may edit the error-database, commands to do\n"
	  "this are 'unresolved' and 'errors'. Each of the commands will print a\n"
	  "help-screen if you give '?' as argument.\n");
  SetIds(({"book","ebook","errors"}));
  SetAds(({"book","of"}));
  SetInfo("This Book can only be used by Wizards.\n");
  SetWeight(0);
  SetValue(0);
  SetNoDrop("Better destroy the Book of Errors instead of dropping it!\n");
  SetNoGive("No, you won't give it away.\n");
  Set(P_NOSELL,1);
  Set(P_NOBUY,1);
  SetOpen(0);
}
