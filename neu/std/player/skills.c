/*---------------------------------------------------------------------------
** std/player/skills
**
** Handling of the autoincreasing skills and stats of a player.
**
** 21-Mar-1994  [Mateese]
**---------------------------------------------------------------------------
*/

#include "/neu/sys/player.h"
#include <skillmaster.h>

inherit "/std/living/stats";

#define THIS  this_object()

public varargs mixed QueryAttr(string name, int sc);  
                                      // std/living/attributes
public varargs mixed SetAttr(string name, mixed arg, int sc);
                                      // std/living/attributes
public int QueryXP();                 // std/living/body

//---------------------------------------------------------------------------
// Undefine TRIAL to activate true autoincreasing skills.
// If defined, the code just pretends to increase it.

#define TRIAL 1

//---------------------------------------------------------------------------

private string *aSkServer;          // SkillServer
private mapping aStatData;          // Number usage of Stats
private mapping aSkillData;         // Number usage of Skills
#ifdef TRIAL
private mapping aSkillTrial;        // Autoincreasing trial numbers.
#endif

private static mapping skillData;  // Datacache

// skillData values per key
#define SKD_EFF    0   // the current effective value
#define SKD_SERVER 1   // responsible Skillmaster
#define SKD_STAT1  2   // Name of the first underlying stat
#define SKD_STAT2  3   // Name of the second underlying stat
#define SKD_APP    4   // Final value of the skill
#define SKD_DCC    5   // Approximation decelleration
#define SKD_DIRTY  7   // TRUE if the skill needs to be recomputed.
#define SKD_SIZE   8

//---------------------------------------------------------------------------

public mapping SetStatData (mapping s) {
  aStatData = copy(s);
  return aStatData;
}
public mapping QueryStatData () { return aStatData; }

public mapping SetSkillData (mapping s) {
  aSkillData = copy(s);
  return aSkillData;
}
public mapping QuerySkillData () { return aSkillData; }

public string *SetSkillserver (string *s) {
  if (pointerp(s))
    aSkServer = s - ({ 0, SKILLMASTER });
  else
    aSkServer = 0;
  return aSkServer;
}
public string *QuerySkillserver () { return aSkServer; }

public void AddSkillserver (string s) {
  if (!aSkServer)
    aSkServer = ({ s });
  else {
    aSkServer -= ({ s });
    aSkServer += ({ s });
  }
  aSkServer -= ({ 0, SKILLMASTER });
}

public void RemoveSkillserver (string s) {
  if (aSkServer)
    aSkServer -= ({ s });
}

#ifdef TRIAL
public mapping QuerySkillTrial() { return aSkillTrial; }
public mapping SetSkillTrial(mapping m) {
  return aSkillTrial = copy(m);
}
public int QueryTrialAttr(string name) {
  return aSkillTrial ? aSkillTrial[name] : 0;
}
public int SetTrialAttr(string name, int val) {
  if (!aSkillTrial)
    aSkillTrial = ([ name : val ]);
  else
    aSkillTrial[name] = val;
  return val;
}
#endif

//---------------------------------------------------------------------------

public int SetStatUsage (string stat, int nr) {
  aStatData[stat, STDATA_USAGE] = nr;
}
public int QueryStatUsage (string stat) { 
  return aStatData[stat, STDATA_USAGE]; 
}

public int SetStatBonus (string stat, int b) {
  aStatData[stat, STDATA_BONUS] = b;
}
public int QueryStatBonus (string stat) { 
  return aStatData[stat, STDATA_BONUS]; 
}

public int SetSkillUsage (string skill, int nr) {
  aSkillData[skill] = nr;
}
public int QuerySkillUsage (string skill) { 
  return aSkillData[skill]; 
}

//---------------------------------------------------------------------------

