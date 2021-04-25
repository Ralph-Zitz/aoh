/*
** Book of Errors
** Overview-Menu
** Author: Thragor@Nightfall, 17jun96
** Update: Thragor@Nightfall, 18jun96
**
** Code continues at line: 37
**
** Functions:
**  string *Set/QueryErrorFiles()
**    Keeps the list of files where errors occured. These are the files which will be
**    offered in the menu.
**
**  void quit()
**    Actually quit the Book of Errors.
**
**  void save_quit()
**    Used for security-check, if the data shall be saved.
**
**  void dir_command()
**    Parse the input the player made and call the corresponding function (or print
**    an error-message).
**    Commands:
**      ? Help
**      d<nr> Delete error in Line
**      q Close Book of Errors
**      e<nr> Edit file
**      <nr> Print more detailed informations about the errors.
**
**  void dir_prompt()
**    Print the prompt and await the input from the user. The prompt consists of a
**    brief help.
**
**  void print_dir()
**    Print an overview of the errors.
*/

#include "ebook.h"

inherit EBOOK("menu-base");

private static string *Perrfiles;

// Prototypes

//public mapping SetErrors(mapping err); // errhandle.c
//public mapping QueryErrors(); // errhandle.c
//public status SetModify(status b); // errhandle.c
//public status QueryModify(); // errhandle.c
//public void print_compile(string err); // menu-compile.c
//public void print_runtime(string err); // menu-runtime.c
public string QueryErrorFile(); // errhandle.c

// End Prototypes

private string *SetErrorFiles(string *f)
{
  return Perrfiles = f;
}

private string *QueryErrorFiles()
{
  return (Perrfiles||({}))+({});
}

private void quit()
{
  SetOpen(0);
  if (TP)
    {
      write("Book of Errors closed.\n");
      show(capitalize(TP->QueryName())+" closes the Book of Errors.\n");
    }
}

public void save_quit(string answer)
{
  if (answer=="y")
    {
      if (!save_data())
	write("--- !!! Save failed! No write access!!!\n");
    }
  else if (answer!="n")
    return print_dir();
  quit();
}

public void dir_command(string str,string *idx)
{
  int line;
  string *errs;

  if (!TP) return;

  write("\n");

  if (str=="?")
    return print_menu_help();

  errs = QueryErrorFiles();

  if (sscanf(str,"%d",line))
    if (!check_line(line,errs,SF(print_dir))) return;
    else return print_err(errs[line-1]);

  if (sscanf(str,"e%d",line))
    if (!check_line(line,errs,SF(print_dir))) return;
    else return edit_err(0,errs[line-1]);

  if (sscanf(str,"d%d",line))
    if (!check_line(line,errs,SF(print_dir))) return;
    else return delete_err(errs[line-1]);

  if (str=="s")
    return save_err(SF(print_dir));

  if (str=="q")
    {
      if (!QueryModify())
	return quit();
      write("You want to close the Book of Errors, but your modifications\n"
	    "haven't been saved yet.\n"
	    "Do you want to save your changes before leaving? (y/n) ");
      input_to("save_quit");
      return;
    }

  write("--- !!! Unknown command!\n");
  print_dir();
}

public void dir_prompt()
{
  write("\nEBook [<nr>, e<nr>, d<nr>, s, q, ?] ");
  input_to("dir_command");
}

public void print_dir()
{
  mapping errors;
  string text,*idx;
  int i;
  if (!TP) return;
  text = "Book of Errors: "+QueryErrorFile()+"\n";
  text+=mkstr(strlen(text)-1,"=")+"\n";
  text+="\n";
  errors = QueryErrors();
  idx = sort_array(m_indices(errors),SF(>));
  SetErrorFiles(idx);
  for(i=0;i<sizeof(idx);i++)
    text+=sprintf("%03d. %s (%s)\n",i+1,idx[i],errors[idx[i],DATE]);
  if (!sizeof(idx))
    text+="No more errors.\n";
  smore(text,TP->QueryPageSize(),ME,"dir_prompt");
}
