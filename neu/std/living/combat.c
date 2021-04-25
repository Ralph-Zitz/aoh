/*---------------------------------------------------------------------------
** std/living/combat
**
** Basic combat handling.
**
** 21-Jan-1994  [Mateese]
**---------------------------------------------------------------------------
*/

#include "/neu/sys/properties.h"
#include "/neu/sys/combat.h"
#include "/neu/sys/combattune.h"
#include <living.h>
#include <messages.h>

#define THIS this_object()
#define TP   this_player()
#define ENV  environment
#define PREV previous_object()

inherit "/neu/std/living/hands";

//---------------------------------------------------------------------------

public int QueryStunned();                  // std/living/actions
public int QueryHP();                       // std/living/body
public int QueryMaxHP();                    // std/living/body
public varargs int DoDamage(int damage, int gentle);
                                            // std/living/body
public int AddXP(int xp);                   // std/living/body
public int QueryInvis();                    // std/living/description
public string QueryName();                  // std/living/description
public int QueryLoad();                     // std/living/restrictions
public int UseSkill(string name, int diff); // std/living/stats

//---------------------------------------------------------------------------

protected int     aCombatStyle;
protected mapping aDefenses;
protected static mapping pWeapons, pArmours;
protected static int damageMalus;
protected static int degradeHand;
protected static int *hitzones, sumHitp;

//---------------------------------------------------------------------------

public void create () {
  int i, j, zone;
  int *tmp;

  // Expand the hitzones array into hitzones.
  tmp = CT_HITZONES;
  sumHitp = 0;
  for (i = 2*AT_HITZONES-2; i >= 0; i -= 2)
    sumHitp += tmp[i];
  hitzones = allocate(2*sumHitp);
  for (i = zone = 0; i < 2*AT_HITZONES; i += 2)
    for (j = tmp[i]; j--; ) {
      hitzones[zone++] = (i / 2)+1;
      hitzones[zone++] = tmp[i+1];
    }
}

//---------------------------------------------------------------------------

public int QueryCombatStyle ()         { return aCombatStyle; }
public int SetCombatStyle (int m)      { return aCombatStyle = m; }

public mapping QueryDefense ()        { return aDefenses; }
public mapping SetDefense (mapping m) { return aDefenses = m; }

public void AddDefense (int dtype, int value)
{
  if (!aDefenses)
    aDefenses = m_allocate(0,1);
  aDefenses[dtype] = value;
}

public int QueryArmourType() { return AT_SKIN; }

public mapping Defense (mapping damage, object weapon, int success, int target)
{
  int *ind;
  int i, dt, d, dam;
  mapping soaked;

  if (!aDefenses)
    return damage;

  ind = m_indices(damage);
  soaked = ([]);
  for (i = sizeof(ind); i--;)
  {
    dt = ind[i];
    d = aDefenses[dt];
    if (d > (dam = damage[dt]))
      d = dam;
    damage[dt] -= d;
    soaked["TOTAL"] += d;
    soaked[dt] += d;
  }
  if (weapon)
    weapon->DegradeWeapon(soaked, damage, success, target);
  return damage;
}

public void DegradeWeapon ( mapping soaked, mapping damage, int success
                          , int target)
{
  degradeHand += soaked["TOTAL"];
}

//---------------------------------------------------------------------------

public int AddArmour (object armour, int flags)
{
  int at;
  string sh;

  if (armour->Query(P_NOWEAR))
    return 0;

  if (!pArmours)
    pArmours = m_allocate(0,ARMOUR_SIZE);

  at = armour->Query(P_ARMOUR_TYPE);
  if (!at)
    return 0;

  if (pArmours[at, ARMOUR_OBJ])
    pArmours[at]->Unwear(flags);

  if (!QueryInvis() && (sh = armour->Short()) && sh != "")
    show(QueryName()+" "+query_verb()+"s "+sh+".\n");
  pArmours[at, ARMOUR_OBJ] = armour;
  pArmours[at, ARMOUR_MSG] = !!function_exists("NotifyDefense", armour);
  armour->Set(P_WORN, THIS);
  return 1;
}

public varargs void RemoveArmour (object armour, int flags)
{
  int at;
  string sh;

  at = armour->Query(P_ARMOUR_TYPE);
  if (!pArmours || !at || pArmours[at, ARMOUR_OBJ] != armour)
    return;
  pArmours = m_delete(pArmours, at);
  if (PREV != armour) { // Prevent recursion
    armour->Unwear(flags);
    if (!QueryInvis() && (sh = armour->Short()) && sh != "")
      show(QueryName()+" "+query_verb()+"s "+sh+".\n");
  }
  armour->Set(P_WORN, 0);
}

