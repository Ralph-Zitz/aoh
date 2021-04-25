/*---------------------------------------------------------------------------
** std/living/hands
**
** The code to simulate the hands of a living.
**
** 28-Oct-1993  [Mateese]
**---------------------------------------------------------------------------
*/

#include <living.h>
#include "/neu/sys/properties.h"

#define THIS this_object()
#define TP   this_player()
#define PREV previous_object

protected        mapping aHands;
protected        int     aFreeHands;
protected static string usedHand;
protected static mixed pEquipObjs;

public string QueryPossessive();                // std/living/description
public varargs object QueryRaceObj(int reload); // std/living/description

//---------------------------------------------------------------------------
public string * NotifyHit ( object attacker, object victim
                          , mapping damage, int success, int rel_dam)

// Return the combat messages for the given attack situation.

{
  string aname, vname, avname, what, elsewhat;
  string *result, *verbs;

  result = allocate(3);

  aname = capitalize(attacker->QueryName());
  if (attacker != victim)
    avname = vname = capitalize(victim->QueryName());
  else {
    vname = attacker->QueryObjective()+"self";
    avname = "yourself";
  }

  if (usedHand) {
    what = "your "+usedHand;
    elsewhat = QueryPossessive()+" "+usedHand;
  }
  else {
    what = "your hand";
    elsewhat = "a hand";
  }
  usedHand = 0;

  verbs = ({ "attack", "attacks" });
  result[0] = "  You "+verbs[0]+" with "+what;
  result[1] = "  "+aname+" "+verbs[1]+" with "+elsewhat;
  result[2] = "  "+aname+" "+verbs[1]+" with "+elsewhat;

  verbs = "/neu/std/combat"->HitMsgs(success > 0 ? rel_dam : -1);

  result[0] += verbs[0]+" "+avname+verbs[2]+".\n";
  result[1] += verbs[1]+" you"+verbs[2]+".\n";
  result[2] += verbs[1]+" "+vname+verbs[2]+".\n";

  return result;
}

//---------------------------------------------------------------------------

public varargs mixed QueryEquipObj(int sc) {
  mixed * rc;

  if (sc)
    return pEquipObjs;
  rc = ({ THIS, QueryRaceObj() }) - ({ 0 });
  return (rc+((pointerp(pEquipObjs) ? pEquipObjs : ({ pEquipObjs }))-rc)) 
         - ({ 0 });
}
public mixed SetEquipObj(mixed m) { 
  if (pointerp(m))
    return pEquipObjs = m - ({ THIS, QueryRaceObj() }); 
  return pEquipObjs = m;
}

public void AddEquipObj (mixed obj)
{
  mixed eqobj;

  eqobj = QueryEquipObj();
  if (!eqobj)
    SetEquipObj(({ obj }));
  else if (!pointerp(eqobj))
    SetEquipObj(({ eqobj, obj}));
  else
    SetEquipObj(eqobj-({ 0, obj })+({ obj }));
}

public void RemoveEquipObj (mixed obj)
{
  mixed eqobj;

  eqobj = QueryEquipObj();
  if (!pointerp(eqobj) && eqobj == obj)
    SetEquipObj(0);
  else if (pointerp(eqobj))
    SetEquipObj(eqobj-({ 0, obj }));
}

//---------------------------------------------------------------------------

public int QueryFreeHands()    { return aFreeHands; }
public int SetFreeHands(int i) { return aFreeHands; }

public int CountHands () {
  string *ind;
  int i;

  aFreeHands = 0;
  ind = m_indices(aHands);
  for (i = sizeof(ind); i--;)
    if (!aHands[ind[i], HAND_OBJ])
      aFreeHands++;
  return aFreeHands;
}

public mapping QueryHands ()        { return aHands; }
public mapping SetHands (mapping h) {
  aHands = h;
  CountHands();
  return aHands;
}

public void AddHand (string sh, mixed dam, mixed obj)
{
  int oldsize;

  if (!aHands)
    aHands = m_allocate(0,HAND_SIZE);

  oldsize = sizeof(aHands);

  if (mappingp(dam))
    aHands[sh, HAND_DAM] = copy(dam);
  else
    aHands[sh, HAND_DAM] = dam;
  if (aHands[sh, HAND_OBJ] || oldsize != sizeof(aHands))
    aFreeHands++;
  aHands[sh, HAND_OBJ] = obj;
  if (obj)
    aFreeHands--;
}

//---------------------------------------------------------------------------
public int Grip (object obj)

// Grip the given <obj> if not already done and enough heads are free.

{
  int nr, i;
  string *h;
  mixed eqobj, *rc;

  if (!aHands)
    return 0;

  for (i = 0; i <sizeof(h); i++)
    if (aHands[h[i], HAND_OBJ] == obj)
      return 1;

  nr = obj->Query(P_NUMBER_HANDS) || 1;
  if (nr > aFreeHands)
    return 0;

  if (eqobj = QueryEquipObj()) {
    if (!pointerp(eqobj))
      eqobj = ({ eqobj });
  }
  else 
    eqobj = ({ });

  rc = map_objects(eqobj-({ 0 }), "ChkGrip", THIS, obj) - ({ EQ_OK });
  if (sizeof(rc))
    return stringp(rc[0]) ? rc[0] : 0;

  h = m_indices(aHands);

  for (i = 0; nr && i <sizeof(h); i++)
    if (!aHands[h[i], HAND_OBJ])
    {
      aHands[h[i], HAND_OBJ] = obj;
      aFreeHands--;
      nr--;
    }

  if (eqobj)
    filter_objects(eqobj-({ 0 }), "NotifyGrip", THIS, obj);

  return 1;
}

//---------------------------------------------------------------------------
public void Ungrip (object obj)

// Release the given <obj>ect if gripped.

{
  int i;
  string *h;
  mixed eqobj, *rc;

  if (!aHands)
    return;

  if (eqobj = QueryEquipObj()) {
    if (!pointerp(eqobj))
      eqobj = ({ eqobj });
  }
  else 
    eqobj = ({ });

  rc = map_objects(eqobj-({ 0 }), "ChkUngrip", THIS, obj) - ({ EQ_OK });
  if (sizeof(rc))
    return stringp(rc[0]) ? rc[0] : 0;

  h = m_indices(aHands);

  for (i = sizeof(h); i--;)
    if (aHands[h[i], HAND_OBJ] == obj)
    {
      aHands[h[i], HAND_OBJ] = 0;
      aFreeHands++;
    }

  if (eqobj)
    filter_objects(eqobj-({ 0 }), "NotifyUngrip", THIS, obj);
}

//---------------------------------------------------------------------------
public varargs, mixed Wield (object obj, int flags) { 
  if (obj->Query(P_WEAPON_TYPE))
    flags = flags | EWF_WEAPON;
  else
    flags = flags & (~(EWF_WEAPON));
  return obj->Wield(THIS, flags);
}
public varargs mixed Unwield (object obj, int flags) { 
  return obj->Unwield(THIS, flags); 
}
public varargs mixed Wear (object obj, int flags) {
  if (obj->Query(P_ARMOUR_TYPE))
    flags = flags | EWF_WEAPON;
  else
    flags = flags & (~(EWF_WEAPON));
  return obj->Wear(THIS, flags); 
}
public varargs mixed Unwear (object obj, int flags)  { 
  return obj->Unwear(THIS, flags); 
}

/***************************************************************************/
