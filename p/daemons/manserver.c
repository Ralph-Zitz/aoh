/*--------------------------------------------------------------------------
 * /p/daemons/manserver.c   -- The OSB manserver
 *
 * Copyright (C) 15jan1998 by Softbyte for OSB
 * All Rights Reserved.
 * Permission granted for non-commercial use.
 * [Softbyte 15jan1998]  V1.0 All functions working
 *--------------------------------------------------------------------------
 */
/**
  @author Softbyte
  @version V1.0
*/
/** @synopsis
       used by the man command
*/
/** @description
  The manserver serves as interface between the man command in std/player
  and the doc directory /doc. It maintains keyword and alias search for
  the documents.

*/

#include <daemons.h>
#include <msgclass.h>

inherit "/std/base";

#define SUBMANSERVER(x)   DAEMONPATH "man/"+(x)
#define SUBMANSAVE(x)     DAEMONSAVEPATH "man/"+(x)

private nosave string *mandirs;

varargs mixed *GetKeyword(string key,string section);

// Creates a submanserver *.c files
private void CreateManserver(string fname)
{
  string s;

  fname=SUBMANSERVER(fname);
  s= "// Automatically created file on "+ctime()+"\n";
  s+="// "+fname+" belongs to the OSB manserver\n\n";
  s+="inherit \""+MANBASE+"\";\n";
  rm(fname);
  write_file(fname,s);
}

/**
  @function
    Creates all subserver *.c files in the subdirectory 'man'
  @param int force
    if force is given the files are created regardless whether the
    do already exist
*/
varargs void CreateServer(int force)
{
  int i;

  if (file_size(SUBMANSERVER(""))!=-2)
    mkdir(SUBMANSERVER(""));
  if (file_size(SUBMANSAVE(""))!=-2)
  {
    mkdir(SUBMANSAVE(""));
    //printf("mkdri %s\n",SUBMANSAVE(""));
  }
  for (i=0;i<sizeof(mandirs);i++)
  {
    if (file_size(SUBMANSERVER(mandirs[i]+".c"))<0 || force)
      CreateManserver(mandirs[i]+".c");
  }
  if (file_size(SUBMANSERVER(".readme"))<0)
  {
     write_file((SUBMANSERVER(".readme")),
       "This directory contains datafiles and computer generated codefiles\n"
       "for the use of the manserver.\n"
       "The whole directory is and will be created automatically.\n");
  }
}

void create()
{
  ::create();
  mandirs = ({ "applied", "concepts", "driver", "efun", "efun.de", "helpdir", "intro"
           , "lfun", "prop", "lib", "LPC", "master", "obj", "sefun", "std", "examples"
           , "w", "daemons", "hook", "internals", "man", "obsolete", "old", "other"
           , "papers", "3.3vs3.5", "deprecated"});

  mandirs=sort_array(mandirs,#'> /*'*/);

  seteuid(getuid());
  CreateServer(0);
}

// Resolves an alias string
// Danger...function is recursive !!!
mixed *ReplaceAlias(mixed data,string section)
{
  mixed n_data;

  if (pointerp(data)) n_data=({data});
  else
  {
    n_data=GetKeyword(data,section);
  }
  return n_data;
}
// Resolves all alias strings in a data array
mixed *ReplaceAllAlias(mixed *data,string section)
{
  mixed *n_data;
  int i;

  n_data=({});
  for (i=0;i<sizeof(data);i++)
  {
    n_data+=ReplaceAlias(data[i],section);
    // n_data+=({data[i]});
  }
  return n_data;
}

/**
  @function
    Gets a keyword array from the database. Its format is
    ({ ({filename,line1,line2}), ... }). If line1 is <0 the whole
    file is meant otherwise only the part between line1..line2-1
  @param string key
    The keyword which shall be searched
  @param string section
    An optional parameter which restricts the search only to the given
    section
*/
varargs mixed *GetKeyword(string key,string section)
{
  mixed *n_data;
  mixed *data;
  string *dirs;
  int i;

  //printf("GetKeyWord for key=%s section=%s\n",key||"NULL",section||"NULL");

  if (!key) return 0;
  if (section) dirs=({section});
  else dirs=mandirs;

  // remove leading p_ a_ for properties and attributes
  if (key[0..1] == "p_") key = key[2..];
  if (key[0..1] == "a_") key = key[2..];


  n_data=({});
  for (i=0;i<sizeof(dirs);i++)
  {
    if (file_size(SUBMANSERVER(dirs[i]+".c")) < 0)
      continue;
    data=({mixed *})call_other(SUBMANSERVER(dirs[i]),"GetKeyword",key);
    if (data && sizeof(data))
    {
      data=ReplaceAllAlias(data,section);
      n_data+=data;
    }
  }
  return n_data;
}

/**
  @function
    Gets all possible section names
  @return
    A string array holding the section names
*/
string *GetSections()
{
  return mandirs;
}

/**
  @function
    Gets all possible keywords of a given section
  @param string section
    The section for which the keywords shall be returned
  @return
    A string array holding all keywords
*/
string *GetKeywords(string section)
{
  string *data;

  if (!section) return ({});
  if (-1 == member(mandirs,section)) return ({});

  data=({string *})call_other(SUBMANSERVER(section),"GetAllKeys");
  if (!data) data=({});

  return data;
}

/**
  @function
    Reinitialises a given section to reread new man files
  @param string section
    The section for which the keywords shall be returned
  @return
    0: Failure, 1: Ok
*/
int InitSection(string section)
{
  if (!section) return 0;
  if (-1 == member(mandirs,section)) return 0;
  return ({int})call_other(SUBMANSERVER(section),"InitModule");
}

/**
  @function
    Creates an index for the given filename and enters it into the
    database
  @param string fname
    The filename of the manfile to be indexed
  @param int verbose
    An optional parameter which will produce some information about the
    indexing process
  @return
    0: Ok, string: An error message
*/
varargs string IndexFile(string fname,int verbose)
{
  string *args;

  if (!fname) return "No filename given.\n";

  // remove a leading doc
  if (fname[0..0]=="/") fname=fname[1..];
  if (fname[0..3]=="doc/") fname=fname[4..];
  args=explode(fname,"/");

  if (sizeof(args)!=2)
    return "No section or file availble.\n";

  if (-1 == member(mandirs,args[0]))
    return "Section '"+args[0]+"' not found.\n";

  if (!({int})call_other(SUBMANSERVER(args[0]),"IndexFile",args[1],verbose))
    return "Error indexing "+args[0]+"/"+args[1]+".\n";

  return 0;
}

