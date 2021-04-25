/*
   a special tool to check rooms
   made by Magictcs - 12 dez 97
   Version: V1.0

   Adapted to bintool for compatibility tests by Softbyte 24sep98
*/

#include <properties.h>
#include <regions.h>
#include <ansi.h>
#include <rooms.h>
#include <msgclass.h>
#include "../bincmd.h"


#define TP this_player()
#define TO this_object();
// right wrap column
#define RWRAP  75
// left wrap column (for output)
#define LWRAP  15
// left wrap minus one (for question)
#define LWRAPM 14

inherit BASICCMD;
inherit "/lib/string";        // yes, we use the string lib

/* 
*  Special bintool stuff 
*/
string * QueryAbrev() {return 0;}
string * QueryCommands() {return ({"check"}); }
string QueryAuthor() {return "Magictcs";}
string QueryVersion() {return "V1.0";}
int QueryEUIDRequired() {return 1;} // maybe 0 is enough
int QueryDestruct() {return 0;} // SMore...
string QueryHelp()
{
  return 
    "SYNOPSIS:\n"
    "          check\n"
    "          check short\n"
    "          check long\n"
    "          check inroom\n"
    "          check regions\n"
    "          check coordinates\n"
    "          check vitems\n"
    "          check items\n"
    "\n"
    "DESCRIPTION:\n"
    "            A small room checker to check some basic properties.\n"
    "            of a room.\n";
}



// variables
object env;
mapping regions;
static string errors;

// property handling:
mapping QueryRegions() { return regions; }

//**********************
// useful help functions
//**********************
// checks if lf at end of string
int check_lf_end(string s) {
  if (closurep(s)) s=funcall(s);
  if (!s) {
    errors+="%^GREEN%^possible forgotten (==0)!%^RESET%^\n";
    return 1;
  }
  if (s[strlen(s)-1]!='\n') {
    errors+="%^GREEN%^no LF at end!%^RESET%^\n";
    return 1;
  }
  return 0;
}

// checks if no lf at end
int check_nolf_end(string s) {
  if (closurep(s)) s=funcall(s);
  if (s[strlen(s)-1]=='\n') {
    errors+="%^GREEN%^LF at end!%^RESET%^\n";
    return 1;
  }
  return 0;
}

// checks if string starts with lowercase
int check_lower_start(string s) {
  if (closurep(s)) s=funcall(s);
//write("s= "+s+"\n");
  if (!s) {
    errors+="%^GREEN%^possible forgotten (==0)!%^RESET%^\n";
    return 1;
  }
  if ( (s[0]<='z') && (s[0]>='a') ) {
    errors+="%^GREEN%^not capitalized!%^RESET%^\n";
    return 1;
  }
  return 0;
}

// checks if string starts with uppercase
int check_upper_start(string s) {
  if (closurep(s)) s=funcall(s);
  if ( (s[0]<='Z') && (s[0]>='A') ) {
    errors+="%^GREEN%^capitalized!%^RESET%^\n";
    return 1;
  }
  return 0;
}

// checks for a max length of string
int check_size(string s,int size) {
  if (closurep(s)) s=funcall(s);
  if (strlen(s)>=size) {
    errors+="%^GREEN%^is longer than "+size+" chars!%^RESET%^\n";
    return 1;
  }
  return 0;
}

// checks if there are additional \n inside the string
int check_add_lf(string s,int max) {
  if (closurep(s)) s=funcall(s);
  if (sizeof(explode(s,"\n"))>(max+1)) {
    errors+="%^GREEN%^More than "+max+" linefeeds inside!%^RESET%^\n";
    return 1;
  }
  return 0;
}

int check_autowrap(string s) {
  string *exp;
  if (closurep(s)) s=funcall(s);
  if (!s) {
    errors+="%^GREEN%^possible forgotten (==0)!%^RESET%^\n";
    return 1;
  }
  exp=explode(s,"\n");
  if (sizeof(exp)!=1) {
    if (sizeof(exp)!=2) {
      errors+="%^GREEN%^More than 1 linefeeds inside\n"
              "--> think about autowrapping!%^RESET%^\n";
      return 1;
    }
    if (exp[1]!="") {
      errors+="%^GREEN%^More than 1 linefeeds inside\n"
              "--> think about autowrapping!%^RESET%^\n";
      return 1;
    }
  }
  return 0;
}

