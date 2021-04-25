// This tool allows you to add DETAILS to a room in the mud without
// editing it. You only need to stand in the room and then you have
// a couple of commands to edit and add details
//
// The DETAILMAKER has been written by Softbyte 10.Dez.1996 as a
// special present to Eilan !!
//
//
//
//
// 10dec96: V0.1b  Creation by Softbyte
// 29dec96: V0.5b  Administration of missing details in rooms
// 09jan97: V0.9b  Questions of NPCs can be added as well
// 25jan97: V0.95b Autoindent and parameters
//
// TODO: Combine and remove details 
//       autoindent


#define VERSION "V1.00"

#include <properties.h>
#include <rooms.h>
#include <moving.h>
#include <config.h>
#include <secure/wizlevels.h>
#include <string.h>

inherit "/std/thing";

#define TP this_player()
#define TO this_object()
#define SLIB "/lib/string"

#define SFB_ID "Softbyte's Detailmaker"
#define LAST_UPDATE "Last update:"
#define MAXOUTPUTCNT 10    // So many outputs per call_out
#define DETAILSERVER "/obj/detailserver"
/* ------------------------------------------------------------ */ 
// Save Variables of the detailmaker
int indent1,indent2;         // how many spaces will be indented
int autoindent1,autoindent2; //internal use for saving the indent widths
int wrap;                    // 0: No wrapping, else wrap width, e.g. 78 
int nocomment;               // 1: No commenting of changes
int nologging;               // 1: No toplist logging
int nobackup;                // 1: No backup file written  
int autoindent;              // 1: Autoindenting is tried
int d_cnt,q_cnt;             // Topscorelist for fixed details
/* ------------------------------------------------------------ */ 

// Variables of the detailmaker
static string indentstr1,indentstr2;
static object glob_tp;
static string *glob_newdets;
static object glob_npc,glob_room;
static string glob_defaultnpc;
static object owner;
object QueryOwner() {return owner;}
string QueryOwnerName() 
{
  if (!owner) return "Nobody";
  else return capitalize(owner->QueryRealName());
}

/* ------------------------------------------------------------ */ 
// Variables of the missing detail administrator
private static string *roomindex;
private static int running=0;
private static string actualroom=0;
static mapping domain;

// Checks whether a command is already running via call_out
int QueryRunning() {return running;}
int SetRunning(int i) {running=i;return 1;}

// Sets the actual room which is processed by default
string QueryActualRoom()
{
  if (actualroom) return actualroom;
  else if (TP) return blueprint(environment(TP));
  else return "/std/void";
}
int SetActualRoom(mixed arg)
{
  if (!arg) actualroom=0;
  else if (stringp(arg)) actualroom=arg;
  else if (intp(arg) && arg<=sizeof(roomindex)) actualroom=roomindex[arg-1];
  else return 0;
  return 1;
}
// Loads a string room and returns its object room
// reports all errors to owner
object GetRoomObject(string sroom)
{
string err;
object oroom;
  oroom=0;
  if (!sroom || !owner) return 0;
  if (err = catch(oroom=load_object(sroom)))
    tell_object(owner,"Cannot load room, reason:"+err+"\n");
  return oroom;
}
varargs string QuerySaveName(string appendix)
{
  return "/players/"+getuid(owner)+"/.detailmaker"+(appendix||"");
}
/* Saveobject */
int SaveMe()
{
  save_object(QuerySaveName());
  return 1;
}
/* ------------------------------------------------------------ */ 
// Creates the indent strings
void MakeIndent()
{
  indentstr1=SLIB->mkstr(indent1);
  indentstr2=SLIB->mkstr(indent2);
}
/* ------------------------------------------------------------ */ 
int Debug(string str)
{
object ob;
  ob=find_player("softbyte");
  if (!ob) return 0;
  tell_object(ob,blueprint(this_object())+"::"+str);
  return 1;
}
/* ------------------------------------------------------------ */ 
// The DETAILMAKER
/* ------------------------------------------------------------ */ 
// makes a string of the format #'closure out of a closure
string MakeClosure(closure cl)
{
  return "#'"+to_string(cl);
}
/* ------------------------------------------------------------ */ 
// Try to find out whether the Detailmaker had already been run on this file 
// For this the idstring "id" is searched and the char after the next \n is
// returned as aktual possition. &pos contains the position of the found id
int QuickFindId(string s,string id,int pos)
{
int i;
  i=strstr(s,id);
  pos=i;
  if (i<0) return -1;
  i=strstr(s,"\n",i);
  if (i<0) return -2;
  else return i+2;
}
// Tries to find a CR backwards from current position
int FindCRReverse(string s,int pos)
{
  while(pos>=0)
  {
//    printf("%c",s[pos]);
    if (s[pos]=='\n') return pos+1;
    pos--;
  }
  return -1;
}

// Checks whether the line after the ID line is an Last update: line
// Returns the char after its \n
int FindLastUpdate(string s,int pos)
{
int p1,p2,p3;
  p1=strstr(s,LAST_UPDATE,pos);
  if (p1<0) return -1;
  p2=strstr(s,"\n",pos);
  if (p2<0) return -2;
  p3=strstr(s,"\n",p2+1);
  if (p3<0) return -3;
  if (p1>p3) return -4;
  return p2+1;
}

