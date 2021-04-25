/*
 * This file contains standart funs for text input and conversion,
 * filehandling and communication
 */

nosave mixed    comm;		/* An alist for internal communication */
nosave string  *edittext;	/* The text given in gettext() */
nosave string   Filename;
nosave string   oldfile;	/* contains the file with the same name as
				 * Filename if exist */
nosave int      written;	/* Is != 0 if Filename is already written */

static string   file_print();	/* Just a prototyp */
static void     abort();	/* */

#include <string.h>
#include <properties.h>
inherit "/lib/string";

mixed Set(string label, mixed arg);

/* Associates the label to the argument */

static void SetLabel(string label, mixed argument) {
  if(!mappingp(comm)) comm = ([label:argument]);
  else comm[label] = argument;
}

/* Returns the associated worth to label */

static mixed QueryLabel(string label) { return comm[label]; }

/* Returns the filename of the aktual object */

string QueryFile() { return Filename; }

/* Returns the variable edittext */

string *EditText() { return edittext; }

/* Clears the edittext variable */

static void ClearText() { edittext = ({}); }

/* Sets edittext to text */

static void SetText(string * text) { edittext = text; }

/* Returns the help string */

static string help()
{
  return
    ("The maker accepts the following commands:\n" +
     "~h ...... shows this text            ~q ...... aborts the execution\n" +
     "~d ...... delete the last line and   ~r ...... show the last text\n" +
     "          redisplay                  ~f ...... shows the file\n" +
     ". / ** .. end the action and return  ~s ...... saves the object in the current\n" +
     "          to the menu                          form\n" +
     "~!<cmd>.. escape and command <cmd>   ~! ...... interrupt, restart with \"~m\"\n");
}


/* Aborts all */

static void abortx()
{
  write("Aborted...\n");
  this_object()->maker_reset();
  return say(this_player()->QueryName()
	     + " stops to write a file and aborts.\n");
}

/* Writes the actual file and returns to getstring() */

static void normal_save_string()
{
  write(file_print()
	+ "Do you want to save this file " + Filename + "? (y/n) ");
  comm["nextfun"] = "gotogetstring";
  input_to("filewrite");
}

/* Tries to write the (string)file_print() as new file */

static void filewrite(string str)
{
  if (str == "n") {
    write("No save done.\n");
    return call_other(this_object(), comm["nextfun"]);
  }
  if (str == "y") {
    write("Trying to write " + Filename + "...\n");
    if (!written && !(file_size(Filename) == -1)) {
      write("File " + Filename + " already exists!\n");
      write("O.k to overwrite " + Filename + "? (y/n) ");
      input_to("correct_save");
      return;
    }
    if (written) {
      if (file_size(Filename[0..<3] + ".old") != -1
	  && !rm(Filename[0..<3] + ".old"))
	return write("Can't write file " + Filename
		     + "! Please control your access rights!\n");
      rename(Filename, Filename[0..<3] + ".old");
    }
    if (!write_file(Filename, file_print())) {
	    write("Can't write file " + Filename + "! Please control your access rights!\n");
	    return abortx();
	  }
    write("Done...\n");
	written = 1;
    return call_other(this_object(), comm["nextfun"]);
  }
  write("Do you want to save this file?? (y/n) ");
  input_to("filewrite");
}

static void correct_save(string str)
{
  if (str == "y") {
    if (file_size(Filename + "~") != -1 && !rm(Filename + "~"))
      return write("Can't write file " + Filename
		   + "! Please control your access rights!\n");
    rename(Filename, Filename + "~");
    return filewrite("y");
  }
  if (str == "n")
    return call_other(this_object(), comm["nextfun"]);
  write("O.k. to overwrite " + Filename + "?? (y/n) ");
  input_to("correct_save");
}

/* Asks befor aborting the player for to write the new file */

static void abort()
{
  write("Do you want to save the file " + Filename + " before? (y/n) ");
  comm["nextfun"] = "abortx";
  input_to("filewrite");
}

/* Gets a new filename or aborts */

static void getrightname(string str)
{
  if (str == "~q")
    return abortx();
  this_player()->command_me("make " + str);
  return;
}

