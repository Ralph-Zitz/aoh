/*
** Approve-Tool
** This tool is for use of Archwizards to keep track of approved objects.
**
** Author: Thragor@Nightfall, 10feb97
** Update:
*/

#include <config.h>

inherit "/std/thing";

#define A_DATE 0
#define A_NAME 1
#define A_SIZE 2

// File-Errors

#define F_OK           01
#define F_NOT_EXIST    02
#define F_NO_ACCESS    03
#define F_INVALID_NAME 04

// UID-Errors

#define U_INVALID      11

#define CHECK_OK           0
#define CHECK_NEW_DATE     1
#define CHECK_NO_FILE      2
#define CHECK_NOT_APPROVED 3

#define TP this_player()
#define TO this_object()
#define SF(x) #'x //'
#define SYM(x) 'x //'
#define STR "/lib/string"

private static string afile;
private static int last_change;

public mapping approved;
// ([<string file>:<int approve-date>;<string approve-name>])

public string QueryAutoObject()
{
  return afile||"";
}

public void SetAutoObject(mixed data)
{
  if (!stringp(data)) return;
  afile = data;
}

private string o_file()
{
  if (!afile) return 0;
  if (afile[<2..]!=".o")
    return afile+".o";
  return afile;
}

private string bare_file()
{
  if (!afile) return 0;
  if (afile[<2..]!=".o")
    return afile;
  return afile[0..<3];
}

private int SaveData()
{
  int date;
  if (!afile) return F_INVALID_NAME;
  date = file_date(o_file());
  save_object(bare_file());
  if (date>=file_date(o_file()))
    return F_NO_ACCESS;
  last_change = file_date(o_file());
  return F_OK;
}

private int LoadData()
{
  approved = approved||m_allocate(0,A_SIZE);
  if (!afile||file_size(o_file())<0)
    return F_NOT_EXIST;
  if (last_change==file_date(o_file()))
    return F_OK;
  restore_object(bare_file());
  approved = approved||m_allocate(0,A_SIZE);
  return F_OK;
}

private int AddApproved(string file)
{
  if (!TP) return U_INVALID;
  file = TP->get_path(file);
  if (getuid(TP)!=getuid(TO))
    return U_INVALID;
  if (file_size(file)<0) return F_NOT_EXIST;
  LoadData();
  approved[file,A_DATE] = time();
  approved[file,A_NAME] = getuid(TP);
  return SaveData();
}

private int RemoveApproved(string file)
{
  if (!TP) return U_INVALID;
  if (getuid(TP)!=getuid(TO))
    return U_INVALID;
  file = TP->get_path(file);
  LoadData();
  m_delete(approved,file);
  return SaveData();
}

private int CheckApproved(string file)
{
  if (!TP) return U_INVALID;
  if (getuid(TP)!=getuid(TO))
    return U_INVALID;
  file = TP->get_path(file);
  if (file_size(file)<0) return CHECK_NO_FILE;
  LoadData();
  if (!member(approved,file))
    return CHECK_NOT_APPROVED;
  if (file_date(file)>approved[file,A_DATE])
    return CHECK_NEW_DATE;
  return CHECK_OK;
}

public string Report(string dir)
{
  string *files,*res,path;
  int i,cres;
  if (!TP) return 0;
  if (getuid(TP)!=getuid(TO))
    return "Your uid is invalid.\n";
  if (!dir)
    // Create report about approved files
    {
      LoadData();
      files = m_indices(approved);
    }
  else
    // Create a report about files in directory
    {
      dir = TP->get_path(dir);
      path = implode(explode(dir,"/")[0..<2],"/")+"/";
      files = get_dir(dir,0x01);
      files = map(files,lambda(({SYM(f)}),({SF(+),path,SYM(f)})));
    }
  i = sizeof(files);
  res = ({});
  while(i--)
    {
      switch(CheckApproved(files[i]))
	{
	case CHECK_OK:
	  break;
	case CHECK_NEW_DATE:
	  res+=({sprintf("%s has changed since approval.",files[i])});
	  break;
	case CHECK_NO_FILE:
	  res+=({sprintf("%s doesn't exist anymore. Moved?",files[i])});
	  break;
	case CHECK_NOT_APPROVED:
	  res+=({sprintf("%s didn't get approved up to now.",files[i])});
	  break;
	default:
	  res+=({sprintf("An error occured reading %s.",files[i])});
	}
    }
  res = sort_array(res,SF(>));
  res = ({ (dir?sprintf("Result for files in %s:",dir)
	       :"Results for approved files:"
           ),
	   "" })
        +res;
  return implode(res,"\n")+"\n";
}