//******************
// checking IntShort
//******************
string check_short() {
  string s;
  errors="";
  s=env->QueryIntShort();
  if (!s)
    return ladjust("IntShort",LWRAPM,".")+
           " %^RED%^No IntShort set!%^RESET%^\n";
  check_lower_start(s);
  check_add_lf(s,0);
  check_size(s,75);
  if ( check_nolf_end(s) )
    return wrap(ladjust("IntShort",LWRAPM,".")+" "+s+errors,RWRAP,LWRAP);
  return wrap(ladjust("IntShort",LWRAPM,".")+" "+s+"\n"+errors,RWRAP,LWRAP);
}

//*****************
// checking IntLong
//*****************
string check_long() {
  string l;
 errors="";
  l=env->QueryIntLong();
  if (!l) {
    return ladjust("IntLong",LWRAPM,".")+
           " %^RED%^No IntLong set!%^RESET%^\n";
  }
  check_lower_start(l);
  check_autowrap(l);
  if ( check_lf_end(l) )
    return wrap(ladjust("IntLong",LWRAPM,".")+" "+l+"\n"+errors,RWRAP,LWRAP);
  return wrap(ladjust("IntLong",LWRAPM,".")+" "+l+errors,RWRAP,LWRAP);
}

//*************
// inroom check
//*************
string check_inroom() {
  mixed in;
  in=env->QueryIndoors();
  if (!intp(in)) {
    return wrap(ladjust("Indoors",LWRAPM,".")+" "+"inside\n"+
           "%^RED%^No integer set!%^RESET%^\n",RWRAP,LWRAP);
  }
  switch(in) {
    case 0:
      return ladjust("Indoors",LWRAPM,".")+" "+"outside"+"\n";
    case 1:
      return ladjust("Indoors",LWRAPM,".")+" "+"inside"+"\n";
    default:
      return wrap(ladjust("Indoors",LWRAPM,".")+" "+"inside\n"+
             "%^GREEN%^but integer != 1 set!%^RESET%^\n",RWRAP,LWRAP);
  }
}

//************
// light check
//************
string check_light() {
  int l;
  l=env->QueryIntLight();
  if (!l)
    return ladjust("IntLight",LWRAPM,".")+" "+l+
           " %^GREEN%^- perhaps forgotten to set IntLight?%^RESET%^\n";
  if ((l<-500) || (l>500))
    return ladjust("IntLight",LWRAPM,".")+" "+l+
           " %^GREEN%^- out of range -500..500%^RESET%^\n";
  return ladjust("IntLight",LWRAPM,".")+" "+l+"\n";
}

//****************
// sunbright check
//****************
string check_sunbright() {
  return ladjust("SunBright",LWRAPM,".")+" "+env->QuerySunBright()+"\n";
}

//***********
// check info
//***********
string check_info() {
  string i;
  i=env->QueryInfo();
  if (!i) return ladjust("Info",LWRAPM,".")+" No info available\n";
  return wrap(ladjust("Info",LWRAPM,".")+" "+i+"\n",RWRAP,LWRAP);
}

//************
// check smell
//************
string check_smell() {
  string s;
  string ret;
  string out;
  string *idx;
  mapping smells;
  int i;
  errors="";
  s=env->QueryIntSmell();
  if (!s) ret=ladjust("IntSmell",LWRAPM,".")+" No IntSmell available\n";
  else {
    check_lower_start(s);
    check_autowrap(s);
    if (s[strlen(s)-1]!='\n')
      ret=wrap(ladjust("IntSmell",LWRAPM,".")+" "+s+
          " %^GREEN%^- forgotten LF at end?%^RESET%^\n"+errors,
          RWRAP,LWRAP);
    else ret=wrap(ladjust("IntSmell",LWRAPM,".")+" "+s+errors,RWRAP,LWRAP);
  }
  smells=env->QuerySmells();
  idx=m_indices(smells);
  if (sizeof(idx)) {
    out=ladjust("Smells",LWRAPM,".")+" ";
    out+=idx[0];
    for (i=1;i<sizeof(idx);i++) {
      out+=", "+idx[i];
    }
    ret+=wrap(out+"\n",RWRAP,LWRAP);
    for (i=0;i<sizeof(idx);i++) {
      errors="";
      check_lower_start(smells[idx[i]]);
      check_lf_end(smells[idx[i]]);
      check_autowrap(smells[idx[i]]);
      if (errors!="") {
        ret+=wrap(mkstr(LWRAP)+idx[i]+" - "+errors,RWRAP,LWRAP);
      }
    }
  }
  return ret;
}