/*
 * Convert a string array into a string, that prints out a string like this:
 * "<array>[0]" + "<array>[1]" + "<array>[2]" ...
 */

static string arrayconvert(mixed myt)
{
  int    i,j;
  string outstring;

  if(stringp(myt)) return myt;
  if(!(j = sizeof(myt))) return "\"\"";
  if(j == 1) return "\"" + lit_string(myt[0]) + "\\n\"\n";
  outstring = "\"" + lit_string(myt[0]) + "\\n\\\n";
  for (i = 1; i < j-1; i++)
    outstring += lit_string(myt[i]) + "\\n\\\n";
  outstring += lit_string(myt[<1]) + "\\n\"\n";
  return outstring;
}

/* Converts a string array to a string */

static string arrayprint(string * text)
{
  int    i;
  string outstring;
  outstring = "";
  for (i = 0; i < sizeof(text); i++)
    outstring += text[i] + "\n";
  return outstring;
}

/*
 * Converts a string array into a "comment-format" like this: "/*
 * <array>[0]\n" + " * <array>[1]\n" + " * <array>[2]\n" ... " +/\n"
 */

static string comment(string * text)
{
  int    i;
  string outstring;
  if (!sizeof(text) || (sizeof(text) == 1 && text[0] == ""))
    return "";
  outstring = "/* " + text[0] + "\n";
  for (i = 1; i < sizeof(text); i++)
    outstring += " * " + text[i] + "\n";
  return outstring;
}

/* Prints the prompt and waits for the input of a string */

static void gotogetstring()
{
  write(comm["prompt"]);
  input_to("getstring");
}

/* Shows the actual file and returns to getstring() */

static void show_file_string()
{
  SetLabel("nextfun","gotogetstring");
  smore(file_print(), 0, this_object(), "gotogetstring");
}

/*
 * Takes a string from the player and tries to interprete them as a maker
 * command. If failed, it calls the function that is assocciated to "insert_
 * string" with the string as argument
 */

static void getstring(string str)
{
  if (str[0..1] == "~!") {
    if (str[2..<1] == "") {
      comm["nextfun"] = "gotogetstring";
      write("You interrupt to make " + Filename + "\nRestart with \"~m\".\n");
      say(this_player()->QueryName() + " interupts to write a file.\n");
      Set("making", 1);
      return;
    }
    this_player()->command_me(str[2..<1]);
    return gotogetstring();
  }
  switch (str) {
  case "":
    return call_other(this_object(), comm["nostring"]);
  case "~q":
    return abort();
  case "~h":
	write(help());
    return gotogetstring();
  case "~f":
	return show_file_string();
  case ".":
    return call_other(this_object(),comm["ready"]);
  case "**":
    return call_other(this_object(), comm["ready"]);
  case "~r":
    return call_other(this_object(), comm["redisplay"]);
  case "~d":
        return call_other(this_object(), comm["delete"]);
  case "~s":
    return call_other(this_object(), comm["save_file"]);
    /* end switch */
  }
  return call_other(this_object(), comm["insert_string"], str);
}

/* Prints the message that is assocciated to "nextwrite" and starts gettext()
 */

static void gotogettext()
{
  write(comm["nextwrite"] + "]");
  input_to("gettext");
}

/* Takes strings and stores them in string *edittext */

