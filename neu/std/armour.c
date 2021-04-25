/*---------------------------------------------------------------------------
** std/armour
**
** Compatibility object, quick and dirty.
**---------------------------------------------------------------------------
*/

#include <equipment.h>
#include "/neu/sys/combat.h"
#define THIS this_object()

inherit "neu/obj/armour";

static object owner, wear_ob, def_ob, remove_ob;
static int worn, weaponhands;

public int QueryOldCombat() { return 1;}

object SetWorn(object o) { worn = !!o; return owner = ::SetWorn(o); }

int SetDefence (int what, int value) { AddDefense(what, value); return value; }
int SetStandard (int type, int ac) { SetType(type, ac); return 1; }

static object wear_ob, def_ob, remove_ob;

int SetAC (int i) { return SetClass(i); }
int QueryAC () { return QueryClass(); }

int SetWeaponHands(int h) { return weaponhands = SetNumberHands(h); }
int QueryWeaponHands() { return QueryNumberHands(); }

object SetWearObj (object ob) { return wear_ob = ob; }
object SetRemoveObj (object ob) { return remove_ob = ob; }
object SetDefendObj (object ob) { return def_ob = ob; }
object QueryWearObj () { return wear_ob; }
object QueryRemoveObj () { return remove_ob; }
object QueryDefendObj () { return def_ob; }

int wear(object own) { return !Wear(own,1); }
int unwear () { return !Unwear(); }

public varargs mixed Wear (object living, int is_armour) {
  object obj;
  if (objectp(obj = QueryWearObj()) && !obj->CheckWear(living))
    return EQ_FORBID;
  return ::Wear(living, is_armour);
}

public int Unwield () {
  object obj, liv;
  liv = QueryWorn();
  if (   objectp(liv) 
      && objectp(obj = QueryRemoveObj()) 
      && !obj->CheckRemove(liv)
     )
    return EQ_FORBID;
  return ::Unwield();
}

int CalcDefend (object enemy, int damage, int dtype) {
  return ::QueryDefense()[dtype];
}

public mapping Defense (mapping damage, object weapon, int success, int target)
{
  object  obj;
  int t,  dam;
  mapping def;
  int     dtype;

  def = ::Defense(damage, weapon, success, target);
  dtype = m_indices(damage)[0];
  dam = damage[dtype];

  if (!(   objectp(obj = QueryDefendObj()) 
         && (t = (int)obj->DefendHit(this_player(), dam, dtype)))
     )
    t = CalcDefend(this_player(), dam, dtype);
  def[dtype] -= t;
  return def;
}

int Defend (object enemy, int damage, int dtype)
{
  object obj;

  if (objectp(obj = QueryDefendObj())) 
    return (int)obj->DefendHit(enemy, damage, dtype);
  return CalcDefend(enemy, damage, dtype);
}
