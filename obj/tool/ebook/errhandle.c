/*
** Book of Errors
** Error-Handling
** Author: Thragor@Nightfall, 05jun96
** Update: Thragor@Nightfall, 18mar97
**
** Code continues at line: 105
**
** Functions:
**   string Set/QueryErrorFile()
**     Handles the file currently read, e. g. /d/shadows/.err
**
**   status Set/QueryModify()
**    A flag showing if the errorfile in memory is the same as the one on disk.
**
**  mapping Set/QueryErrors()
**    Handles the error-mapping. This mapping will be modified and printed
**    and later saved. This is the format of the mapping (using the defines from
**    ebook.h):
**      ([<errfile>(string) : DATE(string);
**                                   ERRORS(mapping)
**      ])
**    where ERRORS(mapping) looks like this:
**      ([COMPILE : ([<line>(int) : ([<error>(string) : COMPILED(string);
**                                                      COMPILELINE(int);
**                                                      LOCATION(string)
**                                             ])
**                        ]);
**        RUNTIME : ([<line>(int) : ([<error>(string) : ENVIRONMENT(string);
**                                                      CREATOR(string);
**                                                      OBJECT(string)
**                                             ])
**                         ])
**      ])
**    i. e. you have a mapping containing informations about runtime- and
**    compile-errors which look like this in log-files (^ marks the beginning
**    of a line while $ marks the ending of a line; all other formats depend
**    on this text; newlines without $ indicate a space; double spaces should
**    be ignored, they are just used to format the text):
**      Compile:
**      ========
**        ^d/shadows/common/room/outskirts/cavetree.c
**            line 41 :Variable P_FLYING not declared !$
**        ^Compiling "/d/shadows/common/room/outskirts/cavetree.c", Line 41$
**      or
**        ^d/halfling/common/inc/halfling.h line 78 before 
**          eatcorpse q:syntax error$
**        ^Compiling "/d/halfling/common/inc/halfling.h", Line 78$
**
**        Description:
**          The first entry specifies the file where the error occured followed
**          by the line which needs to be edited to remove the error. Finally
**          the error is described.
**          Optionally the object which got compiled is given in the following
**          line. Most of the time this will be the same as the actual file
**          where the error occured in.
**          In some special cases also the location of the error is given.
**
**        Evokation:
**          raise_error(string err)
**
**      Runtime:
**      ========
**        ^Obj: d/woodland/common/monster/barman#33747,
**          File: d/woodland/common/monster/barman.c, Line: 39:$
**        ^Created by /d/woodland/common/lorinia/singlebar$
**        ^  Bad argument 1 to call_other()$
**      or
**        ^Obj: d/avalon/common/obj/plainarrow#11021 in
**          /d/avalon/common/plains/crossing,
**          File: d/avalon/common/obj/plainarrow.c,
**          Line: 23:$
**        ^  add_action from object that was not present.$
**        Description:
**          'File:' specifies the file where the error actually occured in,
**          i. e. which file needs to be fixed, while 'Obj:' describes which
**          file 'detected' the error (or which object caused the error). If
**          this object has an environment this is specified after 'in' behind
**          the object.
**          'Line:' hopefully shows the line which to edit to remove the
**          error. This information may be wrong, if there has been an error
**          before this line or if the line couldn't be evaluated (in this
**          case the line is normally set to -1). Optionally there may be also
**          the creator of the object given.
**          This is done in the second line. The error will move to the third
**          line then.
**
**        Evokation:
**          #include <config.h>
**          MASTER->runtime_error(string err, string file, string obj, int line)
**
**  mapping AddError()
**    Is used to build up the error-mapping step by step (i. e. error by error).
**
**  void write_runtime_error()
**    Write a runtime-error back to the .err-file.
**
**  status get_runtime_error()
**    Extracts all data from an error-entry, if it is a runtime-error. If it is
**    not, 0 is returned.
**
**  void write_compile_error()
**  status get_compile_error()
**    Same as above for compile-errors.
**
**  void read_data(string errfile)
**    Read (and compile) the errors from a given error-file. Several settings will
**    be done automaticlly like setting the error-file.
**
**  status save_data()
**    Delete the old error-file and save the new (modified) one. If this fails, e. g.
**    if the user lacks of write-access, 0 is returned.
*/

