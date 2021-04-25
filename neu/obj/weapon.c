/*---------------------------------------------------------------------------
** obj/weapon
**
** Generic weapon.
** TODO: Reasonable default values.
**
** 04-Jan-1994  [Mateese]
**---------------------------------------------------------------------------
*/

#include <config.h>
#include "/neu/sys/combat.h"
#include <moving.h>
#include <classes.h>
#include <equipment.h>

inherit "/neu/std/combat";
inherit "/std/thing";

#define THIS this_object()
#define TP   this_player()
#define ENV  environment()

private static int pMaxClass, pMinClass;

//---------------------------------------------------------------------------

public int QueryMaxClass ()    { return pMaxClass; }
public int SetMaxClass (int i) { return pMaxClass = i; }

public int QueryMinClass ()    { return pMinClass; }
public int SetMinClass (int i) { return pMinClass = i; }

//---------------------------------------------------------------------------

public void init () {
  thing::init();
  combat::init();
}

public varargs int move (mixed dest, int method, mixed extra) {
  int rc;
  rc = thing::move(dest, method, extra);
  if (rc == ME_OK)
    notify_move(dest, method, extra);
  return rc;
}

public int remove() {
  notify_move(0, M_DESTRUCT);
  return thing::remove();
}

public int QueryValue()         { return modifyValue(::QueryValue()); }
public string Long(string what) { return modifyLong(::Long(what)); }
public string InvShort (string what) { 
  return modifyInvShort(::InvShort(what)); 
}
public string Short (string what) { 
  return modifyShort(::Short(what)); 
}

//---------------------------------------------------------------------------
public varargs int SetClass (int class)

// Setup a weapon.

{
  int     wpq, limit;
  mapping dam;

  if (   class < (limit = QueryMinClass())
      || class > (limit = QueryMaxClass())
     ) class = limit;

  SetUsage(EQM_WIELD);
  SetDefense(0);
  SetAPperQP(0);

  switch (QueryWeaponType()) {
    case CS_KNIFE:
      dam = ([DT_SLASH: class/2, DT_PIERCE: class/2]);
      wpq = 2+class/5;
      break;
    case CS_SWORD:
      dam = ([ DT_SLASH: (4*class)/5, DT_PIERCE: class/10
             , DT_BLUDGEON: class/10
             ]);
      wpq = 5+class/10;
      break;
    case CS_AXE:
      dam = ([DT_SLASH: (2*class)/3, DT_BLUDGEON: class/3]);
      wpq = 10+class;
      break;
    case CS_SPEAR:
      dam = ([DT_PIERCE: (2*class)/3, DT_BLUDGEON: class/3]);
      wpq = 10+class/10;
      break;
    case CS_CLUB:
      dam = ([DT_BLUDGEON: class]);
      wpq = 10+class/10;
      break;
    case CS_STAFF:
      SetUsage(EQM_BOTH);
      SetNumberHands(2);
      dam = ([DT_BLUDGEON: (2*class)/3, DT_PIERCE: class/3]);
      wpq = 5+class/10;
      SetDefense(([ DT_SLASH: (2*class)/5, DT_BLUDGEON: (2*class)/5
                  , DT_PIERCE: class/5
                ]));
      SetAPperQP(5+class);
      break;
    case CS_WHIP:
      SetNumberHands(1);
      dam = ([DT_SLASH: class]);
      wpq = 5+class/10;
      break;
    case CS_INFANTRY:
      SetNumberHands(2);
      dam = ([DT_BLUDGEON: (2*class)/3, DT_PIERCE: class/3]);
      wpq = 10+class/10;
      break;
    case CS_CHAIN:
      SetNumberHands(1);
      dam = ([DT_BLUDGEON: (2*class)/3, DT_SLASH: class/3]);
      wpq = 5+class/10;
      break;
    default:
      raise_error("Weapon of illegal type.\n");
      return 0;
  }

  SetWPperQP(wpq);
  SetDamage(dam);
  return ::SetClass(class);
}

//---------------------------------------------------------------------------
public varargs void SetType (mixed type, int class)

// Setup a weapon.

{
  string short, short2, *verbs;
  int    weight, value;

  if (pointerp(type)) {
    class = type[1];
    type = type[0];
  }

  switch (type || QueryWeaponType()) {
    case CS_KNIFE:
      short = "a knife"; short2 = "knife";
      verbs = ({ "stab", "stabs" });
      value = 50;
      weight = 300;
      break;
    case CS_SWORD:
      short = "a sword"; short2 = "sword";
      verbs = ({ "slash", "slashes" });
      value = 500;
      weight = 1000;
      break;
    case CS_AXE:
      short = "an axe"; short2 = "axe";
      verbs = ({ "swing", "swings" });
      value = 700;
      weight = 2000;
      break;
    case CS_SPEAR:
      short = "a spear"; short2 = "spear";
      verbs = ({ "stab", "stabs" });
      value = 500;
      weight = 2000;
      break;
    case CS_CLUB:
      short = "a club"; short2 = "club";
      verbs = ({ "swing", "swings" });
      value = 500;
      weight = 2000;
      break;
    case CS_STAFF:
      short = "a staff"; short2 = "staff";
      verbs = ({ "charge", "charges" });
      value = 300;
      weight = 2000;
      break;
    case CS_WHIP:
      short = "a whip"; short2 = "whip";
      verbs = ({ "lash", "lashes" });
      value = 300;
      weight = 500;
      break;
    case CS_INFANTRY:
      short = "a halber"; short2 = "halberd";
      verbs = ({ "hack", "hacks" });
      value =5300;
      weight = 2000;
      break;
    case CS_CHAIN:
      short = "a morningstar"; short2 = "morningstar";
      verbs = ({ "flail", "flails" });
      value =5300;
      weight = 1500;
      break;
    default:
      raise_error("Weapon of illegal type.\n");
      return 0;
  }

  SetShort(short);
  SetLong("An ordinary "+short2+"\n");
  SetIds(({ "weapon", short2 }));
  SetValue(value);
  SetWeight(weight);
  SetAttackVerbs(verbs);
  SetWeaponType(type || QueryWeaponType());
  SetNumberHands(1);
  SetClass(class);
}

//---------------------------------------------------------------------------

public void create() {
  ::create();
  SetUsage(EQM_WIELD);
  AddClassId(C_WEAPON);
  SetMinClass(0);
  SetMaxClass(100/CLASS_SCALE);
  SetType(CS_KNIFE, 5/CLASS_SCALE);
}

/***************************************************************************/