//************
// check noise
//************
string check_noise() {
  string s;
  string ret;
  string out;
  string *idx;
  mapping noise;
  int i;
  s=env->QueryIntNoise();
  if (!s) ret=ladjust("IntNoise",LWRAPM,".")+" No IntNoise available\n";
  else {
    check_lower_start(s);
    check_autowrap(s);
    if (s[strlen(s)-1]!='\n')
      ret=wrap(ladjust("IntNoise",LWRAPM,".")+" "+s+
          " %^GREEN%^- forgotten LF at end?%^RESET%^\n"+errors,
          RWRAP,LWRAP);
    else ret=wrap(ladjust("IntNoise",LWRAPM,".")+" "+s+errors,RWRAP,LWRAP);
  }
  noise=env->QueryNoises();
  idx=m_indices(noise);
  if (sizeof(idx)) {
    out=ladjust("Noises",LWRAPM,".")+" ";
    out+=idx[0];
    for (i=1;i<sizeof(idx);i++) {
      out+=", "+idx[i];
    }
    ret+=wrap(out+"\n",RWRAP,LWRAP);
    for (i=0;i<sizeof(idx);i++) {
      errors="";
      check_lower_start(noise[idx[i]]);
      check_lf_end(noise[idx[i]]);
      check_autowrap(noise[idx[i]]);
      if (errors!="") {
        ret+=wrap(mkstr(LWRAP)+idx[i]+" - "+errors,RWRAP,LWRAP);
      }
    }
  }
  return ret;
}

//***************
// check teleport
//***************
// will return always 3, if wizmode of the player is on!!!!!
string check_teleport() {
  int tport;
  string ret;
  int wizmode;
  wizmode=TP->Query(P_NOWIZ);
  TP->Set(P_NOWIZ, 1);      // off
  tport=env->QueryTPort();
  TP->Set(P_NOWIZ, wizmode);
  ret=ladjust("Teleport",LWRAPM,".")+" ";
  switch (tport) {
    case 0: ret+="No teleport allowed\n";
             break;
    case 1: ret+="Teleport in allowed\n";
             break;
    case 2: ret+="Teleport out allowed\n";
             break;
    case 3: ret+="Teleport in and out allowed\n";
             break;
    default: ret+="%^RED%^- forgotten LF at end?%^RESET%^\n";
             break;
  }
  if (tport!=3) {
    if (!env->QueryTPortReason()) {
      ret+=mkstr(LWRAP)+
           "%^GREEN%^- forgotten to set TPortReason?%^RESET%^\n";
    }
  }
  return ret;
}

//*************
// region check
//*************
string check_region() {
  string ret;
  int reg;
  int i;
  reg=env->Query(P_REGION);
  if (!reg)
    return ladjust("Region",LWRAPM,".")+" %^RED%^NO REGION set!%^RESET%^\n";
  ret=ladjust("Region",LWRAPM,".")+" ";
  for (i=0;i<sizeof(regions);i++) {
    if (reg & (1<<i)) ret+=regions[1<<i]+" "; 
  }
  ret+="\n";
  return wrap(ret,RWRAP,LWRAP);
}

//*************
// coords check
//*************
string check_coords() {
  string ret;
  int *c;
  c=env->QueryCoordinates();
  if (!c)
    return ladjust("Coordinates",LWRAPM,".")+
           " %^RED%^NO COORDS set?!%^RESET%^\n";
  if (sizeof(c)!=3)
    return ladjust("Coordinates",LWRAPM,".")+
           " %^RED%^Wrong number of coords ?!%^RESET%^\n";
  if ((c[0]==0) && (c[1]==0) && (c[2]==0))
    return ladjust("Coordinates",LWRAPM,".")+" 0,0,0 "+
           "%^GREEN%^NO COORDS set?!%^RESET%^\n";
  return ladjust("Coordinates",LWRAPM,".")+
         " ({"+c[0]+","+c[1]+","+c[2]+"})\n";
}

//***************
// check commands
//***************
string check_commands() {
  string ret;
  string *idx;
  int i;
  mapping com;
  com=env->QueryCommands()||([]);
  idx=m_indices(com);
  ret=ladjust("Commands",LWRAPM,".")+" ";
  if (sizeof(idx)) {
    ret+=idx[0];
    for (i=1;i<sizeof(idx);i++) {
      ret+=", "+idx[i];
    }
    ret+="\n";
  } else {
    ret+="<none>\n";
  }
  return wrap(ret,RWRAP,LWRAP);
}