#include "ebook.h"

inherit "/lib/string";

private static mapping Perrors;
private static string Perrfile;
private static status Pmodified;

public string SetErrorFile(string err)
     // Set the error-file which gets currently read.
{
  return Perrfile = err;
}

public string QueryErrorFile()
     // Return the error-file which gets currently read.
{
  return Perrfile;
}

public status SetModify(status b)
     // Set, if the file and the data in memory are the same.
{
  return Pmodified = b;
}

public status QueryModify()
     // Query, if we modified anything compared from memory to file.
{
  return Pmodified;
}

/*
** =================
** Handle the errors
** =================
*/

public mapping SetErrors(mapping err)
{
  return Perrors = err;
}

public mapping QueryErrors()
{
  return (Perrors||m_allocate(0,2))+m_allocate(0,2);
}

public varargs mapping AddError(string errorkind,
				string file,string date,int line,
				string error,
				string compiled_or_environment,
				mixed compileline_or_creator,
				string location_or_obj)
     // <errorkind> is either COMPILE or RUNTIME.
     // If it is COMPILE the args are:
     // AddError(...,string file,string data,int line,string error,
     //          string compiled,int compileline,string compileline)
     // If it is RUNTIME the args are:
     // AddError(...,string file,string date,int line,string error,
     //          string environment,string creator,string obj)
{
  mapping errors,err;
  errors = QueryErrors();
  if (!member(errors,file))
    errors[file,ERRORS] = ([COMPILE:([]),RUNTIME:([])]);
  errors[file,DATE] = date;
  err = errors[file,ERRORS][errorkind];
  switch(errorkind)
    {
    case COMPILE:
      if (!member(err,line))
	err[line]=m_allocate(0,3);
      err[line][error,COMPILED] = compiled_or_environment;
      err[line][error,COMPILELINE] = compileline_or_creator;
      err[line][error,LOCATION] = location_or_obj;
      break;
    case RUNTIME:
      if (!member(err,line))
	err[line]=m_allocate(0,3);
      err[line][error,ENVIRONMENT] = compiled_or_environment;
      err[line][error,CREATOR] = compileline_or_creator;
      err[line][error,OBJECT] = location_or_obj;
      break;
    default:
      return QueryErrors();
    }
  errors[file,ERRORS][errorkind] = err;
  return SetErrors(errors);
}

/*
** =====================
** Handle Runtime-Errors
** =====================
*/

protected void write_runtime_error(string obj,
				 string env,
				 string file,
				 string creator,
				 int line,
				 string error)
     // Write the runtime-error to the error-file.
{
  string dest;
  dest = QueryErrorFile();
  // If the environment is given, add this to the object-description
  if (env) obj = sprintf("%s in %s",obj,env);
  // Write the first line off the error, describing, where the error
  // occured.
  write_file(dest,sprintf("Obj: %s, File: %s, Line: %d:\n",obj,file,line));
  // If we know the creator, put him into an extra line.
  if (creator)
    write_file(dest,sprintf("Created by %s\n",creator));
  // Finally write the error to the file.
  write_file(dest,sprintf("  %s\n",error));
}

