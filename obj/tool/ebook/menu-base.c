/*
** Book of Errors
** Basic functions for the menues
** Author: Thragor@Nightfall, 05jun96
** Update: Thragor@Nightfall, 18jun96
**
** Code continues at line: 62
**
** Functions:
**  status check_line()
**    As the wizard will choose the entries by numbers, I have to check if the
**    number is in the possible range. This is done via check_line().
**
**  string Set/QueryError()
**    Set the error we're currently editing. This will be the file where the error occured,
**    i. e. the key of the errors-mapping from errhandle.c.
**
**  mapping Set/QueryLines()
**    This contains the entry in ERRORS(mapping) after COMPILE (see errhandle.c).
**    The entry will look like this:
**      ([<line>(int) : ([<error>(string) : COMPILED(string); COMPILELINE(int)])])
**
**  void wait()
**    Wait until <CR> got pressed and return to a specified function. This function
**    sends the input_to and receives it. A small hack (the 'prompt'-flag) makes this
**    possible.
**
**  void ed_file()
**    Forces a wizard to edit a file by calling the player-internal function fed()
**    (this function is called, too, if you type 'ed <file>').
**
**  void ed_really()
**    Because of the input_to()-handling the Book of Errors has to be closed to
**    edit a file. That's the reason why there needs to be a security-check if the book
**    shall really be closed if the data in memory got edit and therefore are different
**    from the file which you can find on the disk.
**    This check is done by ed_really()
**
**  void edit_err()
**    Edit the file where errors occured. Includes a security-check if the edited .err-file
**    should be saved.
**
**  void save_err()
**    Save all changes that have been done to the .err-file.
**
**  void delete_err()
**    Deletes all errors which appeared in a certain line. This is simply done by
**    removing one entry from the Lines()-Mapping.
**
**  void print_menu_help()
**    Prints a help on the available commands in this menu.
**
**  void help_err()
**    Calls the error-database ERRORBASE to print a help-text about the current
**    error. This should be a little help to debug the file.
**
**  void print_err()
**    Print more informations about the error (well, just all remaining informations
**    which you can find in the .err-file).
**
*/

#include "ebook.h"

inherit "/lib/string";

private static string Perror;
private static mapping Plines;

// Prototypes

public void print_dir(); // menu-overview.c
public varargs void print_compile(string err); // menu-compile.c
public varargs void print_runtime(string err); // menu-runtime.c
public status SetOpen(status i); // ebook.c
public status save_data(); // errhandle.c
public mapping SetErrors(mapping err); // errhandle.c
public mapping QueryErrors(); // errhandle.c
public status SetModify(status b); // errhandle.c
public status QueryModify(); // errhandle.c

// End Prototypes

protected string SetError(string err)
{
  return Perror = err;
}

protected string QueryError()
{
  return Perror;
}

protected mapping SetLines(mapping l)
{
  return Plines = l;
}

protected mapping QueryLines()
{
  return Plines;
}

public varargs void wait(string fasel,int prompt,closure function,mixed args)
{
  if (!TP) return;
  if (prompt)
    {
      write("\n--- Press ENTER to continue...");
      input_to("wait",0,0,function,args);
      return;
    }
  write("\n");
  if (pointerp(args))
    apply(function,args);
  else
    funcall(function,args);
}

protected status check_line(int line,mixed data,closure function)
{
  if (line<1||line>sizeof(data))
    {
      write("Number out of range.\n");
      wait("",1,function);
      return 0;
    }
  return 1;
}

public void ed_file(string file)
{
  if (!TP) return;
  file = "/"+blueprint(file);
  if (file_size(file)<0)
    {
      if (file_size(file+".c")<0)
	{
	  write("No file "+file+"!\n");
	  wait("",1,SF(print_dir));
	  return;
	}
      file+=".c";
    }
  SetOpen(0);
  write("Book of Errors closed.\n");
  show(capitalize(TP->QueryName())+" closes the Book of Errors.\n");
  TP->fed(file);
}
	      
public varargs void ed_really(string answer,string file,
			      closure function,mixed args)
{
  if (!TP) return;
  if (answer=="c")
    {
      if (pointerp(args))
	apply(function,args);
      else
	funcall(function,args);
    }
  if (answer=="s")
    if (!save_data())
      write("!!! Save failed! No write access!!!\n");
  ed_file(file);
}

protected varargs void edit_err(string kind,string file)
{
  closure retfun;
  if (!QueryModify())
    return ed_file(QueryError());
  write("EBook will quit now to edit the file.\n"
	"As you modified the error-file you may now choose to:\n"
	"<c>ancel, <s>ave or <g>o on without saving.\n"
	"Please make your choice: ");
  retfun = kind?kind==COMPILE?SF(print_compile):SF(print_runtime):SF(print_dir);
  input_to("ed_really",0,file||QueryError(),retfun);
  return;
}