//****************
// check read msgs
//****************
string check_reads() {
  string ret;
  string *idx;
  int i;
  mapping reads;
  reads=env->QueryReadMsgs();
  idx=m_indices(reads);
  ret=ladjust("ReadMsgs",LWRAPM,".")+" ";
  if (sizeof(idx)) {
    ret+=idx[0];
    for (i=1;i<sizeof(idx);i++) {
      ret+=", "+idx[i];
    }
    ret+="\n";
    for (i=0;i<sizeof(idx);i++) {
      errors="";
      check_lower_start(reads[idx[i]]);
      check_lf_end(reads[idx[i]]);
      check_autowrap(reads[idx[i]]);
      if (errors!="") {
        ret+=idx[i]+" - "+errors;
      }
    }
  } else {
    ret+="<none>\n";
  }
  return wrap(ret,RWRAP,LWRAP);
}

//**************
// check details
//**************
string check_details() {
  string ret;
  string *idx;
  int i;
  mapping details;
  details=env->QueryDetails();
  idx=m_indices(details);
  ret=ladjust("Details",LWRAPM,".")+" ";
  if (sizeof(idx)) {
    ret+=idx[0];
    for (i=1;i<sizeof(idx);i++) {
      ret+=", "+idx[i];
    }
    ret+="\n";
    for (i=0;i<sizeof(idx);i++) {
      errors="";
      if (funcall(details[idx[i]])==0) continue;
      check_lower_start(details[idx[i]]);
      check_lf_end(details[idx[i]]);
      check_autowrap(details[idx[i]]);
      if (errors!="") {
        ret+=idx[i]+" - "+errors;
      }
    }
  } else {
    ret+="<none>\n";
  }
  return wrap(ret,RWRAP,LWRAP);
}

//*************
// check vitems
//*************
string check_vitems() {
  string ret;
  mixed *idx;
  int i;
  idx=m_values(env->QueryVItems());
  ret=ladjust("Vitems",LWRAPM,".")+" ";
  if (sizeof(idx)) {
    ret+=val_to_str(idx[0][P_IDS]);
    for (i=1;i<sizeof(idx);i++) {
      ret+=", "+val_to_str(idx[i][P_IDS]);
    }
    ret+="\n";
  } else {
    ret+="<none>\n";
  }
  return wrap(ret,RWRAP,LWRAP);
}

//************
// check items
//************
string add_ref_flag(int r) {
  string ret;
  ret="";
  if (r & REFRESH_DESTRUCT)  ret+="REFRESH_DESTRUCT ";
  if (r & REFRESH_REMOVE)    ret+="REFRESH_REMOVE ";
  if (r & REFRESH_MOVE_HOME) ret+="REFRESH_MOVE_HOME ";
  if (r & REFRESH_HOME)      ret="REFRESH_HOME ";   // = is not a bug !!
                             // do not replace it with += !!
  if (r & REFRESH_RESET)     ret+="REFRESH_RESET ";
  if (r & REFRESH_PROPS)     ret+="REFRESH_PROPS ";
  if (r & REFRESH_FUNC)      ret+="REFRESH_FUNC ";
  return wrap(ret,RWRAP,5);
}

string check_items() {
  string ret;
  mixed *idx;
  int i;
  idx=env->QueryItems();
  ret=ladjust("Items",LWRAPM,".")+" ";
  if (sizeof(idx)) {
    ret+="\""+idx[0][0]+"\" "+add_ref_flag(idx[0][1]);
    for (i=1;i<sizeof(idx);i++) {
      ret+="\n\""+idx[i][0]+"\" "+add_ref_flag(idx[i][1]);
    }
    ret+="\n";
  } else {
    ret+="<none>\n";
  }
  return wrap(ret,RWRAP,LWRAP);
}

//************
// check exits
//************
string add_exit_hidden(int e) {
  string ret;
  ret="";
  if (e & HIDE_OPEN)   ret+=" %^BLUE%^HIDE_OPEN%^RESET%^";
  if (e & HIDE_CLOSED) ret+=" %^BLUE%^HIDE_CLOSED%^RESET%^";
  if (e & HIDE_ALWAYS) ret=" %^BLUE%^HIDE_ALWAYS%^RESET%^";
  if (e & HIDE_BOTH)   ret+=" %^BLUE%^HIDE_BOTH%^RESET%^";
  return ret;
}

