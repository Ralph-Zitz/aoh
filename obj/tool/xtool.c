/*----------------------------------------------------------------------*/
/* The Xtool - Copyright 1992 by Hyp                                    */
/*                                                                      */
/* File: xtool.c                                                        */
/*----------------------------------------------------------------------*/

#pragma no_range_check

#include "xtool/xtool.h"
#include <driver/object_info.h>

nosave object  cloner;
nosave object  lastroom;
nosave object *lastinv;
nosave object *lastenv;
nosave object *netdead;
nosave object *snoopee;
nosave object *invisobjs;
nosave object  msgto;
nosave string *manpath;
nosave string  morefile;
nosave string *scriptline;
nosave string *history;
nosave int     moreflag;
nosave int     moreoffset;
nosave int     hbeats;
nosave int     aliaslevel;
nosave int     term;
nosave int     scriptsize;
nosave int     nostore;
nosave int     xlight;
nosave mapping variable;
int    morelines;
int    modi;
mapping aliases;

void initialize_globals() {
  lastroom=NULL;
  lastinv=({});
  lastenv=({});
  netdead=({});
  snoopee=({});
  invisobjs=({});
  msgto=NULL;
  manpath=({TOOL_PATH+"/",
    "/doc/lib/",
    "/doc/other/",
    "/doc/daemons/",
    "/doc/hook/",
    "/doc/examples/",
    "/doc/master/",
    "/doc/applied/",
    "/doc/internals/",
    "/doc/sefun/",
    "/doc/obj/",
    "/doc/driver/",
    "/doc/efun/",
    "/doc/intro/",
    "/doc/obsolete/",
    "/doc/efun.de/",
    "/doc/lfun/",
    "/doc/old/",
    "/doc/w/",
    "/doc/helpdir/",
    "/doc/LPC/",
    "/doc/std/",
    "/doc/concepts/",
    "/doc/prop/",
    ""});
  morefile=NULL;
  scriptline=NULL;
  history=allocate(MAX_HISTORY);
  moreflag=FALSE;
  moreoffset=1;
  hbeats=NULL;
  aliaslevel=NULL;
  term=NULL;
  scriptsize=NULL;
  nostore=FALSE;
  xlight=0;
  variable=([]);
  morelines=MORE_LINES;
#if MUD_NF || MUD_MG
  modi=(MODE_HEART|MODE_FIRST|MODE_PROTECT|MODE_SHORT);
#else
  modi=(MODE_PROTECT|MODE_SHORT);
#endif
  aliases=([]);
}

/*----------------------------------------------------------------------
 * check some security aspects
 */

int query_prevent_shadow(object ob) { return 1; }

int security() {
  object prev;

  if(prev=PREV) {
    if(!cloner) return TRUE;
#if MUD_NF || MUD_MG
    if(getuid(prev)==ROOTID) return TRUE;
    return getuid(prev)==getuid()&&geteuid(prev)==geteuid()&&cloner==RTP;
#endif
    return cloner==RTP;
  } else return cloner==NULL;
}

/*----------------------------------------------------------------------
 * own write function
 */

void Write(string str) {
  if(!cloner) efun::write(str);
  else efun::tell_object(cloner, str);
}

/*----------------------------------------------------------------------
 * own command function
 */

int Command(string str) {
  int i;
  nostore++;
  if(MODE(MODE_ECHO)) WLN("Doing: "+str);
#if MUD_NF || MUD_MG
  i=({int})cloner->command_me(str);
#endif
#if MUD_TA || MUD_BB || MUD_TM
  i=command(str, cloner);
#endif
  nostore--;
  return i;
}

/*----------------------------------------------------------------------
 * object searching
 */

object XFindObj(string str) {
  object obj, env;
  string *strs;
  int i, s;

  if(!str) return NULL;
  env=ENV(cloner);
  str=string_replace(str, "\\.", "");
  str=string_replace(str, "\\^", "");
  str=string_replace(str, "\\$", "");
  str=string_replace(str, "\\\\", "\\");
  s=sizeof(strs=strip_explode(str, "."));
  while(s--) {
    if(obj=FindObj(strs[i++], env)) env=obj;
    else break;
  }
  return obj;
}

object FindObj(string str, object env) {
  object obj, *inv;
  int num, e;

  if(!(str&&env)) return NULL;
  str=string_replace(str, "", ".");
  while(str[e++]=='^');
  str=RIGHT(str, --e);
  str=string_replace(str, "", "^");
  if(obj=VarToObj(str));
  else if(str[0]=='') str=string_replace(str, "", "$");
  else if(1==sscanf(str, "%d", num)&&(""+num==str)&&(inv=all_inventory(env))) {
    if(num>0&&num<=sizeof(inv)) {
      obj=inv[num-1];
    } else {
      WDLN("Specified object number out of range [1-"+sizeof(inv)+"]");
      return NULL;
    }
  }
  if(obj||(obj=present(str, env))||
     (obj=find_player(LOWER(str)))||
     (obj=find_living(str))||
     (obj=find_object(long_path(str)))) {
    while(e--) {
      if(!(obj=ENV(obj))) {
	W("Specified object has no environment [");
	while(e--) W("^");
	WDLN(str+"]");
        return NULL;
      }
    }
  } else WDLN("Specified object does not exist ["+str+"]");
  return obj;
}

/*----------------------------------------------------------------------
 * object variable handling
 */

object VarToObj(string str) {
  if(!(str&&str[0]=='$')) return NULL;
  switch(str) {
  case "$m":
  case "$me":
    return cloner;
  case "$h":
  case "$here":
    return ENV(cloner);
  default:
    return variable[RIGHT(str, 1)];
  }
  return NULL;
}

string VarToFile(string str) {
  return source_object_name(VarToObj(str));
}

string VarToPureFile(string str) {
  return pure_object_name(VarToObj(str));
}

/*----------------------------------------------------------------------
 * object description printing
 */

