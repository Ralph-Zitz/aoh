/*---------------------------------------------------------------------------
** std/combat
**
** Everything needed to make a thing usable for combat.
** Optional functions: ModifyAttack(), NotifyDefense()
**
** 24-May-1994  [Mateese]
**---------------------------------------------------------------------------
*/

#include <config.h>
#include <search.h>
#include <equipment.h>
#include "/neu/sys/properties.h"
#include "/neu/sys/combat.h"

inherit "/std/equipment";

#define THIS this_object()
#define TP   this_player()
#define ENV  environment

//---------------------------------------------------------------------------

private int     pClass;
private int     pAPperQP, pWPperQP;
private int     pAType;
private mapping pDamage, pDefense;
private string  pWType, *pAVerbs;

//---------------------------------------------------------------------------

public int QueryClass ()               { return pClass; }
public varargs int SetClass (int i)    { return pClass = i; }

public int QueryArmourType ()          { return pAType; }
public int SetArmourType (int i)       { return pAType = i; }

public string QueryWeaponType ()       { return pWType; }
public string SetWeaponType (string s) { return pWType = s; }

public int QueryAPperQP ()             { return pAPperQP; }
public int SetAPperQP (int i)          { return pAPperQP = i; }

public int QueryWPperQP ()             { return pWPperQP; }
public int SetWPperQP (int i)          { return pWPperQP = i; }

//---------------------------------------------------------------------------
public void DegradeArmour ( mapping soaked, mapping damage, object weapon
                          , int success, int target)

// Degrade the armours quality

{
  int s, apq;

  s = soaked["TOTAL"];
  apq = QueryAPperQP();
  if (apq && s >= apq)
    SetQuality(QueryQuality() - s/apq);
}

//---------------------------------------------------------------------------
public void DegradeWeapon ( mapping soaked, mapping damage, int success
                          , int target)

// Degrade the weapons quality

{
  int s, wpq;

  s = soaked["TOTAL"];
  wpq = QueryWPperQP();
  if (wpq && s >= wpq)
    SetQuality(QueryQuality() - s/wpq);
}

//---------------------------------------------------------------------------

public mapping QueryDefense ()        { return pDefense; }
public mapping SetDefense (mapping m) { return pDefense = m; }

public void AddDefense (int dtype, int value)
{
  if (!pDefense)
    pDefense = m_allocate(0,1);
  pDefense[dtype] = value;
}

//---------------------------------------------------------------------------

public mapping QueryDamage ()        { return pDamage; }
public mapping SetDamage (mapping m) { return pDamage = m; }

public void AddDamage (int dtype, int value)
{
  if (!pDamage)
    pDamage = m_allocate(0,1);
  pDamage[dtype] = value;
}

//---------------------------------------------------------------------------

public string * QueryAttackVerbs ()
{
  return pAVerbs || ({ "attack", "attacks" });
}
public string * SetAttackVerbs (string *m) { return pAVerbs = m; }

//---------------------------------------------------------------------------
private static void _damage (int type, int val, int quality) {
  val = (val * quality) / 105;
}

public mapping Damage (object victim, int success)

// Return the max. damage against victim.

{
  mapping dam;
  dam = copy(QueryDamage());
  walk_mapping(dam, #'_damage, QueryQuality()+5);
  return dam;
}

//---------------------------------------------------------------------------
public mapping Defense (mapping damage, object weapon, int success, int target)

// Reduce the given damage; physical damage only in the target zone.

{
  int *ind;
  int dt, i, q, d, dam;
  mapping soaked;

  if (!pDefense)
    return damage;

  ind = m_indices(damage);
  q = QueryQuality()+5;
  soaked = ([]);
  for (i = sizeof(ind); i--;)
  {
    dt = ind[i];
    if (dt >= 0 && target && pAType != target)
      continue;
    d = (pDefense[dt] * q) / 105;
    if (d > (dam = damage[dt]))
      d = dam;
    soaked["TOTAL"] += d;
    soaked[dt] += d;
    damage[dt] -= d;
  }
  DegradeArmour(soaked, damage, weapon, success, target);
  weapon->DegradeWeapon(soaked, damage, success, target);
  return damage;
}

//---------------------------------------------------------------------------
public string * HitMsgs (int rel_dam)

// Return the hit messages parts for the given hit success.

{
  string *result;

  result = allocate(3);

  if (rel_dam < 0)
  {
    result[0] = ", but miss";
    result[1] = ", but misses";
    result[2] = "";
  }
  else if (!rel_dam) {
    result[0] = ", but don't hurt";
    result[1] = ", but doesn't hurt";
    result[2] = "";
  }
  else
  {
    switch (rel_dam)
    {
    case  1.. 17:
      result[0] = "touch"; result[1] = "touches";
      result[2] = " slightly";
      break;
    case 18..35:
      result[0] = "graze"; result[1] = "grazes";
      result[2] = "";
      break;
    case 36..46:
      result[0] = "hit"; result[1] = "hits";
      result[2] = "";
      break;
    case 47..58:
      result[0] = "hit"; result[1] = "hits";
      result[2] = " hard";
      break;
    case 59..70:
      result[0] = "hit"; result[1] = "hits";
      result[2] = " very hard";
      break;
    case 71..90:
      result[0] = "smash"; result[1] = "smashes";
      result[2] = " with a bonecrushing sound";
      break;
    default:
      result[0] = "massacre"; result[1] = "massacres";
      result[2] = " to small fragments";
      break;
    }
    result[0] = " and "+result[0];
    result[1] = " and "+result[1];
  }
  return result;
}

//---------------------------------------------------------------------------
public string * NotifyHit ( object attacker, object victim
                          , mapping damage, int success, int rel_dam)

// Return the combat messages for the given attack situation.

{
  string aname, vname, avname, what;
  string *result, *verbs;

  result = allocate(3);

  aname = capitalize(attacker->QueryName());
  if (attacker != victim)
    avname = vname = capitalize(victim->QueryName());
  else {
    vname = attacker->QueryObjective()+"self";
    avname = "yourself";
  }

  what = THIS->Short();
  verbs = QueryAttackVerbs();
  result[0] = "You "+verbs[0]+" with "+what;
  result[1] = "  "+aname+" "+verbs[1]+" with "+what;
  result[2] = "  "+aname+" "+verbs[1]+" with "+what;

  verbs = HitMsgs(success > 0 ? rel_dam : -1);

  result[0] += verbs[0]+" "+avname+verbs[2]+".\n";
  result[1] += verbs[1]+" you"+verbs[2]+".\n";
  result[2] += verbs[1]+" "+vname+verbs[2]+".\n";

  return result;
}

/***************************************************************************/