protected status get_runtime_error(string str,
				 string obj,
				 string env,
				 string file,
				 string creator,
				 int line,
				 string error)
     // Extract all data from an logical error-line in the .err-file.
     // <str> contains the data which got read, the others have to be
     // called by reference (&obj...), so that they contain the extracted
     // data afterwards.
{
  string *lines;
  int pos;
  lines = explode(str,"\n"); // split the lines
  lines = map(lines,SF(strip_blanks)); // ignore all doubled blanks
  pos = 0; // We start to scan in the first line
  // If <str> contains no data, return.
  if (sizeof(lines)<=pos) return 0;
  if (sscanf(lines[pos],"Obj: %s, File: %s, Line: %d:",obj,file,line)!=3)
    // If the first line doesn't fit to the runtime-scheme, quit.
    return 0;
  // Check if an environment is given in the object-description.
  if (sscanf(obj,"%s in %s",str,env)==2)
    obj = str;
  else
    env = 0;
  // Check, if the object is a blueprint. If yes, replace the number
  // by three dots. This is to ease the search which errors belong
  // together. And it's quite unneccessary to see what object-number
  // the file had.
  if (blueprint(obj)!=obj)
    obj = blueprint(obj)+"#...";
  pos++; // Go on to the next line
  // If there are no more lines left, the runtime-error is corrupted, as
  // the error-message itself is missing. So: return.
  if (sizeof(lines)<=pos) return 0;
  // Scan for the creator and if present increase the line to scan.
  if (sscanf(lines[pos],"Created by %s",creator))
    pos++;
  // If there are no more lines left, the runtime-error is corrupted, as
  // the error-message itself is missing. So: return.
  if (sizeof(lines)<=pos) return 0;
  error = lines[pos]; // Finally we got the error.
  return 1;
}

/*
** =====================
** Handle Compile-Errors
** =====================
*/

private void write_compile_error(string file,
				 int line,
				 string error,
				 string compiled,
				 int compileline,
				 string location)
     // Write the compile-error to the eror-file
{
  string dest;
  dest = QueryErrorFile();
  if (location)
    write_file(dest,sprintf("%s line %d %s:%s\n",file,line,location,error));
  else
    write_file(dest,sprintf("%s line %d :%s\n",file,line,error));
  if (compiled)
    write_file(dest,sprintf("Compiling \"%s\", Line %d\n",
			    compiled,compileline));
}

private status get_compile_error(string str,
				 string file,
				 int line,
				 string error,
				 string compiled,
				 int compileline,
				 string location)
     // Extract all data from an logical error-line in the .err-file.
     // <str> contains the data which got read, the others have to be
     // called by reference (&file...), so that they contain the extracted
     // data afterwards.
{
  int pos;
  string *lines;
  location = 0;
  lines = explode(str,"\n");
  lines = map(lines,SF(strip_blanks));
  pos = 0;
  if (sizeof(lines)<=pos)
    return 0;
  if (sscanf(lines[pos],"%s line %d :%s",file,line,error)!=3)
    if (sscanf(lines[pos],"%s line %d %s:%s",file,line,location,error)!=4)
      return 0;
  pos++;
  if (sizeof(lines)>pos)
    if (sscanf(lines[pos],"Compiling \"%s\", Line %d",compiled,compileline)!=2)
      return 0;
  return 1;
}

/*
** ============================
** Read and Save the error-file
** ============================
*/

public void read_data(string errfile)
{
  int start; // Line-Position in the error-file
  string read, // The line which got currently read
    *lines; // all lines belonging to one error
  SetErrorFile(errfile);
  SetErrors(0); // Empty the errors
  SetModify(0); // The data in memory are equal to those in the file.
  start = 1; // Start to read in line 1
  while(read = read_file(errfile,start,1))
    {
      string datestr;
      start++;
      // The first entry has to be a date. If not, the file is corrupted and
      // we'll skip reading to the next line.
      if (sscanf(read," -- %s --\n",datestr)!=1) continue;
      lines = ({});
      while(read = read_file(errfile,start,1))
	{
	  string hs, // help-string-variable
	    file,  // This and the following are used as variables to retrieve
	    error, // either a compile- or a runtime-error.
	    compiled, obj, env, creator, location;
	  int hi, // help-integer-variable
	    line,compileline;
	  // Initialize all variables
	  hs = file = error = compiled = obj = env = creator = 0;
	  hi = line = compileline = 0;
	  // If we already read an error, check if currently a new error
	  // begins. Then this error ended and will be added to the
	  // database.
	  if (sizeof(lines))
	    if (      read=="" // EOF
		   || sscanf(read," -- %s --\n",hs) // New date
		   || sscanf(read,"%s line %d :%s\n",hs,hi,hs)==3
		   || sscanf(read,"%s line %d %s:%s\n",hs,hi,hs,hs)==4
		   || sscanf(read,"Obj: %s, File: %s, Line: %d\n",hs,hs,hi)==3
		   )
	      {
		if (get_compile_error(implode(lines,""),
				      &file,&line,&error,&compiled,
				      &compileline,
				      &location)
		    )
		  AddError(COMPILE,file,datestr,line,error,
			   compiled,compileline,location);
		else if (get_runtime_error(implode(lines,""),
					   &obj,&env,&file,&creator,&line,
					   &error)
			 )
		  AddError(RUNTIME,file,datestr,line,error,env,creator,obj);
		break; // leave the 'read-one-error'-loop
	      }
	  lines+=({read}); // The line read belongs to the error
	  start++; // continue with the next line
	}
    }
}