protected void save_err(closure fun)
{
  save_data();
  return funcall(fun);
}

protected varargs void delete_err(mixed line,string kind)
{
  mapping errors;
  string reverse;
  errors = QueryErrors();
  if (stringp(line))
    {
      m_delete(errors,line);
      SetErrors(errors);
      SetModify(1);
      return print_dir();
    }
  reverse = kind==COMPILE?RUNTIME:COMPILE;
  m_delete(errors[QueryError(),ERRORS][kind],line);
  SetModify(1);
  if (!sizeof(errors[QueryError(),ERRORS][kind]||([])))
    {
      if (!sizeof(errors[QueryError(),ERRORS][reverse]||([])))
	m_delete(errors,QueryError());
      SetErrors(errors);
      return print_dir();
    }
  SetErrors(errors);
  if (kind==COMPILE)
    return print_compile();
  else
    return print_runtime();
}

protected varargs void print_menu_help(string kind)
{
  closure fun;
  fun = kind?kind==COMPILE?SF(print_compile):SF(print_runtime):SF(print_dir);
  if (!kind)
    write("Book of Errors, Helppage\n\n"
	  " ?     Print this help\n"
	  " <nr>  Go to the errors in file no. <nr>\n"
	  " e<nr> Edit the file, where errors occured;\n"
	  "       This will automatically close the Book of Errors.\n"
	  " d<nr> Delete the errors in file no. <nr>\n"
	  " s     Save the current changes to the error-file.\n"
	  " q     Close the Book of Errors\n");
  else
    write("Book of Errors, Helppage - "+kind+" Errors\n\n"
	  " ?     Print this help\n"
	  " <nr>  Print detailed informations about errors\n"
	  " e     Edit the file, where errors occured;\n"
	  "       This will automatically close the Book of Errors.\n"
	  " d<nr> Delete the corresponding errors\n"
	  " h<nr> Give a help to the corresponding errors. This give you\n"
	  "       a hint, how to remove the errors. If the error isn't stored\n"
	  "       in the database yet, it will be reported to Thragor. So it\n"
	  "       might be added soon.\n"
	  " s     Save the current changes to the error-file.\n"
	  " q     Return to the overview\n");
  wait("",1,fun);
}

protected void help_err(int line,string retfun)
{
  string *errs;
  errs = m_indices(QueryLines()[line]);
  ERRORBASE->print_help(errs,retfun);
}

public void runtime_or_compile(string choice,string err)
{
  if (choice=="r")
    return print_runtime(err);
  else if (choice=="c")
    return print_compile(err);
  write("--- !!! Unrecognized answer!\n");
  print_dir();
}

protected varargs void print_err(mixed line,string kind)
{
  string text,*errarr;
  mapping errors;
  int i,comp,run;
  if (!kind)
    {
      errors = QueryErrors();
      comp = sizeof(errors[line,ERRORS][COMPILE]);
      run = sizeof(errors[line,ERRORS][RUNTIME]);
      if (!comp)
	return print_runtime(line);
      else if (!run)
	return print_compile(line);
      write("There are runtime- and compile-errors logged for\n"
	    "  "+line+".\n"
	    "Look at <r>untime or <c>ompile errors? ");
      input_to("runtime_or_compile",0,line);
      return;
    }
  text = sprintf("Book of Errors, Errors in: %s, Line: %d\n",
		 QueryError(),line);
  text+= mkstr(strlen(text),"=")+"\n\n";
  i = sizeof(errarr = m_indices(errors = QueryLines()[line]));
  if (kind==RUNTIME)
    while(i--)
      text+=sprintf("Error: %s\nObject: %s\nCreator: %s\nEnvironment: %s\n",
		    errarr[i],errors[errarr[i],OBJECT]||"<none>",
		    errors[errarr[i],CREATOR]||"<none>",
		    errors[errarr[i],ENVIRONMENT]||"<none>");
  else
    while(i--)
      text+=errors[errarr[i],LOCATION]?
	    sprintf("Error: %s\nCompiling: %s, Line: %d (%s)\n",
		    errarr[i],errors[errarr[i],COMPILED]||"<none>",
		    errors[errarr[i],COMPILELINE],
		    errors[errarr[i],LOCATION]):
	    sprintf("Error: %s\nCompiling: %s, Line: %d\n",
		    errarr[i],errors[errarr[i],COMPILED]||"<none>",
		    errors[errarr[i],COMPILELINE]);
  text+="\n";
  smore(text,TP->QueryPageSize(),ME,"print_"+lower_case(kind));
}