public mapping QueryArmours () {
  return pArmours;
}

public mapping SetArmours (mapping m) {
  pArmours = m;
  return pArmours;
}

//---------------------------------------------------------------------------

public int AddWeapon (object weapon)
{
  string sh;

  if (weapon->Query(P_NOWIELD))
    return 0;

  if (!pWeapons)
    pWeapons = m_allocate(0,WEAPON_SIZE);

  if (!pWeapons[weapon])
  {
    pWeapons[weapon, WEAPON_TYPE] = weapon->Query(P_WEAPON_TYPE);
    pWeapons[weapon, WEAPON_MOD] = !!function_exists("ModifyAttack", weapon);
    weapon->Set(P_WIELDED, THIS);
    if (!QueryInvis() && (sh = weapon->Short()) && sh != "")
      show(QueryName()+" "+query_verb()+"s "+sh+".\n");
  }
  return 1;
}

public varargs void RemoveWeapon (object weapon, int flags)
{
  string sh;

  if (!pWeapons || !pWeapons[weapon])
    return;
  pWeapons = m_delete(pWeapons, weapon);
  if (PREV != weapon) { // Prevent recursion
    weapon->Unwield(flags);
    if (!QueryInvis() && (sh = weapon->Short()) && sh != "")
      show(QueryName()+" "+query_verb()+"s "+sh+".\n");
  }
  weapon->Set(P_WIELDED, 0);
}

public mapping QueryWeapons () { return pWeapons; }
public mapping SetWeapons (mapping m) { return pWeapons = m; }

//---------------------------------------------------------------------------
public varargs int Hit (object victim, mixed weapon, int gentle)

// Hit a victim with the weapon.

{
  int success;
  int rc;
  mapping damage;
  string *msgs;

  if (objectp(weapon))
  {
    success = UseSkill(pWeapons[weapon, WEAPON_TYPE], -QueryCombatStyle());
    if (pWeapons[weapon, WEAPON_MOD])
      success = weapon->ModifyAttack(victim, success);
    damage = 0;
    usedHand = 0;
  }
  else
  {
    success = UseSkill(CS_HAND, 0);
    damage = aHands[weapon, HAND_DAM];
    usedHand = weapon;
  }

  if (success > 0) {
#ifdef CT_LOAD_PENALTY
    CT_LOAD_PENALTY
#else
    // Load = 40%..200%: success reduces down to 1/20
    rc = QueryLoad() - LOAD_THRESH/2;
    if (rc > 0)
      success -= (success * 9 * rc) / (1000 - 5*LOAD_THRESH);
#endif
    success -= victim->Dodge(weapon, success);
  }

  if (success > 0) // We did hit the victim
  {
    if (objectp(weapon))
      damage = weapon->Damage(victim, success);
    else
    {
      if (intp(damage))
        damage = ([ DT_BLUDGEON:damage ]);
      else if (closurep(damage))
        damage = funcall(damage, weapon, victim, success);
      weapon = THIS;
    }
    rc = victim->Defend(damage, weapon, success, gentle);
    AddXP(rc);
  }
  else // We missed the victim
  {
    victim->NotifyMiss(weapon, success);
    say(({ MSG_MISS, victim, weapon }));
    if (objectp(weapon)) {
#ifdef CT_SELF_WEAPON
      CT_SELF_WEAPON
#else
      degradeHand += (-success)/2; // Will be divided by 10 later.
#endif
      msgs = weapon->NotifyHit(TP, victim, damage, success, 0);
    }
    else {
#ifdef CT_SELF_HAND
      CT_SELF_HAND
#else
      degradeHand += (-success)/4; // Will be divided by 10 later.
#endif
      msgs = NotifyHit(TP, victim, damage, success, 0);
    }

    if (msgs)
    {
      tell_object(TP, msgs[0]);
      tell_object(victim, msgs[1]);
      show_room(ENV(TP), msgs[2], ({ TP, victim }) );
    }
    rc = success;
  }
  if (degradeHand > 10) {
    degradeHand /= 10;
    tell_object(TP, "  You hurt yourself.\n");
    damageMalus += degradeHand;
    if (damageMalus > 50)
      damageMalus = 50;
    if (degradeHand > 0)
      DoDamage(degradeHand, 2);
    degradeHand = 0;
  }
  return rc;
}

