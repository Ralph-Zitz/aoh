/*
** /obj/lib/register
**
** Object to register different objects.
** Just do SetSaveFile(<file>) in the create
** of this object.
**
** 01oct92 [m] Creation
** 11apr94 [t] Some small changes
** 13jun95 [t] Made a /obj/lib-file of the register
** 15jun95 [t] PrintList added
**
** [m] Mateese
** [t] Thragor
*/

inherit "/lib/string";

#define REGISTER_DIR "/log/register/"
#define TMP_REGISTER "/open/finance"  // where file is initially written
#define P_LIST "RegisterList" // to temporary store the register list
#define TP this_player()
#define SF(x) #'x //'
#define SYM(x) 'x //'

private nosave string savefile;
private nosave string loaded;
public mapping register;
// ([<filename>:({<creators>})])

protected string build_file(string s)
{
  if (!s) s = savefile||"FOOBAR"; // Small and rough fix... Sique Sep 23 95
  if (  s
      &&s[<2..]==".o")
    s = s[0..<3];
  if (sizeof(explode(s,"/"))==1)
    s = REGISTER_DIR+s;
  return s;
}

public varargs void LoadRegister(string s)
{
  seteuid(getuid());
  s = build_file(s);
  if (!register||loaded!=s)
    if (file_size(s+".o")>=0)
      restore_object(s);
    else
      register = ([]);
  loaded = s;
  seteuid(0);
}

public varargs void SaveRegister(string s)
{
  seteuid(getuid());
  s = build_file(s);
  if (register)
    save_object(s);
  seteuid(0);
}

public string QueryLoaded()
{
  return loaded;
}

public string SetSaveFile(string s)
{
  if (object_name(this_object())=="/obj/lib/register")
    return 0;
  return savefile = s;
}
public string QuerySaveFile() { return savefile; }

public varargs mapping SetRegister(mapping r,string s)
{
  register = r;
  SaveRegister(s);
  return register;
}
public varargs mapping QueryRegister(string s)
{
  LoadRegister(s);
  return register||([]);
}

public varargs void Register(mixed ob,string s)
{
  status save;
  string creator,fn;

  if (objectp(ob))
    {
      fn = object_name(ob);
      ob = blueprint(ob);
    }
  else if (stringp(ob))
    {
      fn = ob;
      ob = blueprint(ob);
    }
  else
    {
      if (object_name(this_object())=="/obj/lib/register")
        fn = object_name(previous_object());
      else
        fn = object_name(this_object());
      ob = blueprint(fn);
    }
  LoadRegister(s);
  if (!member(register,ob))
    {
      register[ob] = ({});
      save = 1;
    }
  if (fn!=ob
      &&member(register[ob],creator = blueprint(({string})ob->QueryCreator()))==-1
      &&!find_savefile(lower_case(creator[1..]))
      &&creator!="/obj/tool/xtool")
    {
      register[ob]+=({creator});
      save = 1;
    }
  if (save) SaveRegister(s);
}

public varargs void RegisterCreator(mixed ob,string s)
{
  status save;
  string creator,fn;

  if (objectp(ob))
    {
      fn = object_name(ob);
      ob = blueprint(ob);
    }
  else if (stringp(ob))
    {
      fn = ob;
      ob = blueprint(ob);
    }
  else
    {
      if (object_name(this_object())=="/obj/lib/register")
        fn = object_name(previous_object());
      else
        fn = object_name(this_object());
      ob = blueprint(fn);
    }
  LoadRegister(s);
  if (!member(register,ob))
    {
      register[ob] = ({});
      save = 1;
    }
  if (fn!=ob
      &&member(register[ob],creator = blueprint(({string})fn->QueryCreator()))==-1
      &&!find_savefile(lower_case(creator[1..]))
      &&creator!="/obj/tool/xtool")
    {
      register[ob]+=({creator});
      save = 1;
    }
  if (save) SaveRegister(s);
}

public varargs void RemoveEntry(mixed ob,string s)
{
  if (objectp(ob))
    ob = blueprint(ob);
  if (!stringp(ob)) return;
  LoadRegister(s);
  m_delete(register,ob);
  SaveRegister(s);
}

public varargs string *ClearEntries(string s)
{
  string *idx,*removed;
  int i;

  LoadRegister(s);
  i = sizeof(idx = m_indices(register));
  removed = ({});
  while(i--)
    if (file_size(idx[i]+".c")<0)
      {
	RemoveEntry(idx[i],s);
        removed+=({idx[i]});
      }
  return removed;
}

