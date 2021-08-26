#include "./wizbelt-defaults.h"
#include <secure/wizlevels.h>
#include <moving.h>

#ifndef VOID
#define VOID "/std/void"
#endif

#define Out(x) ({string})"/lib/string"->smore(x,({int})this_player()->QueryPageSize())

string WFindFilename(string str);
string WGrep(string file, string pat);
string WHead(string lines, string file2);
string WRml(int num, string file);
string WRepl(string file, string find, string repl, int backup);
string Replace(string file, string find, string repl, int backup);
string WFind(string str, string pat);
string WReplAll(string file, string find, string repl, int bup);
string WGrepAll(string str, string pat, int dirs);
string WHeadAll(string file1, string file2);
string WRmlAll(int num, string file1);
int WLoadAll(string str, int clone, int destr);
string MoveFile(string old, string new);
string ReplaceWithJoker(string file, string find1, string find2,
                        string repl1, string repl2, int backup);
int    WLoad(string str, int clone, int destr);
int    WClone(string what, int destr);

string WGrep(string file, string pat) {
   int i, j, k, num;
   string ret, t, *tem, *lines;
   ret = "";
   if (!(pat && file)) return ret;
   for (i=1; t=read_file(file, i, 1000); i+=1000) {
       lines = explode(t, "\n");
       tem = regexp(lines - ({""}), pat);
       if (sizeof(tem)) {
          if (!j++) ret+=">>> File: "+file+" <<<\n";
          for (k=0; k<sizeof(tem); k++) {
              num = member(lines, tem[k]) + i;
              ret += "Line " + num + ": " + tem[k] + "\n";
          }
          // ret+=implode(tem, "\n") +"\n";
       }     
   }
   return ret;
}

string WRml(int num, string file) {
   string file2, *lines;
   file2 = read_file(file);
   lines = explode(file2, "\n");
   lines -= ({ lines[num-1] });
   rm(file);
   file2 = implode(lines, "\n");
   write_file(file, file2);
   return "Changed "+file+".\n";
}
   
string WHead(string lines, string file2) {
   string lines2;
   int ok;
   lines2 = read_file(file2);
   rm(file2);
   ok = write_file(file2, lines);
   ok = write_file(file2, lines2);
   if (!ok) return "Couldn't write " + file2 + ".\n";
   return "Changed " + file2 + ".\n";
}

string WRepl(string file, string find, string repl, int backup) {
   mixed *res;
   string ret, a, b, c, d;
   ret = "";
   if (!(find && file)) return ret;
   res = ({mixed *})this_player()->parse_fname(file, 0);
   if (res[3] == -1) return ret += file + " does not exist.\n";
   /*path = res[1];*/
   if (sscanf(find, "%s*%s", a, b) && sscanf(repl, "%s*%s", c, d)) 
      return ret += ReplaceWithJoker(file, a, b, c, d, backup);
   else return ret += Replace(file, find, repl, backup);
}

string ReplaceWithJoker(string file, string find1, string find2,
                        string repl1, string repl2, int backup) {
   string ret, new, line, find3, rest;
   string *lines;
   int i, j;
   ret = "#Changed file: "+file+"\n";
   for (i=1; line=read_file(file, i, 1000); i+=1000) {
       lines = explode(line, "\n");
       for (j=0; j<sizeof(lines); j++) {
           new = lines[j];
           if (sscanf(lines[j], "%~s" + find1 + "%s" + find2 + "%~s", rest) > 0) { 
              if (sizeof(regexp( ({lines[j]}), find1)))
                 new=({string})"/lib/string"->string_replace(lines[j], find1, repl1);
              find3 = rest + find2;
              if (sizeof(regexp( ({lines[j]}), find3)))
                 new=({string})"/lib/string"->string_replace(new, find3, rest+repl2);
              new += "\n";
           }
           if (!write_file(file+".new", new)) {
              return "Error while trying to write file "+file+
              ".new. Backup is in "+file+"\n";
           }
       }
   }
   MoveFile(file+".new", file);
   if (!backup) rm(file+".new");
   return ret;
}

string Replace(string file, string find, string repl, int backup) {
   string ret, new, line;
   string *lines;
   int i, j;
   ret = "Changed file: "+file+"\n";
   for (i=1; line=read_file(file, i, 1000); i+=1000) {
       lines = explode(line, "\n");
       for (j=0; j<sizeof(lines); j++) {
           if (sizeof(regexp( ({lines[j]}), find)))
              new=({string})"/lib/string"->string_replace(lines[j], find, repl)+"\n";
              else new=lines[j]+"\n";
           if (!write_file(file+".new", new)) {
              return "Error while trying to write file "+file+
                     ".new. Backup is in "+file+"\n";
           }
       }
   }
   MoveFile(file+".new", file);
   if (!backup) rm(file+".new");
   return ret;
}