//---------------------------------------------------------------------------
public int Dodge(mixed weapon, int success)

// Return the value by which the attacker's success is to be
// decreased due to a successful dodge.

{
  int rc;

  if (!QueryStunned()) {
#ifdef CT_DODGE
    CT_DODGE
#else
    rc = UseSkill(CS_DODGE, damageMalus+QueryLoad()/2+QueryCombatStyle());
    damageMalus = 0;
#endif
  }
  return rc >= 0 ? rc : 0;
}

//---------------------------------------------------------------------------
public mapping CalcDefense ( mapping damage, mixed weapon
                           , int success, int target )

// Defend a hit by 'weapon' of max. 'damage' at zone 'target',
// returning the remaining damage.

{
  mixed *ind;
  int i, at;
  mapping defense;

  if (pArmours)
  {
    ind = m_indices(pArmours);
    for (i = sizeof(ind); i--;)
    {
      at = ind[i];
      damage = pArmours[at, ARMOUR_OBJ]->Defense(damage,weapon,success,target);
    }
  }

  return Defense(damage, weapon, success, target);
}

//---------------------------------------------------------------------------
public varargs int Defend ( mapping damage, object weapon, int success
                          , int gentle)

// Defend a hit by 'weapon', done with given 'success' causing 'damage'
// If 'weapon' is 0, it is no real combat damage, just an
// elaborate 'DoDamage()'

{
  int lasthp, damhp;
  string *msgs;
  int target, factor;

  damhp = 0;
  lasthp = QueryHP() || 1;

  damage = copy(damage);

  if (!weapon || success > 0)  // Compute damage done
  {
    mixed *ind;
    int i, j, dt;

    if (!weapon) {
      // No weapon, no hitzone.
      target = 0;
      factor = 10;
    }
    else {
      i = random(sumHitp)*2;
      target = hitzones[i];
      factor = hitzones[i+1];

      ind = m_indices(damage);
      for (i = sizeof(ind); i--;)
      {
        dt = ind[i];
#ifdef CT_DAMAGE_MOD
        CT_DAMAGE_MOD
#else
        if (!(damage[dt] = (damage[dt] * (success+25)) / 100))
          damage[dt] = 1;
#endif
      }
    } // if (weapon)

    damage = CalcDefense(damage, weapon, success, target);

    ind = m_indices(damage);
    for (i = sizeof(ind); i--;)
    {
      dt = ind[i];
      j = damage[dt];
#ifdef CT_FINAL_DAMAGE
      CT_FINAL_DAMAGE
#else
      if (dt >= 0)
        j = (j * factor) / 10;
      if (j > 0)
        damhp += j;
#endif
    }
    if (damhp < 1)
      damhp = 1;

    if (gentle && (   !query_once_interactive(THIS) 
                   || 0 > TP->UseSkill(  stringp(weapon) 
                                       ? weapon : weapon->QueryWeaponType()
                                       , 100-success))
       )
      gentle = 0;

    if (gentle) {
      if (gentle < 0) 
        gentle = (-gentle)*QueryMaxHP() / 100;
      if (lasthp-damhp < gentle)
        damhp = lasthp-gentle;
    }
  }

  say(({ MSG_HIT, TP, THIS, weapon, success, damage, (damhp*100)/lasthp }));
  if (weapon &&
      (msgs = weapon->NotifyHit(TP, THIS, damage, success, (damhp*100)/lasthp))
     )
  {
    tell_object(TP, msgs[0]);
    if (THIS != TP) tell_object(THIS, msgs[1]);
    show_room(ENV(TP), msgs[2], ({ TP, THIS }) );
  }

  if (   success > 0 && pArmours && target > 0
      && pArmours[target, ARMOUR_MSG] && pArmours[target, ARMOUR_OBJ]
     )
  {
    msgs = pArmours[target, ARMOUR_OBJ]->NotifyDefense( weapon, damage, success
                                                      , (damhp*100) / lasthp);
    if (msgs && weapon)
    {
      tell_object(TP, msgs[0]);
      if (THIS != TP) tell_object(THIS, msgs[1]);
      show_room(ENV(TP), msgs[2], ({ TP, THIS }) );
    }
  }

#ifdef CT_DAMAGE_MALUS
  CT_DAMAGE_MALUS
#else
  damageMalus = (damhp * 100) / QueryMaxHP();
  if (damageMalus > 50)
    damageMalus = 50;
#endif
  return DoDamage(damhp, gentle);
}

/***************************************************************************/