protected string list_strings(string *arr)
{
  if (!sizeof(arr)) return "<none>";
  return implode(arr[0..<2],", ")+
         (sizeof(arr)>1?" and ":"")+arr[<1];
}

protected void message(string msg)
{
  if (  !TP
      ||({int})TP->QueryEarmuffs()
      ||({int})TP->QueryNetdead())
    return;
  write(msg);
}

protected int copyfile(string src,string dst)
// copyfile taken from /std/player/filesys.c
{
  int fpos, todo, chunk;
#if __VERSION__ > "3.5.2"
  bytes data;
#else
  string data;
#endif

  seteuid(getuid());
  todo = file_size(src);
  fpos = 0;
  while (todo > 0)
    {
      chunk = todo;
      if (chunk > 10000) chunk = 10000;
      data = read_bytes (src, fpos, chunk);
      todo -= chunk;
      fpos += chunk;
#if __VERSION__ > "3.5.2"
      if (!write_file(dst, to_text(data, "UTF-8"))) return 0;
#else
      if (!write_file(dst, data)) return 0;
#endif
    }
  seteuid(0);
  return 1;
}

protected string adjust_info(string str)
{
  return implode(map(explode(str,"\n"),
    lambda(({SYM(s)}),({SF(+),"       ",SYM(s)}))),"\n");
}

public varargs void PrintList(string s,string file,closure fun,
                              mapping remaining,string *idx,int pos)
// Output may be redirected to <file>
// fun will be called to get additional info; if fun returns 0
//   the entry will be removed
// remaining is only for internal use
{
  status clones;
  if (!TP||({string})TP->QueryNetdead()) return;
  if (!remaining)
    {
      LoadRegister(s);
      remaining = register;
      idx = sort_array(m_indices(remaining),SF(>));
      pos = 0;
    }
  clones = sizeof(m_values(remaining)-({({})}))>0;
  while(   get_eval_cost() > 600000
        && pos < sizeof(idx)
       )
    {
      string extrainfo,text;
      extrainfo = 0;
      if (file_size(idx[pos]+".c")<0)
        RemoveEntry(idx[pos],s);
      else
        if (fun)
	  {
	    if (!extrainfo = funcall(fun,idx[pos],pos==sizeof(idx)-1))
              RemoveEntry(idx[pos],s);
	    else
	      extrainfo = adjust_info(extrainfo);
	  }
        else
          extrainfo = "";
      if (clones)
        text = wrap(radjust(to_string(pos+1),3)+". "+
                    idx[pos]+": "+list_strings(remaining[idx[pos]])+"\n",
                    0,sizeof(idx[pos])+2)
	       +(extrainfo||"  (removed)")+"\n";
      else
        text = radjust(to_string(pos+1),3)+". "+idx[pos]+
               (!extrainfo?": removed\n":"")+
               (extrainfo?extrainfo==""?"\n":":\n"+extrainfo+"\n":"");
      if (file)
	{
	  message(radjust(to_string(pos+1),3)+"/"+
                  radjust(to_string(sizeof(idx)),3)+" "+idx[pos]+"\n");
          seteuid(getuid());
      if (file_size(TMP_REGISTER) == -1) // oops directory doesn't exist
         if (!mkdir(TMP_REGISTER))
         {
            write("Directory " + TMP_REGISTER + " does not exist and "
                  "could not be created. Aborting action.\n");
            return 0;
         }
	  write_file(file+"."+getuid(TP),text);
          seteuid(0);
	}
      else
	write(text);
      pos++;
    }
  if (pos<sizeof(idx))
    call_out(SF(PrintList),10,s,file,fun,remaining,idx,pos);
  else
    {
      write("Register-action finished.\n");
      if (file)
        {
          seteuid(getuid());
	  rm(file);
          seteuid(0);
	  copyfile(file+"."+getuid(TP),file);
          seteuid(getuid());
	  rm(file+"."+getuid(TP));
          seteuid(0);
          write("Results can be found in "+file+".\n");
        }
    }

}

public void create()
{
  if (is_clone())
    destruct(this_object());
  if (file_size(REGISTER_DIR) != -2)
    mkdir(REGISTER_DIR);
}
