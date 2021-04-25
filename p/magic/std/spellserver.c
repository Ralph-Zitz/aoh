//---------------------------------------------------------------------------
// /p/magic/std/spellserver.c
//
// The basic spell handling of OSB
// (c) Softbyte@OSB 20jun98
// V1.0:  Basic spell handling and functions
//---------------------------------------------------------------------------

#include <config.h>
#include <properties.h>
#include <moving.h>
#include <daemons.h>
#include <msgclass.h>
#include <skills.h>
#include <search.h>
#include <magic.h>

inherit "/std/base";
inherit "/lib/config"; // inifile handling


#define TO this_object()
#define TP this_player()

static string * sp_classes; // All available classes
static mapping sp_classmap; // maps spell to class
static mapping sp_spellmap; // maps class to spelllist

/*
  Access functions on the variables
*/
string *QueryClasses() {return sp_classes;}
mapping QueryClassMap() {return sp_classmap;}
mapping QuerySpellMap() {return sp_spellmap;}
string QueryClass(string spell) {return sp_classmap[spell];}
string *QuerySpells(string class) {return sp_spellmap[class];}

/* Debug only */
Debug(string str)
{
object ob;
  ob=find_player("softbyte");
  if (ob && ob->Query("Debug") ) tell_object(ob,str);
}

/* 
  Check whether the spell modules is existand an loadable
  Returns 0 on ok or else an error string
*/
string CheckSpellError(string filename)
{
  string s;
  s= catch(call_other(filename,"???"));
  return s;
}

/*
  Returns the filename of the spell module
*/
string GetFilename(string spell)
{
  string class;

  if (!spell) return 0;

  class=QueryClass(spell);

  return SP_MOD(class+"/"+spell);
}

/*
  Checks whether a living is allowed to cast any spell
*/
string CannotCast(object liv,int sptype)
{
  mixed res;

  if (!liv) return "Nobody can do nothing";
  if (liv->QueryGhost()) return "As ghost you cannot use any magic.\n";
  if (liv->QueryCombatDelay()>0)
    return ("You can't do that: "
      +(liv->QueryCombatDelayMsg()||"You are unconcious.\n"));
  if (res = environment(liv)->QueryIsMagicForbidden(sptype))
     return res;
  if (res=liv->Query(P_ISCASTING)) 
  {
    return "You are already using the "+res+" spell.\n";
  }
  return 0;
}