// Find the create function beginning from pos
// &pos later marks the finding of "create" -> allows multiple search
int FindCreate(string s,int pos)
{
int i,j1,j2,j3,e1;

  i=strstr(s,"create",pos);    // search create
  pos=-1;
  if (i<0) return -1;
  pos=i;
  j1=strstr(s,"(",i);      // search Brackets ()
  if (j1<0) return -2;
  j2=strstr(s,")",j1);   
  if (j2<0) return -3;
  e1=strstr(s,"\n",i);    // check for return before the brackets 
  if (e1<j2) return -4;
  j3=strstr(s,"{",j2);   
  if (j3<0) return -5;
  return j3;
}
// Returns the position of a searchstring like "AddDetail"
// &start contains the position of the found string
// It is only searched between position p1 and p2
int FindAddFunction(string s,string searchwhat,int p1,int p2,int start)
{
int i;
  i=strstr(s,searchwhat,p1);   // seek AddDetail
  start=i-1;
  if (i<0 || i>p2) return -1;
  i=strstr(s,"(",i);           // and its bracket
  if (i<0 || i>p2) return -2;
  return i;
}
// Find Corresponding brackets given with flag
#define ID_BRACKET_SQUARE 2 
#define ID_BRACKET_NORMAL 1 
#define ID_BRACKET_CURLED 0 
varargs int FindBracket(string s,int p1,int flag)
{
int cnt,p2,len,p_o,p_c;
string b_o,b_c;
  if (flag==ID_BRACKET_NORMAL) 
  {
    b_o="(";
    b_c=")";
  }
  else if (flag==ID_BRACKET_SQUARE) 
  {
    b_o="[";
    b_c="]";
  }
  else
  {
    b_o="{";
    b_c="}";
  }
  cnt=0;
  p2=p1;
  p_o=-1;
  p_c=-1;
  len=strlen(s);
  do
  {
   if (p2>p_o) p_o=strstr(s,b_o,p2);
   if (p2>p_c) p_c=strstr(s,b_c,p2);
   if (p_c<0 && p_o<0)
   {
     p2=len;
   }
   else if (p_c<0 ||  (p_o<p_c && p_o>=0) )   // found bracket open
   {
     cnt++;
     p2=p_o+1;
   }
   else if ( p_o<0 || (p_c<p_o && p_c>=0) )   // found bracket close
   {
     cnt--;
     p2=p_c+1;
   }
  }while(cnt>0 && cnt<10 && p2<len);
  return p2;
}