void PrintObj(object obj) {
  object item;
  string str;
  mixed m;
  int i;

  SECURE1();
  if(!obj) return;
  PrintShort("Short: ", obj);
  WLN("Long :");
#if MUD_NF
  if( (m=({mixed})obj->QueryIntLong())
   || (m=({mixed})obj->QueryLong())
    )
  {
    if (pointerp(m))
      str = "-- Look--\n"+m[0]+"-- Examine --\n"+m[1];
    else
      str = m;
  }
  else str="- no long description -\n";
  W(str);
#endif
#if MUD_MG
  if(str=(string)obj->Query(P_INT_LONG));
  else if(str=(string)obj->Query(P_LONG));
  else str="- no long description -\n";
  W(str);
#endif
#if MUD_TA || MUD_BB || MUD_TM
  if(str=(string)obj->long())
    W(str);
#endif
  FORALL(item, obj) {
    if(!i) WLN("Content:");
    PrintShort(ARIGHT(++i+". ", 4, " "), item);
  }
}

string ObjFile(object obj) {
  return "["+short_path(obj?object_name(obj):"<destructed>")+"]";
}

void PrintShort(string pre, object obj) {
  string str;

  SECURE1();
  if(!obj) return;
  if(MODE(MODE_SHORT)) {
#if MUD_NF
    if(!((str=({string})obj->QueryIntShort())||
	 (str=({string})obj->Short()))) {
      if(is_player(obj)) str=CRNAME(obj)+" (invisible)";
      else str="- no short description -";
    }
#endif
#if MUD_MG
    if(!((str=(string)obj->Query(P_INT_SHORT))||
	 (str=(string)obj->Query(P_SHORT)))) {
      if(is_player(obj)) str=CRNAME(obj)+" (invisible)";
      else str="- no short description -";
    }
#endif
#if MUD_TA || MUD_BB || MUD_TM
    if(!(str=(string)obj->short())) {
      if(is_player(obj)) str=CRNAME(obj)+" (invisible)";
      else str="- no short description -";
    }
#endif
    str=ALEFT(str+" ", 32, ".")+" ";
  } else str="";
  if(interactive(obj)) str+="i";
  else if(living(obj)) str+="l";
  else str+=".";
  str+=(shadow(obj, 0) ? "s" : ".");
  WLN(FIRST(pre+CAP(str)+" "+ObjFile(obj), 77));
}

void DeepPrintShort(object env, int indent, string pre) {
  int i;
  object item;

  SECURE1();
  if(!env) return;
  for(i=indent; i--; ) W("  ");
  i++;
  if(pre) W(pre);
  PrintShort("", env);
  FORALL(item, env)
    DeepPrintShort(item, indent+1, ARIGHT((++i)+". ", 4, " "));
}

