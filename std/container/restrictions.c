//---------------------------------------------------------------------------
// container/restrictions.c
//
// The light and weight handling of a container.
//---------------------------------------------------------------------------

#include <search.h>
#include <lock.h>

inherit "/std/room/restrictions";
inherit "/std/thing/restrictions";

//---------------------------------------------------------------------------

#define TO   this_object()
#define THIS this_object()
#define TP   this_player()
#define PRE previous_object()
#define ENV environment()

#define TRANS	  QueryTransparency() / 1000

public varargs int QueryTransparency(mixed m); // forward

//---------------------------------------------------------------------------

public static int Ptrans, Pweightchg;

public void create() {
  "/room/restrictions"::create();
  SetIndoors(0); // Turn off autolight
  SetIndoors(1);
  Ptrans = 1000;
  Pweightchg = 1000;
}

//---------------------------------------------------------------------------

public int QuerySunLight() {
  return ENV ? Ptrans * (({int})ENV->QuerySunLight()) / 1000 : ::QuerySunLight();
}

// tell_chef(msg) { object o;
//  if (o=find_player("mateese")) tell_object(o,msg);
// }
public void light_from_inside(int l) {
// tell_chef( sprintf("%O: lfi(%O, %O) by %O\n", THIS, l, l*TRANS, PRE));
//  if (l) {
    ::light_from_inside(l);
    ::emit_light(l * TRANS);
//  }
}

public void light_from_outside(int l) {
// tell_chef( sprintf("%O: lfo(%O, %O) by %O\n", THIS, l, l*TRANS, PRE));
//  if (l *= TRANS)
//    ::emit_intlight(l);
    ::emit_intlight(l*TRANS);
}

public void emit_light(int l) {
// tell_chef( sprintf("%O: el(%O, %O) by %O\n", THIS, l, l*TRANS, PRE));
//  if(l) {
    ::emit_light(l);
    ::emit_intlight(l * TRANS);
//  }
}

public void emit_intlight(int l) {
// tell_chef( sprintf("%O: eil(%O, %O) by %O\n", THIS, l, l*TRANS, PRE));
  if(l) {
    ::emit_intlight(l);
    ::emit_light(l * TRANS);
  }
}

public int SetTransparency(int t) {
  int oldenv, oldin;
  oldenv = ((ENV ? ({int})ENV->QueryIntLight() : 0) - QueryLight()) * TRANS;
  oldin = QueryLight()-QueryBright();
  // Remove our light from env/inv, then add new light.
  // During the removal, P_TRANS must be zero to avoid
  // circular miscalculations.
  Ptrans = 0;
  ::emit_light(-oldin);
  ::emit_intlight(-oldenv);
  Ptrans = t;
  ::emit_light((QueryIntLight() - QueryBright()) * TRANS);
  if (ENV)
    ::emit_intlight((({int})ENV->QueryIntLight() - QueryBright()) * TRANS);
  return Ptrans;
}
public varargs int QueryTransparency(mixed m) { return Ptrans; }

public int SetTransparent(int t) { return SetTransparency(t); }
public int QueryTransparent() { return QueryTransparency(); }

public int SetBright (int l) {
  "/room/restrictions"::SetBright(l);
  return "/thing/restrictions"::SetBright(l);
}

public int QueryBright () { return "/thing/restrictions"::QueryBright(); }

public varargs mixed Locate (mixed what, int mode) {
  if (   !(mode & SEARCH_IGNLIGHT)
      && QueryTransparency() < SEARCH_MIN_TRANSP
      && ({int})THIS->QueryLockState() != LOCK_OPEN
      && member(deep_inventory(THIS), TP) == -1
     )
    return (mode & SM_OBJECT) ? 0 : ({});
  return ::Locate(what, mode);
}

//---------------------------------------------------------------------------

private int CHARGE (int w) {
  return (w > 1000000 || w < -1000000)
         ? (w / 1000) * Pweightchg
	 : (w * Pweightchg) / 1000
  ;
}

public mixed FilterWeatherData (object client, mixed *data, int newstate) {
  if (QueryTransparency() < 500)
    return 0;
  return data;
}

public int MayAddWeight(int w) {
  int myweight;
  // The inherited MayAddWeight() doesn't care about the objects
  // own weight, so we have to compensate here for it.
  myweight =  QueryWeight() - QueryWeightContent();
  return !w || (::MayAddWeight(CHARGE(w) +  myweight) &&
		(ENV ? ({int})ENV->MayAddWeight(CHARGE(w)) : 1));
}

public void AddIntWeight (int w) {
  ::AddIntWeight(w);
  if(ENV) ENV->AddIntWeight(CHARGE(w));
}

public varargs int QueryWeight (int flag) {
  return ::QueryWeight() + (flag ? 0 : CHARGE(QueryWeightContent()));
}

public int QueryWeightChg() { return Pweightchg; }
public int SetWeightChg(int c) {
  if(QueryWeightContent() && ENV &&
     !({int})ENV->AddIntWeight(CHARGE((  QueryWeight()
				      + CHARGE(QueryWeightContent()
				   ))
			     * (c - Pweightchg))))
    return Pweightchg;
  return Pweightchg = c;
}