// Is the Detail key already in another detail newdets contained
string IsInDetail(string key,mapping newdet)
{
int i;
string *new_ind;
 new_ind=m_indices(newdet); 
 for (i=0;i<sizeof(new_ind);i++)
 {
   if (newdet[new_ind[i],0]==key)
   {
     return new_ind[i];
   }
 }
 return 0;
}
// print out the details of the room room
// flag=1: short output, 0=long output
// mode=0: details of a room are meant
// mode=1: questions of a npc are mean
string GetAllDetails(object room,int flag,int mode)
{
mapping details;
string *det_ind;
string *alt;
string fit,s,tmp;
int i,j;
mapping newdet;
mixed arg;
  if (mode==1) details=room->QueryQuestions();
  else details=room->QueryDetails();
  if (!details)
  {
    if (mode==1) return "There are no questions defined.\n";
    return "There are no details definded.\n";
  }
  det_ind=m_indices(details);
  newdet=m_allocate(sizeof(det_ind),2);
  newdet=([ det_ind[0]:details[det_ind[0]];({})  ]);
  for (i=1;i<sizeof(det_ind);i++)
  {
    if ((fit=IsInDetail(details[det_ind[i]],newdet))!=0)
    {
      newdet[fit,1]=newdet[fit,1]+({det_ind[i]});
    }
    else
    {
      newdet+=([det_ind[i]:details[det_ind[i]];({}) ]);
    }
  }
  s="";
  det_ind=m_indices(newdet);
  for (i=0;i<sizeof(det_ind);i++)
  {
    alt=newdet[det_ind[i],1];
    tmp=det_ind[i];
    if (sizeof(alt)) tmp+=", ";
    for (j=0;j<sizeof(alt);j++)
    {
      tmp+=alt[j];
      if (j<sizeof(alt)-1) tmp+=", ";
    }
    if (flag!=1) tmp+=":\n";
    else tmp+=".\n";
    s+=SLIB->wrap(tmp,78,2);
    if (flag!=1)  // not short output
    {
      arg=details[det_ind[i]];
      if (mode==1)
        s+=SLIB->wrap("    "+
          (closurep(arg)?(MakeClosure(arg)+"\n"):to_string(arg)),78,4);
      else
      {
        s+=SLIB->wrap("    "+room->GetDetail(det_ind[i]),78,4);
        if (closurep(arg)) s+=SLIB->wrap("    <"+MakeClosure(arg)+">\n",78,4);
      }
    }
  }
  return s;
}
/* ------------------------------------------------------------ */ 
// Creates the new function, e.g. AddDetail which will be inserted
// into the file
// keys are the new keywords, str is the text or a clousure and 
// mode=0: details of a room are meant
// mode=1: questions of a npc are mean
string FunctionToCommand(string *keys,mixed str,int mode)
{
string s,res;
string *newsp;
int i;
  if (mode==1) s=indentstr1+"AddQuestion( ({";
  else s=indentstr1+"AddDetail( ({";
  res="";

  for (i=0;i<sizeof(keys);i++)
  {
    if (wrap>0 && strlen(s)+strlen(keys[i])+3 >wrap)
    {
      res+=s+"\n";
      s=indentstr2;
    }
    s+="\""+keys[i]+"\"";
    if (i<sizeof(keys)-1) s+=",";
    else s+="}),";
  }
  res+=s;
  if (stringp(str) && str[0..1]=="#'")   // input a closure 
  {
    s=(explode(str,"\n")-({})-({""}))[0]; 
  }
  else if (stringp(str))
  {
    if (wrap>0) str=SLIB->wrap(str,wrap,0); // Wrapping on
    s="\n";
    s+=indentstr2+"\"";
    newsp=explode(str,"\n")-({})-({""});
    s+=implode(newsp, "\\n\"\n"+indentstr2+"\"");
    s+="\\n\"";
    // tmp=SLIB->lit_string(str); // regreplace(str,"\n","\\n",1);
  }
  else
  {
    s=MakeClosure(str);
  }
  res+=s;
  res+=");\n";
  return res;
}
int FindAutoindent(string s,int pos)
{
int i,pos_n;
int flag;
  if (autoindent==0) return -2;  // autoindent disabled 
  pos_n=strstr(s,"\n",pos);      // search CR
  if (pos_n<0) return -1;
  for (i=pos;i<pos_n;i++)
  {
    if (s[i]!=' ')  // Found first indent
    {
      indent1=i-pos;
      break;
    }
  }
  flag=0;
  do 
  {
    for (i=pos_n+1;i<pos_n+15;i++)
    {
      if (s[i]!=' ' && s[i]=='\"')  // Found second indent
      {
	indent2=i-pos_n-1;
	flag=1;
	break;
      }
      else if (s[i]!=' ')
      {
        break;
      }
    } 
    if (flag==0) pos_n=strstr(s,"\n",i);  // search CR
  }while(pos_n>0 && flag==0);
  MakeIndent();
  return 1;
}
/* ------------------------------------------------------------ */ 
// mode=0: details of a room are meant
// mode=1: questions of a npc are mean
int MakeDetail(object tp,string *newdets,mixed oldstr,object env,int mode)
{
string room,s,news,tmp;
string *res;
int pos1,pos2,pos3,pos4,i,j,len,cpos;

  if (!tp || !IS_WIZARD(tp) || tp!=owner) return 0;
  room=blueprint(env);
  if (file_size(room+".c")<=0)
  {
    tell_object(tp,"The file '"+room+".c' does not exist.\n");
    return 1;
  }
  if (mode==1) tell_object(tp,"NPC:"+room+".c\n");
  else tell_object(tp,"Room:"+room+".c\n");
  s=read_file(room+".c");
  pos1=-1;
  if (nocomment) pos2=-1;  // force first time use
  else pos2=QuickFindId(s,SFB_ID,&pos3);
  if (pos2<0)
  {
    tell_object(tp,"File had not yet been edited with "+SFB_ID+".\n");
    cpos=0;
    do
    {
      pos1=FindCreate(s,&cpos);
      if (pos1<0 && cpos>0) write("Omitted a strange create ("+pos1+"/"+cpos+")...\n");
      if (cpos>0) cpos++;   // to avoid searching the same create over and over again
    }while(pos1<0 && cpos>0);
    if (pos1<0)
    {
      tell_object(tp,"Cannot find create ("+pos1+").\n");
      return 1;
    }
    pos2=FindBracket(s,pos1,ID_BRACKET_CURLED);
    if (pos2<0)
    {
      tell_object(tp,"Cannot find end of create ("+pos2+").\n");
      return 1;
    }
//  If Details should be added after old details
    if (mode==1) pos3=FindAddFunction(s,"AddQuestion",pos1,pos2,&pos4);
    else pos3=FindAddFunction(s,"AddDetail",pos1,pos2,&pos4);
    if (pos3>pos2 || pos3<0)
    {
      write("Could not find an AddDetail/Question line...proceeding anyways.\n");
    }
    else
    {
      pos3=FindCRReverse(s,pos4);
      if (pos3<0 && pos3<pos1)
      {
	write("Could not find an CR in and AddDetail/Question line...\n");
      }
      else 
      {
        pos2=pos3+1;    // Insert before AddDetails
        FindAutoindent(s,pos3);
      }
    }
  }
  else
  {
    tell_object(tp,"File has already been edited with "+SFB_ID+".\n");
  }

// Check whether we are run the first time
  if (pos1>=0)
  {
    if (nocomment) news="";   // don't comment the changes
    else
    {
      // Write the copyright stuff...
      news="// --- Added with "+SFB_ID+" by "+
	 capitalize(TP->QueryRealName())+"\n";
      news+="// "+LAST_UPDATE+" "+
	 capitalize(TP->QueryRealName())+" at "+ctime()+"\n";
    }
    if (nobackup)   // don't create backups
    {
      rm(room+".c");
    }
    else 
    {
      rm(room+".bak");
      rename(room+".c",room+".bak");
    }
    pos3=pos2;
  }
  else
  {
    if (nocomment)
    {
      news="";   // don't comment the changes
      pos3=pos2;
    }
    else
    {
      // Write the copyright stuff...
      if (strstr(s[(pos3+1)..pos2],capitalize(TP->QueryRealName()))<0)
      {
	news=", "+capitalize(TP->QueryRealName())+"\n";
	pos3=pos2;
	pos2-=1;   // Cut the CR
      }
      else   // TP has already added a detail/question
      {
	news="";
	pos3=pos2;
      }
      pos4=FindLastUpdate(s,pos3);
      if (pos4>0) pos3=pos4+1;
      news+="// "+LAST_UPDATE+" "+
	 capitalize(TP->QueryRealName())+" at "+ctime()+"\n";
      FindAutoindent(s,pos3-1);
    }

    rm(room+".c");
  }
// Now add the details/questions
  news+=FunctionToCommand(newdets,oldstr,mode);

// And write the file
  i=write_file(room+".c",s[0..(pos2-2)]+news+s[(pos3-1)..<1]);
  if (!i)  // shit ...
  {
    tell_object(tp,"Could not write the file '"+room+".c'. Try to:\n"+
          "       'cp "+room+".bak "+room+".c'.\n"+
          "to restore the old file.\n");
    // Try to rescue the file
    tmp="detail_"+to_string(random(100000))+".bak";
    i=log_file("detailmaker/"+tmp,"//FILE: "+room+"\n"+s);
    if (i) tell_object(tp,"File rescued to /log/detailmaker/"+tmp+".\n");
  }
  else
  {
    if (stringp(oldstr) && oldstr[0..1]=="#'")   // input a closure 
    {
      tmp=oldstr[2..<1];
      tmp=(explode(tmp,"\n")-({})-({""}))[0]; 
      oldstr=call_other(env,tmp);
    }
    if (mode==1)
    {
      env->AddQuestion(newdets,oldstr);
      q_cnt++;  // Fixed one more question
      tell_object(tp,"Added questions '"+implode(newdets,", ")+
        "' and wrote file.\n");
    }
    else
    {
      env->AddDetail(newdets,oldstr);
      d_cnt++;  // Fixed one more detail
      // Removing new details from the missing detail list
      for (i=0;i<sizeof(newdets);i++) 
        DETAILSERVER->RemoveDetail(newdets[i],env,0);
      tell_object(tp,"Added details '"+implode(newdets,", ")+
        "' and wrote file.\n");
    }
    DETAILSERVER->AddToplist(tp,d_cnt,q_cnt,nologging);
    SaveMe();
  }
  return 1;
}
question_done(string str)
{
  if (!str)
  {
    tell_object(glob_tp,"Question cancelled.\n");
    return;
  }
  MakeDetail(glob_tp,glob_newdets,str,glob_npc,1); 
}
detail_done(string str)
{
  if (!str)
  {
    tell_object(glob_tp,"Detail cancelled.\n");
    return;
  }
  MakeDetail(glob_tp,glob_newdets,str,glob_room,0);
}
// mode=0: Details, mode==1: Question
string *CheckDetails(string *dets,object env,int mode)
{
string *orgdet;
string *omit;
int i;
  if (mode==1) orgdet=m_indices(env->QueryQuestions()||([]));
  else orgdet=m_indices(env->QueryDetails()||([]));
  omit=({});
  for (i=0;i<sizeof(dets);i++)
  {
    if (member(orgdet, dets[i])>=0) omit+=({dets[i]});
  }
  for (i=0;i<sizeof(omit);i++)
  {
    dets-=({omit[i]});
    if (mode==1) write("Question '"+omit[i]+"' already present.\n");
    else write("Detail '"+omit[i]+"' already present.\n");
  }
  return dets;
}

