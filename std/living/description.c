/* std/living/description
**
** The outer appearance and senses of the living (or do you want to have
** /std/living/skin, /std/living/eyes, ... ?)
** It's just everything not covered by life-functions or combat.
** This includes informations like 'QueryIsPlayer()' or the vision functions.
**
**  26-Jan-94 [Mateese]
**  19-nov-97 [Softbyte]  QueryIsWizard added
*/

#include <secure/wizlevels.h>
#include <driver/object_info.h>
#include <properties.h>
#include <attributes.h>
#include <search.h>
#include <living.h>
#include <combat.h>

inherit "/std/room/description";
inherit "/std/thing/description";

#define THIS  this_object()
#define TP    this_player()
#define ENV   environment

#define SUBID  (to_string(this_object())+":subid")

/*--------------------------------------------------------------------------*/

public mixed QueryAttr(string name);       // std/living/attributes
public int QueryHP();                      // std/living/body
public int QueryMaxHP();                   // std/living/body
public string QueryHealthMsg();            // std/living/body
public varargs void DropOverload(int all); // std/living/restrictions
public varargs int QueryLoad();            // std/living/restrictions
//public mapping QueryArmours();             // std/living/combat
//public mapping QueryWeapons();             // std/living/combat
//public mapping QueryHands();               // std/living/hands
public mixed QueryArmours();             // std/living/combat
public mixed QueryWeapons();             // std/living/combat
public mixed QueryHands();               // std/living/hands

public int QueryIsPlayer();                // forward
#if __VERSION__ > "3.5.0"
public varargs object CanSeeLiving(string|object liv, int distance);  // forward
#else
public varargs object CanSeeLiving(object liv);  // forward
#endif
/*--------------------------------------------------------------------------*/

public int aLState; // Ghost need to be saved... everything else needs to
                    // have an extra-handling for players
private int aIVision, aUVision, aLevel, aGender, aAlign;
private int aInvis, aActions, aIsPet /* aMagicDef */;
private string aName, aRace;
private string *aMsgIn, *aMsgOut, *aMMsgIn, *aMMsgOut;
private mixed raceObject;

/*--------------------------------------------------------------------------*/

public int QueryActions ()        { return aActions > 0 ? aActions : 1; }
public int SetActions (int i)     { return aActions = i; }

public int QueryInvis ()          { return aInvis; }
public int SetInvis (int i)       { return aInvis = i; }

public int QueryIVision ()        { return aIVision; }
public int SetIVision (int i)     { return aIVision = i; }

public int QueryUVision ()        { return aUVision; }
public int SetUVision (int i)     { return aUVision = i; }

public int QueryLevel ()          { return aLevel; }
public int SetLevel (int i)       { return aLevel = i; }

public int QueryLState ()          { return aLState; }
public int SetLState (int i) {
  if (-1 == member( ({ LSTATE_NORMAL, LSTATE_GHOST, LSTATE_FROG }), i))
    aLState = LSTATE_NORMAL;
  else if (aLState != LSTATE_FROG || i != LSTATE_NORMAL || QueryIsPlayer())
    aLState = i;
  if (aLState != LSTATE_NORMAL)
    DropOverload();
  return aLState;
}

public int QueryFrog() { return QueryLState() == LSTATE_FROG; }
public int SetFrog(int i) {
  if ((QueryLState() != LSTATE_GHOST) || i)
    SetLState(i ? LSTATE_FROG : LSTATE_NORMAL);
  return QueryLState() == LSTATE_FROG;
}

public int QueryGhost() { return QueryLState() == LSTATE_GHOST; }
public int SetGhost(int i) {
  return SetLState(i ? LSTATE_GHOST : LSTATE_NORMAL) == LSTATE_GHOST;
}

public int QueryGender () {
  return aGender;
}
public int SetGender (int i) {
  if (i < 0 || i > GENDER_OTHER)
    aGender = GENDER_OTHER;
  else
    aGender = i;
  return QueryGender();
}

/* The following gender-mender lfuns are actually redundant, but
 * make debugging easier.
 */
int QueryNeuter() { return QueryGender() == GENDER_OTHER; }
int QueryMale()   { return QueryGender() == GENDER_MALE; }
int QueryFemale() { return QueryGender() == GENDER_FEMALE; }

