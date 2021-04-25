/*
** A small tool to read .err-files
**
** Author: Thragor@Nightfall, 20jun94
** Update: Thragor@Nightfall, 21jun95
*/

#include <properties.h>
#include <config.h>

#define VERSION "V2.1"
#define DATE "Jun 21st, 1995"
#define ME this_object()
#define TP this_player()
#define STR "/lib/string"
#define PREVIOUS "previous"
#define NEXT "next"
#define CURRENT "current"
#define SF(x) #'x //'

#define ERR_OBJ 0
#define ERR_FILE 1
#define ERR_LINE 2
#define ERR_ERROR 3
#define ERR_CREATOR 4
#define ERR_COMPLINE 5
#define ERR_COMPILED 6
#define ERR_SIZE 7
inherit "/std/thing";

private static mapping err_entries;
private static int cur_line,oldsize;
private static string path,*err_idx;

protected void build_screen();

public int QueryAutoObject() { return 1; }

public int create()
{
  if (!is_clone(ME)) return 1;
  (::create());
  SetShort("the book of errors");
  SetLong(STR->wrap(
    "The book of errors contains the evil bugs creeping through "
    "your directories.\n"
    "This tool allows you to browse through .err-files like "
    "you read your mail. You just have to type: eread [<dir>].\n"
    "The dir shows, where to search for the .err-file. This might "
    "be an falias or something with leading ~, +, etc.\n",75,0));
  SetReadMsg("Try 'eread [<dir>]'.\n");
  SetWeight(0);
  SetValue(0);
  Set(P_NOSELL,1);
  Set(P_NOBUY,1);
  SetNoDrop(1);
  SetNoGive(1);
  SetIds(({"tool","etool","ebook","error","errors","book"}));
  SetAds(({"the","book","of","error"}));
  return 0;
}

public void init()
{
  if (!is_clone(ME)) return;
  (::init());
  add_action("cmd_eread","eread");
}

protected string read_error_file(string file)
{
  string res;
  int fpos, todo, chunk;
  todo = file_size(file);
  fpos = 0;
  res = "";
  while(todo>0)
    {
      chunk = todo;
      if (chunk>10000) chunk = 10000;
      res += read_bytes(file,fpos,chunk);
      fpos+=chunk;
      todo-=chunk;
    }
  return res;
}

protected mixed filter_errors(string *l)
{
  string obj,file,error,creator,compiled;
  int line,i,compileline;
  mixed res,entry;
  
  res = allocate(0);

  while(i<sizeof(l))
    {
      if (sscanf(l[i],"%s line %d :%s",file,line,error)==3)
	{
	  entry = allocate(ERR_SIZE);
          entry[ERR_FILE] = file;
          entry[ERR_LINE] = line;
          entry[ERR_ERROR] = error;
          if (sscanf(l[i+1],"Compiling \"%s\", Line %d",compiled,compileline))
	    {
	      i++;
	      entry[ERR_COMPILED] = compiled;
	      entry[ERR_COMPLINE] = compileline;
	    }
          res+=({entry});
          i++;
	  continue;
	}
	  
      if (sscanf(l[i],"Obj: %s, File: %s, Line: %d:",obj,file,line)!=3)
        {
          i++;
          continue;
        }
  
      if (i<sizeof(l)-1)
        {
          if (sscanf(l[i+1],"Obj: %s, File: %s, Line: %d:",obj,file,line)==3)
            {
	      i++;
	      continue;
            }
        }
      else
        {
          i++;
          continue;
        }
      if (sscanf(l[i+1],"Created by %s",creator))
	i++;
      error = l[i+1][2..];
      entry = allocate(ERR_SIZE);
      entry[ERR_OBJ] = obj;
      entry[ERR_FILE] = file;
      entry[ERR_LINE] = line;
      entry[ERR_ERROR] = error;
      entry[ERR_CREATOR] = creator;
      res+=({entry});
      i+=2;
    }
  if (!sizeof(res)) return 0;
  return res;
}

  
protected string get_title(string str)
{
  string res;
  sscanf(str," -- %s --",res);
  return res;
}