/* ------------------------------------------------------------ */ 
// detail <detail> same as <olddetail>: Add another keyword to an old detail
// detail <detail> new                : Ask for the input of a new detail
int cmd_detail(string str)
{
object env;
string newdet,olddet;
mixed oldstr;
string *newdets;
  if (!TP || !IS_WIZARD(TP) || TP!=owner) return 0;
  if (!str) return notify_fail("Detail what?\n"),0;
  env=environment(TP);
  glob_room=env;  // Setting the ctual room
  if (2==sscanf(str,"%s same as %s",newdet,olddet))
  {
    newdets=explode(newdet,",");
    newdets=CheckDetails(newdets,env,0);
    oldstr=env->GetDetail(olddet);
    if (!oldstr) return
      notify_fail(olddet+" is no detail in this room.\n"),0;
    if (!sizeof(newdets)) return
      notify_fail("No NEW detail given.\n"),0;
    oldstr=env->QueryDetails()[olddet];   // maybe we are a closure ...
    MakeDetail(TP,newdets,oldstr,glob_room,0);
  }
  else if (1==sscanf(str,"%s new",newdet))
  {
    newdets=explode(newdet,",");
    newdets=CheckDetails(newdets,env,0);
    if (!sizeof(newdets)) return
      notify_fail("No NEW detail given.\n"),0;
    write("Please enter the text for the detail(s):\n");
    glob_tp=TP;
    glob_newdets=newdets; 
    SLIB->input_text("detail_done"); // start editor 
  }
  else 
  {
    return 
      notify_fail("Cannot understand the detail command.\n"),0;
  }
  return 1;
}
/* ------------------------------------------------------------ */ 
// question <ask> same as <oldask>: Add another keyword to an old question
// question <ask> new             : Ask for the input of a new question
// Thje NPC for which the question is will be prompted
int cmd_question(string str)
{
object env;
string *newdets;
object *npcs;
object *newnpcs;
int i,cnt,flag;
  if (!TP || !IS_WIZARD(TP) || TP!=owner) return 0;
  if (!str) return notify_fail("Question what?\n"),0;
  env=GetRoomObject(QueryActualRoom());
  npcs=filter_objects(all_inventory(env),"QueryIsNPC");
  glob_tp=TP;
  if (!npcs || !sizeof(npcs) )
    return notify_fail("There is no NPC present.\n"),0;
  newdets=({});
  newnpcs=({});
  cnt=0;
  flag=0;
  for (i=0;i<sizeof(npcs);i++)
  {
    if (member(newdets,blueprint(npcs[i]))<0)
    {
      if (glob_defaultnpc==blueprint(npcs[i]))
      {
        write("*");
        flag=1;
	glob_npc=npcs[i];
      }
      else 
      {
        write(" ");
      }
      write(sprintf("%2d: %s (%s)\n",cnt+1,blueprint(npcs[i]),npcs[i]->QueryShort()));
      newdets+=({blueprint(npcs[i])});
      newnpcs+=({npcs[i]});
      cnt++;
    }
  } 
  if (flag==0)
  {
    glob_defaultnpc=0;  // No default NPC yet
    write("Please choose NPC [1.."+(cnt)+"]: ");
  }
  else write("Please choose NPC [return for default npc marked by the *]: ");
  input_to("choose_npc",0,str,newnpcs);
  return 1;
}
void choose_npc(string input,string str,object *npcs)
{
int nr;
mixed oldstr;
string *newdets;
string newdet,olddet,npc;
  if (!input && !glob_defaultnpc)
  {
    tell_object(glob_tp,"Selection cancelled.\n");
    return;
  }
  else if (input) 
  {
    if (!input || input=="")
    {
      // glob_npc and glob_defaultnpc already set !!!
    }
    else
    {
      nr=to_int(input)-1;
      if (nr<0 || nr>sizeof(npcs))
      {
	tell_object(glob_tp,"Your selection "+(nr+1)+" is not valid.\n");
	return;
      }
      glob_npc=npcs[nr];
      glob_defaultnpc=blueprint(glob_npc);
    }
  }
  write("NPC is "+glob_defaultnpc+".\n");
  if (2==sscanf(str,"%s same as %s",newdet,olddet))
  {
    newdets=explode(newdet,",");
    newdets=CheckDetails(newdets,glob_npc,1); 
    oldstr=glob_npc->QueryQuestions()[olddet];   // maybe we are a closure ...
    if (closurep(oldstr)) oldstr=MakeClosure(oldstr);
    if (!oldstr)
    { 
      tell_object(glob_tp,olddet+" is no question of "+glob_npc+".\n");
      return ;
    }
    if (!sizeof(newdets))
    { 
      tell_object(glob_tp,"No NEW question given.\n");
      return ;
    }
    oldstr=glob_npc->QueryQuestions()[olddet];   // maybe we are a closure ...
    MakeDetail(TP,newdets,oldstr,glob_npc,1); 
  }
  else if (1==sscanf(str,"%s new",newdet))
  {
    newdets=explode(newdet,",");
    newdets=CheckDetails(newdets,glob_npc,1); 
    if (!sizeof(newdets))
    {
      tell_object(glob_tp,"No NEW question given.\n");
      return ;
    }
    tell_object(glob_tp,"Please enter the text for the question(s):\n");
    glob_tp=TP; // not needed
    glob_newdets=newdets; 
    SLIB->input_text("question_done"); // start editor 
  }
  else 
  {
    tell_object(glob_tp,"Cannot understand the question command.\n");
    return ;
  }
  return ;
}
/* ------------------------------------------------------------ */ 
// ATTENTION this function removes all details from a room !!!
int combine(string str)
{
int pos1,pos2,pos_add,pos_edd,pos_n,pos_start,i,cpos;
object env;
string room,s,tmp;
  if (!TP || !IS_WIZARD(TP) || TP!=owner) return 0;
  if (!str || (str!="detail"&&str!="details") ) return
    notify_fail("Combine what?\n"),0;
  env=environment(TP);
  room=blueprint(env);
  if (file_size(room+".c")<=0)
  {
    tell_object(TP,"The file '"+room+".c' does not exist.\n");
    return 1;
  }
  tell_object(TP,"Room:"+room+".c\n");
  s=read_file(room+".c");
  pos1=-1;
  cpos=0;
  pos1=FindCreate(s,&cpos);
  if (pos1<0)
  {
    tell_object(TP,"Cannot find create ("+pos1+").\n");
    return 1;
  }
  pos2=FindBracket(s,pos1,ID_BRACKET_CURLED);
  if (pos2<0)
  {
    tell_object(TP,"Cannot find end of create ("+pos2+").\n");
    return 1;
  }
  pos_edd=pos1;
  do
  {
    pos_add=FindAddFunction(s,"AddDetails",pos_edd,pos2,&pos_start);
    if (pos_add>=0)
    {
      write("Found AddDetail:\n");
      pos_edd=FindBracket(s,pos_add,ID_BRACKET_NORMAL);
      if (pos_edd>0) pos_edd=strstr(s,";",pos_edd);  // search ";"
      if (pos_edd>0) pos_n=strstr(s,"\n",pos_edd);  // search CR
      else pos_n=-1;
      if (pos_n>0)
      {
        tmp=SLIB->strip_blanks(s[(pos_edd+1)..(pos_n-1)]);
	if (tmp==" " || tmp=="") pos_edd=pos_n;   // CR to be removed too
	write(pos_start+" "+pos_add+" "+pos_edd+" "+pos_n+" '"+tmp+"'\n");
      }
        s=s[0..pos_start]+s[(pos_edd+1)..<1];
	pos_edd=pos_start;
    }

  }while(pos_add>=0 && pos_edd>=0);


  rm(room+".bak");
  rename(room+".c",room+".bak");
  i=write_file(room+".c",s);
  if (!i)
  {
    tell_object(TP,"Could not write the file '"+room+".c'. Try to:\n"+
          "       'cp "+room+".bak "+room+".c'.\n"+
          "to restore the old file.\n");
  }
  else
  {
    tell_object(TP,"Combine all details and wrote file.\n");
  }
  return 1;
}
/* ------------------------------------------------------------ */ 
// restore <room>   : Restores the backup file of <room>
int cmd_restore(string str)
{
string err;
  if (!TP || !IS_WIZARD(TP) || TP!=owner) return 0;
  if (!str) return
    notify_fail("Restore "+blueprint(environment(TP))+"?\n"),0;
  str=resolve_file(str,environment(TP));
  if (file_size(str+".c")<=0)
  {
    write("Your argument '"+str+"' is no valid filename.\n");
    write("Nothing done...\n");
    return 1;  
  }
  rm(str+".detail");
  rename(str+".c",str+".detail");
  rename(str+".bak",str+".c");
  write("Restoring backup file '"+str+".bak'.\n");
  GetRoomObject(str);
  return 1;
}
/* ------------------------------------------------------------ */ 
// The missing detail ADMINISTRATOR
/* ------------------------------------------------------------ */ 
// Put all missing details of room into a neat string
// Room can be either the object room or its filename/blueprint
// flag==0: Standard output, flag==1: shortend output
varargs string GetDetailString(mixed room,int flag)
{
string str,res;
string *ind;
int i,nr;
mapping troom;
  if (!room) room=environment(this_player())||previous_object();
  if (!room)
  {
    notify_fail("No room given!\n");
    return 0;
  }
  nr=DETAILSERVER->QueryNumberOfDetails(room);
  troom=DETAILSERVER->QueryDetails(room);
  if (!troom)
  {
    return("There are no details missing in this room.\n");
  }
  if (objectp(room)) room=blueprint(room);
  if (!troom)
  {
    if (flag==1)
      res="";
    else
      res="Missing details ("+to_string(nr)+") in '"+room+"' are:\n";
    res+="None.\n";
  }
  else
  {
    if (flag==1)
      res="";
    else
      res="Missing details ("+to_string(sizeof(ind))+"/"+
          to_string(nr)+") in '"+room+"' are:\n";
    ind=m_indices(troom);
    for (i=0;i<sizeof(ind);i++)
    {
      res+=sprintf("    - %2d: %s (%3d)\n",i,ind[i],troom[ind[i]]);
    }
    if (!sizeof(ind)) res+="No missing details found.\n";
  }
  return res;
}
// Can be called from outside to get a decent output
// Room can be either the object room or its filename/blueprint
// flag==0: Standard output, flag==1: shortend output
varargs int WriteDetails(mixed room,int flag)
{
string s;
  s=GetDetailString(room,flag);
  if (s) write(s);
  else return 0;
  return 1;
}