int SetNeuter() { return SetGender(GENDER_OTHER); }
int SetMale()   { return SetGender(GENDER_MALE); }
int SetFemale() { return SetGender(GENDER_FEMALE); }


public string QueryRace ()        { return aRace; }
public string SetRace (string s)  {
  if (aRace != s && !QueryLong())
  {
    aRace = s; // prevent endless recursion
    RACEMASTER->InitRace(THIS, aRace);
  }
  return aRace = s;
}

public varargs object QueryRaceObj(int reload) {
  if (intp(raceObject) && raceObject && !reload)
    return 0;
  if (!objectp(raceObject)) {
    if (   catch(raceObject = ({mixed})RACEMASTER->RaceObject(QueryRace(), THIS))
        || !raceObject)
      raceObject = -1;
  }
  return objectp(raceObject) ? raceObject : 0;
}

public varargs string QueryName (mixed true_name) {
  return (aInvis & !true_name) ? "Someone" : aName;
}
public varargs string SetName (string s, int no_sln) {
  // TODO: That only clones can get a living_name might cause problems
  // TODO:: in special npcs. None of such kind are yet known, though.
  if (!no_sln && is_clone() )
    set_living_name (lower_case(s));
  return aName = SetShort(s);
}

public string QueryRealName () {
  return query_once_interactive(THIS) ? getuid(THIS) : lower_case(aName||"");
}
// No SetRealName()!

public int SetAlign (int v)      { return aAlign = v; }
public int QueryAlign ()         { return aAlign; }
public string QueryAlignString(int align) {
  int i;
  i = (1000 + align) * 17 / 2000;
  if (i > 16) i = 16; else if (i < 0) i = 0;
  return ({ "demonic", "evil", "rotten", "nasty", "foul", "heartless",
            "cruel", "mean", "neutral", "friendly", "kindly", "good",
            "compassionate", "holy", "saintly", "angelic", "godlike"
         })[i];
}
public int UpdateAlign (int up) {
  return SetAlign( (9*QueryAlign())/10 - up);
}

public varargs string *QueryMsgIn (int sc) {
  if (sc || (sc = QueryLState()) == LSTATE_NORMAL)
    return aMsgIn || ({ "arrives", "Someone arrives" });
  switch (sc) {
    case LSTATE_FROG: return ({ "hops in", "You hear some hopping" });
    case LSTATE_GHOST: return ({ "drifts in", "You feel a swift of mist" });
  }
  return ({ "arrives", "Someone arrives" });
}