protected mapping read_errors(string path)
{
  string *l,*d,file;
  int lcount,dcount,i,dbegin,dend,pos;
  mapping erg;
  mixed errors;

  file = "/"+path+"/.err";
  if ((oldsize!=-1)&&(oldsize==file_size(file)))
    return err_entries;
  if ((oldsize=file_size(file))<1)
    return notify_fail("No entries in "+file+".\n")&&0;

  l = explode(read_error_file(file),"\n"); // the lines
  dcount = sizeof(d = regexp(l,"^ -- ... ... .. ..:..:.. .... --")); // the dates
  // First delete all double dates
  for (i=0;i<dcount-1;i++)
    if (d[i+1]==d[i])
      {
	pos = member(l[member(l,d[i])+1..],d[i]);
	l = l[0..pos-1]+l[pos+1..];
      }

  dcount = sizeof(d = regexp(l,"^ -- ... ... .. ..:..:.. .... --"));

  erg = m_allocate(0,1);
  // erg: ([<date>:({({<obj>,<file>,<line>,<error>,<creator>,<compiled>,
  //                   <compileline>})})])
  for (i=0;i<dcount;i++)
    {
      dbegin = member(l,d[i])+1;
      if (i<dcount-1)
        dend = member(l,d[i+1])-1;
      else
	dend = sizeof(l)-1;
      if (dend<dbegin)
	continue;
      if (errors = filter_errors(l[dbegin..dend]))
        erg[get_title(d[i])] = errors;
    }
  
  return erg;
}

protected int dec_line()
{
  cur_line--;
  if (cur_line<0) 
    {
      write("Line out of range.\n");
      cur_line=0;
      return 0;
    }
  return 1;
}

protected int inc_line()
{
  cur_line++;
  if (cur_line==sizeof(err_idx)) 
    {
      write("Line out of range.\n");
      cur_line=sizeof(err_idx)-1;
      return 0;
    }
  return 1;
}
  
protected void check_line()
{
  if (cur_line<0) cur_line=0;
  else if (cur_line>sizeof(err_idx)-1)
    cur_line=sizeof(err_idx)-1;
}

protected void list_errors()
{
  int i;
  
  check_line();
  
  for(i=0;i<sizeof(err_idx);i++)
    write(((i==cur_line)?"*":" ")+
          STR->radjust(to_string(i+1),3)+". "+err_idx[i]+
          " ("+to_string(sizeof(err_entries[err_idx[i]]))+" errors)\n");
}

protected string error_entry(mixed entry)
{
  return
    (entry[ERR_OBJ]?
    "Object:  "+entry[ERR_OBJ]+"\n":"")+
    "File:    "+entry[ERR_FILE]+"\n"
    "Line:    "+to_string(entry[ERR_LINE])+"\n"+
    (entry[ERR_CREATOR]?
    "Creator: "+entry[ERR_CREATOR]+"\n":"")+
    (entry[ERR_COMPILED]?
    "Compiled: "+entry[ERR_COMPILED]+"\n"+
    "CLine:    "+entry[ERR_COMPLINE]+"\n":"")+
    "Error:   "+entry[ERR_ERROR]+"\n";
}

protected void display_error(string what)
{
  string *idx;
  if (what==PREVIOUS)
    if (!dec_line())
      return build_screen();
  if (what==NEXT)
    if (!inc_line())
      return build_screen();

  STR->smore("\n"
             "Errors on "+err_idx[cur_line]+":\n"+STR->mkstr(75,"-")+"\n"+
             implode(map(err_entries[err_idx[cur_line]],
                               SF(error_entry)),"\n")+
             STR->mkstr(75,"-")+"\n\n",
             TP->QueryPageSize(),ME,"build_screen");
}