public static void RecomputeOneSkill ( string name)
{ 
  int val, rating, usage, dcc, app;
  mixed server;

  server = skillData[name, SKD_SERVER];
  if ((usage = QuerySkillUsage(name)) > 0) 
  {
    app = skillData[name, SKD_APP];
    dcc = skillData[name, SKD_DCC];
#ifdef TRIAL
    val = QueryTrialAttr(name) || QueryAttr(name);
    while (0 < usage--)
      val += (app - val + dcc + 1) / dcc;
    SetTrialAttr(name, val);
    SetTrialAttr(name+"-eff", (int)(server)->ComputeTrialSkill(name));
#else
    val = QueryAttr(name);
    while (0 < usage--)
      val += (app - val + dcc + 1) / dcc;
    SetAttr(name, val);
#endif
  }
  skillData[name, SKD_EFF] = (int)(server)->ComputeSkill(name);
  skillData[name, SKD_DIRTY] = 0;
  SetSkillUsage(name, 0);
}

public void RecomputeSkills () {
  string *key;
  int     i;
  key = m_indices(skillData);
  for (i = sizeof(key); i--; )
    skillData[key[i], SKD_DIRTY] = 1;
}

public void RecomputeStats () {
  string *key, stat;
  int *usage;
  int i, sum, nominal, new;

  // Count the uses done by skill applications
  key = m_indices(aSkillData);
  for (i = sizeof(key); i-- > 0; ) {
    stat = skillData[key[i], SKD_STAT1];
    if (stat)
      aStatData[stat, STDATA_USAGE] += aSkillData[key[i]];
    stat = skillData[key[i], SKD_STAT2];
    if (stat)
      aStatData[stat, STDATA_USAGE] += aSkillData[key[i]];
  }

  // Recompute stat values
  nominal = 25/STAT_SCALE + ((QueryXP() * (100/STAT_SCALE)) / 2000000);
  if (nominal > 75/STAT_SCALE)
    nominal = 75/STAT_SCALE;
  key = m_indices(aStatData);
  usage = m_values(aStatData);
  for (i = sum = 0; i < sizeof(usage); i++)
    sum += usage[i];
  for (i = sizeof(key); i--; ) {
    new =   nominal * (aStatData[key[i], STDATA_USAGE] * 6) / sum 
          + (nominal * aStatData[key[i], STDATA_BONUS]) / 100;
#ifdef TRIAL
    SetTrialAttr(key[i], new);
#else
    SetAttr(key[i], new);
#endif
  }

  RecomputeSkills();
}

//---------------------------------------------------------------------------
public static int LoadSkillData (string name) {
  string *sks;
  int i;
  mixed *data;

  sks = aSkServer + ({ SKILLMASTER });
  data = 0;
  for (i = 0; i < sizeof(sks) && !data; i++)
    data = sks[i]->ResolveSkill(name);
  if (!data)
    return 0;
  i--;
  skillData[name, SKD_SERVER] = sks[i];
  skillData[name, SKD_STAT1] = data[SKX_STAT1];
  skillData[name, SKD_STAT2] = data[SKX_STAT2];
  skillData[name, SKD_APP] = data[SKX_MAX_VALUE];
  skillData[name, SKD_DCC] = data[SKX_LEARN_FACTOR];
  skillData[name, SKD_DIRTY] = 1;
  skillData[name, SKD_EFF] = (int)sks[i]->ComputeSkill(name, data);
  aSkillData[name] = 0;
  return 1;
}

//---------------------------------------------------------------------------
// Basic routines.

public varargs int UseStat(string name, int diff) {
  aStatData[name, STDATA_USAGE] += 1;
  return ApplyValue(STAT_SCALE*QueryAttr(name), diff);
}

public varargs int UseSkill(string name, int diff) {
  int use, val;

  if (   !m_contains(&use, &val, skillData, name)
      && !LoadSkillData (name))
    val = 0;
  else {
    if (skillData[name, SKD_DIRTY])
      RecomputeOneSkill(name);
    val = skillData[name, SKD_EFF];
    aSkillData[name] += 1;
  }
  return ApplyValue(val, diff);
}

//---------------------------------------------------------------------------

public void create() {
  aStatData = m_allocate(0,STDATA_SIZE);
  aSkillData = ([]);
  skillData = m_allocate(0,SKD_SIZE);
}

public void reset() {
  RecomputeStats();
}

/***************************************************************************/