#if MUD_NF
void props_to_tmpfile(object obj) {
  mapping props;
  string  *index, data;
  int     i;
  props = ({mapping})obj->QueryProperties()||([]);
  index = sort_array(m_indices(props), #'> /*'*/);
  data = "";
  for (i = 0; i < sizeof(index); i++)
     data += ALEFT(index[i]+" ", 18, ".")+
	     " = "+mixed_to_string(props[index[i]], MAX_RECURSION)+"\n";
  write_file(TMP_FILE, data);
}
#elif MUD_MG
void filter_hardcoded(string func, object obj) {
  if(func[0..4] == "Query") {
    if(func != "QueryAttr" && func != "QueryProp" && func != "QueryProperties") {
      write_file(TMP_FILE, ALEFT(func[5..]+" ", 18, ".")+
		 " = "+mixed_to_string(call_other(obj, func), MAX_RECURSION)+
		 "\n");
    }
  }
}
#else
void filter_hardcoded(string func, object obj) {
  if(func[0..5] == "query_") {
    write_file(TMP_FILE, ALEFT(func[6..]+" ", 18, ".")+
	       " = "+mixed_to_string(call_other(obj, func), MAX_RECURSION)+
	       "\n");
  }
}
#endif

void DumpProperties(object obj) {
  SECURE1();
  if(!obj) return;
  else if(file_size(TMP_FILE)>0) rm(TMP_FILE);
#if !MUD_NF
  filter(functionlist(obj), "filter_hardcoded", TO, obj);
#else
  props_to_tmpfile(obj);
#endif
  XMoreFile(TMP_FILE, FALSE);
}

/*----------------------------------------------------------------------
 * moving objects
 */

int MoveObj(object obj1, object obj2, int silent) {
  int err;
  object oldenv;

  SECURE2(FALSE);
  if(!(obj1&&obj2)) return FALSE;
#if MUD_NF
  oldenv=ENV(obj1);
  err=({int})obj1->move(obj2, M_SILENT, 0);
  if(!silent) {
    switch(err) {
    case ME_NO_MOVE:
      WDLN("Object returned ME_NO_MOVE");
      if(function_exists("move", obj1))
	WDLN("Object has a lfun move() defined");
      return FALSE;
    case ME_OK:
      WDLN("Object returned ME_OK");
      if(ENV(obj1) == obj2) {
	PrintShort("Move: ", obj1);
	PrintShort("into: ", obj2);
	return TRUE;
      } else
	WDLN("Object has not been moved");
      return FALSE;
    case ME_DESTRUCTED_SELF:
      WDLN("Object returned ME_DESTRUCTED_SELF");
      if(obj1)
	WDLN("Object is still valid");
      return FALSE;
    case ME_NO_LEAVE:
      WDLN("Object returned ME_NO_LEAVE");
      if(oldenv && oldenv != ENV(obj1))
	WDLN("Object has been moved");
      return FALSE;
    case ME_NO_ENTER:
      WDLN("Object returned ME_NO_ENTER");
      if(oldenv && oldenv != ENV(obj1))
	WDLN("Object has been moved");
      return FALSE;
    case ME_TOO_HEAVY:
      WDLN("Object returned ME_TOO_HEAVY");
      if(oldenv && oldenv != ENV(obj1))
	WDLN("Object has been moved");
      return FALSE;
    default:
      WDLN("Object returned unknown return value");
      return FALSE;
    }
  }
#endif
#if MUD_MG
  oldenv=ENV(obj1);
  err=(int)obj1->move(obj2, M_SILENT, 0);
  if(!silent) {
    switch(err) {
    case ME_PLAYER:
      WDLN("Object returned ME_PLAYER");
      if(oldenv && oldenv != ENV(obj1))
	WDLN("Object has been moved");
      return FALSE;
    case ME_TOO_HEAVY:
      WDLN("Object returned ME_TOO_HEAVY");
      if(oldenv && oldenv != ENV(obj1))
	WDLN("Object has been moved");
      return FALSE;
    case ME_CANT_TPORT_IN:
      WDLN("Object returned ME_CANT_TPORT_IN");
      if(oldenv && oldenv != ENV(obj1))
	WDLN("Object has been moved");
      return FALSE;
    case ME_CANT_TPORT_OUT:
      WDLN("Object returned ME_CANT_TPORT_OUT");
      if(oldenv && oldenv != ENV(obj1))
	WDLN("Object has been moved");
      return FALSE;
    default:
      WDLN("Object returned unknown return value");
      return FALSE;
    }
  }
#endif
#if MUD_TA || MUD_BB || MUD_TM
  move_object(obj1, obj2);
  if(!silent) {
    PrintShort("Move: ", obj1);
    PrintShort("into: ", obj2);
  }
#endif
  return TRUE;
}

/*----------------------------------------------------------------------
 * save destructing of objects
 */

void Destruct(object obj) {
  if(!obj) return;
  if (getuid(this_object()) == "joan" && query_once_interactive(obj))
      return;
#if MUD_NF || MUD_MG
  catch(obj->remove());
#endif
  if(obj) destruct(obj);
}

void DeepClean(object obj) {
  if(!obj) return;
  filter(filter(deep_inventory(obj), "is_not_player", TO),
	       "Destruct", TO);
  if(is_not_player(obj)) Destruct(obj);
}

/*----------------------------------------------------------------------
 * Show the inheritance tree of objects
 */

object *SubNodes(object obj) {
  int s;
  object *objs;
  string *inlist;

  if(!obj) return NULL;
  inlist=inherit_list(obj);
  s=sizeof(inlist);
  objs=({});
  while(s-->1) objs=({find_object(inlist[s])})+objs;
  return objs;
}

void Inheritance(object obj, string func, string pre) {
  int i, s;
  object *ln;
  string str;

  if(!obj) return;
  str=pre+" "+ObjFile(obj);
  if(func) {
    str=ALEFT(str+" ", 50, ".");
    if(function_exists(func, obj)==object_name(obj))
      str+=ARIGHT(" "+func, 19, ".");
    else str+=ARIGHT("", 19, ".");
  }
  WLN(str);
  ln=SubNodes(obj);
  for(i=0; i<sizeof(ln); i++) ln=ln-SubNodes(ln[i]);
  s=sizeof(ln);
  for(i=0; i<s; i++) Inheritance(ln[i], func, pre+".....");
}

/*----------------------------------------------------------------------
 * file name handling
 */

string XFile(string file) {
  if(file) {
    switch(file[0]) {
    case '@':
      return source_object_name(XFindObj(RIGHT(file, 1)));
    case '$':
      return source_object_name(XFindObj(file));
    default:
      return long_path(file);
    }
  }
  return NULL;
}

string XFindFile(string file) {
  if(file=XFile(file)) {
    if(file_size(file)>=0) return file;
    if(LAST(file, 3)!=".c"&&file_size(file+".c")>0) return file+".c";
  }
  WDLN("File not found or readable ["+short_path(file)+"]");
  return NULL;
}

/*----------------------------------------------------------------------
 * file printing, searching and executing
 */

void XMoreFile(string file, int flag) {
  SECURE1();
  if(!file) return;
#if MUD_NF
  term=({string})cloner->QueryTTY()!="dumb";
#endif
#if MUD_MG
  term=(string)cloner->Query(P_TTY)!="dumb";
#endif
#if MUD_TA || MUD_BB || MUD_TM
  term=0;
#endif
  if(file_size(morefile=long_path(file))>0) MoreFile(NULL);
  else if(flag) WDLN("Cannot read file");
}

void MoreFile(string str) {
  int i;
  string l, r;

  SECURE1();
  if(str) {
//    if(term) W("\e[1m");
    if(str == "") str = " ";
    switch(str[0]) {
    case 'q':
    case 'x':
      moreflag=FALSE;
      moreoffset=1;
      if(morefile==TMP_FILE) rm(morefile);
      return NULL;
      break;
    case 'P':
    case 'U':
      moreflag=FALSE;
      moreoffset=moreoffset-morelines;
    case 'p':
    case 'u':
      moreoffset=moreoffset-2*morelines;
      break;
    case 'D':
    case 'N':
      moreoffset+=morelines;
    case 0:   /* RETURN */
    case ' ':
    case 'd':
    case 'n':
      if(moreflag) {
	moreflag=FALSE;
	moreoffset=1;
	if(morefile==TMP_FILE) rm(morefile);
	return;
      }
      break;
    case '/':
      i=moreoffset-morelines;
      r=RIGHT(str, 1);
      if(!regexp(({"dummy"}), r))
	WDLN("Bad regular expression");
      else while((l=read_file(morefile, i++, 1))&&!sizeof(regexp(({l}), r)));
      if(l) {
	WLN("*** Skipping ...");
	moreoffset=i-1;
      } else {
	WLN("*** Pattern not found");
	moreoffset-=morelines;
      }
      break;
    case '0'..'9':
      sscanf(str, "%d", i);
      moreoffset=i;
      break;
    }
  } else {
    moreoffset=1;
    moreflag=FALSE;
  }
  if(moreoffset<1) moreoffset=1;
  if(CatFile()) {
    W("*** More: q,u,U,d,D,/<regexp>,<line> ["+(moreoffset-1)+"] *** ");
    if(term) W("\e[39;49m\e[0m");
//    if(term) W("\e[39;49m\e[0;10m");
    cloner->print_eor();
  } else {
    W("*** More: q,u,U,d,D,/<regexp>,<line> ["+(moreoffset-1)+"=EOF] *** ");
    if(term) W("\e[39;49m\e[0m");
//    if(term) W("\e[39;49m\e[0;10m");
    cloner->print_eor();
    moreflag=TRUE;
  }
  input_to("MoreFile");
  return;
}

int CatFile() {
  int end;
  string l;

  end=moreoffset+morelines;
  while(moreoffset<end) {
    if(l=read_file(morefile, moreoffset, 1)) {
      moreoffset++;
      W(l);
    } else return FALSE;
  }
  if(read_file(morefile, moreoffset+1, 1)) return TRUE;
  else return FALSE;
}

int XGrepFile(string pat, string file) {
  int i, f;
  string tfile, *ts, t;

  SECURE2(FALSE);
  if(!(pat&&file)) return FALSE;
  tfile=TMP_FILE;
  for(i=1; t=read_file(file, i, 1000); i+=1000) {
    if(sizeof(ts=regexp(strip_explode(t, "\n"), pat))) {
      if(!f++) write_file(tfile, "*** File: "+file+" ***\n");
      write_file(tfile, implode(ts, "\n")+"\n");
    }
  }
  return TRUE;
}

void XExecFile(int line) {
  int i;

  if(!scriptline) return;
  for(i=line; i<scriptsize&&i<line+EXEC_LINES; i++) {
    if(!scriptline[i]) continue;
    if(!Command(scriptline[i])) {
      scriptline=NULL;
      return;
    }
  }
  if(i<scriptsize) call_out("XExecFile", EXEC_TIME, i);
  else scriptline=NULL;
}

void XmtpScript(string dir, string file, string opt) {
  int s, t;
  string *files;

  s=sizeof(files=get_dir(dir+"/*"));
  while(s--) {
    t=sizeof(files[s])-1;
    if(files[s] == ".." || files[s] == "." || files[s][t] == '~' ||
       (files[s][0] == '#' && files[s][t] == '#'))
      continue;
    if(file_size(dir+"/"+files[s])==-2) {
      write_file(file, "mkdir "+files[s]+" ; cd "+files[s]+"\n");
      XmtpScript(dir+"/"+files[s], file, opt);
      write_file(file, "cd ..\n");
    } else
      write_file(file, "mtp -r "+opt+" "+dir+"/"+files[s]+"\n");
  }
}

/*----------------------------------------------------------------------
 * player properties handling
 */

string PlayerIdle(object obj) {
  string str;
  int i;

  if(!obj) return NULL;
  if(!interactive(obj))
    str="<dsc>";
  else if((i=query_idle(obj))>=60) {
    str=ARIGHT(""+(i/3600), 2, "0");
    i-=(i/3600)*3600;
    str+="'"+ARIGHT(""+(i/60), 2, "0");
  } else str=".....";
  return str;
}

string PlayerAge(object obj) {
  string str;
  int i;

  if(!obj) return NULL;
#if MUD_NF
  i=({int})obj->QueryAge();
#endif
#if MUD_MG || MUD_TA || MUD_BB || MUD_TM
  i=(int)obj->query_age();
#endif
  str=" "+ARIGHT(""+(i/43200), 3, ".");
  i-=(i/43200)*43200;
  return str+":"+ARIGHT(""+(i/1800), 2, "0");
}

#if MUD_NF
string PlayerJobs(object obj, int flag) {
  string str;
  mixed *data, *userinfo;
  int i;

  if(!obj) return NULL;
  str=(flag) ? ALEFT(CRNAME(obj)+" ", 12, ".")+" " : "";
  if (IS_WIZARD(TP))
    str+=ARIGHT(LEVEL(obj)+"", 2, " ")+" ("+(query_user_tag(obj, 1)||"Player")+")";
  else
    str+=query_user_tag(obj, 1)||"Player";
  userinfo=({mixed *})MASTER->find_userinfo(getuid(obj));
  data=userinfo && userinfo[3];
  if (sizeof(data)) {
    for (i=0; i < sizeof(data); i++) {
      str+=", ";
      switch (data[i][0]) {
        case '*': str+=(({int})obj->QueryGender() < 2?"Lord":"Lady")+" of "+capitalize(data[i][1..]); break;
        case '+': str+="Vice of "+capitalize(data[i][1..]); break;
        default : str+="Member of "+capitalize(data[i]); break;
      }
    }
  }
  return str;
}
#endif

string PlayerWho(object obj) {
  object tmp;
  string str;

#if MUD_OSB
  str=ARIGHT(query_user_tag(obj), 4, " ");
#else
#if MUD_NF
  if (IS_WIZARD(TP))
    str=ARIGHT(""+LEVEL(obj),  4, " ");
  else
    str=ALEFT(LEVEL(obj) ? "Wiz" : "",  4, " ");
#else
  str=ARIGHT(""+LEVEL(obj),  3, " ");
#endif
#endif
  str+=ALEFT(" "+CRNAME(obj)+" ", 12, ".");
  str+=PlayerAge(obj);
#if MUD_NF
  str+=(({int})obj->QueryGender()==1 ? " m " : " f ");
  str+=(({int})obj->QueryFrog())    ? "f" : ".";
  str+=(({int})obj->QueryGhost())   ? "g" : ".";
  str+=(({int})obj->QueryInvis())   ? "i" : ".";
#endif
#if MUD_MG
  str+=((int)obj->Query(P_GENDER)==1 ? " m " : " f ");
  str+=(obj->Query(P_FROG))  ? "f" : ".";
  str+=(obj->Query(P_GHOST)) ? "g" : ".";
  str+=(obj->Query(P_INVIS)) ? "i" : ".";
#endif
#if MUD_TA || MUD_BB || MUD_TM
  str+=(" "+SUBSTR(""+(string)obj->query_gender_string(), 0, 0)+" ");
  str+=(obj->query_frog())  ? "f" : ".";
  str+=(obj->query_ghost()) ? "g" : ".";
  str+=(obj->short()) ? "." : "i";
#endif
  str+=(query_editing(obj)||query_input_pending(obj) ? "e" : ".");
  str+=" "+PlayerIdle(obj)+" ";
  str+=(tmp=ENV(obj)) ? ObjFile(tmp) : "- fabric of space -";
  return str;
}

string PlayerMail(object obj, int flag) {
  string pre;

  pre=(flag) ? ALEFT(CRNAME(obj)+" ", 12, ".")+" " : "";
#if MUD_NF
  return pre+"mail: "+({string})obj->QueryMailaddr();
#endif
#if MUD_MG
  return pre+"mail: "+obj->query_mailaddr();
#endif
#if MUD_TA || MUD_BB || MUD_TM
  return pre+"mail: "+obj->query_mailaddr();
#endif
}

string PlayerIP(object obj, int flag) {
  string pre;

  pre=(flag) ? ALEFT(CRNAME(obj)+" ", 12, ".")+" " : "";
  return pre+"host: "+query_ip_name(obj)+" ("+query_ip_number(obj)+")";
}

#if MUD_NF
string PlayerRace(object obj, int flag) {
  string tmp, pre;

  pre=(flag) ? ALEFT(CRNAME(obj)+" ", 12, ".")+" " : "";
  pre=pre+"race: "+ALEFT(({string})obj->QueryRace()+" ", 10, ".")+
    " guild: ";
  tmp=({string})obj->QueryGuild();
  return tmp ? pre+tmp : pre+"- none -";
}
#endif

#if MUD_MG
string PlayerRace(object obj, int flag) {
  string tmp, pre;

  pre=(flag) ? ALEFT(CRNAME(obj)+" ", 12, ".")+" " : "";
  pre=pre+"race: "+ALEFT(obj->Query(P_RACE)+" ", 10, ".")+
    " guild: ";
  tmp=(string)obj->Query(P_GUILD);
  return tmp ? pre+tmp : pre+"- none -";
}
#endif

string PlayerDomain(object obj, int flag) {
  int i, s;
  mixed *uinfo;
  string *domains, pre, title;

  pre=(flag) ? ALEFT(CRNAME(obj)+" ", 12, ".")+" " : "";
  pre+="domain: ";
#if MUD_NF || MUD_MG
  title=({int})obj->Query(P_GENDER)==1?"Lord":"Lady";
  uinfo=({mixed *})MASTER->find_userinfo(getuid(obj));
  if(uinfo&&(domains=uinfo[USER_DOMAIN])&&(s=sizeof(domains))) {
    for(i=0; i<s; i++) {
      switch(domains[i][0]) {
      case '*':
	pre=pre+title+" of "+CAP(RIGHT(domains[i], 1));
	break;
      case '+':
	pre=pre+"Vice "+title+" of "+CAP(RIGHT(domains[i], 1));
	break;
      default:
	pre+=CAP(domains[i]);
	break;
      }
      if(i<s-1) pre+=", ";
    }
  }
#endif
#if MUD_BB || MUD_TM
  domains=(string*)obj->query_domains();
  if(!sizeof(domains)) return pre+"-";
  pre+=implode(map(domains, "cap_string"), ", ");
#endif
  return pre;
}

string PlayerStats(object obj, int flag) {
  string pre;

  pre=(flag) ? ALEFT(CRNAME(obj)+" ", 12, ".")+" " : "";
#if MUD_NF
  pre+="hp="+ARIGHT(({int})obj->QueryHP(), 3, "0");
  pre+="/"+ARIGHT(({int})obj->QueryMaxHP(), 3, "0");
  pre+=" sp="+ARIGHT(({int})obj->QuerySP(), 3, "0");
  pre+="/"+ARIGHT(({int})obj->QueryMaxSP(), 3, "0");
  pre+=" food="+ARIGHT(({int})obj->QueryFood(), 3, "0");
  pre+="/"+ARIGHT(({int})obj->QueryMaxFood(), 3, "0");
  pre+=" drink="+ARIGHT(({int})obj->QueryDrink(), 3, "0");
  pre+="/"+ARIGHT(({int})obj->QueryMaxDrink(), 3, "0");
  pre+=" exps="+({int})obj->QueryXP();
#endif
#if MUD_MG
  pre+="hp="+ARIGHT(obj->Query(P_HP), 3, "0");
  pre+="/"+ARIGHT(obj->Query(P_MAX_HP), 3, "0");
  pre+=" sp="+ARIGHT(obj->Query(P_SP), 3, "0");
  pre+="/"+ARIGHT(obj->Query(P_MAX_SP), 3, "0");
  pre+=" food="+ARIGHT(obj->Query(P_FOOD), 3, "0");
  pre+="/"+ARIGHT(obj->Query(P_MAX_FOOD), 3, "0");
  pre+=" drink="+ARIGHT(obj->Query(P_DRINK), 3, "0");
  pre+="/"+ARIGHT(obj->Query(P_MAX_DRINK), 3, "0");
  pre+=" exps="+obj->Query(P_XP);
#endif
#if MUD_TA || MUD_BB || MUD_TM
  pre+="hp="+ARIGHT(obj->query_hp(), 3, "0");
  pre+="/"+ARIGHT(obj->query_max_hp(), 3, "0");
  pre+=" sp="+ARIGHT(obj->query_sp(), 3, "0");
  pre+="/"+ARIGHT(obj->query_max_sp(), 3, "0");
  pre+=" str="+ARIGHT(obj->query_str(), 3, "0");
  pre+=" con="+ARIGHT(obj->query_con(), 3, "0");
  pre+=" dex="+ARIGHT(obj->query_dex(), 3, "0");
  pre+=" int="+ARIGHT(obj->query_int(), 3, "0");
  pre+=" exps="+obj->query_exp();
#endif
  return pre;
}

string PlayerSnoop(object obj, int flag) {
  string pre;
  object victim;

  pre=(flag) ? ALEFT(CRNAME(obj)+" ", 12, ".")+" " : "";
  pre=pre+"is snooped by: ";
  if(victim=query_snoop(obj))
    pre+=ARIGHT(" "+CRNAME(victim), 12, ".");
  else
    pre+="............";
  return pre;
}

/*----------------------------------------------------------------------
 * msg input to objects
 */

void XMsgSay(string str) {
  if(str=="."||str=="**") {
    WLN("[End of message]");
    say("[End of message]\n");
  } else {
    say(str+"\n");
    input_to("XMsgSay");
  }
}

void XMsgTell(string str) {
  if(str=="."||str=="**") {
    WLN("[End of message]");
    efun::tell_object(msgto, "[End of message]\n");
  } else {
    efun::tell_object(msgto, str+"\n");
    input_to("XMsgTell");
  }
}

void XMsgShout(string str) {
  if(str=="."||str=="**") {
    WLN("[End of message]");
    shout("[End of message]\n");
  } else {
    shout(str+"\n");
    input_to("XMsgShout");
  }
}

/*----------------------------------------------------------------------
 * own object moving
 */

varargs int move(mixed dest) {
  move_object(TO, cloner?cloner:dest);
  return TRUE;
}

#if MUD_TA || MUD_BB || MUD_TM
int get() {
  return 1;
}

int drop() {
  return 1;
}
#endif

#if MUD_NF
int QueryNoGet() {
  return 0;
}

int QueryNoDrop() {
  return 1;
}
#endif

/*----------------------------------------------------------------------
 * object id
 */

int id(string str) {
  if(!security()&&MODE(MODE_SCANCHK)&&RTP)
    WDLN(CRNAME(RTP)+" scanned you (id) ["+query_verb()+"] "+
	 (PREV ? ObjFile(PREV) : "[destructed object]"));
  return LOWER(str)==LOWER(TOOL_NAME);
}

/*----------------------------------------------------------------------
 * short and long description
 */

varargs string QueryShort() {
  if(cloner) {
    if((!security())&&MODE(MODE_SCANCHK))
      WDLN(CRNAME(RTP)+" scanned you (short) ["+query_verb()+"] "+
	   (PREV ? ObjFile(PREV) : "[destructed object]"));
    return add_gen(CRNAME(cloner))+" "+TOOL_TITLE+" ["+
      SUBSTR(ctime(time()), 11, 18)+"]";
  }
  return TOOL_TITLE;
}

varargs string QueryLong() {
  if(cloner&&!security()&&MODE(MODE_SCANCHK)) {
    WDLN(CRNAME(RTP)+" scanned you (long) ["+query_verb()+"] "+
	 (PREV ? ObjFile(PREV) : "[destructed object]"));
  }
  return "Just a simple wizard tool created by Hyp. "+
#ifdef MUD_OSB
    "(AoH version).\n"+
#else
#ifdef MUD_NF
    "(Nightfall version).\n"+
#endif
#endif
#ifdef MUD_MG
    "(MorgenGrauen version).\n"+
#endif
#ifdef MUD_TA
    "(TappMUD version).\n"+
#endif
#ifdef MUD_BB
    "(Bubba version).\n"+
#endif
#ifdef MUD_TM
    "(TubMUD version).\n"+
#endif
    "Gamedriver patchlevel: "+__VERSION__+" master object: "+__MASTER_OBJECT__+
    "\n\nDo 'xhelp' for more information.\n";
}

#if MUD_NF
string QueryHelpMsg() {
  return
"This is the XTool "+TOOL_VERSION
                    +", a powerful tool for wizards created by Hyp.\n"
"Do 'xhelp' for more information.\n"
  ;
}
#endif

#if MUD_MG
string _query_short() {
  return QueryShort();
}

string _query_long() {
  return QueryLong();
}
#endif

#if MUD_NF
varargs string Short() {
  return QueryShort();
}

varargs string InvShort() {
  return Short() || "";
}

varargs string Long() {
  return QueryLong();
}

varargs string ExaLong() {
  return QueryLong();
}
#endif

#if MUD_TA || MUD_BB || MUD_TM
string short() {
  return QueryShort();
}

string long() {
  return QueryLong();
}
#endif

/*----------------------------------------------------------------------
 * light stuff
 */

#if MUD_NF
int QueryLight() {
  return xlight;
}

int SetLight(int x) {
  return xlight;
}
#endif

#if MUD_MG
int _query_light() {
  return xlight;
}

int _set_light(int x) {
  return xlight;
}
#endif

#if MUD_TA || MUD_BB || MUD_TM
int query_light() {
  return xlight;
}

int set_light(int x) {
  return xlight;
}
#endif

/*----------------------------------------------------------------------
 * Autoloading
 */

mixed *QueryAutoObject() {
  return AUTOLOAD_ARGS;
}

void SetAutoObject(mixed *args) {
  WLN(TOOL_TITLE+" ...");
  if(!pointerp(args));
  else if(sizeof(args)!=4);
  else if(!stringp(args[0]));
  else if(!intp(args[1]));
  else if(!intp(args[2]));
  else if(!mappingp(args[3]));
  else {
    if(args[0]!=TOOL_INTERNAL) {
      WLN("*****************************");
      WLN("***      NEW EDITION      ***");
      WLN("***  do 'xtool news' for  ***");
      WLN("***   more information    ***");
      WLN("*****************************");
    }
    modi=args[1];
    morelines=args[2];
    aliases=args[3];
    return;
  }
  W("(bad autoload, using default)\n");
}

#if MUD_MG
mixed *_query_autoloadobj() {
  return QueryAutoObject();
}

void _set_autloadobj(mixed *args) {
  SetAutoObject(args);
}
#endif

#if MUD_TA || MUD_BB || MUD_TM
mixed *query_auto_load() {
  return ({source_object_name(TO)})+QueryAutoObject();
}

void init_arg(mixed *args) {
  SetAutoObject(args);
}
#endif

/*----------------------------------------------------------------------
 * creation, updating and initialization stuff
 */

void update_tool(mixed *args, object obj) {
  SECURE1();
  if(!(obj&&args)) return;
  Destruct(PREV);
  SetAutoObject(args);
  move(obj);
}

#if MUD_NF || MUD_MG
void create() {
#if !MUD_NF
  base::create();
#elif MUD_OSB
  thing::create();
#endif
  initialize_globals();
  if(member(object_name(), '#')<0)
    return;
  if(!cloner&&!((cloner=TP)||(cloner=ENV(TO)))&&!interactive(cloner))
    destruct(TO);
  set_heart_beat(MODE(MODE_HEART));
}
#if MUD_NF
int clean_up (int refs) {
  if (ENV(TO) && query_once_interactive(ENV(TO)))
    return 1;
  if (refs >= 1)
    return 1;
  return thing::clean_up(refs);
  //return base::clean_up(refs);
}
#endif
#endif

#if MUD_TA || MUD_BB || MUD_TM
void reset(int arg) {
  object obj;

  if(arg || member(object_name(), '#')<0)
    return;
  initialize_globals();
  if(!cloner&&!((cloner=TP)||(cloner=ENV(TO)))&&!interactive(cloner))
    destruct(TO);
  set_heart_beat(MODE(MODE_HEART));
}
#endif

void init() {
  object first;

  if(member(object_name(), '#')<0) return;
  first=first_inventory(ENV(TO));
  if(MODE(MODE_PROTECT)&&is_player(first)) {
    WDLN(CRNAME(first)+" tried to move into your inventory");
    tell_object(first, "You cannot move yourself into "+
		CRNAME(cloner)+"'s inventory.\n");
    WDLN("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
    write(first);write(ENV(cloner));
    MoveObj(first, ENV(cloner), FALSE);
    return;
  } else if(MODE(MODE_FIRST)&&first!=TO) move(cloner);
  else actions();
}

void actions() {
  add_action("inventory", "i");
  add_action("Xalias"   , "xal", 1);
  add_action("Xcall"    , "xcall");
  add_action("Xcallouts", "xcallo", 1);
  add_action("Xcindent" , "xci", 1);
  add_action("Xclean"   , "xcle", 1);
  add_action("Xclone"   , "xclo", 1);
  add_action("Xcmds"    , "xcm", 1);
  add_action("Xdate"    , "xda", 1);
  add_action("Xdbg"     , "xdb", 1);
  add_action("Xdclean"  , "xdc", 1);
  add_action("Xddes"    , "xdd", 1);
  add_action("Xdes"     , "xde", 1);
  add_action("Xdlook"   , "xdl", 1);
  add_action("Xdo"      , "xdo");
  add_action("Xdupdate" , "xdu", 1);
  add_action("Xeval"    , "xev", 1);
  add_action("Xforall"  , "xfo", 1);
  add_action("Xgoto"    , "xgo", 1);
  add_action("Xgrep"    , "xgr", 1);
  add_action("Xhbeats"  , "xhb", 1);
  add_action("Xhelp"    , "xhe", 1);
#if MUD_NF || MUD_MG
  add_action("Xids"     , "xid", 1);
#endif
  add_action("Xinfo"    , "xinf", 1);
  add_action("Xinherit" , "xinh", 1);
  add_action("Xlight"   , "xli", 1);
  add_action("Xload"    , "xloa", 1);
  add_action("Xlook"    , "xloo", 1);
  add_action("Xlpc"     , "xlp", 1);
  add_action("Xman"     , "xma", 1);
  add_action("Xmore"    , "xmor", 1);
  add_action("Xmove"    , "xmov", 1);
  add_action("Xmsg"     , "xms", 1);
  add_action("Xmtp"     , "xmt", 1);
  add_action("Xprof"    , "xprof", 1);
  add_action("Xprops"   , "xprop", 1);
  add_action("Xquit"    , "xqu", 1);
#if MUD_NF
  add_action("Xrevive"  , "xre", 1);
#endif
  add_action("Xscan"    , "xsc", 1);
  add_action("Xset"     , "xse", 1);
  add_action("Xsh"      , "xsh");
  add_action("Xstop"    , "xst", 1);
  add_action("Xtool"    , "xto", 1);
  add_action("Xtrace"   , "xtrac", 1);
  add_action("Xtrans"   , "xtran", 1);
  add_action("Xunalias" , "xun", 1);
  add_action("Xupdate"  , "xup", 1);
  add_action("Xwho"     , "xwh", 1);
  add_action("CommandScan", "", 1);
}

/*----------------------------------------------------------------------
 * the heart beat of the tool including the checking stuff
 */

#if MUD_NF
void PlayerNetdead (int arg) {
  set_heart_beat(!arg && MODE(MODE_HEART));
}
#endif

void heart_beat() {
  int t;
  object *inv, *env, *t2;
//  object *inv, *env, *t1, *t2;

  if(!cloner) return;
#if MUD_NF
  if (({int})cloner->QueryNetdead()) /* This will also stop the heartbeat in case */
    return;
#endif
  hbeats++;
  inv=all_inventory(cloner);
  if (ENV(cloner))
    env=all_inventory(ENV(cloner));
  else
    env=({});
  t=sizeof(t2=env-lastenv);
  if(MODE(MODE_FIRST)&&((lastroom!=ENV(cloner))||
			(first_inventory(cloner)!=TO)||t))
    move(cloner);
//  if(MODE(MODE_INVCHECK)) filter(t1, "write_newinvobj", TO);
  if(MODE(MODE_ENVCHECK)) filter(t2, "write_newenvobj", TO);
  if(MODE(MODE_NDCHECK))  NetDeadCheck(TRUE);
  if(MODE(MODE_VARCHECK)) VarCheck(TRUE);
  if(MODE(MODE_SNOOPCHK)) SnoopCheck();
  if(MODE(MODE_INVISCHK)) InvisCheck();
  lastinv=inv;
  lastenv=env;
  lastroom=ENV(cloner);
}

void VarCheck(int show) {
  int i, s;

  mixed indi = m_indices(variable);
  for(i=0, s=sizeof(indi); i<s; i++) {
    if(variable[indi[i]]) continue;
    if(show) WDLN("*** Variable $"+variable[indi[i]]+" has been destructed");
    m_delete(variable, indi[i]);
    i--;s--;
  }
}

void NetDeadCheck(int show) {
  object *old;

  old=netdead;
  netdead=filter(all_inventory(ENV(cloner)), "is_netdead", TO);
  if(show) {
    filter(netdead-old, "write_netdead", TO);
    filter(filter(old, "is_alive", TO), "write_alive", TO);
  }
}

void SnoopCheck() {
  object *old;

  old=snoopee;
  snoopee=filter(all_inventory(ENV(cloner)), "is_snoopee", TO);
  filter(snoopee-old, "write_snoopee", TO);
  filter(filter(old, "is_nosnoop", TO), "write_nosnoop", TO);
}

void InvisCheck() {
  object *old, *objs, *lvngs, *actvs;

  old=invisobjs;
  invisobjs=filter(all_inventory(ENV(cloner)), "is_invis", TO);
  objs=invisobjs-old;
  objs -=(lvngs=filter(objs , "is_living", TO));
  lvngs-=(actvs=filter(lvngs, "is_player", TO));
  filter(objs , "write_invisobj", TO);
  filter(lvngs, "write_invislvg", TO);
  filter(actvs, "write_invisply", TO);

  objs=filter(old, "is_visib", TO);
  objs -=(lvngs=filter(objs , "is_living", TO));
  lvngs-=(actvs=filter(lvngs, "is_player", TO));
  filter(objs , "write_visibobj", TO);
  filter(lvngs, "write_visiblvg", TO);
  filter(actvs, "write_visibply", TO);
}

int write_newinvobj(object obj) {
  if(obj) WDLN("*** New object in inventory "+ObjFile(obj));
  return 1;
}
int write_newenvobj(object obj) {
  if(obj) WDLN("*** New object in environment "+ObjFile(obj));
  return 1;
}
int write_netdead(object who) {
  if(who) WDLN("*** "+CRNAME(who)+" is netdead");
  return 1;
}
int write_alive(object who) {
  if(who) WDLN("*** "+CRNAME(who)+" becomes alive");
  return 1;
}
int write_snoopee(object who) {
  object snooper;

  if(who&&(snooper=query_snoop(who)))
    WDLN("*** "+CRNAME(who)+" is snooped by "+CRNAME(snooper));
    return 1;
  }
int write_nosnoop(object who) {
  if(who) WDLN("*** "+CRNAME(who)+" isn't snooped anymore");
  return 1;
}
int write_invisobj(object who) {
  if(who) WDLN("*** Invisible object detected "+ObjFile(who));
  return 1;
}
int write_invislvg(object who) {
  if(who) WDLN("*** Invisible living detected "+ObjFile(who));
  return 1;
}
int write_invisply(object who) {
  if(who) WDLN("*** "+CRNAME(who)+" is invisible");
  return 1;
}
int write_visibobj(object who) {
  if(who) WDLN("*** Object becomes visible "+ObjFile(who));
  return 1;
}
int write_visiblvg(object who) {
  if(who) WDLN("*** Living becomes visible "+ObjFile(who));
  return 1;
}
int write_visibply(object who) {
  if(who) WDLN("*** "+CRNAME(who)+" becomes visible");
  return 1;
}

/*----------------------------------------------------------------------
 * catch all commands, absorb forces, check history and aliases
 */

int CommandScan(string arg) {
  string verb;
  object rtp;
  rtp=RTP;

  if(!cloner&&!(cloner=rtp)) destruct(TO);

  if((!MODE(MODE_PROTECT))||security()) {
    verb=query_verb();
    if(verb&&(DoHistory(verb+(arg ? " "+arg : ""))||DoAlias(verb, arg)))
      return TRUE;
    nostore=FALSE;
    return FALSE;
  } else {
    if(rtp) {
      WDLN("Your "+TOOL_TITLE+" protects you from a force by "+CRNAME(rtp)+
	   " ["+query_verb()+(arg ? " "+arg+"]" : "]"));
      tell_object(rtp, CRNAME(cloner)+"'s "+TOOL_TITLE+
		  " absorbes your force.\n");
    } else {
      WDLN("Your "+TOOL_TITLE+" protects you from a force ["+
	   query_verb()+(arg ? " "+arg+"]" : "]"));
    }
    return TRUE;
  }
  return FALSE;
}

void history() { write(implode(history, "\n")+"\n"); }

int DoHistory(string line) {
  int i;
  string cmd, *strs;

  SECURE2(FALSE);
  if(!line) return TRUE;
  else if(line=="%!") {
    WLN("Current command history:");
    for(i=MAX_HISTORY; i; --i) {
      if(history[i-1]) {
	W(" "+ARIGHT(""+i, 2, " ")+": ");
	if(sizeof(history[i-1])>70) WLN(ALEFT(history[i-1], 70, " "));
	else WLN(history[i-1]);
      }
    }
    return TRUE;
  } else if(SUBSTR(line, 0, 1)=="%%"&&(cmd=history[0]+RIGHT(line, 2))) {
    Command(cmd);
    return TRUE;
  } else if(line[0]=='^'&&(strs=strip_explode(line, "^"))) {
    if(sizeof(strs)&&strs[0]&&(cmd=history[0])) {
      if(sizeof(strs)==2) cmd=string_replace(cmd, strs[0], strs[1]);
      else cmd=string_replace(cmd, strs[0], "");
      nostore--;
      Command(cmd);
      nostore++;
      return TRUE;
    }
  } else if(line[0]=='%'&&(sscanf(RIGHT(line, 1), "%d", i))) {
    i= i>0 ? i : 1;
    i= i<=MAX_HISTORY ? i : MAX_HISTORY;
    if(cmd=history[i-1]) Command(cmd);
    return TRUE;
  } else if(line[0]=='%') {
    for(i=0; i<MAX_HISTORY; i++) {
      if(history[i]&&
	 SUBSTR(history[i], 0, sizeof(line)-2)==RIGHT(line, 1)) {
	Command(history[i]);
	return TRUE;
      }
    }
  } else if(nostore<1) history=({line})+history[0..MAX_HISTORY-2];
  return FALSE;
}

int DoAlias(string verb, string arg) {
  int i, s, f;
  string cmd, *args, tmp;

  SECURE2(FALSE);
  if(!verb) return TRUE;
  else if(cmd=aliases[verb]) {
    if(arg&&(args=new_explode(arg, " "))) {
      s=sizeof(args);
      i=s--;
      if(f=sizeof(regexp(({cmd}),"![0-9]*\\*")))
	cmd=string_replace(cmd, "!*", arg);
      arg="";
      while(i--) {
	tmp=cmd;
	if(f) cmd=string_replace(cmd, "!"+(i+1)+"*",implode(args[i..s]," "));
	cmd=string_replace(cmd, "!"+(i+1), args[i]);
	if(!f&&tmp==cmd) arg=" "+args[i]+arg;
      }
      cmd+=arg;
    }
    if(++aliaslevel>MAX_RECURSION)
      WDLN("*** Too deep recursion. Aborting execution");
    else {
      nostore++;
      Command(cmd);
      nostore--;
    }
    aliaslevel=0;
    return TRUE;
  }
  return FALSE;
}

/*----------------------------------------------------------------------
 * command functions
 */

#include "xtool/xtool.cmds.c"