public void display_regexp(string str)
{
  string *err,*to_display,title;
  int i;

  title = "List of errors in /"+path+"/.err:";
  to_display=({});
  if (!str)
    to_display = err_idx;
  else
    {
      title = "Errors fitting to regexp '"+str+"':";
      for(i=0;i<sizeof(err_idx);i++)
        if (sizeof(regexp(({err_idx[i]}),str))
            ||sizeof(regexp(map(err_entries[err_idx[i]],
                                      SF(error_entry)),str)))
          to_display+=({err_idx[i]});

      if (!sizeof(to_display||({})))
        {
          write("No fitting entries in /"+path+"/.err.\n");
          build_screen();
          return;
        }
    }
  
  err = allocate(0);
  for(i=0;i<sizeof(to_display);i++)
    if (sizeof(regexp(({to_display[i]}),str||".")))
      err+=({" -- "+to_display[i]+" --\n"+
             implode(map(err_entries[to_display[i]],SF(error_entry)),
                     "\n")});
    else
      err+=({" -- "+to_display[i]+" --\n"+
             implode(
               regexp(
                 map(err_entries[to_display[i]],
                           SF(error_entry)),
                 str||"."),"\n")});

  STR->smore("\n"+title+"\n"+STR->mkstr(75,"-")+"\n"+
             implode(err,"\n")+
             STR->mkstr(75,"-")+"\n\n",
             TP->QueryPageSize(),ME,"build_screen");
}

protected void display_matching_errors()
{
  write("\nEnter regexp: ");
  input_to("display_regexp");
}

protected void display_number(int nr)
{
  if ((nr<1)||(nr>sizeof(err_idx))) 
    {
      write("Line number out of range.\n");
      return build_screen();
    }
  cur_line=nr-1;
  display_error(CURRENT);
}

protected void move_error(string what)
{
  if (what==PREVIOUS)
    if (!dec_line())
      return;
  if (what==NEXT)
    if (!inc_line())
      return;
}

protected int save_errors()
{
  int i,j;
  
  if (!rm("/"+path+"/.err"))
    {
      write("Failed to save updated .err-file.\n");
      return 0;
    }
  if (!sizeof(err_idx))
    {
      write("Deleted /"+path+"/.err.\n");
      return 1;
    }
  for(i=0;i<sizeof(err_idx);i++)
    {
      write_file("/"+path+"/.err"," -- "+err_idx[i]+" --\n");
      for (j=0;j<sizeof(err_entries[err_idx[i]]);j++)
        if (!err_entries[err_idx[i]][j][ERR_OBJ])
          write_file("/"+path+"/.err",
                     err_entries[err_idx[i]][j][ERR_FILE]+
                     " line "+to_string(err_entries[err_idx[i]][j][ERR_LINE])+
                     " :"+err_entries[err_idx[i]][j][ERR_ERROR]+"\n"+
                     (err_entries[err_idx[i]][j][ERR_COMPILED]?
                      "Compiling \""+err_entries[err_idx[i]][j][ERR_COMPILED]+
                      "\", Line "+to_string(
                        err_entries[err_idx[i]][j][ERR_COMPLINE])+"\n":""));
        else
          write_file("/"+path+"/.err",
                   "Obj: "+err_entries[err_idx[i]][j][ERR_OBJ]+", "+
                   "File: "+err_entries[err_idx[i]][j][ERR_FILE]+", "+
                   "Line: "+to_string(err_entries[err_idx[i]][j][ERR_LINE])+":\n"+
                   "  "+err_entries[err_idx[i]][j][ERR_ERROR]+"\n");
    }
  return 1;
}
  
protected void delete_error()
{
  string *err;
  mapping errors;
  int i;

  err_idx-=({err_idx[cur_line]});
  if (save_errors())
    write("\n");
  return build_screen();
}

public void delete_regexp(string str)
{
  string *to_delete;
  mixed entries,del_entries;
  int i,j,count;
  
  if (!str||str=="")
    {
      write("Deleting aborted.\n");
      build_screen();
      return;
    }
  to_delete = ({});
  for(i=0;i<sizeof(err_idx);i++)
    if (sizeof(regexp(({err_idx[i]}),str))
        ||sizeof(regexp(map(err_entries[err_idx[i]],
                                  SF(error_entry)),str)))
      to_delete+=({err_idx[i]});

  if (!sizeof(to_delete))
    {
      write("No fitting entries in /"+path+"/.err.\n");
      build_screen();
      return;
    }

  for(i=0;i<sizeof(to_delete);i++)
    if (sizeof(regexp(({to_delete[i]}),str)))
      {
	err_idx-=({to_delete[i]});
	count+=sizeof(err_entries[to_delete[i]]);
      }
    else
      {
        entries = err_entries[to_delete[i]];
        del_entries = ({});
	for(j=0;j<sizeof(entries);j++)
          if (sizeof(regexp(({error_entry(entries[j])}),str)))
            del_entries+=({entries[j]});
        if (!sizeof(entries-=del_entries))
          err_idx-=({to_delete[i]});
        else
          err_entries[to_delete[i]] = entries;
	count+=sizeof(del_entries);
      }
  if (save_errors())
    write(to_string(count)+" entries deleted.\n\n");
  build_screen();
}