#if __VERSION__ > "3.5.0"
public string *SetMsgIn (string *|string m) {
  if (stringp(m))
    aMsgIn = ({m, m});
  else
#else
public string *SetMsgIn (string *m) {
#endif
    aMsgIn = m;
  return QueryMsgIn();
}

public varargs string *QueryMsgOut (int sc) {
  if (sc || (sc = QueryLState()) == LSTATE_NORMAL)
    return aMsgOut || ({ "leaves", "Someone leaves" });
  switch (sc) {
    case LSTATE_FROG: return ({ "hops out", "You hear some hopping" });
    case LSTATE_GHOST: return ({ "drifts out"
                               , "You feel a swift of mist drifting"});
  }
  return ({ "leaves", "Someone leaves" });
}
public string *SetMsgOut (string *m) {
  aMsgOut = m;
  return QueryMsgOut();
}

public varargs string *QueryMMsgIn (int sc) {
  if (sc || (sc = QueryLState()) == LSTATE_NORMAL)
    return aMMsgIn || ({ "arrives in a puff of smoke"
                       , "You hear a gentle 'foop'" });
  switch (sc) {
    case LSTATE_FROG: return ({ "suddenly hits the ground and curses storks"
                              , "You hear a croaky voice cursing storks" });
    case LSTATE_GHOST: return ({ "drifts coldly through your body"
                               , "A faint cold fear runs through your veins"});
  }
  return ({ "arrives in a puff of smoke", "You hear a gentle 'foop'" });
}
#if __VERSION__ > "3.5.0"
public string *SetMMsgIn (string *|string m) {
  if (stringp(m))
    aMMsgIn = ({m, m});
  else
#else
public string *SetMMsgIn (string *m) {
#endif
    aMMsgIn = m;
  return QueryMMsgIn();
}

public varargs string *QueryMMsgOut (int sc) {
  if (sc || (sc = QueryLState()) == LSTATE_NORMAL)
    return aMMsgOut || ({ "disappears in a puff of smoke"
                        , "You hear a gentle 'wopp'" });
  switch (sc) {
    case LSTATE_FROG: return ({ "is suddenly caught away by a stork"
                              , "Something fast and winged passes by" });
    case LSTATE_GHOST: return ({ "whines in agony and fades away"
                               , "Somehow you feel more comfortable" });
  }
  return ({ "disappears in a puff of smoke", "You hear a gentle 'wopp'" });
}
public string *SetMMsgOut (string *m) {
  aMMsgOut = m;
  return QueryMMsgOut();
}

public varargs int QuerySize() {
  return (QueryLState() == LSTATE_FROG) ? PSIZE_SMALL : ::QuerySize();
}

/*--------------------------------------------------------------------------*/
// Gender supporting functions.

public string QueryGenderString() {
  return ({ "unsure", "male", "female", "other" })[QueryGender()];
}
public string QueryPronoun() {
  return ({ "it", "he", "she", "they" })[QueryGender()];
//  return ({ "it", "he", "she", "it" })[QueryGender()];
}
public string QueryPossessive() {
  return ({ "its", "his", "her", "them" })[QueryGender()];
//  return ({ "its", "his", "her", "its" })[QueryGender()];
}
public string QueryObjective() {
  return ({ "it", "him", "her", "their" })[QueryGender()];
//  return ({ "it", "him", "her", "it" })[QueryGender()];
}

//--------------------------------------------------------------------------
/* static */ string *_armours ()

// Auxiliary function, used to build the (internal) long description of
// livings.

{
  mapping map;
  mixed   arm;

  arm = QueryArmours();
  arm += filter(all_inventory(THIS)-arm
                     , lambda(({'o /*'*/}), ({#'==, /*'*/ THIS
                              , ({#'call_other, /*'*/  'o, /*'*/ "QueryProp", P_WORN})
                                      }))
                     );
  if (pointerp(arm))
    return map_objects(arm - ({ 0 }), "Short");
  map = arm || ([]);
  return map_objects(m_indices(map)-({map[AT_SKIN]}), "Short");
}

//--------------------------------------------------------------------------
static string *_list_weapons (string poss)

// Auxiliary function, used to build the (internal) long description of
// livings.

{
  string *result, str;
  int i, h, j;
  mixed *hands, arr;

  result = ({});
  hands = QueryHands();
  arr = QueryWeapons();
  if (sizeof(arr) && sizeof(arr -= ({ 0 })))
  {
    object weapon;

    for (i = sizeof(arr) + 1; --i;)
    {
      weapon = arr[<i];
      if (!objectp(weapon))
        continue;
      str = ({string})weapon->Short() +" with";
      for (h = sizeof(hands) + 1, j = 0; --h;) {
        if (hands[<h][HAND_WEAPON] == weapon)
        {
          str += (j++?" and ":" ")+poss+" "+hands[<h][HAND_SHORT];
        }
      }
      result += ({ str });
    }
  }

  return result;
}

//--------------------------------------------------------------------------
/* static */ string *_weapons (string possessive)

// Auxiliary function, used to build the (internal) long description of
// livings.

{
  string *result, str;
  int i, h, j;
  mapping map, hands;
  mixed *ind, *hind;

  if (!mappingp(QueryHands()))
    return _list_weapons(possessive);
  hands = QueryHands();
  hind = m_indices(hands);
  map = QueryWeapons();
  result = ({});
  if (map)
  {
    object weapon;

    ind = m_indices(map);
    for (i = sizeof(ind) + 1; --i;)
    {
      weapon = ind[<i];
      str = ({string})weapon->Short() +" with";
      for (h = sizeof(hind) + 1, j = 0; --h;) {
        if (hands[hind[<h], HAND_OBJ] == weapon)
        {
          str += (j++?" and ":" ")+possessive+" "+hind[<h];
        }
      }
      result += ({ str });
    }
  }

  return result;
}

//--------------------------------------------------------------------------
/* static */ string *_equipment ()

// Auxiliary function, used to build the (internal) long description of
// livings.

{
  mixed   objs;

  objs = filter(all_inventory(THIS)
                     , lambda(({'o}), ({#'==, THIS
                              , ({#'call_other, 'o, "QueryProp", P_EQUIPPED})
                                      }))
                     );
  return fold_objlist(objs - ({ 0 }));
}

//--------------------------------------------------------------------------
/* static */ string *_hands ()

// Auxiliary function, used to build the (internal) long description of
// livings.

{
  string * result;
  mixed hands;
  mapping mhands;
  mixed obj;
  int i;

  result = ({});
  hands = QueryHands();
  if (!mappingp(hands))
  {
    for (i = sizeof(hands) + 1; --i;)
    {
      if (!sizeof(hands[<i]))
	continue;
      obj = hands[<i][HAND_WEAPON];
      if (objectp(obj)
       && !({object})obj->Query(P_WORN)
       && !({object})obj->Query(P_WIELDED)
       && !({object})obj->Query(P_EQUIPPED)
       && -1 == member(result, obj)
         )
      {
        result += ({ obj });
      }
    }
  }
  else
  {
    mhands = hands;
    hands = m_indices(mhands);
    for (i = sizeof(hands) + 1; --i;)
    {
      obj = mhands[hands[<i], HAND_OBJ];
      if (objectp(obj)
       && !({object})obj->Query(P_WORN)
       && !({object})obj->Query(P_WIELDED)
       && !({object})obj->Query(P_EQUIPPED)
       && -1 == member(result, obj)
         )
      {
        result += ({ obj });
      }
    }
  }

  return map_objects(result, "Short") - ({ "", 0 });
}

//--------------------------------------------------------------------------
public string QueryHealthLoadMsg (int third)
{
  string str, result;

  result = QueryHealthMsg();

  switch (QueryLoad()) {
    case -50.. -1:
      str = third ? ", but seems to float" : ", but seem to float";
      break;
    case   0.. 10:
      str = "";
      break;
    case  11.. 50:
      str = " and slightly packed";
      break;
    case  51.. 80:
      str = third ? " and carries quite a pack" : " and carry quite a pack";
      break;
    case  81..100:
      str = " and heavily packed";
      break;
    case 101..140:
      str = third ? " and struggles under "+QueryPossessive()+" load"
                  : " and struggle under your load";
      break;
    case 141..190:
      str = third ? ", but has severe problems to keep "
                    +QueryObjective()+"self straight"
                  : ", but have severe problems to keep yourself straight";
      break;
    case 191..201:
      str = third ? " and almost collapsing under "+QueryPossessive()+" load"
                  : " and almost collapse under your load";
      break;
    case 202..250:
      str = ", but shouldn't be able to stand at all";
      break;
    default:
      str = third ? ", but carries an impossible load"
                  : ", but carry an impossible load";
      break;
  }
  return result+str;
}

//--------------------------------------------------------------------------
/* static */ string *_long (string str)

// Auxiliary function, used to build the (internal) long description of
// livings.

{
  string *strs, *result;
  string cp, pr;
  int i;

  strs = map_objects(all_inventory(), "QueryProp", P_EXTRALOOK) - ({ 0, "" });
  if (sizeof(strs))
    result = ({ str || "", implode(strs, ""), 0, 0, 0, 0, 0 });
  else
    result = ({ str || "", "", 0, 0, 0, 0, 0 });

  if (TP && ({string})TP->QueryCmdData(SUBID))
    return result;

  pr = QueryPronoun();
  cp = capitalize(pr);

  result[2] = cp+" is "+QueryHealthLoadMsg(1)+".\n";

  result[3] = "";
  if (i = sizeof(strs = _armours()))
  {
    if (i == 1)
      str = strs[0];
    else
      str = implode(strs[0..<2], ", ")+" and "+strs[<1];
    result[3] = cp+" wears "+str+".\n";
  }

  result[4] = "";
  if (sizeof(strs = _weapons(QueryPossessive())))
    result[4] = cp+" wields "+implode(strs, ".\n"+cp+" wields ")+".\n";

  result[5] = "";
  if (sizeof(strs = _equipment()))
    result[5] = implode_list(strs
                            , capitalize(QueryPronoun())+" is equipped with "
                            );

  result[6] = "";
  if (sizeof(strs = _hands()))
    result[6] = implode_list(strs
                            , capitalize(QueryPronoun())+" holds "
                            );

  return result;
}

//--------------------------------------------------------------------------
// Adapt the perceptional functions.
// Also take care of invisibility and ghostness.

public varargs string Short (string what) {
  mixed str, invis;
  int state;

#if 0
  if (what)
    return description::Short(what);
#endif
#if 0
  if (aInvis && (!IS_IMMORTAL(TP) || TP->Query(P_NOWIZ))
             && (!TP || IS_IMMORTAL(THIS) || !TP->Query(P_SEE_INVIS)))
    return 0;
#endif
  if (TP && !({object})TP->CanSeeLiving(THIS))
    return 0;

  state = QueryLState();
  if (state == LSTATE_FROG) {
    str = QueryName();
    if (!str) str = "a"+(aInvis ? "n invisible":" green")+" frog";
    else      str += " the "+(aInvis ? "invisible ":"")+"frog";
  }
  else {
    invis = aInvis ? "invisible ": "";
    str = description::Short();
    if (pointerp(str)) {
      if (state < sizeof(str))
        return str[state];
      str = str[0];
    }
    str = str || QueryName();
    str = (state == LSTATE_GHOST ? ("the "+invis+"ghost of ") : invis)+str;
  }
  return str;
}

public varargs string IntShort (string what) {
  mixed str, invis;
  int state;

#if 0
  if (what)
    return description::IntShort(what);
#endif
#if 0
  if (aInvis && (!IS_IMMORTAL(TP) || TP->Query(P_NOWIZ))
             && !TP->Query(P_SEE_INVIS))
    return 0;
#endif
  if (TP && !({object})TP->CanSeeLiving(THIS))
    return 0;

  state = QueryLState();
  if (state == LSTATE_FROG) {
    str = QueryName();
    if (!str) str = "a"+(aInvis ? "n invisible":" green")+" frog";
    else      str += " the "+(aInvis ? "invisible ":"")+"frog";
  }
  else {
    invis = aInvis ? "invisible ": "";
    str = description::IntShort();
    if (pointerp(str)) {
      if (state < sizeof(str))
        return str[state];
      str = str[0];
    }
    str = str || QueryName();
    str = (state == LSTATE_GHOST ? ("the "+invis+"ghost of ") : invis)+str;
  }
  return "carried by "+str;
}

private string MakeLong(mixed str)
{
  mixed invis, desc;

  if (aInvis)
    invis = capitalize(QueryPronoun())+" is currently rather invisible.\n";
  else
    invis = "";

  desc = QueryAttr(A_DESCRIPTION);

  switch (QueryLState()) {
  case LSTATE_FROG:
    str = QueryName();
    if (!str) str = "a"+(aInvis ? "n invisible":" green")+" frog.\n";
    else      str = str+", a"+(aInvis ? "n invisible ":" green")+"frog.\n";
    str = "This is "+str;
    if (pointerp(desc) && LSTATE_FROG >= sizeof(desc) && desc[LSTATE_FROG])
      str += desc[LSTATE_FROG];
    break;
  case LSTATE_GHOST:
    if (!pointerp(str) || LSTATE_GHOST >= sizeof(str) || !str[LSTATE_GHOST])
      str = "This is "+Short()+".\n";
    else
      str = str[LSTATE_GHOST]+invis;
    if (stringp(desc))
      str += desc;
    else if (pointerp(desc) && LSTATE_GHOST >= sizeof(desc) && desc[LSTATE_GHOST])
      str += desc[LSTATE_GHOST];
    break;
  default:
    if (pointerp(str)) {
      if (LSTATE_NORMAL >= sizeof(str))
        str = "This is "+Short()+".\n";
      else
        str =  str[LSTATE_NORMAL]+invis;
    }
    if (stringp(desc))
      str += desc;
    else if (pointerp(desc) && LSTATE_NORMAL >= sizeof(desc) && desc[LSTATE_NORMAL])
      str += desc[LSTATE_NORMAL];
  }
  return implode(_long(str), "");
}

public varargs string Long (string what) {
#if 0
  if (what)
    return description::Long(what);
#endif
#if 0
  if (aInvis && (!IS_IMMORTAL(TP) || TP->Query(P_NOWIZ))
             && !TP->Query(P_SEE_INVIS))

    return 0;
#endif

  return MakeLong(description::Long());

}

public varargs string ExaLong (string what) {
#if 0
  return what ? description::ExaLong(what) : Long();
#else
  return MakeLong(description::ExaLong());
#endif
}

public varargs string IntLong (string what) {
  mixed str, invis;

#if 0
  if (what)
    return description::IntLong(what);
#endif
#if 0
  if (aInvis && (!IS_IMMORTAL(TP) || TP->Query(P_NOWIZ))
             && !TP->Query(P_SEE_INVIS))

    return 0;
#endif

  if (aInvis)
    invis = capitalize(QueryPronoun())+" is currently rather invisible.\n";
  else
    invis = "";

  switch (QueryLState()) {
  case LSTATE_FROG:
    str = QueryName();
    if (!str) str = "a"+(aInvis ? "n invisible":" green")+" frog.\n";
    else      str = str+", a"+(aInvis ? "n invisible ":" green")+"frog.\n";
    str = "You are carried by "+str;
    break;
  case LSTATE_GHOST:
    str = description::IntLong();
    if (!pointerp(str) || LSTATE_GHOST >= sizeof(str) || !str[LSTATE_GHOST])
      str = "You are carried by "+Short()+".\n";
    else
      str = str[LSTATE_GHOST]+invis;
    break;
  default:
    str = description::IntLong();
    if (pointerp(str)) {
      if (LSTATE_NORMAL >= sizeof(str))
        str = "You are carried by "+IntShort()+".\n";
      else
        str =  str[LSTATE_NORMAL]+invis;
    }
  }
  return implode(_long(str), "");
}

public varargs string ExaIntLong (string what) {
  return what ? description::ExaIntLong(what) : IntLong();
}

public string QueryPreContent() {
  return capitalize(QueryPronoun())+" carries: \n";
}

public varargs string Content(string what, mixed excl, object pl) {
  mixed map;

  if (intp(excl) && excl)
  {
    excl = ({});
    map = QueryArmours();
    if (pointerp(map))
      excl += map - ({ 0 });
    else if (mappingp(map))
      excl += m_values(map) - ({ map[AT_SKIN] });
    map = QueryWeapons();
    if (pointerp(map))
      excl += map - ({ 0 });
    else if (mappingp(map))
      excl += m_indices(map);
  }
  return ::Content(what, excl, pl);
}

/*--------------------------------------------------------------------------*/
// Classification of the living.

public int QueryIsLiving() { return living(THIS); }

public int QueryIsPlayer() {
  return query_once_interactive(THIS);
}

nomask public int QueryIsWizard() {
  return (IS_WIZARD(THIS) && !({int})THIS->Query(P_NOWIZ) );
}

public int QueryIsNPC() {
  return living(THIS) && !query_once_interactive(THIS) && !Query(P_IS_PET);
}

public int SetIsPet(int i) { return aIsPet = i; }
public int QueryIsPet() { return aIsPet; }

/*--------------------------------------------------------------------------
 * Search an item in the environment and the inventory with respect to the
 * visibility.
 */

public varargs mixed Search (mixed what, int mode) {
  return search(THIS, what, mode&(~(SM_LOCATE)));
}

public varargs mixed * SearchM (mixed what, int mode, closure pred) {
  return searchm(THIS, what, mode&(~(SM_LOCATE)), pred);
}

/*--------------------------------------------------------------------------*/
// Functions to test if we can see in the actual environment.
// CanSeeLiving(liv) returns non-zero if we can see 'liv', else 0.
// CanSee() returns non-zero if we can see; else 0.
// CantSee() returns <0 if its too dark, >0 if its too bright and 0 if its o.k.

// [Softbyte] 10nov97: Compatible extension of the CanSeeLiving function
// Arguments: liv: string|object=The living which TO tries to detect
//            distance: int=The distance for which the check shall be
//                          performed (-1:whole mud,0:environment only
//                                    (any number>0: distance in meter)
// Returns:   the object which we see or 0 if it cannot be seen or found
#if __VERSION__ > "3.5.0"
public varargs object CanSeeLiving(string|object liv, int distance)
#else
public varargs object CanSeeLiving(object liv, int distance)
#endif
{
object newliv;

  if (!liv) liv=TP;     // default argument is this_player()
                        // and distance==0, i.e the ENV only

  if (stringp(liv))
  {
    newliv=present(liv,environment(this_object()));
	// we need only to search globally if distance is !=0
	if (distance!=0)
	{
      if (!newliv) newliv=find_player(liv);
	  if (!newliv) newliv=find_living(liv);
    }
	// Now it gets expensive
	if (!newliv) newliv=Search(liv,SEARCH_ENV_INV|SEARCH_OBJECT);
    // Nothing found
	if (!newliv) return 0;
	liv=newliv;
  }

  if (!objectp(liv)) return 0;  // Wrong usage

  // TODO: if distance !=0 && distance >0 check distance to TO-liv
  //       before returning !=0

  // Can see visible livings
  if (!({int})liv->Query(P_INVIS)) return liv;

  // Wizards can see everything
  if (IS_IMMORTAL(THIS) && !Query(P_NOWIZ)) return liv;

  // Non-wizards never can see invisible wizards
  if (IS_IMMORTAL(liv)) return 0;

  // Ability to see invis
  // TODO: Skill checks !!!
  if (Query(P_SEE_INVIS) && ((Query(P_SEE_INVIS_CHANCE)||1000) > random(1000)))
      return liv;
  // Being a ghost helps, too.
  if (LSTATE_GHOST == QueryLState()) return liv;

  // Else fall through to return 0.
  return 0;
}

public varargs int CanSee(object env) {
  int l, ivision, uvision, state;

  if (env || (env = ENV()))
  {
    if (({int})env->CanSeeHere(this_object()))
      return 1;
    state = QueryLState();
    if (state == LSTATE_GHOST)
      return 1;
    ivision = QueryIVision();
    uvision = QueryUVision();
    if (state == LSTATE_FROG)
    {
      l = uvision - ivision;
      ivision -= l/4;
      uvision += l/4;
    }
    l = ({int})env->QueryIntLight();
    return l >= ivision && l <= uvision;
  }
  return 0;
}

public varargs int CantSee(object env) {
  int l, ivision, uvision, state;

  if (env || (env = ENV()))
  {
    if (({int})env->CanSeeHere(this_object()))
      return 0;
    state = QueryLState();
    if (state == LSTATE_GHOST)
      return 0;
    ivision = QueryIVision();
    uvision = QueryUVision();
    if (state == LSTATE_FROG)
    {
      l = uvision - ivision;
      ivision -= l/4;
      uvision += l/4;
    }
    l = ({int})env->QueryIntLight();
    if (l < ivision) return l-ivision;
    if (l > uvision) return l-uvision;
  }
  return 0;
}

/*-------------------------------------------------------------------------
** Compatibility:
**
** int See (string msg)
**   If the player can see in the actual environment, tell it the
**   given <msg> and return non-zero.
**
** int See (string *msg)
**   <msg> is an array of two strings.
**   If the player can see in the actual environment, it is told the first
**   string.
**   If it can't see, it is told the second.
**   Returns non-zero in both cases.
*/

int See (mixed msg) {
  object env;
  int elight;
  if (!(env = environment(this_object()))
      || aIVision > (elight = ({int})env->QueryIntLight())
      || aUVision < elight
     ) {
    if (!pointerp(msg)) return 0;
    msg = msg[1];
  }
  else if (pointerp(msg)) msg = msg[0];
  tell_object(this_object(), msg);
  return 1;
}

/*--------------------------------------------------------------------------
 * The id() function which takes respect to some attributes.
 */

public int id (string s) {
  int state;
  if (!s || (aInvis && TP != THIS && !IS_IMMORTAL(TP)))
    return 0;
  if ("thing/description"::id(s))
    return 1;
  s = lower_case(s);
  state = QueryLState();
  return    s == lower_case(QueryName()||"")
         || s == lower_case(QueryRealName()||QueryName()||"")
         || (!query_once_interactive(THIS) && s == lower_case(QueryRace()||""))
         || (state == LSTATE_GHOST && s == "ghost")
         || (state == LSTATE_FROG  && s == "frog")
         || (THIS == TP && (s == "me" || s == "myself"))
        ;
}

/****************************************************************************/