/*
  Call the cleanup function of the spell
*/
private void perform_cleanup(string filename, object liv,
                  string target,object *targetob,int delay)
{
  if (delay>0)  // wait until spell sets in
  {
    call_out(#'perform_cleanup,delay,filename,liv,target, targetob,0);
  }
  else
  {
    call_other(filename,"CleanUp",liv,target,targetob,SP_SUCCESS);
  }
}

/*
  Perform the OnSucceedCasting and proceed with the cleanup
*/
private void perform_casting(string filename,int spelltype, object liv,
                  string target,object *targetob,int delay)
{
int cleandelay;
int value;
string name;

  if (delay>0)  // wait until spell sets in
  {
    call_out(#'perform_casting,delay,filename,spelltype,liv,target,
             targetob,0);
    name=call_other(filename,"QueryCastName");
    TP->Set(P_ISCASTING,name);  // stop casting prop
  }
  else
  {
    value=call_other(filename,"OnSucceedCasting",liv,target,targetob);
    TP->Set(P_ISCASTING,0);  // stop casting prop
    cleandelay=call_other(filename,"QueryDuration");
    // route to local function to clean up the spell
    perform_cleanup(filename,liv,target,targetob,cleandelay);
  }
}

/* 
  The main cast function which will be called from any add_action
  All analyis of the input will be performed here
*/
int Cast(string str)
{
  string *strs;
  int s_cnt;
  int delay;
  int strength;
  string name,target;
  object *targetob;
  mixed res;
  string filename,s;
  int value,spelltype,sp;

  s_cnt=0;
  strs=norm_id(str,1);

  if (!strs || sizeof(strs)<=0) return (int)notify_fail(
        "Cast what?\n",NOTIFY_NOT_VALID);

  // First determine whether the spell has a specified strength
  if (strs[s_cnt]=="minor" || strs[s_cnt]=="lesser") 
  {
    strength=-1;
    s_cnt++;
  }
  else if (strs[s_cnt]=="major" || strs[s_cnt]=="greater") 
  {
    strength=1;
    s_cnt++;
  }
  else strength=0;

  // Now extract the cast name of the spell
  if (sizeof(strs)<=s_cnt)  return (int)notify_fail(
       "Cast what?\n",NOTIFY_NOT_VALID);
  name=strs[s_cnt];
  s_cnt++;

  // Now throw out any at, to, on's
  if (sizeof(strs)>s_cnt)
  {
    if (-1!=member(({"in","on","at","to","under","over"}),strs[s_cnt]))
    {
      s_cnt++;
    }
  }

  // Now find out the target(s)
  if (sizeof(strs)>s_cnt)
  {
    target=implode(strs[s_cnt..]," "); 
    res=TP->Search(target,SEARCH_ENV_INV|SM_MULTI);
    if (pointerp(res)) targetob=res;
    else if (mappingp(res)) targetob=res[FOUND_MULTI];
    else targetob=0;
  }
  else
  {
    target=0;
    targetob=({});
  }

  filename=GetFilename(name);
  if (!filename || CheckSpellError(filename)) return (int)notify_fail(
     "That spell does not exists.\n",NOTIFY_NOT_VALID);

  spelltype=call_other(filename,"QuerySpellType");

  // Hardcoded checks in the spellserver itself
  s=CannotCast(TP,spelltype);
  if (s) return (int)notify_fail(s,NOTIFY_NOT_VALID);

  s=call_other(filename,"CannotCast",TP,target,targetob);
  if (s) return (int)notify_fail(s,NOTIFY_NOT_VALID);

  // Reduce the SP if it returns 0 no check is performed
  sp=call_other(filename,"QueryCastCost");
  if (sp)
  {
    if (!TP->ReduceSP(sp)) return (int)notify_fail(
        "You don't have enough spellpoints.\n",NOTIFY_NOT_VALID);

  }
  // Allow the spell to write something to the player, etc.
  call_other(filename,"OnCastMessage",TP,target,targetob);

  value=call_other(filename,"CheckSuccess",TP);
  if (value>0)
  {
    delay=call_other(filename,"QueryCastDelay");
    // route to local function to cast the spell
    perform_casting(filename,spelltype,TP,target,targetob,delay);
  }
  else
  {
    value=call_other(filename,"OnFailCasting",TP,target,targetob,SP_FAIL);
    call_other(filename,"CleanUp",TP,target,targetob,SP_FAILURE);
  }
  return 1;

}

/*
   Returns a string containing a description and ifo for the given spell
*/
string GetSpellInfo(string name)
{
  string s,filename;


  filename=GetFilename(name);
  if (!filename || CheckSpellError(filename)) return 
     "That spell does not exists.\n";

  s="Information on the "+call_other(filename,"QueryShort")+":\n";
  s+=call_other(filename,"QueryLong");
  s+="\n";
  s+="Cast via '"+call_other(filename,"QueryCastName")+"'\n";
  s+="Cost: "+to_string(call_other(filename,"QueryCastCost"))+"\n";
  s+="Backfire probability:"+
     to_string(call_other(filename,"QueryBackfireChance")/10)+"%\n";
  s+="Cast difficulty:"+
     to_string(call_other(filename,"QueryDifficulty")/10)+"%\n";
  return s;
}
/*
  can be called by an add_Action to give out the information on a spell
*/
int Info(string str)
{
  string s;

  if (!str ) return (int)notify_fail(
        "Information on what?\n",NOTIFY_NOT_VALID);

  s=GetSpellInfo(str);
  msg_write(CMSG_GENERIC,s);
  return 1;
}

/*
  Load the spell data from an ini file
*/
int LoadData()
{
  string file;
  int i,j; 
  string *spells;

  file="spells";
  sp_classmap=([]);
  sp_spellmap=([]);

  // Does the inifile exist?
  if (file_size(SP_INI(file+".ini"))<=0) return 0;

  sp_classes=GetKeys(SP_INI(file+".ini"),"classes");
  // loop classes to get spells
  for (i=0;i<sizeof(sp_classes);i++)
  {
    spells=GetKeyValue(SP_INI(file+".ini"),"classes",sp_classes[i]);
    for (j=0;j<sizeof(spells);j++)
    {
      sp_classmap+=([spells[j]:sp_classes[i]]);
    }
    sp_spellmap+=([sp_classes[i]:spells]);
  }
  return 1;
}

create()
{
  ::create();
  if (is_clone())
  {
    destruct(TO);
    return;
  }
  seteuid(getuid());
  LoadData();
}

/*
  Forward this function to the spell modules
*/
int QueryStrengthLevel(string spell,int value)
{
  string filename;
  filename=GetFilename(spell);
  if (CheckSpellError(filename))
  {
    raise_error("No such spell available.\n");
    return 0;
  }
  if (filename) return call_other(filename,"QueryStrengthLevel",value);
  else 
  {
    raise_error("No such spell available.\n");
    return 0;
  }
}

