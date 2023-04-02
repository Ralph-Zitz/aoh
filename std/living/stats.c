/*---------------------------------------------------------------------------
** std/living/stats
**
** Primitive handling of stats and skills of a living.
** It does not feature the advanced handling of true (n)pcs.
**
** 05-Jun-1994  [Mateese]
** 19-Nov-1997  [Softbyte] Stats/Skill handling removed, New Stats Added
** 12-Mar-1998  [Softbyte] Talent support added
** 13-Jun-1998  [Softbyte] Skill forwarding functions
** 23-Jun-1998  [Softbyte] Spell/Skill access functions
**---------------------------------------------------------------------------
*/

#include <living.h>
#include <attributes.h>
#include <skills.h>

#define THIS  this_object()

public varargs mixed QueryAttr(string name, int sc);  // std/living/attributes
public varargs mixed SetAttr(string name, mixed arg, int sc);
public void AddHeart(mixed hook); // std/living/heart
public int SetMaxHP(int i);    // std/living/body
public int SetMaxSP(int i);    // std/living/body
public int SetMaxPEP(int i);    // std/living/body
public int SetMaxMEP(int i);    // std/living/body


//---------------------------------------------------------------------------
varargs int *CalculateSemiStats(int queryonly); // internal

//---------------------------------------------------------------------------

private int Str, Int, Con;
private int Cha,Qui,Agi,Wis;

private mapping Talents;   // Talents of a living
private nosave string *defTalents; // Talents defined in std/living/talents
private nosave mapping mCmdIdx;    // aux mapping to find talent functions


//---------------------------------------------------------------------------
// Internal stuff

// Remove tailing '.c'
string RemoveC(string s)
{
  if (s[<2..<1]==".c") return s[0..<3];
  return s;
}

// forward remove() to all talents
void RemoveTalent(string name,int value,string filename,mixed extradata)
{
  string file;
  if (!filename) file="/std/living/talents/"+name;
  else file=filename;
  call_other(file,"RemoveTalent",this_object(),value,extradata);
}

// distpach the command actions to the approriate files
int dispatchtalent(string str)
{
  string verb,file,name;
  string *cmds;
  int i,tmp,flag;

  verb=query_verb();
  if (!member(mCmdIdx,verb)) return 0; // not defined

  cmds=mCmdIdx[verb];
  if (!cmds) return 0;                 // error

  flag=0;    // return 0

  // walk commands and call "cmd_xxx" functions
  for (i=0;i<sizeof(cmds);i++)
  {
    name=cmds[i];
    if (!member(Talents,name))  // remove entry on the fly
    {
      mCmdIdx[verb]-=({cmds[i]});
      if (sizeof(mCmdIdx[verb])==0)
        mCmdIdx=m_delete(mCmdIdx,verb);
    }
    else
    {
      file=Talents[name,1];
      if (!file) file="/std/living/talents/"+name;
      tmp=({int})call_other(file,"cmd_"+verb,str,
        this_object(),Talents[name,0],Talents[name,2]);
      if (tmp) flag=tmp;  // someone returned TRUE
    }
  }

  return flag;

}

// Add the add_actions for the talents
void AddTalentCommands(string name,int value,string filename,mixed extradata)
{
  string *cmds;
  string file;
  int i;
  object ob;

  if (!mCmdIdx) mCmdIdx=([]);
  if (!interactive()) return;  // no actions for NPC's
  // TODO: what shall NPCs do with their talent commands?

  if (!filename) file="/std/living/talents/"+name;
  else file=filename;

  cmds=({string *})call_other(file,"QueryActions",this_object(),value,extradata);
  if (!cmds) return;

  for (i=0;i<sizeof(cmds);i++)
  {
    // forbid overiding of some basic commands
    if (-1!=member( ({"tell","say","who","score","ls","cd","pwd",
            "read","quit","goto"}),cmds[i] ) )
    {
      // log illegal action calls
      ob=this_interactive()||this_player()||previous_object();
      log_file("living.log",(ob?(({string})ob->QueryRealName()||"Nobody"):"None")+
      " added talent command '"+cmds[i]+"' from file '"+file+"' for living '"+
      object_name(this_object())+"' on "+ctime()+".\n");
    }
    else
    {
      add_action("dispatchtalent",cmds[i]);

      if (member(mCmdIdx,cmds[i]))  // cmd already in mapping
      {
        if (-1==member(mCmdIdx[cmds[i]],name)) // file not yet registered
           mCmdIdx[cmds[i]]+=({name});
      }
      else
      {
        mCmdIdx[cmds[i]]=({name});
      }
    } // end else
  } // next i
}

// forward the init() to all slave talents
void InitTalent(string name,int value,string filename,mixed extradata)
{
  string file;
  if (!filename) file="/std/living/talents/"+name;
  else file=filename;
  call_other(file,"InitTalent",this_object(),value,extradata);
  AddTalentCommands(name,value,filename,extradata);

}