static void gettext(string str)
{
  string thefile, rest;
  string *include, *restarr;
  int    btes;
  int    firstline,lastline;
  if (str[0..1] == "~!") {
    if (str[2..<1] == "") {
      SetLabel("nextfun", "gotogettext");
      write("You interrupt to make " + Filename + "\nRestart with \"~m\".\n");
      say(this_player()->QueryName() + " interupts to write a file.\n");
      Set("making", 1);
      return;
    }
    this_player()->command_me(str[2..<1]);
    write("]");
    input_to("gettext");
    return;
  }
  if (str[0..1] == "~i") {
    if (str[2..] == "") {
      write("Usage: ~i <file> [<line>][-][<line>].\n]");
      input_to("gettext");
      return;
    }
    str = str[2..];
    if(str[0] == ' ') str = str[1..];
    if(!sscanf(str,"%s %s",thefile,rest))
      thefile = str;
    thefile = (string)this_player()->get_path(thefile);
    if((btes = file_size(thefile)) == -2) {
      write(thefile + " is a directory.\n]");
      input_to("gettext");
      return;
    }
    if(btes == -1) {
      write("No such file: " + thefile + ".\n]");
      input_to("gettext");
      return;
    }
    include = explode(read_file(thefile),"\n");
    if(!rest) {
      edittext += include;
      write("Including " + thefile + "...\n]");
      input_to("gettext");
      return;
    }
    if(restarr[0] == "") restarr[0] = "0";
    if(sizeof(restarr) == 1) restarr += restarr;
    if(restarr[1] == "") restarr [1] = "" + (sizeof(include)-1);
    rest = implode(restarr," - ");
    if(!sscanf(rest,"%d - %d",firstline,lastline)) {
      write("Wrong second argument. Usage: ~i <file> [<line>][-][<line>].\n]");
      input_to("gettext");
      return;
    }
    if(firstline > sizeof(include)-1) {
      write(thefile + " hasn't that many lines.\n]");
      input_to("gettext");
      return;
    }
    if(lastline > sizeof(include)-1)
      lastline = sizeof(include) -1;
    if(lastline < firstline) lastline = firstline;
    edittext += include[firstline..lastline];
    write("Including " + thefile + " line");
    if(lastline == firstline) write(" " + lastline + ".\n]");
    else write("s " + firstline + " - " + lastline + ".\n]");
    input_to("gettext");
    return;
  }
  switch (str) {
    case "~q":
      return abort();
    case "~r":
      write(arrayprint(edittext) + "]");
      input_to("gettext");
      return;
    case "~d":
      if (!sizeof(edittext)) {
	write("No lines in the buffer.\n]");
	input_to("gettext");
	return;
      }
      edittext = edittext[0..<2];
      comm["nextwrite"] = arrayprint(edittext);
      return gotogettext();
    case ".":
      return call_other(this_object(), QueryLabel("ready"));
    case "**":
      return call_other(this_object(), QueryLabel("ready"));
    case "~f":
      return call_other(this_object(), QueryLabel("show_file"));
    case "~s":
      SetLabel("nextfun","gotogettext");
      SetLabel("nextwrite","Continue...\n");
      write(file_print() + "Do you want to save this file? (y/n) ");
      input_to("filewrite");
      return;
    case "~h":
      write(help() + "~i <file> includes the file <file>\n" +
                     "          (optional: begin - end)\n]");
      input_to("gettext");
      return;
      /* end switch */
    }
  if(sizeof(str) > QueryLabel("lenmax")) {
    write("This line ist too long. Please enter again!\n]");
    input_to("gettext");
    return;
  }
  if (!str)
    edittext += ({""});
  else
    edittext += ({str});
  write("]");
  input_to("gettext");
}

static void getfilename()
{
  SetLabel("prompt","]");
  SetLabel("nostring","gotomenu");
  SetLabel("abort","abortx");
  SetLabel("undo","gotomenu");
  SetLabel("ready","gotomenu");
  SetLabel("redisplay","filename_redisplay");
  SetLabel("delete","delete_filename");
  SetLabel("save_file","normal_save_string");
  SetLabel("insert_string","store_filename");
  write(
    "----- Actual filename: --------------------------------------------------------\n" +
    Filename + "\n" +
    "-------------------------------------------------------------------------------\n" +
    "Please edit the filename of the room (help with \"~h\")!\n");
  return gotogetstring();
}

/* Shows the filename and returns to the getstring loop */

static void filename_redisplay()
{
  write("Filename is: " + Filename + "\n");
  return gotogetstring();
}

/* Writes any delete message and returns to getstring */

static void delete_filename()
{
  write("Filename deleted. Please enter a new filename!\n");
  gotogetstring();
}

/* Store the new filename after expanding it through TP->get_path() */

static void store_filename(string str)
{
  if(str[<2..<1] != ".c") str += ".c";
  str = (string)this_player()->get_path(str);
  if(str == Filename) {
    write("Filename hasn't changed.\n");
    return this_object()->gotomenu();
  }
  Filename = str;
  written = 0;
  write("Filename is now: " + Filename + "\n");
  this_object()->gotomenu();
}