private status check_greater(string datea,string dateb)
     // check_greater is used, because we need to save the errors in the
     // right order (i. e. lowest date first). As we don't have the date
     // as number, we need to analyze the string.
{
  string aday,amonth,
         bday,bmonth,
         *months;
  int adaynr,ahour,aminute,asecond,ayear,
      bdaynr,bhour,bminute,bsecond,byear;
  sscanf(datea,"%s %s %d %d:%d:%d %d",aday,amonth,adaynr,ahour,aminute,
	 asecond,ayear);
  sscanf(dateb,"%s %s %d %d:%d:%d %d",bday,bmonth,bdaynr,bhour,bminute,
	 bsecond,byear);
  if (ayear!=byear) return ayear>byear;
  months = ({"Jan","Feb","Mar","Apr","May","Jun",
	     "Jul","Aug","Sep","Oct","Nov","Dec"});
  if (amonth!=bmonth) return member(months,amonth)>member(months,bmonth);
  if (adaynr!=bdaynr) return adaynr>bdaynr;
  if (ahour!=bhour) return ahour>bhour;
  if (aminute!=bminute) return aminute>bminute;
  return asecond>bsecond;
}

public status save_data()
{
  mapping errors;
  string *dates,*idx,errfile;
  int i;
  errfile = QueryErrorFile();
  if (rm(errfile)==0) return 0; // no write access
  SetModify(0);
  if (!sizeof(errors = QueryErrors())) return 1;
  // Identify the error-file as modified by the Book of Errors
  // This might be useful, if the format of error-messages change,
  // and the book of errors reports false.
  write_file(errfile,"*** Book of Errors, "+ctime()+" ***\n");
  dates = m_values(errors);
  // Sort the dates, so that the errors will be saved in chronological order.
  dates = sort_array(dates,SF(check_greater));
  idx = m_indices(errors);
  while(sizeof(dates))
    {
      i = sizeof(idx);
      write_file(errfile,sprintf(" -- %s --\n",dates[0]));
      while(i--)
	if (errors[idx[i],0]==dates[0])
	  {
	    int j,k,*lines;
	    string *eidx;
	    mapping lineerrs,errs;
	    lines = sort_array(m_indices(lineerrs = errors[idx[i],1][COMPILE]),
			       SF(<));
	    j = sizeof(lines);
	    while(j--)
	      {
		errs = lineerrs[lines[j]];
		k = sizeof(eidx = m_indices(errs));
		while(k--)
		  write_compile_error(idx[i],lines[j],eidx[k],
				      errs[eidx[k],COMPILED],
				      errs[eidx[k],COMPILELINE],
				      errs[eidx[k],LOCATION]);
	      }
	    lines = sort_array(m_indices(lineerrs = errors[idx[i],1][RUNTIME]),
			       SF(<));
	    j = sizeof(lines);
	    while(j--)
	      {
		errs = lineerrs[lines[j]];
		k = sizeof(eidx = m_indices(errs));
		while(k--)
		  write_runtime_error(errs[eidx[k],OBJECT],
				      errs[eidx[k],ENVIRONMENT],
				      idx[i],
				      errs[eidx[k],CREATOR],
				      lines[j],
				      eidx[k]);
	      }
	  }
      dates-=({dates[0]});
    }
  return 1;
}