protected void delete_matching_errors()
{
  write("\nEnter regexp ('.' to delete all, <CR> to quit): ");
  input_to("delete_regexp");
}

protected void show_help()
{
  write(
    "Error Reader "+VERSION+", "+DATE+"\n\n"
    ".\tshow current error\n"
    "+/n\tshow next error (with/without printing)\n"
    "-/p\tshow previous error (with/without printing)\n"
    "d\tdelete error\n"
    "D\tdelete all matching errors (regexp)\n"
    "s\tshow errors\n"
    "S\tshow all matching errors(regexp)\n"
    "q\tquit\n\n");
}

protected void write_prompt()
{
  write("\nError command (1-"+to_string(sizeof(err_idx))+
        " . + - n p d D s S q ?) -> ");
}

protected int sort_dates(string date1,string date2)
{
  int mt1,mt2,    // index of the month (Jan, Feb, Mar, Apr, May, Jun,
                  //                     Jul, Aug, Sep, Oct, Nov, Dec)
      d1,d2,      // the day in the month
      h1,h2,      // the hour
      m1,m2,      // the minute
      s1,s2,      // the second
      y1,y2;      // the year
  string dow,month;
  
  sscanf(date1,"%s %s %d %d:%d:%d %d",dow,month,d1,h1,m1,s1,y1);
  mt1 = member(({"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep",
                 "Oct","Nov","Dec"}),month);
  sscanf(date2,"%s %s %d %d:%d:%d %d",dow,month,d2,h2,m2,s2,y2);
  mt2 = member(({"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep",
                 "Oct","Nov","Dec"}),month);


  return
    (y1!=y2)
    ?y1>y2
    :((mt1!=mt2)
      ?mt1>mt2
      :((d1!=d2)
        ?d1>d2
        :((h1!=h2)
          ?h1>h2
          :((m1!=m2)
            ?m1>m2
            :s1>s2))));
}

public void build_screen()
{
  if (!err_entries = read_errors(path))
    {
      write("No more errors. Quitting.\n");
      return;
    }
  err_idx = sort_array(m_indices(err_entries),SF(sort_dates));
  list_errors();
  write_prompt();
  input_to("loop");
}

protected void leave_loop()
{
  write("Ok.\n");
}

public void loop(string str)
{
  int nr;
  
  switch(str)
    {
    case "":
    case ".":
      return display_error(CURRENT);
    case "+":
      return display_error(NEXT);
    case "-":
      return display_error(PREVIOUS);
    case "n":
      move_error(NEXT);
      break;
    case "p":
      move_error(PREVIOUS);
      break;
    case "d":
      delete_error();
      break;
    case "D":
      return delete_matching_errors();
    case "S":
      return display_matching_errors();
    case "s":
      return display_regexp(0);
    case "q":
      return leave_loop();
    case "?":
      show_help();
      break;
    default:
      if (sscanf(str,"%d",nr))
	return display_number(nr);
      else
	write("Unknown command.\n");
    }
  build_screen();
}

public int cmd_eread(string str)
{
  string file;
  
  if (!str) str = WPATH+getuid(TP);
  path = TP->get_path(str)[1..];
  file = "/"+path+"/.err";
  if (file_size(file)<0)
    return (int)notify_fail("No file "+file+".\n")&&0;
  if (file_size(file)==0) 
    return (int)notify_fail("File "+file+" is empty.\n")&&0;
  oldsize = -1;
  if (!err_entries = read_errors(path))
    return (int)notify_fail("No errors found in "+file+".\n");
  cur_line = 0;
  build_screen();
  return 1;
}
