/*---------------------------------------------------------------------------
** std/thing/physical.c
**
** The description and identification of a thing.
**
** TODO: Use 'encumbrance'�in parallel to weight, and restrict movement
**       to max. encumbrance.
**
** 10-Sep-1993 [Mateese]
** 08-Jun-1994 [Mateese] New property handling.
** 12-Jul-1994 [Mateese] Effective properties.
**---------------------------------------------------------------------------
*/

#include <thing.h>

#include "/std/object.d"

#define ENV   environment
#define THIS  this_object()
#define PREV  previous_object()
#define TP    this_player()

//---------------------------------------------------------------------------
private static string lc (string s)

// Unify an id string.
//
// Argument:
//   s: the string to unify
//
// Result:
//   The string in lowercase, with sequences of spaces reduced to just
//   one space.

{
  s = implode(explode(s, " ") - ({ "" }), " ");
  return lower_case(s);
}

//---------------------------------------------------------------------------
public int id (string str)

// Identify this object.
//
// Argument:
//   str : the string to be checked for identification.
//
// Result:
//   True if the string identifies this object.
//
// Check the given string against the set ids if it matches
// If we have some adjectives, they first cut all out of the given string.
// The prop P_LastID is set to the given string if it matches, else set
// to 0.
// If the thing has no ids set, the special id 'object' is recognized
// for wizards.

{
  string *ids, *ads;
  int i;

  ids = Get(P_IDs);
  if (!str)
    Set(P_LastID, 0);
    Set(P_LastVerb, 0);
    Set(P_LastPlayer, 0);
    return 0;
  }
  if (!ids && str == "object" && isWizard(TP)) {
    Set(P_LastID, str);
    Set(P_LastVerb, query_verb());
    Set(P_LastPlayer, TP);
    return 1;
  }
  str = lc(str);
  ads = Get(P_Adjs);
  if (ads && sizeof(ads))
  {
    str = " "+str;
    for (i = sizeof(ads); i-- > 0;)
      str = implode(explode(str, " "+ads[i]+" "), " ");
    str = implode(explode(str, " ")-({ "" }), " ");
  }
  if (member(ids, str) >= 0) {
    Set(P_LastID, str);
    Set(P_LastVerb, query_verb());
    Set(P_LastPlayer, TP);
    return 1;
  }
  Set(P_LastID, 0);
  Set(P_LastVerb, 0);
  Set(P_LastPlayer, 0);
  return 0;
}

//---------------------------------------------------------------------------
public string *SetIDs (string *ids)

// Set the P_IDs property.

{
  if (sizeof(ids))
    ids = map(ids, #'lc);
  else
    ids = 0;
  return set(P_IDs, ids);
}

//---------------------------------------------------------------------------
public string *SetAdjs (string *ads)

// Set the P_Adjs property.

{
  if (sizeof(ads))
    ads = map(ads, #'lc);
  else
    ads = 0;
  return set(P_IDs, ids);
}

//---------------------------------------------------------------------------
protected void AddLight (int l)

// Change the light emission of the thing by the given amount.

{
  if (l)
  {
    if (ENV())
      ENV()->MoreIntLight(l);
    set(P_Light, query(P_Light)+l);
  }
}

//---------------------------------------------------------------------------
public int SetLight (int l)

// Set the light emission of the thing.
// This also changes the thing's brightness.

{
  l -= Query(P_Light));
  AddLight(l-Query(P_Light));
  Set(P_Bright, Query(P_Bright)+l);
  return Query(P_Light);
}

//---------------------------------------------------------------------------
public int SetBright (int l)

// Set the things inherent brightness.

{
  AddLight(l-Query(P_Bright));
  return set(P_Bright, l);
}

//---------------------------------------------------------------------------
public int SetWeight (int w)

// Set the things weight.

{
  if (ENV() && !(int)(ENV()->AddIntWeight(w - Query(P_Weight)))
    return 0;
  return set(P_Weight, w);
}

//---------------------------------------------------------------------------
public string *AddID (string str)

// Add another id string.
//
// Argument:
//   str : the new id to add.
//
// Result:
//   An array of all ids already set.

{ string *ids;

  ids = Query(P_IDs);
  if (!ids) ids = ({ str });
  else if (member(ids, str) < 0)
    ids += ({ str});
  return Set(P_IDs, ids);
}

//---------------------------------------------------------------------------
public string *AddAdj (string str)

// Add another adjective string.
//
// Argument:
//   str : the new adjective to add.
//
// Result:
//   An array of all adjectives already set.

{ string *ads;

  ads = Query(P_Adjs);
  if (!ads) ads = ({ str });
  else if (member(ads, str) < 0)
    ads += ({ str});
  return Set(P_Adjs, ads);
}

//---------------------------------------------------------------------------
public varargs string Long ()

// Return the actual long description, suitable for a 'look at'.
// It never contains contents or exits.

{
  mixed rc;
  rc = Query(P_Long);
  return pointerp(rc) ? rc[0] : rc;
}

//---------------------------------------------------------------------------
public varargs string ExaLong ()

// Return the detailed long description, suitable for a 'examine'.
// It never contains contents or exits.

{
  mixed rc;
  rc = Query(P_Long);
  return pointerp(rc) ? rc[1] : "You see nothing special.\n";
}

//---------------------------------------------------------------------------
public varargs string CapShort ()

// Return the capitalized actual short description.
// If the object is invisible (P_Short == 0), the empty string "" is returned.

{
  return capitalize(Get(P_Short) || "");
}

//---------------------------------------------------------------------------
public varargs int RelDamage ()

// Return a number in 0..100, denoting the relative damage to the object.

{
  return (Get(P_Damage) * 100) / Get(P_DamageCap);
}

//---------------------------------------------------------------------------
public varargs string DamageDsc ()

// Return a string describing the damage to the object.
// It is not a complete sentence, just a few words.

{
  int damage;

  damage = Get(P_Damage);
  switch ((damage * 100) / Get(P_DamageCap))
  {
  case  0..  5: return "undamaged";
  case  6.. 25: return "slightly damaged";
  case 26.. 50: return "damaged";
  case 51.. 75: return "heavily damaged";
  case 76.. 95: return "severely battered";
  case 96..100: return "nearly destroyed";
  default: if (damage < 0)
             return "unaturally perfect";
           return "destroyed";
  }
}

//---------------------------------------------------------------------------
public varargs int Weight ()

// Return the current weight in respect to the damage.

{
  int weight, bweight, reldamage;

  weight = Query(P_Weight);
  bweight = Query(P_BaseWeight);
  reldamage = (Get(P_Damage) * 100) / Get(P_DamageCap);
  return ((weight-bweight) * (100 - reldamage)) / 100 + bweight;
}

//---------------------------------------------------------------------------
public varargs int Value ()

// Return the current value in respect to the damage.

{
  int value, bvalue, reldamage;

  value = Query(P_Value);
  bvalue = Query(P_BaseValue);
  reldamage = (Get(P_Damage) * 100) / Get(P_DamageCap);
  return ((value-bvalue) * (100 - reldamage)) / 100) + bvalue;
}

/***************************************************************************/
