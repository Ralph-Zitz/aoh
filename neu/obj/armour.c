/*---------------------------------------------------------------------------
** obj/armour
**
** Generic armour.
** TODO: Reasonable default values.
**
** 24-May-1994  [Mateese]
**---------------------------------------------------------------------------
*/

#include <config.h>
//#include "/neu/sys/combat.h"
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
public int SetClass (int class)

// Setup an armour.

{
  int     apq, limit;
  mapping def;

  if (   class < (limit = QueryMinClass())
      || class > (limit = QueryMaxClass())
     ) class = limit;

  switch (QueryArmourType()) {
    case AT_RING:
      def = ([]);
      apq = 5;
      break;
    case AT_AMULET:
      def = ([]);
      apq = 5;
      break;
    case AT_BOOTS:
      def = ([ DT_SLASH: (3*class)/5, DT_PIERCE: class/5
             , DT_BLUDGEON: class/5
            ]);
      apq = 1+class/10;
      break;
    case AT_CLOAK:
      def = ([ DT_SLASH: (3*class)/5, DT_PIERCE: class/5
             , DT_BLUDGEON: class/5
            ]);
      apq = 1+class/10;
      break;
    case AT_MAIL:
      def = ([ DT_SLASH: (2*class)/5, DT_PIERCE: class/5
             , DT_BLUDGEON: (2*class)/5
            ]);
      apq = 5+class/10;
      break;
    case AT_GLOVES:
      def = ([ DT_SLASH: (3*class)/5, DT_PIERCE: class/5
             , DT_BLUDGEON: class/5
            ]);
      apq = 1+class/10;
      break;
    case AT_HELMET:
      def = ([ DT_SLASH: (2*class)/5, DT_PIERCE: class/5
             , DT_BLUDGEON: (2*class)/5
            ]);
      apq = 5+class;
      break;
    case AT_SHIELD:
      def = ([ DT_SLASH: (2*class)/5, DT_PIERCE: class/5
             , DT_BLUDGEON: (2*class)/5
            ]);
      apq = 5+class/10;
      break;
    case AT_TROUSERS:
      def = ([ DT_SLASH: (3*class)/5, DT_PIERCE: class/5
             , DT_BLUDGEON: class/5
            ]);
      apq = 1+class/10;
      break;
    default:
      raise_error("Armour of illegal type.\n");
      return 0;
  }

  SetDefense(def);
  SetAPperQP(apq);
  return ::SetClass(class);
}

//---------------------------------------------------------------------------
public varargs void SetType (mixed type, int class)

// Setup an armour.

{
  string short, short2;
  int    weight, value, hands;

  if (pointerp(type)) {
    class = type[1];
    type = type[0];
  }

  switch (type = type || QueryArmourType()) {
    case AT_RING:
      short = "a ring"; short2 = "ring";
      value = 200;
      weight = 10;
      hands = 0;
      break;
    case AT_AMULET:
      short = "an amulet "; short2 = "amulet";
      value = 500;
      weight = 50;
      hands = 0;
      break;
    case AT_BOOTS:
      short = "a pair of boots "; short2 = "boots";
      value = 500;
      weight = 1500;
      hands = 0;
      break;
    case AT_CLOAK:
      short = "a cloak"; short2 = "";
      value = 630;
      weight = 1000;
      hands = 0;
      break;
    case AT_MAIL:
      short = "a mail"; short2 = "mail";
      value = 2000;
      weight = 6000;
      hands = 0;
      break;
    case AT_GLOVES:
      short = "a pair of gloves"; short2 = "gloves";
      value = 150;
      weight = 300;
      hands = 0;
      break;
    case AT_HELMET:
      short = "a helmet"; short2 = "helmet";
      value = 350;
      weight = 500;
      hands = 0;
      break;
    case AT_SHIELD:
      short = "a shield"; short2 = "shield";
      value = 1200;
      weight = 2000;
      hands = 1;
      break;
    case AT_TROUSERS:
      short = "a pair of trousers"; short2 = "trousers";
      value = 150;
      weight = 500;
      hands = 0;
      break;
    default:
      raise_error("Armour of illegal type.\n");
      return 0;
  }

  SetShort(short);
  if (type != AT_BOOTS && type != AT_GLOVES && type != AT_TROUSERS)
    SetLong("An ordinary "+short2+"\n");
  else
    SetLong("A pair of ordinary "+short2+".\n");
  SetIds(({ "armour", short2 }));
  SetValue(value);
  SetWeight(weight);
  SetArmourType(type || AT_AMULET);
  SetNumberHands(hands);
  SetClass(class);
}

//---------------------------------------------------------------------------

public void create() {
  ::create();
  SetUsage(EQM_WEAR);
  AddClassId(C_ARMOUR);
  SetMinClass(0);
  SetMaxClass(100/CLASS_SCALE);
  SetType(AT_AMULET, 5/CLASS_SCALE);
}

/***************************************************************************/