public int Approve(string str)
{
  int res;
  res = AddApproved(&str);
  if (res == U_INVALID)
    return notify_fail("Your uid doesn't corrospond with the uid of the "
		       "tool.\n",NOTIFY_NOT_VALID),0;
  if (res == F_NOT_EXIST)
    return notify_fail("The file you wanted to approve doesn't exist.\n",
		       NOTIFY_NOT_VALID),0;
  if (res == F_INVALID_NAME)
    return notify_fail("The approved-datafile-name is invalid.\n",
		       NOTIFY_NOT_VALID),0;
  if (res == F_NO_ACCESS)
    return notify_fail("You have no write-access to the data-file.\n",
		       NOTIFY_NOT_VALID),0;
  if (res == F_OK)
    return write(str+" got approved.\n"),1;
  return notify_fail("Unknown error.\n",NOTIFY_NOT_VALID),0;
}

public int Remove(string str)
{
  int res;
  res = RemoveApproved(str);
  if (res == U_INVALID)
    return notify_fail("Your uid doesn't corrospond with the uid of the "
		       "tool.\n",NOTIFY_NOT_VALID),0;
  if (res == F_INVALID_NAME)
    return notify_fail("The approved-datafile-name is invalid.\n",
		       NOTIFY_NOT_VALID),0;
  if (res == F_NO_ACCESS)
    return notify_fail("You have no write-access to the data-file.\n",
		       NOTIFY_NOT_VALID),0;
  if (res == F_OK)
    return write(str+" got removed from approval-list.\n"),1;
  return notify_fail("Unknown error.\n",NOTIFY_NOT_VALID),0;
}

public int AFile(string str)
{
  if (!TP) return 0;
  if (str) str = TP->get_path(str);
  if (!str)
    {
      if (afile)
	printf("Approve-file is %s.\n",o_file());
      else
	write("Currently there's no approve-file set.\n");
      return 1;
    }
  if (afile)
    printf("Approve-file changed from %s to %s.\n",afile,str);
  else
    printf("Approve-file set as %s.\n",str);
  afile = str;
  return 1;
}

public void create()
{
  (::create());
  seteuid(getuid());
  SetWeight(0);
  SetNoDrop(1);
  SetNoGive(1);
  SetShort("approval tool");
  SetLong("This tool is meant to help archwizards to keep track of "
	  "approved objects. You may approve an object and if it gets "
	  "changed or deleted (moved?) afterwards, a report will inform "
	  "you about this.\n"
	  "Vanished files won't be deleted automatically from the database "
	  "as they might be a hint, that the files moved and that you "
	  "have to correct the 'pointer'.\n"
	  "To get brief help of the commands type 'help approval tool'.\n");
  SetIds(({"approval","tool","atool"}));
  SetAds(({"approval"}));
  SetHelpMsg("The commands of the approval tool:\n\n"
	     "aappr[ove] <file>   Approve a file\n"
	     "arem[ove] <file>    Remove file form database\n"
	     "arep[ort] [<dir>]   Give either a report of the files stored\n"
	     "                    in the database or in the specified dir\n"
	     "afile [<file>]      Set or query the current database-file\n");
  
  SetValue(0);
}

public int cmd_report(string str)
{
  STR->smore(Report(str));
  return 1;
}

public void init()
{
  (::init());
  add_action("Remove","arem",1);
  add_action("Approve","aappr",1);
  add_action("Approve","aapr",1); // work on typos too... ;-)
  add_action("AFile","afile");
  add_action("cmd_report","arep",1);
}