// Init a talent from a restore_object call or SetTalent call
// i.e. make SetTalent call to slave object and inti the talent
void LoadTalent(string name,int value,string filename,mixed extradata)
{
  string file;
  if (!filename) file="/std/living/talents/"+name;
  else file=filename;
  call_other(file,"SetTalent",this_object(),value,extradata);
  InitTalent(name,value,filename,extradata);
}

void CreateTalents()
{
  Talents=([]);
  mCmdIdx=([]);
  if (file_size("/std/living/talents")==-2)  // dir exists
  {
    defTalents=get_dir("/std/living/talents/*.c");
    defTalents=map(defTalents,#'RemoveC /*'*/);
  }
  else
  {
    defTalents=({});
  }
}

//---------- End of Internal stuff -----------------------------------------

//---------------------------------------------------------------------------
// Setting and querying of the stats

public int QueryStr()    { return Str; }
public int SetStr(int v) {
  AddHeart(HEART_BODY);
  Str = !intp(v) ? Str : v >= 0 ? v : 0;
  CalculateSemiStats();
  return Str;
}

public int QueryInt()    { return Int; }
public int SetInt(int v) {
  AddHeart(HEART_BODY);
  Int = !intp(v) ? Int : v >= 0 ? v : 0;
  CalculateSemiStats();
  return Int;
}

public int QueryCon()    { return Con; }
public int SetCon(int v) {
  AddHeart(HEART_BODY);
  Con = !intp(v) ? Con : v >= 0 ? v : 0;
  CalculateSemiStats();
  return Con;
}

// Compat only...dex is (Agi+Qui)/2
public int QueryDex()    { return (Agi+Qui)/2; }
public int SetDex(int v) {
  AddHeart(HEART_BODY);

  Agi= !intp(v) ? Agi : v >= 0 ? v : 0;
  Qui= !intp(v) ? Qui : v >= 0 ? v : 0;
  CalculateSemiStats();
  return (Agi+Qui)/2;
}

public int QueryCha()    { return Cha; }
public int SetCha(int v) {
  AddHeart(HEART_BODY);
  Cha = !intp(v) ? Cha : v >= 0 ? v : 0;
  CalculateSemiStats();
  return Cha;
}

public int QueryWis()    { return Wis; }
public int SetWis(int v) {
  AddHeart(HEART_BODY);
  Wis = !intp(v) ? Wis : v >= 0 ? v : 0;
  CalculateSemiStats();
  return Wis;
}

public int QueryQui()    { return Qui; }
public int SetQui(int v) {
  AddHeart(HEART_BODY);
  Qui = !intp(v) ? Qui : v >= 0 ? v : 0;
  CalculateSemiStats();
  return Qui;
}

public int QueryAgi()    { return Agi; }
public int SetAgi(int v) {
  AddHeart(HEART_BODY);
  Agi = !intp(v) ? Agi : v >= 0 ? v : 0;
  CalculateSemiStats();
  return Agi;
}

// if the optional argument queryonly is set the stats are not
// set but returned as an array
varargs int *CalculateSemiStats(int queryonly)
{
  int hp,sp,pep,mep;
  hp=(3*QueryCon()+QueryStr())*10/12;
  sp=(QueryInt()+QueryWis())*10/6;
  pep=(QueryCon()+QueryInt())*10/6;
  mep=(QueryCon()+QueryStr()+QueryWis()*2)*10/12;
  if (!queryonly)
  {
    SetMaxHP(hp>0?hp:1);
    SetMaxSP(sp>0?sp:1);
    SetMaxPEP(pep>0?pep:1);
    SetMaxMEP(mep>0?mep:1);
    return 0;
  }
  else
  {
    return ({hp,sp,pep,mep});
  }
}
//-------- Skill  & Spells --------------------------------------------------
// Return the value of a skill
int QuerySkill(string skill)
{
  mapping sk;

  if (!skill) return 0;
  sk=QueryAttr(A_SKILLS);
  if (!sk) // set it on the fly
  {
    sk=([]);
    SetAttr(A_SKILLS,sk);
  }
  return sk[skill];
}
// Set the value of a skill, return the old value
int SetSkill(string skill, int value)
{
  int res;
  mapping sk;

  if (!skill) return 0;
  sk=QueryAttr(A_SKILLS);
  if (!sk) sk=([]);
  if (member(sk,skill))  // faster !
  {
    res=sk[skill];
    sk[skill]=value;
  }
  else
  {
    res=0;
    sk+=([skill:value]);
  }
  SetAttr(A_SKILLS,sk);
  return res;
}

mapping QuerySkills() {return QueryAttr(A_SKILLS)||([]);}

// Return the value of a spell
int QuerySpell(string spell)
{
  mapping sk;

  if (!spell) return 0;
  sk=QueryAttr(A_SPELLS);
  if (!sk) // set it on the fly
  {
    sk=([]);
    SetAttr(A_SPELLS,sk);
  }
  return sk[spell];
}
// Set the value of a spell, return the old value
int SetSpell(string spell, int value)
{
  int res;
  mapping sk;

  if (!spell) return 0;
  sk=QueryAttr(A_SPELLS);
  if (!sk) sk=([]);
  if (member(sk,spell))  // faster !
  {
    res=sk[spell];
    sk[spell]=value;
  }
  else
  {
    res=0;
    sk+=([spell:value]);
  }
  SetAttr(A_SPELLS,sk);
  return res;
}
mapping QuerySpells() {return QueryAttr(A_SPELLS)||([]);}

//---------------------------------------------------------------------------
// Talents of a living

// Set the talent 'name' (e.g. infravision) to the 'value' (0..1000)
// optional arguments are 'filename' (the file where the talent is defined
// default=/std/living/talents/name.c) and 'extra'. The 'extra' arguments
// is not processed but forwarded to all talent functions
varargs int SetTalent(string name,int value,string filename,mixed extra)
{
  string file;

  if (!Talents || !defTalents) CreateTalents();

  if (value==0)      // remove talent
  {
    if (member(Talents,name))
    {
      if (!filename) file="/std/living/talents/"+name;
      else file=filename;
      // Tell the talent file that the talent is off
      call_other(file,"RemoveTalent",
          this_object(),Talents[name,0],Talents[name,2]);
      Talents=m_delete(Talents,name);
      return 1;
    }
    return 0;  // not existent
  }

  if (!filename && member(defTalents,name)<0)
  {
    // No talent file found
    return 0;
  }

  if (member(Talents,name))  // Change talent
  {
    Talents[name,0]=value;
    Talents[name,1]=filename;
    Talents[name,2]=extra;
  }
  else Talents+=([name:value;filename;extra]);

  LoadTalent(name,value,filename,extra);
  return 1;
}

// Return the value (0..1000) for a talent
int QueryTalent(string name)
{
  if (!Talents) CreateTalents();
  if (!name || !member(Talents,name)) return 0;
  return Talents[name,0];
}
// Return the Filename of the talent
int QueryTalentFile(string name)
{
  if (!Talents) CreateTalents();
  if (!name || !member(Talents,name)) return 0;
  return Talents[name,1];
}
// Return the Extra data of the talent
int QueryTalentData(string name)
{
  if (!Talents) CreateTalents();
  if (!name || !member(Talents,name)) return 0;
  return Talents[name,2];
}

// Query all talents present in this living
string *QueryTalents()
{
  if (!Talents) CreateTalents();
  return m_indices(Talents||([]));
}

// Call this to init all talents from restore_object() call's
// i.e. the mapping is walked and all SetTalent funs called
void InitLoadedTalents()
{
  if (!Talents) CreateTalents();
  walk_mapping(Talents,#'LoadTalent /*'*/);
}

// Debugging only
mapping QueryMTalents() {return Talents;}
string *QueryDefTalents() {return defTalents;}

mapping QueryCmdIdx() {return mCmdIdx;}




//---------------------------------------------------------------------------
// Main functions
public void create()
{
  CreateTalents();
}

public void init()
{
  if (Talents && sizeof(Talents) ) walk_mapping(Talents,#'InitTalent /*'*/);
}

public varargs int remove(int arg)
{
  if (Talents || sizeof(Talents) ) walk_mapping(Talents,#'RemoveTalent /*'*/);
  Talents=([]);
  return 0;
}

//-------- Skill forwarding -------------------------------------------------
int UseSkill(string skill,int diff)
{
   return ({int})call_other(SKILLSERVER,"UseSkill",this_object(),skill,diff);
}

varargs int ApplySkill(string skill,int diff, mixed p)
{
   return ({int})call_other(SKILLSERVER,"ApplySkill",this_object(),skill,diff,p);
}


#ifdef NF
//---------------------------------------------------------------------------
public int SetSkill(string name, int value)

// Set attribute 'name' to the value a pc would have for that skill.

{
  SetAttr(name, value);
  return SetAttr(name, SKILLMASTER->ComputeSkill(name));
}

//---------------------------------------------------------------------------
// Basic routines.

public int ApplyValue(int value, int diff)
{
  int res;
  res = random(100) + value - 100 - diff;
  return res > 100 ? 101 : res < -100 ? -101 : res;
}

public int ApplyStat(string name, int diff) {
  return ApplyValue(STAT_SCALE * QueryAttr(name), diff);
}

public int ApplySkill(string name, int diff) {
  if (!QueryAttr(name, 1))
    SetSkill(name, 0);
  return ApplyValue(QueryAttr(name), diff);
}

public int UseStat(string name, int diff) {
  return ApplyStat(name, diff);
}

public int UseSkill(string name, int diff) {
  return ApplySkill(name, diff);
}

public int EffectiveSkill (string name) {
  if (!QueryAttr(name, 1))
    SetSkill(name, 0);
  return QueryAttr(name);
}
#endif

/***************************************************************************/
