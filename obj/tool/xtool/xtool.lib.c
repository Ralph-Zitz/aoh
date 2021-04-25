/*----------------------------------------------------------------------*/
/* The Xtool - Copyright 1992 by Hyp                                    */
/*                                                                      */
/* File: xtool.lib.c                                                    */
/*----------------------------------------------------------------------*/

#include "dependencies.h"
#if MUD_OSB
#include <driver/lpctypes.h>
#else
#include <lpctypes.h>
#endif

#define TO     this_object()
#define TP     this_player()
#define ENV    environment()

#if MUD_NF || MUD_MG
#ifndef MASTER
#define MASTER "/secure/master"
#endif
#define RNAME  getuid(TP)
#endif
#if MUD_TA || MUD_BB || MUD_TM
#define MASTER "/obj/master"
#define RNAME  (string)TP->query_real_name()
#endif

#if MUD_NF
#define PWD    (string)TP->QueryCurrentDir()
#endif
#if MUD_MG
#define PWD    (string)TP->Query(P_CURRENTDIR)
#endif
#if MUD_TA || MUD_BB || MUD_TM
#define PWD    (string)TP->query_path()
#endif

#if MUD_NF
#define SUBSTR(x,y,z)      (x)[y..z]
#define FIRST(x,y)         (x)[0..y]
#define LAST(x,y)          (x)[<y..]
#define RIGHT(x,y)         (x)[y..]
#define LEFT(x,y)          (x)[0..<y]
#else
#define SUBSTR(x,y,z)      extract(x,y,z)
#define FIRST(x,y)         extract(x, 0, y)
#define LAST(x,y)          extract(x, -(y), -1)
#define RIGHT(x,y)         extract(x, y, -1)
#define LEFT(x,y)          extract(x, 0, -(y))
#endif

string *old_explode(string str, string del) {
  int s, t;
  string *strs;

  if(del == "")
    return ({str});
  strs=efun::explode(str, del);
  t=sizeof(strs)-1;
  while(s<=t && strs[s++] == "");s--;
  while(t>=0 && strs[t--] == "");t++;
  if(s<=t)
    return strs[s..t];
  return 0;
}

string *new_explode(string str, string del) {
  return efun::explode(str, del);
}

string *strip_explode(string str, string del) {
  return efun::explode(str, del)-({""});
}

#ifdef __ALISTS__
mixed *remove_alist(mixed key, mixed *alist) {
  int i, s, p, l;

  if(!pointerp(alist)) return 0;
  if((s=sizeof(alist))<2) return 0;
  if((p=assoc(key, alist[0]))>=0) {
    l=sizeof(alist[0])-1;
    for(i=0;i<s;i++) {
      if(!pointerp(alist[i])) return 0;
      alist[i]=alist[i][0..p-1]+alist[i][p+1..l];
    }
  }
  return alist;
}
#endif

string cap_string(string str) {
  return capitalize(str);
}

#if MUD_NF || MUD_MG
string short_path(string file) {
  int s;

  if(!file) return 0;
  if(TP) {
    if(FIRST(file, 8)=="/players/") {
      s=sizeof(RNAME)+8;
      return "~"+(SUBSTR(file, 9, s)==RNAME ?
		  RIGHT(file, s+1) : RIGHT(file, 9));
    }
  }
  if(FIRST(file, 2)=="/d/") return "+"+RIGHT(file, 3);
  if(FIRST(file, 2)=="/p/") return "&"+RIGHT(file, 3);
  else return file;
}
#endif
#if MUD_TA
string short_path(string file) {
  int s;
  string tmp;

  if(!file) return 0;
  if(TP) {
    if(FIRST(file, 1)=="w/") {
      s=sizeof(RNAME)+1;
      return "~"+(SUBSTR(file, 2, s)==RNAME ?
		  RIGHT(file, s+1) : RIGHT(file, 2));
    }
  }
  if(FIRST(file, 1)=="d/") return "+"+RIGHT(file, 2);
  else return file;
}
#endif
#if MUD_BB || MUD_TM
string short_path(string file) {
  int s;
  string tmp;

  if(!file) return 0;
  if(TP) {
    if(FIRST(file, 7)=="players/") {
      s=sizeof(RNAME)+7;
      return "~"+(SUBSTR(file, 8, s)==RNAME ?
		  RIGHT(file, s+1) : RIGHT(file, 8));
    }
  }
  if(FIRST(file, 7)=="domains/") return "+"+RIGHT(file, 8);
  else return file;
}
#endif

string long_path(string file) {
#if MUD_NF || MUD_MG
  return ({string})MASTER->make_path_absolute(file);
#endif
#if MUD_TA || MUD_BB || MUD_TM
  return "/"+({string})MASTER->make_path_absolute(file);
#endif
}