string MoveFile(string old, string new) {
  int i,j;
  string line;
  string *lines;
  rm(new);
  for (i=1; line=read_file(old, i, 1000); i+=1000) {
       lines = explode(line, "\n");
       for (j=0; j<sizeof(lines); j++) {
           if (!write_file(new, lines[j]+"\n")) {
              return "Error while trying to write file "+new+
                     ".new. Backup is in "+old+"\n";
	   }
       }
  }
  return "";
}

string WFindFilename(string str) {
  string a, ret;
  ret = "";
  if (!str) return ret;
  if (str=="$h") return object_name(environment(this_player()));
  if (this_player() && ({string})this_player()->is_dir(str))
     return ({string})this_player()->is_dir(str);
  if (sscanf(str,"@^%s",a) == 1) return object_name(environment(find_living(a)));
  if (sscanf(str,"~/%s",a) == 1) return "/players/" +
             ({string})this_player()->QueryRealName() + "/" + a;
  ret = ({string})this_player()->exp_fname(str);
  if (sscanf(ret,"/%s", a) < 1) 
     ret = ({string})this_player()->QueryCurrentDir()+"/"+ret;
  if (resolve_file(ret)) return resolve_file(ret);
  return ret;
}

int wrml(string str) {
  string a;
  int b;
  if (!str || (sscanf(str, "%s,%d", a, b) < 1)) {
     notify_fail("Usage: wrml <num> <file|pattern>.\n");
     return 0;
  } 
  Out(WRmlAll(b, a));
  return 1;
}

int wgrep(string str) {
  string fname, file, pat, a, b, ausgabe;
  string *words;
  int dirs, logging;

  if (!Allowed()) return 0;
  if (!str) return 0;
  dirs = 0;    logging = 0;
  words = explode(str, " ");
  if (member(words, "-r") > -1) {
     words -= ({"-r"});
     dirs = 1;
  }
  if (member(words, "-l") > -1) {
     words -= ({"l"});
     logging = 1;
  }
  if (sizeof(words)<2) {
       notify_fail("wgrep [-l] [-r] <regexp> <file>\n");
       return 0;
  }                                
  b = words[sizeof(words)-1];
  words -= ({b});
  a = implode(words, " ");
  if (!(file=WFindFilename(b))) return 1;
  pat = a;
  if (sscanf(file, "%s*%s", a, b) > 0) {
     ausgabe = WGrepAll(file, pat, dirs);
     if (!logging) Out(ausgabe);
     else {
       fname = "/players/" + ({string})this_player()->QueryRealName() + "/wgrep.log";
       write("Writing file to " + fname +".\n");
       if (file_size(fname) > 0) rm(fname); 
       write_file(fname, ausgabe);
     }
     return 1;
  }
  ausgabe = WGrep(file, pat);
  if (!logging) Out(ausgabe);
  else {
     fname = "/players/" + ({string})this_player()->QueryRealName() + "/wgrep.log";
     write("Writing file to " + fname +".\n");
     if (file_size(fname) > 0) rm(fname); 
     write_file(fname, ausgabe);
  }
  write("Wgrep done.\n");
  return 1;
}

int wload(string str) {
  string file, a;
  if (!Allowed()) return 0;
  if (!(file=WFindFilename(str))) return 0;
  if (sscanf(file, "%~s*%s", a) > 0) {
     if (a != ".c") file+=".c";
     WLoadAll(file, 0, 0);
     return 1;
  }
  WLoad(file, 0, 0);
  return 1;
}

int wclone(string str) {
    string file, a, b, c, *parts;
    int flag;
    if (!Allowed()) return 0;
    c = str;
    if (!c) { notify_fail("Wclone <file>,<option>\n"); return 0; }
    flag = 0;
    parts = explode(c, ",");
    if (sizeof(parts) > 1) {
       a = parts[0];
       flag = 1;
    }
    else a = parts[0];
    if (!(file=WFindFilename(a))) return 0;
    if (sscanf(file, "%s*%s", a, b) > 0) {
       if (b != ".c") file+=".c";
       WLoadAll(file, 1, flag);
       return 1;
    }
    WLoad(file, 1, flag);
    return 1;
}