// Writes out all rooms which have missing details
// flag==0: short, flag==1: long
// abr: arbitrary string which will be matched against the filename
varargs int WriteRooms(int flag,string abr,int maxcnt,int nr)
{
int i,cnr;
string dm,dummy;
string *ind;
  if (nr==0)
  {
    if (running)
    {
      notify_fail("WriteRooms already running.\n"),0;
      return 0;
    }
    domain=([]);
    if (!abr) write("..... Missing details in all rooms are:\n");
    else write("..... Missing details in all rooms containing '"+abr+"' are:\n");
    roomindex=DETAILSERVER->QueryIndex(1);  // sorted
    if (sizeof(roomindex))
    {
      nr=0;
      call_out(#'WriteRooms,1,flag,abr,maxcnt,nr+1);
      running=1;
    }
    else
    {
      write("No rooms available.\n");
    }
    return 1;
  }
  if (maxcnt<=0) maxcnt=sizeof(roomindex);
  nr--;
  for (i=nr;i<nr+MAXOUTPUTCNT && i<sizeof(roomindex) && i<maxcnt;i++)
  {
    cnr=DETAILSERVER->QueryNumberOfDetails(roomindex[i]);
    if (2!=sscanf(roomindex[i],"/d/%s/%s",dm,dummy)) dm="default";
    if (member(domain,dm)) domain[dm]+=cnr;
    else domain[dm]=cnr;
    if (!abr || strstr(roomindex[i],abr)>=0)
    {
      write(sprintf("%3d:..... %s (%3d)\n",
        i+1,roomindex[i],cnr));
      if (flag==1)
      {
        WriteDetails(roomindex[i],1);
      }
    }
  }
  if (i<sizeof(roomindex) && i<maxcnt)
     call_out(#'WriteRooms,1,flag,abr,maxcnt,nr+MAXOUTPUTCNT+1);
  else
  {
    write("All rooms ("+sizeof(roomindex)+") with missing details processed...\n");
    ind=m_indices(domain);
    write("Domain Toplist:\n");
    for (i=0;i<sizeof(ind);i++)
    {
      write(ind[i]+":"+domain[ind[i]]+"\n");
    }
    running=0;
  }
  return 0; 
}


/* ------------------------------------------------------------ */ 
//  "clear detail <detail>    : Removes the missing <detail> from actual room\n"
//  "clear all room details   : Removes the room from the missing detail list\n"
/* ------------------------------------------------------------ */ 
int cmd_clear(string str)
{
string room,detail;
int res;
  if (!TP || !IS_WIZARD(TP) || TP!=owner) return 0;
  if (!str) return notify_fail("Clear what?\n"),0;
  if (str=="all room details" || str=="all details from room")
  {
    room=QueryActualRoom();
    if (room) res=DETAILSERVER->RemoveDetail(0,room,1); // force all details
    if (res>0) write("Removing all missing details from '"+room+"'.\n");
    else write("Could not remove all missing details from '"+room+"'.\n");
  }
  else if (1==sscanf(str,"detail %s",detail))
  {
    room=QueryActualRoom();
    if (room) res=DETAILSERVER->RemoveDetail(detail,room,0); // just one detail
    if (res>0) write("Removing detail '"+detail+"' from '"+room+"'.\n");
    else write("Could not remove detail '"+detail+"' from '"+room+"'.\n");
  }
  else
    return notify_fail("Clear what?\n"),0;
  return 1;
}


/* ------------------------------------------------------------ */ 
// COMMON functions
/* ------------------------------------------------------------ */ 
// show parameters        : Shows the configuration of the detailmaker
// show toplist           : Shows the toplist of detailfixer
// show details           : Shows the details of a room
// show details s[hort]   : Brief output of the details
// show questions of <whom>  : Show all questions of an NPC
// show questions of <whom> short: Brief output of all questions
// show missing [details] : shows what details are missing in a room
// show rooms [abr]       : Shows what details in what rooms are missing
//                          if <abr> is given it is matched against the
//                          the blueprint of the room and only if it
//                          matches partly it will be shown
// show rooms s[hort]     : Brief output of show rooms 
// show rooms [abr] s[hort]  : Brief output of show rooms with keyword
// Per default all commands work on the room you are standing in
// otherwise specify a room with 'set derfault room to <room>'
// where $h or $here works as well and is default
int cmd_show(string str)
{
string s,abr,nr;
int flag;
object room;
  if (!TP || !IS_WIZARD(TP) || TP!=owner) return 0;
  if (!str) return notify_fail("Show what?\n"),0;
  if (str=="parameters" || str=="parameter")
  {
    write("The configuration of your detailmaker "+VERSION+" is:\n");
    write("- Autoindent is "+(autoindent?"on":"off")+".\n");
    if (!autoindent)
    {
      write("   Indents are set to "+indent1+" and "+indent2+".\n");
    }
    write("- Backups are "+
      (nobackup?"not written (not recommended)":"written")+".\n");
    write("- Comments are "+(nocomment?"off":"on")+".\n");
    write("- The default working room is: "+QueryActualRoom()+".\n");
    write("- Wrapping is "+(wrap>0?("set to "+to_string(wrap)):"off")+".\n");
    if (nologging)
      write("- Your details and questions are NOT written to the toplist.\n");
    else
      write("- Your details and questions are written to the toplist.\n");
    if (d_cnt==0 && q_cnt==0) 
      write("Unfortunately, you didn't add any detail or question yet.\n");
    else if (d_cnt==0)
      write("You added "+q_cnt+" question"+(q_cnt==1?"":"s")+
         " but no detail yet.\n");
    else if (q_cnt==0)
      write("You added "+d_cnt+" detail"+(d_cnt==1?"":"s")+
         " but no question yet.\n");
    else 
      write("You added "+d_cnt+" detail"+(d_cnt==1?"":"s")+" and "+
        q_cnt+" question"+(q_cnt==1?"":"s")+"!\n");
    return 1;
  }
  else if (str=="toplist" || str=="fixlist")
  {
    write(DETAILSERVER->WriteToplist());
    return 1;
  }
  else if (str=="default" || str=="default room")
  {
    write("The default working room is:\n"+QueryActualRoom()+".\n");
    return 1;
  }
  else if (str=="details")
  {
    room=GetRoomObject(QueryActualRoom());
    s="Details of '"+blueprint(room)+":\n";
    s+=GetAllDetails(room,0,0);
    SLIB->SMore(s);
    return 1;
  }
  else if (str=="details s" || str=="details short")
  {
    room=GetRoomObject(QueryActualRoom());
    if (!room) return notify_fail("Cannot find your environment.\n"),0;
    s="Details of '"+blueprint(room)+":\n";
    s+=GetAllDetails(room,1,0);
    SLIB->SMore(s);
    return 1;
  }
  else if (str=="missing" || str=="missing details")
  {
    room=GetRoomObject(QueryActualRoom());
    if (!room) return notify_fail("Cannot find your environment.\n"),0;
    return WriteDetails(room,0);
  }
  else if (str=="rooms")
  {
    return WriteRooms(1,0,0,0);
  }
  else if (str=="rooms s" || str=="rooms short")
  {
    return WriteRooms(0,0,0,0);
  }
  else if ( (1==sscanf(str,"#=%s rooms short",nr) ||
           1==sscanf(str,"#=%s rooms s",nr)) && to_int(nr)>0 )
  {
    return WriteRooms(0,0,to_int(nr),0);
  }
  else if (1==sscanf(str,"#=%s rooms",nr) && to_int(nr)>0 )
  {
    return WriteRooms(1,0,to_int(nr),0);
  }
  else if ( ( (2==sscanf(str,"#=%s rooms %s short",nr,abr)) || 
          (2==sscanf(str,"#=%s rooms %s s",nr,abr)) ) && to_int(nr)>0 )
  {
    return WriteRooms(0,abr,to_int(nr),0);
  }
  else if (2==sscanf(str,"#=%s rooms %s",nr,abr) && to_int(nr)>0)
  {
    return WriteRooms(1,abr,to_int(nr),0);
  }
  else if ( (1==sscanf(str,"rooms %s short",abr)) || 
          (1==sscanf(str,"rooms %s s",abr)) )
  {
    return WriteRooms(0,abr,0,0);
  }
  else if (1==sscanf(str,"rooms %s",abr))
  {
    return WriteRooms(1,abr,0,0);
  }
  else if (1==sscanf(str,"questions of %s short",abr))
  {
    room=present(abr,environment(TP));
    if (!room) return notify_fail(abr+" is not here.\n"),0;
    if (!room->QueryIsNPC()) return notify_fail(abr+" is no NPC.\n"),0;
    s="Questions of '"+blueprint(room)+":\n";
    s+=GetAllDetails(room,1,1);
    SLIB->SMore(s);
    return 1;
  }
  else if (1==sscanf(str,"questions of %s",abr))
  {
    room=present(abr,environment(TP));
    if (!room) return notify_fail(abr+" is not here.\n"),0;
    if (!room->QueryIsNPC()) return notify_fail(abr+" is no NPC.\n"),0;
    s="Questions of '"+blueprint(room)+":\n";
    s+=GetAllDetails(room,0,1);
    SLIB->SMore(s);
    return 1;
  }

  return notify_fail("Show WHAT?\n"),0;
}
// Jump to the actual workroom
int cmd_goto(string str)
{
object room;
string actroom;
int roomnr;
  if (!TP || !IS_WIZARD(TP) || TP!=owner) return 0;
  if (!str) return 0;
  if (1==sscanf(str,"room %d",roomnr) && roomnr>0)
  {
      if (roomnr>sizeof(roomindex)) 
      {
	write("There are not so many rooms in the roomlist or the\n"
	      "list hasd not yet been shown.\n");
	return 1;
      } 
      actroom=roomindex[roomnr-1];
      room=GetRoomObject(actroom);
  }
  else if (str!="default" && str!="default room") return 0;
  else room=GetRoomObject(QueryActualRoom());
  if (!room) return notify_fail("Cannot goto there.\n"),0;
  if (room==environment(TP))
  { 
    write("Your are already there.\n");
    return 1;
  }
  TP->move(room);
  TP->LookAfterMove();
  return 1;
}
// Set parameters of the detailmaker
// e.g Set the actual working room to a file, $here or $env (always the
// environment)
int cmd_set(string str)
{
mixed where;
string indent,what;
  if (!TP || !IS_WIZARD(TP) || TP!=owner) return 0;
  if (!str) return 0;
  if (1==sscanf(str,"backups %s",what))
  {
    if (what=="off")
    {
      nobackup=1;
      write("Disabling backups.\n");
      write("WARNING: The use of this option is strongly discouraged as\n"
            "         the detailmaker is an automatic program which changes\n"
            "         files. This procedure could be errorours in some cases.\n"
            "         Then you might widsh to have a backup handy!\n");
    }
    else if (what=="on")
    {
      nobackup=0;
      write("Enabling backups.\n");
    }
    else
    {
      return notify_fail("Command should be on/off.\n"),0;
    }
  }
  else if (1==sscanf(str,"comments %s",what))
  {
    if (what=="off")
    {
      nocomment=1;
      write("Disabling comments.\n");
      write("WARNING: The use of this option is not recommended as it is\n"
            "         usually helpful to have a filedocumentation on who\n"
            "         changed a file. Also the use of this comment line\n"
            "         improves processing speed when you edit a file again.\n");
    }
    else if (what=="on")
    {
      nocomment=0;
      write("Enabling comments.\n");
    }
    else
    {
      return notify_fail("Command should be on/off.\n"),0;
    }
  }
  else if (1==sscanf(str,"toplist %s",what))
  {
    if (what=="off")
    {
      nologging=1;
      write("Disabling logging to the toplist of fixed details.\n");
    }
    else if (what=="on")
    {
      nologging=0;
      write("Enabling logging of your work to the toplist of fixed details.\n");
    }
    else
    {
      return notify_fail("Command should be on/off.\n"),0;
    }
    DETAILSERVER->AddToplist(TP,d_cnt,q_cnt,nologging);
  }
  else if (1==sscanf(str,"wrapping %s",what))
  {
    if (what=="off")
    {
      wrap=0;
      write("Disabling wrapping.\n");
    }
    else if (to_int(what)>0)
    {
      wrap=to_int(what);
      write("Enabling wrapping ("+wrap+").\n");
    }
    else
    {
      return notify_fail("Wrapping should be <number>/off.\n"),0;
    }
  }
  else if (1==sscanf(str,"autoindent %s",what))
  {
    if (what=="off")
    {
      autoindent=0;
      indent1=autoindent1;
      indent2=autoindent2;
      write("Disabling autoindenting.\n");
    }
    else if (what=="on")
    {
      autoindent=1;
      autoindent1=indent1;
      autoindent2=indent2;
      write("Enabling autoindenting.\n");
      write("NOTE: This is still an experimental feature.\n");
    }
    else
    {
      return notify_fail("Command should be on/off.\n"),0;
    }
  }
  else if (1==sscanf(str,"first indent to %s",indent)) 
  {
    indent1=to_int(indent);
    write("All commands will be preceeded by "+indent1+" whitespaces.\n");
    MakeIndent();
  }
  else if (1==sscanf(str,"second indent to %s",indent)) 
  {
    indent2=to_int(indent);
    write("All text will be preceeded by "+indent2+" whitespaces.\n");
    MakeIndent();
  }
  else if (1==sscanf(str,"default room to %s",where))
  {
    if (to_int(where)>0)
    {
      where=to_int(where);
      if (where>sizeof(roomindex)) 
      {
	write("There are not so many rooms in the roomlist or the\n"
	      "list hasd not yet been shown.\n");
	return 1;
      } 
    }
    else if (where=="$h" || where=="$here") where=blueprint(environment(TP));
    else if (where=="$e" || where=="$env") where=0;
    else where=resolve_file(where,environment(TP));
    SetActualRoom(where);
    write("Set actual room to '"+QueryActualRoom()+"'.\n");
  }
  else
  {
   return notify_fail("Set default room to <where>?\n"),0;
  }
  SaveMe();
  return 1;
}
int cmd_help(string str)
{
string s;
  if (!TP || !IS_WIZARD(TP) || TP!=owner) return 0;
  notify_fail("Help on what?\n");
  if (!str) return 0;
  if (str=="detailmaker")
  {
    s="You have following commands with the DETAILMAKER:\n"
    "show details [short]     : shows all details in the room.\n"
    "                           Use 'short' to minimize the output.\n"
    "show questions of <whom> [short] : Show all questions of an NPC.\n"
    "                           Use 'short' to minimize the output.\n"
    "restore <room>           : restores the room from before any changes\n"
    "                           Attention: The room is restore to the status\n"
    "                           before ANY details had been added. But a copy\n"
    "                           is made to *.detail to preserve the details.\n"
    "                           e.g. restore /d/woodland/common/lorinia/well\n"
    "detail <{keys}> same as <olddetail>:\n"
    "                           adds other keyword(s) key1,key2,... for an\n"
    "                           existing detail\n"
    "                           e.g. detail rose,tulip same as flower\n"
    "detail <{keys}> new      : adds a new detail with the keys 1,2,...\n"
    "                           and prompts for its description\n"
    "                           Entering #'foo gives you a closure\n"
    "                           e.g. detail rose,tulip new\n"
    "question <{keys}> same as <oldquestion>:\n"
    "                           adds other keyword(s) key1,key2,... for an\n"
    "                           existing question\n"
    "                           e.g. question giant,thief same as orc\n"
    "question <{keys}> new      : adds a new question with the keys 1,2,...\n"
    "                           and prompts for its description\n"
    "                           Entering #'foo gives you a closure\n"
    "                           e.g. question orc,giant new\n"
    "\n";
  }
  else if (str=="administrator")
  {
    s="You have following commands with the missing detail ADMINISTRATOR:\n"
    "show toplist             : shows the toplist of the best detailfixer!\n"
    "show missing [details]   : shows what details are missing in a room\n"
    "show rooms [abr]         : Shows what details in what rooms are missing\n"
    "                           if <abr> is given it is matched against the\n"
    "                           the blueprint of the room and only if it\n"
    "                           matches partly it will be shown\n"
    "show rooms s[hort]       : Brief output of show rooms\n"
    "show rooms [abr] s[hort] : Brief output of show rooms with keyword\n"
    "show #=<max> rooms [abr] s[hort] : Same as above but only <max> rooms\n"
    "                                 are displayed\n"
    "clear detail <detail>    : Removes the missing <detail> from actual room\n"
    "clear all room details   : Removes the room from the missing detail list\n"
    "\n";
  }
  else if (str=="general details")
  {
    s="You have following GENERAL commands:\n"
    "set default room to <room>:Set the current working room. Per default all\n"
    "                           commands work in the room you are in =$env\n" 
    "                           or =$e. $here and $h set the working room\n"
    "                           fixed to your current room. Recommended is $env\n"
    "                           If room is an integer number the corresponding\n"
    "                           room from the last output list is taken.\n"
    "show default room        : Shows the current working room.\n"
    "goto default [room]      : Jump to the default room.\n"
    "\n"
    "set first indent to <nr> : AddDetail is preceeded by <nr> spaces\n"
    "set second indent to <nr>: The following text is preceeded by <nr> spaces\n"
    "set comments [on|off]    : Adds a comment line of the changes to the file\n" 
    "set backups [on|off]     : Write backup of the changed file.\n"
    "set toplist [on|off]     : No entry of you is shown in the toplist.\n"
    "set autoindent [off|on]  : Detailmaker tries to set indents itself.\n"
    "set wrapping [<nr>|off]  : Set wrapping to <nr> chars or switch it off.\n"
    "show parameters          : Shows the configuration of the detailmaker.\n"
    "\n";
  }
  else return 0;
  s+="Help is available for:\n"+
     "detailmaker,administrator,general details.\n";
  SLIB->SMore(s);
  return 1;
}
/* ------------------------------------------------------------ */ 
string QueryLong()
{
  return process_string(::QueryLong());
}
void create()
{
  ::create();
  if (sizeof(explode(object_name(this_object()),"#"))<2) return;
  SetLong(
  "This seems to be the cute little detailmaker of @@QueryOwnerName@@.\n"
  "With the detailmaker, (c) Softbyte@Nightfall, you can add details to\n"
  "a room or questions to a NPC without the need to edit the file.\n"
  "Also it is possible to check for missing details in a room or several\n"
  "rooms as well as for questions of an NPC.\n"
  "Try 'help detailmaker', 'help administrator' and 'help general details'\n"
  "and 'show parameters' for more information.\n"
  "In case of command collisions all commands can be preceeded with a 'dm '\n"
  "as well.\n"
  "The actual workroom is '@@QueryActualRoom@@'.\n" );
  SetShort("a detailmaker "+VERSION);
  Set(P_AUTOOBJECT,1);
  AddAdjective("detail");
  AddAdjective("maker");
  AddId("detail");
  AddId("maker");
  AddId("detailmaker");
  owner=TP;
  seteuid(getuid());
  roomindex=({});
  running=0;
  actualroom=0;

  // Load saved values
  if ( file_size(QuerySaveName(".o"))<=0)  /* New Savefile */
  {
    indent1=2;
    indent2=4;
    wrap=0;
    nocomment=0;
    nobackup=0;
    autoindent=0;
    d_cnt=0;
    q_cnt=0;
    SaveMe();
  }
  else
  {
    restore_object(QuerySaveName());
  }
  MakeIndent();
}
int cmd_dm(string str)
{
string command,parameter;
  if (!str) return notify_fail("What detailmaker command?\n"),0;
  if (2==sscanf(str,"%s %s",command,parameter))
  {
    // nothing to be done
  }
  else if (1==sscanf(str,"%s",command)) parameter=0;
  else return notify_fail("Dm <command> <parameter>.\n"),0;
  switch(command)
  {
    case "det":
    case "detail":
    case "item":
      return cmd_detail(parameter);
    break;
    case "quest":
    case "question":
      return cmd_question(parameter);
    break;
    case "restore":
      return cmd_restore(parameter);
    break;
    case "show":
      return cmd_show(parameter);
    break;
    case "clear":
      return cmd_clear(parameter);
    break;
    case "help":
      return cmd_help(parameter);
    break;
    case "set":
      return cmd_set(parameter);
    break;
    case "goto":
      return cmd_goto(parameter);
    break;
    default:
      return notify_fail("Unknown command.\n"),0;
  }
  return notify_fail("Unknown command.\n"),0;
}

init()
{
  ::init();
  add_action("cmd_detail","det",1);
  add_action("cmd_detail","item",1);
  add_action("cmd_question","quest",1);
  add_action("cmd_restore","restore");
  add_action("cmd_show","show");
  add_action("cmd_clear","clear");
  add_action("cmd_help","help");
  add_action("cmd_set","set");
  add_action("cmd_goto","goto");


  // Just in case someone overrides our commands
  add_action("cmd_dm","dm");
  add_action("cmd_dm","detailmaker");
}