string pure_object_name(object obj) {
#if MUD_NF || MUD_MG
  return obj ? explode(object_name(obj), "#")[0] : 0;
#endif
#if MUD_TA || MUD_BB || MUD_TM
  return obj ? explode("/"+object_name(obj), "#")[0] : 0;
#endif
}

string source_object_name(object obj) {
#if MUD_NF || MUD_MG
  return obj ? explode(object_name(obj), "#")[0]+".c" : 0;
#endif
#if MUD_TA || MUD_BB || MUD_TM
  return obj ? explode("/"+object_name(obj), "#")[0]+".c" : 0;
#endif
}

string *long_get_dir(string pat, int all) {
  int i, s;
  string str, dir, *file, *tmp;

  if(!pat) return ({});
  pat=long_path(pat);
  if(tmp=explode(pat, "/")) dir="/"+implode(tmp[0..sizeof(tmp)-2], "/")+"/";
  else dir="/";
  s=sizeof(tmp=get_dir(pat));
  file=({});
  for(i=0;i<s;i++) {
    str=dir+tmp[i];
    if(all||file_size(str)>=0) file+=({str});
  }
  return file;
}

int string_compare(string str1, string str2) {
  if(str1==str2) return 0;
  if(str1>str2) return 1;
  else return -1;
}

string string_replace(string str, string old, string new) {
  return implode(efun::explode(str, old), new);
}

string lit_string(string str) {
  str=string_replace(str, "\\", "\\\\");
  str=string_replace(str, "\b", "\\b");
  str=string_replace(str, "\n", "\\n");
  str=string_replace(str, "\r", "\\r");
  str=string_replace(str, "\t", "\\t");
  return string_replace(str, "\"", "\\\"");
}

string mixed_to_string(mixed mix, int lvl) {
  int i, j, s, t;
  string str;
  mixed *keys;

  if(lvl) {
    switch(typeof(mix)) {
    default:
    case T_INVALID:
      return "<invalid>";
    case T_LVALUE:
      return "&"+mixed_to_string(mix, lvl-1);
    case T_NUMBER:
    case T_FLOAT:
      return to_string(mix);
    case T_STRING:
      return "\""+lit_string(mix)+"\"";
    case T_POINTER:
      return "({"+implode(map(mix,"mixed_to_string",TO,lvl-1),",")+"})";
    case T_OBJECT:
      return "["+short_path(object_name(mix))+"]";
    case T_MAPPING:
      s=sizeof(keys=m_indices(mix));
      t=get_type_info(mix, 1);
      str="([";
      for(i=0;i<s;i++) {
	str+=mixed_to_string(keys[i], lvl-1);
	if(t) {
	  str+=":"+mixed_to_string(mix[keys[i],0], lvl-1);
	  for(j=1;j<t;j++)
	    str+=";"+mixed_to_string(mix[keys[i],j], lvl-1);
	}
	if(i<s-1) str+=",";
      }
      return str+"])";
    case T_CLOSURE:
    case T_SYMBOL:
      return sprintf("%O", mix);
    case T_QUOTED_ARRAY:
      return "'"+mixed_to_string(funcall(lambda(0, mix)), lvl-1);
    }
  }
  return "...";
}

// string mixed_to_string(mixed mix, int lvl) {return sprintf("%O",mix);}

int is_player(object obj) {
#if MUD_NF || MUD_MG
  return obj&&obj==find_player(getuid(obj));
#endif
#if MUD_TA || MUD_BB || MUD_TM
  return obj&&(obj->query_real_name()&&
	       obj==find_player((string)obj->query_real_name()));
#endif
}

int is_not_player(object obj) {
#if MUD_NF || MUD_MG
  return obj&&obj!=find_player(getuid(obj));
#endif
#if MUD_TA || MUD_BB || MUD_TM
  return obj&&!(obj->query_real_name()&&
		obj==find_player((string)obj->query_real_name()));
#endif
}

int is_living(object obj) {
  return obj&&living(obj);
}
int is_not_living(object obj) {
  return obj&&!living(obj);
}
int is_netdead(object obj) {
  return is_player(obj)&&!interactive(obj);
}
int is_alive(object obj) {
  return is_player(obj)&&interactive(obj);
}
int is_snoopee(object who) {
  return who&&objectp(query_snoop(who));
}
int is_nosnoop(object who) {
  return who&&!objectp(query_snoop(who));
}
int is_invis(object who) {
  string str;

  if(!who) return 0;
#if MUD_NF || MUD_MG
  str=({string})who->QueryShort();
#endif
#if MUD_TA || MUD_BB || MUD_TM
  str=(string)who->short();
#endif
  return !str || !str[0];
}
int is_visib(object who) {
  string str;

  if(!who) return 0;
#if MUD_NF || MUD_MG
  str=({string})who->QueryShort();
#endif
#if MUD_TA || MUD_BB || MUD_TM
  str=(string)who->short();
#endif
  return str&&str[0];
}