string check_exits() {
  string ret;
  mixed *idx;
  mapping exits;
  int i;
  exits=env->QueryExits();
  idx=m_indices(exits);
  ret="Exits:\n";
  if (sizeof(idx)) {
    ret+=ladjust(" "+idx[0],LWRAPM-2,".");
    ret+=" "+val_to_str(exits[idx[0]])+add_exit_hidden(exits[idx[0],3]);
    ret+="\n";
    for (i=1;i<sizeof(idx);i++) {
      ret+=ladjust(" "+idx[i],LWRAPM-2,".");
      ret+=" "+val_to_str(exits[idx[i]])+add_exit_hidden(exits[idx[i],3]);
      ret+="\n";
    }
    ret+="\n";
  } else {
    ret+="<none> - %^GREEN%^ Exits are missing?%^RESET%^\n";
  }
  return wrap(ret,RWRAP,2);
}

//*************
// main parsing
//*************
int cmd_check(string str) {
  string out;

  if (CheckHelp(str)) return 1;  // Softbyte for the bintool

  env=environment(TP);
  if (!env) return 0;
  out=ladjust("Checking",LWRAPM,".")+" %^BLUE%^"+object_name(env)+"%^RESET%^\n";
  switch (str||"") {
    case "": out+=check_short();
             out+=check_long();
             out+=check_inroom();
             out+=check_light();
             out+=check_sunbright();
             out+=check_info();
             out+=check_smell();
             out+=check_noise();
             out+=check_teleport();
             out+=check_reads();
             out+=check_region();
             out+=check_coords();
             out+=check_commands();
//             out+=check_details();
             out+=check_vitems();
             out+=check_items();
             out+=check_exits();
         break;
    case "short": out+=check_short();
         break;
    case "long": out+=check_long();
         break;
    case "inroom": out+=check_inroom();
         break;
    case "smell": out+=check_smell();
         break;
    case "noise": out+=check_noise();
         break;
    case "region": out+=check_region();
         break;
    case "light": out+=check_light();
         break;
    case "coordinates": out+=check_coords();
         break;
    case "vitems": // out+=check_vitems();
         break;
    case "items": // out+=check_items();
         break;
  }
  SMore(out);
  return 1;
}

//*************************************
// load all regions from /sys/regions.h
//*************************************

// c is type of "0x00100" or similar
// bad coding style, but working very fast!
int convert_to_int(string c) {
  switch (c) {
    case "0x00000001": return 1;
    case "0x00000002": return 1<<1;
    case "0x00000004": return 1<<2;
    case "0x00000008": return 1<<3;

    case "0x00000010": return 1<<4;
    case "0x00000020": return 1<<5;
    case "0x00000040": return 1<<6;
    case "0x00000080": return 1<<7;

    case "0x00000100": return 1<<8;
    case "0x00000200": return 1<<9;
    case "0x00000400": return 1<<10;
    case "0x00000800": return 1<<11;

    case "0x00001000": return 1<<12;
    case "0x00002000": return 1<<13;
    case "0x00004000": return 1<<14;
    case "0x00008000": return 1<<15;

    case "0x00010000": return 1<<16;
    case "0x00020000": return 1<<17;
    case "0x00040000": return 1<<18;
    case "0x00080000": return 1<<19;

    case "0x00100000": return 1<<20;
    case "0x00200000": return 1<<21;
    case "0x00400000": return 1<<22;
    case "0x00800000": return 1<<23;

    case "0x01000000": return 1<<24;
    case "0x02000000": return 1<<25;
    case "0x04000000": return 1<<26;
    case "0x08000000": return 1<<27;

    case "0x10000000": return 1<<28;
    case "0x20000000": return 1<<29;
    case "0x40000000": return 1<<30;
    case "0x80000000": return 1<<31;
  }
}

void load_regions() {
  string *regs;
  string *reg;
  int i;
  regs=explode(read_file("/sys/regions.h"),"\n");
  regs=regexp(regs,
       "^#define[ \t]*REGION_[a-zA-Z]*[ \t]*0x[0-9]*");
  for (i=0;i<sizeof(regs);i++) {
    reg=regexplode(regs[i],"^#define[ \t]*REGION_");
    reg=norm_id(reg[2],1);
    regions+=([ convert_to_int(reg[1]) : reg[0] ]);
  }
}

//********************
// the create as usual
//********************
create() {
  ::create();
  regions=([]);
  load_regions();
}

