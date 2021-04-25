/*
 * std/weapon.c
 *
 * A luxury frontend to the generic weapon.
 * The real work is done in std/weapon/combat
 *
 * Version 4.1  compat/native compatible
 * Version 4.2  Pepel fixed lots of things. Threw out many backward compats
 *              Mateese made it plushy
 * Version 4.3  Cadra changed SetStandard a little bit 09jan97
 * Version 4.4  Thragor changed SetStandard that it fits to AddItem, i. e.
 *              it now accepts an array as first argument, telling what
 *              weapon-type the weaopn has, which wc and which size, 21jan97
 */

#include <combat.h>
#include <moving.h>
#include <classes.h>
#include <properties.h>

inherit "std/thing";
inherit "std/weapon/combat";

#define TO this_object();

public int QueryOldCombat() { return 1;}

static int helpcalc(int wc,int size) {
  int i;
  switch (size) {
    case PSIZE_LARGE: i=3; break;
    case PSIZE_SMALL: i=1; break;
    default: i=2;
    }
  if (wc < 10) return i;
  if (wc < 16) return i+1;
  return i+2;
}

varargs int SetStandard (mixed weapon_type, int wc, int size)
  // weapon_type may also be an array of the form:
  // ({weapon_type, wc, size})
{
  int wt;

  // spread array to the values (if weapon_type is an array)
  if (pointerp(weapon_type))
    return apply(#'SetStandard /*'*/,weapon_type);

  if (!VALID_WEAPON_TYPE(weapon_type)) return 0;
  wt = member(({ WT_SWORD, WT_AXE, WT_CLUB, WT_SPEAR, WT_KNIFE, WT_STAFF, WT_WHIP
	, WT_INFANTRY, WT_CHAIN, WT_RAPIER, WT_HAMMER })
		    , weapon_type);
  SetShort ( ({ "a sword"
	      , "an axe"
	      , "a club"
	      , "a spear"
	      , "a knife"
	      , "a staff"
	      , "a whip"
	      , "a halberd"
	      , "a morningstar"
	      , "a rapier"
	      , "a hammer"
	     })[wt]
	   );
  SetLong ("An ordinary "+strip_a(QueryShort())+".\n");
  SetSize(size);
  SetResetQuality(99);
  SetWeight ( ({ 1500 * helpcalc(wc,size)
	       , 2000 * helpcalc(wc,size)
	       , 1600 * helpcalc(wc,size)
	       , 1200 * helpcalc(wc,size)
	       ,   30 * helpcalc(wc,size)
	       ,  700 * helpcalc(wc,size)
	       ,  500 * helpcalc(wc,size)
	       , 2100 * helpcalc(wc,size)
	       , 1600 * helpcalc(wc,size)
	       ,  900 * helpcalc(wc,size)
	       , 2000 * helpcalc(wc,size)
	      })[wt]
	   );
  SetWPperQP ( ({ 10
                ,  9
                , 12
                ,  5
                , 12
                ,  8
                , 15
                ,  6
                ,  8
                ,  7
                ,  9
               })[wt]
             );
  SetValue ( ({ 500 + wc * 350 /* 500 .. 7500 */
	      , 700 + wc * 365 /* 700 .. 8000 */
	      , 500 + wc *  50 /* 500 .. 1500 */
	      , 500 + wc * 350 /* 500 .. 7500 */
	      ,  50 + wc *  47 /*  50 .. 1000 */
	      , 300 + wc *  35 /* 300 .. 1000 */
	      , 200 + wc *  20 /* 200 ..  600 */
	      , 850 + wc * 350 /* 850 .. 7850 */
	      , 500 + wc * 300 /* 500 .. 6500 */
	      , 500 + wc * 350 /* 500 .. 7500 */
	      , 500 + wc * 150 /* 500 .. 3500 */
	     })[wt]
	   );
  // Values approved by VAW of Finance, Thragor, 16jan97
  SetWeaponType (weapon_type); /* Note: Does a SetDamType() on its own */
  SetWC (wc);
  SetWeaponHands (wc <= 12 ? 1 : 2);

  return 1;
}

public varargs int move (mixed dest, int method, mixed extra) {
  int rc;
  rc = thing::move(dest, method, extra);
  if (rc == ME_OK)
    inform_move(dest, method, extra);
  return rc;
}

public varargs int remove() {
  inform_move(0, M_DESTRUCT);
  return thing::remove();
}

public mixed QueryHelpMsg()
{
  mixed rc;
  rc = ::QueryHelpMsg();
  if (!rc)
  {
    rc = "This "+strip_a(QueryShort())
	       +" helps fighting enemies while being wielded.\n";
    if (QueryWeaponHands() == 1)
      rc += "You need one hand to wield it.\n";
    else if (QueryWeaponHands() > 1)
      rc += "You need "+itoa(QueryWeaponHands())+" hands to wield it.\n";
  }
  return rc;
}

public varargs int QueryValue(mixed flag)
{
  if (flag) return (::QueryValue());
  return modifyValue(::QueryValue());
}

public varargs string Long(string what) { return modifyLong(::Long(what)); }
public varargs string InvShort (string what) {
  int q;
  string rc;
  q = QueryQuality();
  SetQuality(100);
  rc = ::InvShort(what);
  SetQuality(q);
  return modifyInvShort(rc);
}
public varargs string Short (string what) {
  return modifyShort(::Short(what));
}

void create() {
  thing::create();
  combat::create();
  AddClassId(C_WEAPON);
  SetStandard(WT_KNIFE, 1);
}

void init() {
  thing::init();  /* does the add_action for 'show' */
  combat::init();
}