int WClone(string what, int destr) {
    int ret, i;
    object env, obj, *objs;
    string filename;
    if (catch(obj = clone_object(what))) {
       write("Can not clone '"+what+"'.\n");
       return 0;
    }
    if (destr) {
       string what2;
       what2 = explode(what, ".")[0];
       write("Checking your inventory for " + what2 + ".\n");
       objs = all_inventory(this_player());
       for (i=0; i<sizeof(objs); i++) {
           filename = explode(object_name(objs[i]), "#")[0];
           if (filename == what2) {
              objs[i]->remove();
              write("Found and destructed.\n");
              break;
           }
       }
    }
    ret = ({int})obj->move(this_player(), M_SILENT);
    if (ret > ME_DESTRUCTED_SELF) {
       if (env = environment(this_player())) return 0;
       if (env == obj) return 0;
       ret = ({int})obj->move(env, M_SILENT);
       if (ret != ME_OK) { obj->remove(); return 0; }
    }
    write("Cloned '"+object_name(obj)+"'.\n");
    return 1;
}

int is_player(object who) { return ({int})who->QueryIsPlayer(); }

int WLoad(string file, int clone, int destr) {
  int i;
  object obj, *inv, vroom;
  string error;
  if (obj=find_object(file)) {
     if(catch(call_other(VOID, "???"))) {
        write("Error: cannot find "+VOID+" to rescue players.\n");
        return 1;
     } else vroom = find_object(VOID);
     if(inv=filter(all_inventory(obj), "is_player", this_object()))
        for(i=0; i<sizeof(inv); i++) inv[i]->move(vroom, M_SILENT);
     destruct(obj);
     write("Update and load: "+file+"\n");
  } else write("Load: "+file+"\n");
  if (error=catch(call_other(file, "???"))) {
     write("Error: "+error[1..]+"\n");
     if (vroom)
        tell_object(vroom, "*** Failed to load room. You are in the void!\n");
  } else if(inv) {
    obj=find_object(file);
    for(i=0; i<sizeof(inv); i++)
      if(inv[i]) inv[i]->move(obj,M_SILENT);
  }
  if (clone) {
     obj = find_object(file);
     if (obj) WClone(file, destr);
  }
  return 1;
}

int wfind(string str) {
  string *words;
  if (!str) { notify_fail("wfind <pattern> <dir>.\n"); return 0; }
  words = explode(str, " ");
  if (!words || sizeof(words) != 2) {
     notify_fail("wfind <pattern> <dir>.\n");
     return 0;
  }
  write(WFind(words[1], words[0]));
  return 1;
}

string WFind(string str, string pat) {
  mixed *ret;
  string *fnames;
  int i;
  string file, path, ausgabe;
  ausgabe = "";
  ret = ({mixed *})this_player()->parse_fname(str, 0);
  if (ret[3] == -1) return str+" does not exist.\n";
  path = ret[1];
  fnames = ({mixed})this_player()->get_flist(ret, 0);
  for (i=0; i<sizeof(fnames); i++) {
      file = fnames[i];
      if (({string})this_player()->is_dir(path+file) &&
          file!="." && file!="..") ausgabe += WFind(path+file+"/*", pat);
      if (pat == ({string})"/lib/string"->ladjcut(file, sizeof(pat)))
         ausgabe += "Found "+file+" in "+path+"\n";
  }
  return ausgabe;
}

int whead(string str) {
 string *words, ausgabe, file1, file2, lines;
 if (!str) {
    notify_fail("whead <headerfile> <file|dir|pattern>\n");
    return 0;
 }
 words = explode(str, " ");
 if (sizeof(words)<2) {
    notify_fail("whead <headerfile> <file|dir|pattern>\n");
    return 0;
 }
 file2 = words[sizeof(words)-1];
 words -= ({ file2 });
 file1 = implode(words, " ");
 if (!file1 || file1 == "") {
    notify_fail("whead <headerfile> <file|dir|pattern>\n");
    return 0;
 }
 if (file_size(file1) < 0) {
    notify_fail(file1 + " not found.\n");
    return 0;
 }
 lines = read_file(file1);
 if (sscanf(file2, "%~s*%~s") > 0) {
     ausgabe = WHeadAll(lines, file2);
     Out(ausgabe);
     return 1;
 }
 ausgabe = WHead(lines, file2);
 Out(ausgabe);
 write("WHead done.\n");
 return 1;
}

