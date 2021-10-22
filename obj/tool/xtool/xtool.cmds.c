/*----------------------------------------------------------------------*/
/* The Xtool - Copyright 1992 by Hyp                                    */
/*                                                                      */
/* File: xtool.cmds.c                                                   */
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------
 * command functions
 */

int Xalias(string str) {
  int s;
  string al, cmd, *strs;

  SECURE2(TRUE);
  USAGE1(str, "xal(ias) <alias> <command>");
  if(!str) {
    mixed indi = m_indices(aliases);
    WLN("Current aliases:");
    s=sizeof(aliases);
    while(s--)
      WLN(" "+ALEFT(indi[s]+" ", 12, ".")+" "+aliases[indi[s]]);
  } else if(sscanf(str, "%s %s", al, cmd)==2) {
    if(!sizeof(strs=strip_explode(al, " "))) return FALSE;
    al=strs[0];
    WDLN("Setting an alias \""+al+"\" for \""+cmd+"\"");
    aliases[al]=cmd;
  } else {
    if(cmd=aliases[str])
      WDLN("Alias \""+str+"\" is set to \""+cmd+"\"");
    else WDLN("An alias \""+str+"\" does not exist");
  }
  return TRUE;
}

int Xcall(string str) {
  object obj, callobj;
  string file, callstr, callfun, callexpr, error, errlog;
  int *ru1, *ru2, time;
  mixed res;

  SECURE2(TRUE);
  USAGE2(str, "xcall <object>-><function>(<arguments>)");
  if(sscanf(str, "%s->%s(%s", callstr, callfun, callexpr)!=3)
    return FALSE;
  if(!(callobj=XFindObj(callstr))) return TRUE;
  else {
    file=LPC_FILE+".c";
    if(file_size(file)>0) rm(file);
    if(obj=find_object(LPC_FILE)) Destruct(obj);
#if MUD_NF || MUD_MG
    write_file(file, "#pragma verbose_errors\n#include <attributes.h>\n#include <daemons.h>\n"+
#endif
#if MUD_TA || MUD_BB || MUD_TM
    write_file(file,
#endif
	       "void create(){}\nobject get(string str){return ({object})previous_object()->XFindObj(str);}\n"+
	       "mixed eval(object obj,object me,object here){return ({mixed})obj->"+callfun+"("+callexpr+";}\n");
    errlog = ERR_FILE;
    if(file_size(errlog)>0) rm(errlog);
    if(error=catch(obj=clone_object(file))) W("Error: "+error[1..]);
    else {
      ru1=rusage();
      error=catch(res=({mixed})obj->eval(callobj, cloner, ENV(cloner)));
      ru2=rusage();
      if(error) W("Error: "+error[1..]);
      else {
	time=ru2[0]-ru1[0]+ru2[1]-ru1[1];
	WDLN("Evaluation time: "+(time<0 ? 0 : time)+" ms");
	WLN("Result: "+mixed_to_string(res, MAX_RECURSION));
	if(objectp(res)) variable["result"]=res;
      }
    }
    rm(file);
  }
  if(obj) Destruct(obj);
  return TRUE;
}

int Xcallouts(string str) {
  object obj;
  mixed callouts, fun;
  string tmp, file;
  int i, s;

  SECURE2(TRUE);
  USAGE1(str, "xcallo(uts) [search pattern]");
  file=TMP_FILE;
  if(file_size(file)>0) rm(file);
  if(!str) str="^\\[~/";
  else if(!regexp(({"dummy"}), str)) {
    WDLN("Bad regular expression");
    return TRUE;
  }
  callouts=call_out_info();
  s=sizeof(callouts);
  for(i=0; i<s; i++) {
    if(callouts[i]&&pointerp(callouts[i])) {
      fun = callouts[i][1];
      obj = closurep(fun) ? to_object(fun) : callouts[i][0];
      tmp=ObjFile(obj)+" "+callouts[i][2]+"s "+ sprintf( "%O", fun)+"("+
      (sizeof(callouts[i])>3?mixed_to_string(callouts[i][3], MAX_RECURSION):"")
        +")";
      if(sizeof(regexp(({tmp}), str)))
	write_file(file, tmp+"\n");
    }
  }
  XMoreFile(file, FALSE);
  return TRUE;
}

int Xcindent(string str) {
  SECURE2(TRUE);
  USAGE2(str, "xci(ndent) <filename>");
  /* commented out by Deepthought, 03-Oct-94, because cindent doesnt
     work with 3.2.1@63.
  if(str=XFindFile(str)) {
    if(cindent(str)) WLN("Cindent: "+short_path(str));
    else WDLN("An cindent error occured");
  }
  return TRUE;
  */
}

int Xclean(string str) {
  object env;

  SECURE2(TRUE);
  USAGE1(str, "xcle(an) [object]");
  if(!str) env=ENV(cloner);
  else env=XFindObj(str);
  if(env) {
    PrintShort("Cleaning: ", env);
    filter(filter(all_inventory(env), "is_not_player", TO),
		 "Destruct", TO);
  }
  return TRUE;
}

int Xclone(string str) {
  object obj;
  string file, errlog, error;

  SECURE2(TRUE);
  USAGE2(str, "xclo(ne) <filename>");
  if(!(file=XFindFile(str))) return TRUE;
  errlog=ERR_FILE;
  if(file_size(errlog)>0) rm(errlog);
  if(obj=find_object(file)) {
    Destruct(obj);
    WLN("Update and clone: "+short_path(file));
  } else WLN("Clone: "+short_path(file));
  if(!(error=catch(obj=clone_object(file)))) {
    variable["clone"]=obj;
    if(!MoveObj(obj, ENV(cloner), TRUE))
      WDLN("Cannot move object into this room");
#if MUD_NF || MUD_MG
    else if(!({int})obj->QueryNoGet()) {
      if(!MoveObj(obj, cloner, TRUE))
	WDLN("Cannot move object into your inventory");
    }
#endif
#if MUD_TA || MUD_BB || MUD_TM
    else if(obj->get()) {
      if(!MoveObj(obj, cloner, TRUE))
	WDLN("Cannot move object into your inventory");
    }
#endif
  } else W("Error: "+error[1..]);
  return TRUE;
}

int Xcmds(string str) {
  object obj;
  string file;
  mixed *cmds;
  int i, s;

  SECURE2(TRUE);
  USAGE1(str, "xcm(ds) [object]");
  if(!str) obj=ENV(cloner);
  else if(!(obj=XFindObj(str)))
    return 1;
  file=TMP_FILE;
  s=sizeof(cmds=query_actions(cloner,1|2|4|8|16));
  for(i=0; i<s; i+=5) {
    if(cmds[i+3]==obj) {
      write_file(file, ALEFT(cmds[i]+" ", 15, ".")+
		 (cmds[i+2] ? " * " : " . ")+cmds[i+4]+"()\n");
    }
  }
  XMoreFile(file, FALSE);
  return 1;
}

int Xdate(string str) {
  SECURE2(TRUE);
  USAGE1(str, "xda(te)");
  WDLN(ctime(time()));
  return TRUE;
}

int Xdbg(string str) {
  object obj;

  SECURE2(TRUE);
  USAGE2(str, "xdb(g) <object>");
  if(obj=XFindObj(str)) {
    debug_info(1, obj);
    debug_info(0, obj);
  }
  return TRUE;
}

int Xdclean(string str) {
  object env;

  SECURE2(TRUE);
  USAGE1(str, "xdc(lean) [object]");
  if(!str) env=ENV(cloner);
  else env=XFindObj(str);
  if(env) {
    PrintShort("Deep cleaning: ", env);
    filter(filter(all_inventory(env), "is_not_player", TO),
		 "DeepClean", TO);
  }
  return TRUE;
}

int Xddes(string str) {
  object obj;

  SECURE2(TRUE);
  USAGE2(str, "xdd(es) <object>");
  if(obj=XFindObj(str)) {
    PrintShort("Deep destruct: ", obj);
    filter(deep_inventory(obj), "Destruct", TO);
    Destruct(obj);
  }
  return TRUE;
}

int Xdes(string str) {
  object obj;

  SECURE2(TRUE);
  USAGE2(str, "xde(s) <object>");
  if(obj=XFindObj(str)) {
    PrintShort("Destruct: ", obj);
    Destruct(obj);
  }
  return TRUE;
}

int Xdlook(string str) {
  object obj;

  SECURE2(TRUE);
  USAGE1(str, "xdl(ook) [object]");
  if(!str) str=RNAME(cloner);
  if(obj=XFindObj(str)) DeepPrintShort(obj, NULL, NULL);
  return TRUE;
}

int Xdo(string str) {
  int i, n, s;
  string *strs, cmd;

  SECURE2(TRUE);
  USAGE2(str, "xdo [<number1>#]<command1>[;[<number2>#]<command2>] ...");
  if(!(s=sizeof(strs=strip_explode(str, ";")))) return FALSE;
  for(i=0; i<s; i++) {
    if(strs[i]) {
      switch(sscanf(strs[i], "%d#%s", n, cmd)) {
      case 0:
	if(!Command(strs[i])) return TRUE;
	break;
      case 1:
	if(cmd&&(!Command(cmd))) return TRUE;
	break;
      case 2:
	n= n<1 ? 1 : n;
	if(cmd) {
	  while(n--)
	    if(!Command(cmd)) return TRUE;
	}
	break;
      }
    }
  }
  return TRUE;
}

int Xdupdate(string str) {
  int i, s;
  object obj;
  string file, *list;

  SECURE2(TRUE);
  USAGE2(str, "xdu(pdate) <filename>");
  if(!(file=XFindFile(str))) return TRUE;
  if(LAST(file, 2)==".c") file=LEFT(file,3);
  if(obj=XFindObj(file)) {
    PrintShort("Deep updating: ", obj);
    list=inherit_list(obj);
    for(s=sizeof(list); i<s; i++) {
      if(obj=find_object(list[i])) Destruct(obj);
    }
  }
  return TRUE;
}

int Xeval(string str) {
  object obj;
  string file, error;
  int *ru1, *ru2, time;
  mixed res;

  SECURE2(TRUE);
  USAGE2(str, "xev(al) <expression>");
  file=LPC_FILE+".c";
  if(file_size(file)>0) rm(file);
  if(obj=find_object(LPC_FILE)) Destruct(obj);
#if MUD_NF || MUD_MG
  write_file(file, "#pragma verbose_errors\n#include <attributes.h>\n#include <daemons.h>\n"+
#endif
#if MUD_TA || MUD_BB || MUD_TM
  write_file(file,
#endif
      "void create() {}\nobject get(string str){return ({object})previous_object()->XFindObj(str);}\n"+
      "mixed eval(object me,object here){return ({mixed})"+str+";}");
  if(error=catch(obj=clone_object(file))) W("Error: "+error[1..]);
  else {
    ru1=rusage();
    error=catch(res=({mixed})obj->eval(cloner, ENV(cloner)));
    ru2=rusage();
    if(error) W("Error: "+error[1..]);
    else {
      time=ru2[0]-ru1[0]+ru2[1]-ru1[1];
      WDLN("Evaluation time: "+(time<0 ? 0 : time)+" ms");
      WLN("Result: "+mixed_to_string(res, MAX_RECURSION));
      if(objectp(res)) variable["result"]=res;
    }
  }
  rm(file);
  if(obj) Destruct(obj);
  return TRUE;
}

int Xforall(string str) {
  int i, s, t, u;
  string pat, cmd, arg, *strs, *files, fh, fr, fe, ft, ff;

  SECURE2(TRUE);
  USAGE2(str, "xfo(rall) <filepattern> <command>");
  if(sscanf(str, "%s %s", pat, arg)!=2) return FALSE;
  files=long_get_dir(pat, FALSE);
  if(!(s=sizeof(files))) {
    WDLN("No matching files found");
    return TRUE;
  }
  strs=explode(files[0], "/");
  fh="/";
  if(t=sizeof(strs)-1) fh+=implode(strs[0..t-1], "/");
  for(i=0; i<s; i++) {
    ft=explode(files[i], "/")[t];
    if((u=sizeof(strs=explode(ft, ".")))&&--u) {
      ff=implode(strs[0..u-1], ".");
      fr=fh+"/"+ff;
      fe=strs[u];
    } else {
      fe="";
      ff=ft;
      fr=files[i];
    }
    cmd=string_replace(arg, "!!", files[i]);
    cmd=string_replace(cmd, "!e", fe);
    cmd=string_replace(cmd, "!f", ff);
    cmd=string_replace(cmd, "!h", fh);
    cmd=string_replace(cmd, "!r", fr);
    cmd=string_replace(cmd, "!t", ft);
    if(!(Command(cmd)))
      break;
  }
  return TRUE;
}

int Xgoto(string str) {
  object obj, tmp;

  SECURE2(TRUE);
  USAGE1(str, "xgo(to) [object]");
  if(!str) str="~/workroom";
  if(!(obj=XFindObj(str))) {
    if(!(str=XFindFile(str))) return TRUE;
    if(catch(call_other(str, "???"))) return TRUE;
    obj=find_object(str);
  }
  tmp=obj;
  while(obj&&living(obj)) obj=ENV(obj);
#if defined(MUD_NF) || defined(MUD_OSB)
  cloner->move(obj ? obj : tmp, 3);
#else
  cloner->move_player("X", obj ? obj : tmp);
#endif
  return TRUE;
}

int Xgrep(string str) {
  int i, s, t;
  string *files, *ts;

  SECURE2(TRUE);
  USAGE2(str, "xgr(ep) <regexp> <filepattern>");
  if(!((ts=old_explode(str, " "))&&((s=sizeof(ts))>1))) return FALSE;
  str=implode(ts[0..s-2], " ");
  if(!(regexp(({"dummy"}), str))) {
    WDLN("Bad regular expression");
    return TRUE;
  }
  if(file_size(TMP_FILE)>0) rm(TMP_FILE);
  if((t=sizeof(files=long_get_dir(XFile(ts[s-1]), FALSE)))&&
     (file_size(files[0])>=0)) {
    for(i=0; i<t; i++) XGrepFile(str, files[i]);
    XMoreFile(TMP_FILE, FALSE);
    rm(TMP_FILE);
  }
  else WDLN("Cannot read file(s)");
  return TRUE;
}

int Xhbeats(string str) {
  object *hbeatinfo;
  string tmp, file;
  int i, s;

  SECURE2(TRUE);
  USAGE1(str, "xhb(eats) [search pattern]");
  file=TMP_FILE;
  if(file_size(file)>0) rm(file);
  if(!str) str=RNAME(cloner);
  else if(!regexp(({"dummy"}), str)) {
    WDLN("Bad regular expression");
    return TRUE;
  }
  s=sizeof(hbeatinfo=heart_beat_info());
  for(i=0; i<s; i++) {
    if(hbeatinfo[i]&&objectp(hbeatinfo[i])) {
      tmp=ObjFile(hbeatinfo[i]);
      if(sizeof(regexp(({tmp}), str)))
	write_file(file, tmp+"\n");
    }
  }
  XMoreFile(file, FALSE);
  return TRUE;
}

int Xhelp(string str) {
  SECURE2(TRUE);
  USAGE1(str, "xhe(lp)");
  XMoreFile(TOOL_MANPAGE, FALSE);
  return TRUE;
}

#if MUD_NF || MUD_MG
int Xids(string str) {
  object obj;

  SECURE2(TRUE);
  USAGE2(str, "xid(s) <object>");
  if(obj=XFindObj(str))
    WLN("UID=\""+getuid(obj)+"\" / EUID=\""+geteuid(obj)+"\"");
  return TRUE;
}
#endif

int Xinfo(string str) {
  object obj;

  SECURE2(TRUE);
  USAGE2(str, "xinf(o) <object>");
  if(obj=XFindObj(str)) {
    if(is_player(obj)) {
      WLN(PlayerWho(obj));
      WLN(PlayerMail(obj, FALSE));
      WLN(PlayerIP(obj, FALSE));
#if MUD_NF || MUD_MG
      WLN(PlayerRace(obj, FALSE));
#endif
      WLN(PlayerDomain(obj, FALSE));
      WLN(PlayerStats(obj, FALSE));
      WLN(PlayerSnoop(obj, FALSE));
    } else WDLN("Sorry, this is not a player");
  }
  return TRUE;
}

int Xinherit(string str) {
  int s;
  object obj;
  string *strs, *inlist;

  SECURE2(TRUE);
  USAGE2(str, "xinh(erit) <object> [function]");
  if(!(strs=strip_explode(str, " "))) return FALSE;
  if(obj=XFindObj(strs[0])) {
    inlist=inherit_list(obj);
    s=sizeof(inlist);
    while(s--) {
      if(catch(call_other(inlist[s], "???"))) {
	WDLN("Failed to load all inheritance objects");
	return TRUE;
      }
    }
    obj=find_object(inlist[0]);
    if(sizeof(strs)==1) Inheritance(obj ,0 ,"");
    else Inheritance(obj, strs[1], "");
  }
  return TRUE;
}

int Xlight(string str) {
  int addlight;

  SECURE2(TRUE);
  USAGE1(str, "xli(ght) [light]");
  if(str) {
    if(!sscanf(str, "%d", addlight)) return FALSE;
    xlight+=addlight;
#if MUD_NF || MUD_MG
    cloner->AddIntLight(addlight);
#endif
#if MUD_TA || MUD_BB || MUD_TM
    efun::set_light(addlight);
#endif
  }
#if MUD_NF || MUD_MG
  WDLN("Current light levels: "+TOOL_NAME+"="+xlight+", room="+
       ({int})ENV(cloner)->QueryIntLight());
#endif
#if MUD_TA || MUD_BB || MUD_TM
  WDLN("Current light levels: "+TOOL_NAME+"="+xlight+", room="+
       efun::set_light(0));
#endif
  return TRUE;
}

int Xload(string str) {
  int i;
  object obj, *inv, vroom;
  string file, errlog;

  SECURE2(TRUE);
  USAGE2(str, "xloa(d) <filename>");
  if(!(file=XFindFile(str))) return TRUE;
  errlog=ERR_FILE;
  if(file_size(errlog)>0) rm(errlog);
  if(obj=find_object(file)) {
    if(catch(call_other(VOID, "???"))) {
      WDLN("Error: cannot find "+VOID+" to rescue players");
      return TRUE;
    } else vroom = find_object(VOID);
    if(inv=filter(all_inventory(obj), "is_player", TO))
      for(i=0; i<sizeof(inv); i++) MoveObj(inv[i], vroom, TRUE);
    Destruct(obj);
    WLN("Update and load: "+file);
  } else WLN("Load: "+file);
#if MUD_NF
  // No catch, to get proper backtraces on errors.
  call_other(file, "???");
  if(inv) {
#else
  if(error=catch(call_other(file, "???"))) {
    W("Error: "+error[1..]);
    if(vroom)
      tell_object(vroom, "*** Failed to load room. You are in the void!\n");
  } else if(inv) {
#endif
    obj=find_object(file);
    for(i=0; i<sizeof(inv); i++)
      if(inv[i]) MoveObj(inv[i], obj, TRUE);
  }
  return TRUE;
}

int Xlook(string str) {
  object obj;

  SECURE2(TRUE);
  USAGE1(str, "xloo(k) [object]");
  if(str) {
    if(obj=XFindObj(str)) PrintObj(obj);
  } else {
    obj=ENV(cloner);
    PrintObj(obj);
  }
  return TRUE;
}

int Xlpc(string str) {
  object obj;
  string file, error;
  int *ru1, *ru2, time;
  mixed res;

  SECURE2(TRUE);
  USAGE2(str, "xlp(c) <lpc code>");
  file=LPC_FILE+".c";
  if(file_size(file)>0) rm(file);
  if(obj=find_object(LPC_FILE)) Destruct(obj);
#if MUD_NF || MUD_MG
  write_file(file, "#pragma verbose_errors\n#include <attributes.h>\n#include <daemons.h>\n"+
#endif
#if MUD_TA || MUD_BB || MUD_TM
  write_file(file,
#endif
	     "void create() {}\nobject get(string str){return ({object})previous_object()->XFindObj(str);}\n"+
       "mixed eval(object me,object here){"+str+"}");
  if(error=catch(obj=clone_object(file))) W("Error: "+error[1..0]);
  else {
    ru1=rusage();
    error=catch(res=({mixed})obj->eval(cloner, ENV(cloner)));
    ru2=rusage();
    if(error) W("Error: "+error[1..]);
    else {
      time=ru2[0]-ru1[0]+ru2[1]-ru1[1];
      WDLN("Evaluation time: "+(time<0 ? 0 : time)+" ms");
      WLN("Result: "+mixed_to_string(res, MAX_RECURSION));
      if(objectp(res)) variable["result"]=res;
    }
  }
#if MUD_NF
  // In cases of errors, it's nice to know what it caused.
  if (!error)
#endif
    rm(file);
  if(obj) Destruct(obj);
  return TRUE;
}

int Xman(string str) {
  string manpage;
  int i, found;

  SECURE2(TRUE);
  USAGE2(str, "xma(n) <manpage>");
  W("Man: ");
  for(i=0, found=0; i<sizeof(manpath); i++) {
    manpage=manpath[i]+str;
    if(file_size(manpage)>=0) {
      WLN(manpage);
      XMoreFile(manpage, FALSE);
      found=1;
      break;
    }
  }
  if(!found) WLN("- no help available -");
  return TRUE;
}

int Xmore(string str) {
  string *args, file;
  int line;

  SECURE2(TRUE);
  USAGE2(str, "xmor(e) <filename> [start]");
  switch(sizeof(args=strip_explode(str, " "))) {
  case 1:
    moreoffset=1;
    break;
  case 2:
    sscanf(args[1], "%d", line);
    moreoffset= line>0 ? line : 1;
    break;
  default:
    return FALSE;
  }
  if(file=XFindFile(args[0]))
    XMoreFile(file, TRUE);
  return TRUE;
}

int Xmove(string str) {
  object obj1, obj2;
  string what, into;

  SECURE2(TRUE);
  USAGE2(str, "xmov(e) <object> into <object>");
  if((sscanf(str, "%s into %s", what, into)==2)&&
     (obj1=XFindObj(what))&&(obj2=XFindObj(into)))
    MoveObj(obj1, obj2, FALSE);
  return TRUE;
}

int Xmsg(string str) {
  string tmp;

  SECURE2(TRUE);
  USAGE1(str, "xms(g) [to <object>|all]");
  if(!str) {
    WDLN("Send message into room");
    WDLN("End message with \".\" or \"**\"");
    say("Message from "+CRNAME(cloner)+":\n");
    input_to("XMsgSay");
    return TRUE;
  } else if(sscanf(str, "to %s", tmp)) {
    if(msgto=XFindObj(tmp)) {
      PrintShort("Send message to: ", msgto);
      WDLN("End message with \".\" or \"**\"");
      tell_object(msgto, "Message from "+CRNAME(cloner)+" to you:\n");
      input_to("XMsgTell");
    }
    return TRUE;
  } else if(str=="all") {
    WDLN("Send message to all players");
    WDLN("End message with \".\" or \"**\"");
    shout("Message from "+CRNAME(cloner)+" to all:\n");
    input_to("XMsgShout");
    return TRUE;
  }
  return FALSE;
}

int Xmtp(string str) {
  int s;
  string *strs, opt, dir, file;

  SECURE2(TRUE);
  USAGE2(str, "xmt(p) [options] <directory> <filename>");
  s=sizeof(strs=explode(str, " "));
  if(s<2)
    return FALSE;
  else if(s==2)
    opt="";
  else opt=implode(strs[0..s-3], " ");
#if MUD_NF || MUD_MG
  if(!(dir="/"+({string})MASTER->valid_read(strs[s-2], geteuid(),
				      "get_dir", TO))) {
    WDLN("No permission to open directory for reading");
    return TRUE;
  }
  if(!(file="/"+({string})MASTER->valid_write(strs[s-1], geteuid(),
					"write_file", TO))) {
    WDLN("No permission to open script file for writing");
    return TRUE;
  }
#endif
#if MUD_TA || MUD_BB || MUD_TM
  if(!(dir="/"+({string})MASTER->valid_read(strs[s-2], RNAME(cloner),
				      "get_dir", TO))) {
    WDLN("No permission to open directory for reading");
    return TRUE;
  }
  if(!(file="/"+({string})MASTER->valid_write(strs[s-1], RNAME(cloner),
					"write_file", TO))) {
    WDLN("No permission to open script file for writing");
    return TRUE;
  }
#endif
  if(file_size(dir)!=-2 || file_size(file)==-2)
    return FALSE;
  XmtpScript(dir, file, opt);
  WDLN("Done");
  return TRUE;
}

int Xprof(string str) {
  string *funcs, inh, tmp;
  mixed *data, *d;
  mapping xpr;
  object obj;
  int i, rn;

  SECURE2(TRUE);
  USAGE2(str, "xprof <<-c>|<-C> <file>>|<object>");
  if(str[0..2]=="-c "||str[0..2]=="-C ") {
    rn=(str[1]=='C');
    if(str=XFindFile(str[3..])) {
      inh=str=str[0..<3];
      if(obj=find_object(inh)) Destruct(obj);
      if(catch(call_other(inh,"???"))) return TRUE;
      obj=find_object(inh);
      if(rn) {
	inh+=".xprof.c";
	rm(inh);
	str+=".c";
	rename(str, inh);
      } else {
	str=XPROF_FILE;
	rm(str);
      }
      tmp="inherit \""+inh+"\";\n#include \""+XPROF_MACRO+"\"\n";
      funcs=m_indices(mkmapping(functionlist(obj, 0x08000001)))-({"__INIT"});
      tmp+="F("+implode(funcs,",(A0,A1,A2,A3,A4,A5,A6,A7,A8,A9))\nF(")+
	",(A0,A1,A2,A3,A4,A5,A6,A7,A8,A9))\n";
      write_file(str, tmp);
      WDLN("Done");
    }
  } else if(obj=XFindObj(str)) {
    if(xpr=({mapping})obj->__query_xprof_data__()) {
      funcs=m_indices(xpr);
      data=m_values(xpr);
      rm(TMP_FILE);
      str="Evaluation cost per function:\nNAME                     "+
	" CALLS |  TOT.EXCL.  TOT.INCL. | REL.EXCL. REL.INCL.\n";
      for(i=sizeof(funcs); i--;) {
	if(d=data[i]) {
	  str+=ALEFT(""+funcs[i]+" ",25,".")+ARIGHT(" "+d[0], 6,".")+" | "+
	    ARIGHT(" "+d[1],10,".")     +" "+ARIGHT(" "+d[3],10,".")+" | "+
	    ARIGHT(" "+d[1]/d[0],9,".") +" "+ARIGHT(" "+d[3]/d[0],9,".")+"\n";
	}
      }
      write_file(TMP_FILE,str);
      str="\nElapsed time per function:\nNAME                     "+
	" CALLS |  TOT.EXCL.  TOT.INCL. | REL.EXCL. REL.INCL.\n";
      funcs=m_indices(xpr);
      for(i=sizeof(funcs); i--;) {
	if(d=data[i]) {
	  str+=ALEFT(""+funcs[i]+" ",25,".")+ARIGHT(" "+d[0], 6,".")+" | "+
	    ARIGHT(" "+(d[2]+5)/10+"ms",10,".")+" "+
	    ARIGHT(" "+(d[4]+5)/10+"ms",10,".")+" | "+
	    ARIGHT(" "+d[2]/d[0]+"us",9,".")+" "+
	    ARIGHT(" "+d[4]/d[0]+"us",9,".")+"\n";
	}
      }
      write_file(TMP_FILE,str);
      XMoreFile(TMP_FILE, FALSE);
    } else WDLN("No profiling information available");
  }
  return TRUE;
}

int Xprops(string str) {
  object obj;

  SECURE2(TRUE);
  USAGE2(str, "xprop(s) <object>");
  if(obj=XFindObj(str))
    DumpProperties(obj);
  return TRUE;
}

int Xquit(string str) {
  SECURE2(TRUE);
  USAGE1(str, "xqu(it)");
#if MUD_NF || MUD_MG
  str=object_name(ENV(cloner));
  if(member(str, '#')<0) cloner->set_home(str);
#endif
  cloner->quit();
  return TRUE;
}

#if MUD_NF
int Xrevive(string str) {
  object obj;

  SECURE2(TRUE);
  USAGE1(str, "xre(vive) <player>");
  if(obj=XFindObj(str)) {
    if(is_player(obj)) {
      WDLN("Reviving player "+CRNAME(obj));
      obj->turn_heart_beat(FALSE);
      obj->turn_heart_beat(TRUE);
      if(member(heart_beat_info(), obj) < 0)
	WDLN("Reviving failed");
    } else
      WDLN("Specified object is not a player");
  }
  return TRUE;
}
#endif

int Xscan(string str) {
  int i, s;
  string *oldvars, *desvars;

  SECURE2(TRUE);
  USAGE1(str, "xsc(an)");
  netdead=({});
  NetDeadCheck(FALSE);
  W("Netdead player(s): ");
  s=sizeof(netdead);
  for(i=0; i<s; i++) {
    W(CRNAME(netdead[i]));
    if(i<s-1) W(", ");
  }
  WLN("");
  W("Destructed variable(s): ");
  oldvars=m_indices(variable);
  VarCheck(FALSE);
  desvars=oldvars;
  s=sizeof(desvars);
  for(i=0; i<s; i++) {
    W("$"+desvars[i]);
    if(i<s-1) W(", ");
  }
  WLN("");
  return TRUE;
}

int Xset(string str) {
  int i;
  mixed obj;
  string name, tmp;

  SECURE2(TRUE);
  USAGE1(str, "xse(t) [$<name>=<object>]");
  if(str) {
    if(member(({"$me","$m","$here","$h"}), str)!=-1)
      WDLN("Sorry, this is a reserved variable ["+str+"]");
    else if(sscanf(str, "$%s=%s", name, tmp)) {
      if(obj=XFindObj(tmp)) {
	variable[name]=obj;
	WLN(" $"+name+"="+ObjFile(obj));
      }
    } else return FALSE;
  } else {
    VarCheck(FALSE);
    WLN("Current settings:");
    mixed indi=m_indices(variable);
    for(i=0; i<sizeof(indi); i++) {
      if(variable[indi[i]])
	WLN(" $"+indi[i]+"="+ObjFile(variable[indi[i]]));
      else m_delete(variable, indi[i]);
    }
    WLN(" $me="+ObjFile(cloner));
    WLN(" $here="+ObjFile(ENV(cloner)));
  }
  return TRUE;
}

int Xsh(string str) {
  SECURE2(TRUE);
  USAGE1(str, "xsh <filename>");
  if(scriptline) {
    WDLN("Cannot execute another script file until last execution has ended");
    return TRUE;
  }
  if(!(str=XFindFile(str))) return TRUE;
  str=read_file(str, 1, 1000);
  if(!(scriptline=explode(str, "\n"))) {
    WDLN("Bad script file");
    return TRUE;
  }
  scriptsize=sizeof(scriptline);
  XExecFile(NULL);
  return TRUE;
}

int Xstop(string str) {
  object item;

  WDLN("Not ready implemented yet");
  return TRUE;

  SECURE2(TRUE);
  USAGE1(str, "xst(op)");
  FORALL(item, ENV(cloner))
    if(living(item)) item->Kill(NULL);
  WDLN("Forced all livings to stop fighting (not implemented!)");
  return TRUE;
}

int Xtool(string str) {
  int m;
  string tmp;
  object obj;

  SECURE2(TRUE);
  USAGE1(str, "xto(ol) [update|heart=<on|off>|first=<on|off>|protect="+
	 "<on|off>|\n"+
	 "                invcheck=<on|off>|envcheck=<on|off>|ndcheck="+
	 "<on|off>|\n"+
	 "                varcheck=<on|off>|snoopchk=<on|off>|invischk="+
	 "<on|off>|\n"+
	 "                scanchk=<on|off>|short=<on|off>|echo=<on|off>|"+
	 "more=<amount|auto>|\n"+
	 "                kill|news|save|load|reset]");
  if(str) {
    if(sscanf(str, "more=%s", tmp) && tmp="auto") {
      morelines = ({int})this_player()->QueryPageSize();
      WDLN("Setting number of displayed lines to " + morelines);
    }
    else if(sscanf(str, "more=%d", m)) {
      if(m<5) WDLN("Sorry, amount of lines should be more then 5");
      else {
	WDLN("Setting amount of displayed lines to "+m);
	morelines=m;
      }
    } else switch(str) {
    case "update":
      if(obj=find_object(TOOL_PATH))
	Destruct(obj);
      if(catch(obj=clone_object(TOOL_PATH)))
	WLN("Updating "+TOOL_TITLE+" failed!");
      else obj->update_tool(AUTOLOAD_ARGS, cloner);
      return TRUE;
      break;
    case "heart=on":
      MODE_ON(MODE_HEART);
      hbeats=0;
      set_heart_beat(TRUE);
      WDLN("Heart beat turned on");
      break;
    case "heart=off":
      MODE_OFF(MODE_HEART);
      set_heart_beat(FALSE);
      WDLN("Heart beat turned off");
      break;
    case "first=on":
      MODE_ON(MODE_FIRST);
      move(cloner);
      WDLN("Automatic moving into pole position of inventory turned on");
      break;
    case "first=off":
      MODE_OFF(MODE_FIRST);
      WDLN("Automatic moving into pole position of inventory turned off");
      break;
    case "protect=on":
      MODE_ON(MODE_PROTECT);
      WDLN("Protection from forces and illegal moves turned on");
      break;
    case "protect=off":
      MODE_OFF(MODE_PROTECT);
      WDLN("Protection from forces and illegal moves turned off");
      break;
    case "invcheck=on":
      MODE_ON(MODE_INVCHECK);
      WDLN("Automatic checking for new objects in inventory turned on");
      break;
    case "invcheck=off":
      MODE_OFF(MODE_INVCHECK);
      WDLN("Automatic checking for new objects in inventory turned off");
      break;
    case "envcheck=on":
      MODE_ON(MODE_ENVCHECK);
      WDLN("Automatic checking for new objects in environment turned on");
      break;
    case "envcheck=off":
      MODE_OFF(MODE_ENVCHECK);
      WDLN("Automatic checking for new objects in environment turned off");
      break;
    case "ndcheck=on":
      MODE_ON(MODE_NDCHECK);
      NetDeadCheck(TRUE);
      WDLN("Automatic netdead checking turned on");
      break;
    case "ndcheck=off":
      MODE_OFF(MODE_NDCHECK);
      WDLN("Automatic netdead checking turned off");
      break;
    case "varcheck=on":
      MODE_ON(MODE_VARCHECK);
      VarCheck(TRUE);
      WDLN("Automatic variable checking turned on");
      break;
    case "varcheck=off":
      MODE_OFF(MODE_VARCHECK);
      WDLN("Automatic variable checking turned off");
      break;
    case "snoopchk=on":
      snoopee=({});
      MODE_ON(MODE_SNOOPCHK);
      WDLN("Automatic snoop checking turned on");
      break;
    case "snoopchk=off":
      MODE_OFF(MODE_SNOOPCHK);
      WDLN("Automatic snoop checking turned off");
      break;
    case "invischk=on":
      invisobjs=({});
      MODE_ON(MODE_INVISCHK);
      WDLN("Automatic invis checking turned on");
      break;
    case "invischk=off":
      MODE_OFF(MODE_INVISCHK);
      WDLN("Automatic invis checking turned off");
      break;
    case "scanchk=on":
      MODE_ON(MODE_SCANCHK);
      WDLN("Scan check turned on");
      break;
    case "scanchk=off":
      MODE_OFF(MODE_SCANCHK);
      WDLN("Scan check turned off");
      break;
    case "echo=on":
      MODE_ON(MODE_ECHO);
      WDLN("Echoing of alias and multiple command execution turned on");
      break;
    case "echo=off":
      MODE_OFF(MODE_ECHO);
      WDLN("Echoing of alias and multiple command execution turned off");
      break;
    case "short=on":
      MODE_ON(MODE_SHORT);
      WDLN("Use of short descriptions turned on");
      break;
    case "short=off":
      MODE_OFF(MODE_SHORT);
      WDLN("Use of short descriptions turned off");
      break;
    case "kill":
      WDLN(TOOL_NAME+" selfdestructs");
      destruct(TO);
      break;
    case "news":
      XMoreFile(TOOL_NEWS, FALSE);
      break;
    case "reset":
      WDLN("Resetting "+TOOL_TITLE);
      TO->initialize_globals();
      break;
    case "load":
      if(file_size(SAVE_FILE+".o")>0) {
	WDLN("Loading "+TOOL_TITLE+" settings");
	restore_object(SAVE_FILE);
	set_heart_beat(MODE(MODE_HEART));
      } else {
	WDLN("Sorry, cannot find file to load settings");
      }
      break;
    case "save":
      WDLN("Saving "+TOOL_TITLE+" settings");
      save_object(SAVE_FILE);
      break;
    default:
      return FALSE;
    }
  } else {
    WLN(TOOL_TITLE+" settings:");
    tmp=(" heart .... = "+(MODE(MODE_HEART) ? "on\n" : "off\n"));
    tmp+=(" first .... = "+(MODE(MODE_FIRST) ? "on\n" : "off\n"));
    tmp+=(" protect .. = "+(MODE(MODE_PROTECT) ? "on\n" : "off\n"));
    tmp+=(" invcheck . = "+(MODE(MODE_INVCHECK) ? "on\n" : "off\n"));
    tmp+=(" envcheck . = "+(MODE(MODE_ENVCHECK) ? "on\n" : "off\n"));
    tmp+=(" ndcheck .. = "+(MODE(MODE_NDCHECK) ? "on\n" : "off\n"));
    tmp+=(" varcheck . = "+(MODE(MODE_VARCHECK) ? "on\n" : "off\n"));
    tmp+=(" snoopchk . = "+(MODE(MODE_SNOOPCHK) ? "on\n" : "off\n"));
    tmp+=(" scanchk .. = "+(MODE(MODE_SCANCHK) ? "on\n" : "off\n"));
    tmp+=(" invischk . = "+(MODE(MODE_INVISCHK) ? "on\n" : "off\n"));
    tmp+=(" echo ..... = "+(MODE(MODE_ECHO) ? "on\n" : "off\n"));
    tmp+=(" short .... = "+(MODE(MODE_SHORT) ? "on\n" : "off\n"));
    tmp+=(" more ..... = "+morelines);
    WLN(sprintf("%-80#s", tmp));
  }
  return TRUE;
}

int Xtrace(string str) {
  string file;
  object obj;

  SECURE2(TRUE);
  USAGE1(str, "xtrac(e) <object>");
  if(!str) {
    trace(0);
    WDLN("Ending trace ["+short_path("/"+traceprefix(0))+"]");
  } else if(obj=XFindObj(str)) {
    PrintShort("Tracing: ", obj);
    file=object_name(obj);
#if MUD_NF || MUD_MG
    file=RIGHT(file, 1);
#endif
    traceprefix(file);
    trace(TRACE_LEVEL);
  }
  return TRUE;
}

int Xtrans(string str) {
  object obj;

  SECURE2(TRUE);
  USAGE2(str, "xtran(s) <object>");
  if((obj=XFindObj(str))&&ENV(obj)) {
    tell_room(ENV(obj), CRNAME(obj)+" vanishes.\n");
    tell_room(ENV(cloner), CRNAME(obj)+
	      " is teleported into this room by "+CRNAME(cloner)+".\n");
    MoveObj(obj, ENV(cloner), TRUE);
    tell_object(obj, "You've been teleported to "+CRNAME(cloner)+".\n");
  } else WDLN("Failed to teleport object");
  return TRUE;
}

int Xunalias(string str) {
  string cmd;

  SECURE2(TRUE);
  USAGE2(str, "xun(alias) <alias>");
  if(cmd=aliases[str]) {
    WDLN("Unalias: \""+str+"\" for \""+cmd+"\"");
    m_delete(aliases, str);
  } else WDLN("An alias \""+str+"\" does not exist");
  return TRUE;
}

int Xupdate(string str) {
  object obj;
  string file;

  SECURE2(TRUE);
  USAGE2(str, "xup(date) <filename>");
  if(!(file=XFindFile(str))) return TRUE;
  if(LAST(file, 2)==".c") file=LEFT(file, 3);
  if((obj=XFindObj(file))&&(obj=find_object(pure_object_name(obj)))) {
    PrintShort("Updating: ", obj);
    Destruct(obj);
  } else WDLN("Object not found");
  return TRUE;
}

int Xwho(string opt) {
  string *strs;

  SECURE2(TRUE);
  USAGE1(opt, "xwh(o) [mail|ip|race|guild|domain|job|stats|snoop]");
  if(!opt) strs=map(users(), "PlayerWho", TO);
  else switch(opt) {
  case "mail":
    strs=map(users(), "PlayerMail", TO, TRUE);
    break;
  case "ip":
    strs=map(users(), "PlayerIP", TO, TRUE);
    break;
#if MUD_NF || MUD_MG
  case "race":
  case "guild":
    strs=map(users(), "PlayerRace", TO, TRUE);
    break;
#endif
#if MUD_NF
  case "job":
  case "jobs":
    strs=map(users(), "PlayerJobs", TO, TRUE);
    break;
#endif
  case "domain":
    strs=map(users(), "PlayerDomain", TO, TRUE);
    break;
  case "stat":
  case "stats":
    strs=map(users(), "PlayerStats", TO, TRUE);
    break;
  case "snoop":
    strs=map(users(), "PlayerSnoop", TO, TRUE);
    break;
  default:
    return FALSE;
  }
  strs=sort_array(strs, "string_compare", TO);
  WLN(implode(strs,"\n"));
  return TRUE;
}

int inventory() {
  int i;
  object item;

  WLN("Inventory:");
  FORALL(item, cloner) PrintShort(ARIGHT(++i+". ", 4, " "), item);
  return TRUE;
}