int wrepl(string str) {
 int inpt, flag, bup;
 string *words;
 string find, repl, file, ausgabe;
 if (!str) {
    notify_fail("wrepl [-l] [-b] [-i] <find> <replace> <file|dir|pattern>\n");
    return 0;
 }
 words = explode(str, " ");
 flag = 0;      bup = 0;     inpt = 0;
 if (member(words, "-l") > -1) {
     words -= ({"-l"});
     flag = 1;
 }
 if (member(words, "-b") > -1) {
    words -= ({"-b"});
    bup = 1;
 }
 if (member(words, "-i") > -1) {
    words -= ({"-i"});
    if (sizeof(words)<1) {
       notify_fail("wrepl [-l] [-b] [-i] <file|dir>\n");
       return 0;
    }
    inpt = 1;
    file = words[sizeof(words)-1];
    write("Find: ");
    input_to("wrepl2", 0, file, flag, bup);
    return 1;
 }
 if (!inpt && sizeof(words)<3) {
    notify_fail("wrepl [-l] [-b] [-i] <find> <replace> <file|dir>\n");
    return 0;
 }
 if (!inpt) {
    file = words[sizeof(words)-1];
    words -= ({ file });
    repl = words[sizeof(words)-1];
    words -= ({ repl });
    find = implode(words, " ");
 }
 if (sscanf(file, "%~s*%~s") > 0) {
     ausgabe = WReplAll(file, find, repl, bup);
     if (flag) Out(ausgabe); else write("WRepl done.\n");
     return 1;
  }
 ausgabe = WRepl(file, find, repl, bup);
 if (flag) Out(ausgabe); else write("WRepl done.\n");
 return 1;
}

int wrepl2(string in, string file, int flag, int bup) {
  write("Replace: ");
  input_to("wrepl3", 0, in, file, flag, bup);
  return 1;
}

int wrepl3(string in, string find, string file, int flag, int bup) {
 string ausgabe;
 if (sscanf(file, "%~s*%~s") > 0) {
     ausgabe = WReplAll(file, find, in, bup);
     if (flag) Out(ausgabe); else write("WRepl done.\n");
     return 1;
 }
 ausgabe = WRepl(file, find, in, bup);
 if (flag) Out(ausgabe); else write("WRepl done.\n");
 return 1;  
}

string WReplAll(string file, string find, string repl, int bup) {
  mixed *ret;
  string *fnames;
  int i;
  string fnam, path, ausgabe;
  ausgabe = "";
  ret = ({mixed *})this_player()->parse_fname(file, 0);
  if (ret[3] == -1) return file+" does not exist.\n";
  path = ret[1];
  fnames = ({mixed})this_player()->get_flist(ret, 0);
  for (i=0; i<sizeof(fnames); i++) {
      fnam = fnames[i];
      ausgabe += WRepl(path+fnam, find, repl, bup);
  }
  return ausgabe;
}

string WHeadAll(string file1, string file2) {
  mixed *ret;
  string *fnames;
  int i;
  string file, path, ausgabe;
  ausgabe = "";
  ret = ({mixed *})this_player()->parse_fname(file2, 0);
  if (ret[3] == -1) return file2 + " does not exist.\n";
  path = ret[1];
  fnames = ({mixed})this_player()->get_flist(ret, 0);
  for (i=0; i<sizeof(fnames); i++) {
      file = fnames[i];
      ausgabe += WHead(file1, path+file);
  }
  return ausgabe;
}

string WRmlAll(int num, string file1) {
  mixed *ret;
  string *fnames;
  int i;
  string file, path, ausgabe;
  ausgabe = "";
  ret = ({mixed *})this_player()->parse_fname(file1, 0);
  if (ret[3] == -1) return file1 + " does not exist.\n";
  path = ret[1];
  fnames = ({mixed})this_player()->get_flist(ret, 0);
  for (i=0; i<sizeof(fnames); i++) {
      file = fnames[i];
      ausgabe += WRml(num, path+file);
  }
  return ausgabe;
}

string WGrepAll(string str, string pat, int dirs) {
  mixed *ret;
  string *fnames;
  int i;
  string file, path, ausgabe;
  ausgabe = "";
  ret = ({mixed *})this_player()->parse_fname(str, 0);
  if (ret[3] == -1) return str+" does not exist.\n";
  path = ret[1];
  fnames = ({mixed})this_player()->get_flist(ret, 0);
  for (i=0; i<sizeof(fnames); i++) {
      file = fnames[i];
      if (({string})this_player()->is_dir(path+file) && dirs &&
          file!="." && file!="..") {
         write("Now searching for "+pat+" in "+path+file+"/*\n");
         ausgabe += WGrepAll(path+file+"/*", pat, dirs);
      }
      else ausgabe += WGrep(path+file, pat);
  }
  return ausgabe;
}

int WLoadAll(string str, int clone, int destr) {
  mixed *ret;
  string *fnames;
  int i;
  string file, path;
  ret = ({mixed *})this_player()->parse_fname(str, 0);
  if (ret[3] == -1) { write(str+" does not exist.\n"); return 1; }
  path = ret[1];
  fnames = ({mixed})this_player()->get_flist(ret, 0);
  for (i=0;  i<sizeof(fnames); i++) {
      file = fnames[i];
      WLoad(path + file, clone, destr);
  }
  return 1;
}
