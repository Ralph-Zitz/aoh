/*---------------------------------------------------------------------------
** std/equipment
**
** Special routines for things which are to be _used_.
**
** 06-Dec-1994  [Mateese]
**---------------------------------------------------------------------------
*/

#include <config.h>
#include <search.h>
#include <properties.h>
#include <equipment.h>
#include <moving.h>

#define THIS this_object()
#define TP   this_player()
#define ENV  environment

public varargs mixed Query(string name, int flag); // std/base
public string *AddAdjective(mixed ads);                // std/thing/description
public string *RemoveAdjective(mixed ads);             // std/thing/description

public string QualityMsg (int quality); // forward

//---------------------------------------------------------------------------

private int     pQuality = 100, pResetQuality = 100;
private int     pNumberHands;
private object  pWielded, pWorn;
private mixed  *pWieldData = ({ "wield", "unwield", "wielded" });
private mixed  *pWearData = ({ "wear", ({ "unwear", "remove" }), "worn" });
private int     pMode = EQM_WEAR;
private string  pQualityMsg;

private int     silentmove = 0; // Flag for silent unwears/unwields.

//---------------------------------------------------------------------------

public int QueryNumberHands ()         { return pNumberHands; }
public int SetNumberHands (int i)      { return pNumberHands = i; }

public int QueryQuality ()             { return pQuality; }
public int SetQuality (int i)          { return pQuality = (i >= 0 ? i : 0); }
public int AddQuality (int delta) {
  return SetQuality(QueryQuality() + delta);
}

public int QueryResetQuality ()        { return pResetQuality; }
public int SetResetQuality (int i) {
  SetQuality(i);
  return pResetQuality = i;
}

public int QueryUsage()            { return pMode; }
public int SetUsage(int i)         { return pMode = i; }

public mixed * QueryWieldData() { return pWieldData; }
public mixed * SetWieldData(mixed * d) {
  return pWieldData = d || ({ "wield", "unwield", "wielded" });
}

public mixed * QueryWearData() { return pWearData; }
public mixed * SetWearData(mixed * d) {
  return pWearData = d || ({ "wear", ({ "unwear", "remove" }), "worn" });
}

public object QueryWielded ()          { return pWielded; }
public object SetWielded (object o) {
  if (o)
    AddAdjective(pWieldData[EQD_ADJ]);
  else
    RemoveAdjective(pWieldData[EQD_ADJ]);
  return pWielded = o;
}

public object QueryWorn ()             { return pWorn; }
public object SetWorn (object o) {
  if (o)
    AddAdjective(pWearData[EQD_ADJ]);
  else
    RemoveAdjective(pWearData[EQD_ADJ]);
  return pWorn = o;
}

//---------------------------------------------------------------------------

void Aloha (int flag)
{
  if (flag < 0)
    SetQuality(QueryResetQuality());
}

//---------------------------------------------------------------------------

static int modifyValue  (int v) { return (v * (40+QueryQuality())) / 140; }

static string modifyLong(string long) {
  long += "It is "+QualityMsg(QueryQuality());
  switch (Query(P_SIZE)) {
    case PSIZE_SMALL : long += " and rather small"; break;
    case PSIZE_NORMAL: long += " and of normal size"; break;
    case PSIZE_LARGE : long += " and rather large"; break;
  }
  return long+".\n";
}

static string modifyShort (string ishort) {
  int qual;

  if ((qual = QueryQuality()) >= 95)
    return ishort;
  return add_a(QualityMsg(qual)+" "+strip_a(ishort));
}

static string modifyInvShort (string ishort) {
  string tmp, adj;
  int qual;

  tmp = (qual = QueryQuality()) < 95 ? QualityMsg(qual) : "";
  adj = pWearData[EQD_ADJ];
  if (QueryWorn() && -1 == strstr(ishort, adj))
    tmp += (tmp != "" ? ", " : "")+adj;
  adj = pWieldData[EQD_ADJ];
  if (QueryWielded() && -1 == strstr(ishort, adj))
    tmp += (tmp != "" ? ", " : "")+adj;
  return tmp == "" ? ishort : ishort+" ("+tmp+")";
}

//---------------------------------------------------------------------------
public string QualityMsg (int quality)

// Return a description of the thing's quality.

{
  switch (quality) {
  case 95.. 99: return "ok";
  case 75.. 94: return "slightly damaged";
  case 50.. 74: return "damaged";
  case 25.. 49: return "heavily damaged";
  case  5.. 24: return "severely battered";
  case  0..  4: return "broken and bent";
  }
  if (quality < 0)
    return "nearly destroyed";
  return "in perfect condition";
}

public string SetQualityMsg (string s) { return pQualityMsg = s; }

public varargs string QueryQualityMsg (int trueq) {
  return (!trueq && pQualityMsg) || QualityMsg(QueryQuality());
}

//---------------------------------------------------------------------------
public varargs mixed Wield (object living, int flags)

// Wield this object by <living>

{
  mixed rc, obj;
  string sh;

  if (QueryWielded())
    return EQ_ALREADY;

  sh = ({string})THIS->QueryShort();

  rc = QueryUsage();
  if (!(EQM_WIELD & rc))
    return EQ_FORBID;
  if ((EQM_MUTUAL & rc) && QueryWorn())
    return EQ_MUTUAL;

  rc = ({mixed})living->CompareSize(THIS);
  if (rc < 0)
    return EQ_LARGE;
  if (rc > 0)
    return EQ_SMALL;

  if (rc = Query(P_NOWIELD))
    return stringp(rc) ? rc : EQ_FORBID;

  if (obj = ({mixed})living->Query(P_EQUIP_OBJ)) {
    if (!pointerp(obj))
      obj = ({ THIS, obj });
    else
      obj = ({ THIS }) + obj;
  }
  else
    obj = ({ THIS });

  rc = map_objects(obj-({ 0 }), "ChkWield", living, THIS, flags) - ({ EQ_OK });
  if (sizeof(rc))
    return stringp(rc[0]) ? rc[0] : EQ_FORBID;

  if (QueryNumberHands() && !({int})living->Grip(THIS))
    return EQ_NOHANDS;

  if ((flags & EWF_WEAPON) && !({int})living->AddWeapon(THIS))
  {
    living->Ungrip(THIS);
    return EQ_FORBID;
  }
  else
  {
    if (!(flags & EWF_SILENT))
      show_room(ENV(living), ({string})living->QueryName()+" wields "+sh+".\n", ({ living }));
    SetWielded(living);
  }

  if (obj)
    filter_objects(obj-({ 0 }), "NotifyWield", living, THIS, flags);

  return EQ_OK;
}

//---------------------------------------------------------------------------
public varargs mixed Wear (object living, int flags)

// Wear this object by <living>

{
  mixed rc, obj;
  string sh;

  if (QueryWorn())
    return EQ_ALREADY;

  sh = ({string})THIS->QueryShort();

  rc = QueryUsage();
  if (!(EQM_WEAR & rc))
    return EQ_FORBID;
  if ((EQM_MUTUAL & rc) && QueryWielded())
    return EQ_MUTUAL;

  rc = ({mixed})living->CompareSize(THIS);
  if (rc < 0)
    return EQ_LARGE;
  if (rc > 0)
    return EQ_SMALL;

  if (rc = Query(P_NOWEAR))
    return stringp(rc) ? rc : EQ_FORBID;

  if (obj = ({mixed})living->Query(P_EQUIP_OBJ)) {
    if (!pointerp(obj))
      obj = ({ THIS, obj });
    else
      obj = ({ THIS }) + obj;
  }
  else
    obj = ({ THIS });

  rc = map_objects(obj-({ 0 }), "ChkWear", living, THIS, flags) - ({ EQ_OK });
  if (sizeof(rc))
    return stringp(rc[0]) ? rc[0] : EQ_FORBID;

  if (QueryNumberHands() && !({int})living->Grip(THIS))
    return EQ_NOHANDS;

  if ((flags & EWF_WEAPON) && !({int})living->AddArmour(THIS))
  {
    living->Ungrip(THIS);
    return EQ_FORBID;
  }
  else
  {
    if (!(flags & EWF_SILENT))
      show_room(ENV(living), ({string})living->QueryName()+" wears "+sh+".\n", ({ living }));
    SetWorn(living);
  }

  if (obj)
    filter_objects(obj-({ 0 }), "NotifyWear", living, THIS, flags);

  return EQ_OK;
}

//---------------------------------------------------------------------------
public varargs mixed Unwield (int flags)

// Remove this object as weapon if wielded.

{
  mixed obj, rc;
  object liv;

  if (!(liv = QueryWielded()))
    return EQ_ALREADY;

  if (rc = Query(P_NOUNWIELD))
    return stringp(rc) ? rc : EQ_FORBID;

  if (obj = ({mixed})liv->Query(P_EQUIP_OBJ)) {
    if (!pointerp(obj))
      obj = ({ THIS, obj });
    else
      obj = ({ THIS }) + obj;
  }
  else
    obj = ({ THIS });

  rc = map_objects(obj-({ 0 }), "ChkUnwield", liv, THIS, flags) - ({ EQ_OK });
  if (sizeof(rc))
    return stringp(rc[0]) ? rc[0] : EQ_FORBID;

  if (QueryNumberHands() && !QueryWorn())
    liv->Ungrip(THIS);

  liv->RemoveWeapon(THIS, flags);
  SetWielded(0);
  if (!silentmove && !(flags & EWF_SILENT))
    show_room(ENV(liv), ({string})liv->QueryName()+" unwields "+({string})THIS->QueryShort()+".\n", ({ liv }));

  if (obj)
    filter_objects(obj-({ 0 }), "NotifyUnwield", liv, THIS, flags);

  return EQ_OK;
}

//---------------------------------------------------------------------------
public varargs mixed Unwear (int flags)

// Remove this object as armour if worn.

{
  mixed obj, rc;
  object liv;

  if (!(liv = QueryWorn()))
    return EQ_ALREADY;

  if (rc = Query(P_NOUNWEAR))
    return stringp(rc) ? rc : EQ_FORBID;

  if (obj = ({mixed})liv->Query(P_EQUIP_OBJ)) {
    if (!pointerp(obj))
      obj = ({ THIS, obj });
    else
      obj = ({ THIS }) + obj;
  }
  else
    obj = ({ THIS });

  rc = map_objects(obj-({ 0 }), "ChkUnwear", liv, THIS, flags) - ({ EQ_OK });
  if (sizeof(rc))
    return stringp(rc[0]) ? rc[0] : EQ_FORBID;

  if (QueryNumberHands() && !QueryWielded())
    liv->Ungrip(THIS);

  liv->RemoveArmour(THIS, flags);
  SetWorn(0);
  if (!silentmove && !(flags & EWF_SILENT))
    show_room(ENV(liv), ({string})liv->QueryName()+" unwears "+({string})THIS->QueryShort()+".\n", ({ liv }));

  if (obj)
    filter_objects(obj-({ 0 }), "NotifyUnwear", liv, THIS, flags);

  return EQ_OK;
}

//===========================================================================
// Placeholders to make life easier.

public varargs mixed ChkWield(object liv, object weapon, int flags) { return EQ_OK; }
public varargs mixed ChkUnwield(object liv, object weapon, int flags) { return EQ_OK; }
public varargs void NotifyWield(object liv, object weapon, int flags) { }
public varargs void NotifyUnwield(object liv, object weapon, int flags) { }

public varargs mixed ChkWear(object liv, object weapon, int flags) { return EQ_OK; }
public varargs mixed ChkUnwear(object liv, object weapon, int flags) { return EQ_OK; }
public varargs void NotifyWear(object liv, object weapon, int flags) { }
public varargs void NotifyUnwear(object liv, object weapon, int flags) { }

//===========================================================================

public void init()
{
  int mode, i;

  mode = QueryUsage();

  if (mode & EQM_WEAR)
  {
    if (stringp(pWearData[EQD_DO]))
      add_action("fwear", pWearData[EQD_DO]);
    else
      for (i = 0; i < sizeof(pWearData[EQD_DO]); i++)
        add_action("fwear", pWearData[EQD_DO][i]);
    if (stringp(pWearData[EQD_UNDO]))
      add_action("funwear", pWearData[EQD_UNDO]);
    else
      for (i = 0; i < sizeof(pWearData[EQD_UNDO]); i++)
        add_action("funwear", pWearData[EQD_UNDO][i]);
  }
  if (mode & EQM_WIELD)
  {
    if (stringp(pWieldData[EQD_DO]))
      add_action("fwield", pWieldData[EQD_DO]);
    else
      for (i = 0; i < sizeof(pWieldData[EQD_DO]); i++)
        add_action("fwield", pWieldData[EQD_DO][i]);
    if (stringp(pWieldData[EQD_UNDO]))
      add_action("funwield", pWieldData[EQD_UNDO]);
    else
      for (i = 0; i < sizeof(pWieldData[EQD_UNDO]); i++)
        add_action("funwield", pWieldData[EQD_UNDO][i]);
  }
}

public varargs void inform_move (mixed dest, int method, mixed extra)
{
  if (method == M_NOCHECK || method == M_SILENT || method == M_DESTRUCT)
    silentmove = 1;
  if (QueryWorn())
    Unwear();
  if (QueryWielded())
    Unwield();
  silentmove = 0;
}

//---------------------------------------------------------------------------
// Predicates to locate the object.

private int isCarried (object obj, object *inv)
{
  return -1 != member(inv, obj);
}

//---------------------------------------------------------------------------
// Get or compute the command data from the player.

private mapping getCData (string verb, string arg)
{
  mapping cdata, found;
  mixed   what;
  closure tmp;
  string  msg;

  cdata = ({mapping})TP->QueryCmdData("/std/equipment:"+verb);
  if (!cdata || cdata[0] != time())
  {
    cdata = ([ 0:time() ]);
    if ("wear" == verb || "wield" == verb)
    {
      what = ({mixed})TP->SearchM(arg, SEARCH_INV_ENV|SM_CMULTI);
      tmp = lambda(({'o}), ({#'!
                            , ({ #'isCarried, 'o, quote(all_inventory(TP)) })
                           })
                  );
      found = sort_findings( what-({ TP })
                           , lambda( ({'o, 't, 'l, 'c})
                             , ({#'sort_f_std, 'o, 't, 'l, 'c, tmp }) /*'*/
                             )
                           );
    }
    else
    {
      what = ({mixed})TP->SearchM(arg, SEARCH_INV|SM_CMULTI
                            , lambda(({'o}), ({#'isCarried, 'o
                                               , quote(all_inventory(TP)) }) /*'*/
                                    )
                        );
      found = sort_findings(what-({ TP }), #'sort_f_default /*'*/);
    }
    what = found[FOUND_SINGLE]||({});

    msg = "";
    if (found[-FOUND_SINGLE])
      msg = "You did not find as much as you wanted "+query_verb()+".\n";

    msg += implode_list( found[FOUND_NOTHANDLE]
                       , "You have to get ", " first.", " and"
                       );
    msg += implode_list( found[FOUND_IDS]
                       , "You don't find ", ".", " or");

    if (!sizeof(what))
    {
      notify_fail(msg+"You find nothing to "+query_verb()+".\n"
                 , NOTIFY_NOT_VALID);
      return 0;
    }
    cdata["what"] = what;
    cdata["msg"] = msg;
    cdata["handled"] = ({});
    cdata["nothandled"] = ({});
    cdata["already"] = ({});
    cdata["fewhands"] = ({});
    cdata["small"] = ({});
    cdata["large"] = ({});
  }
  return cdata;
}


/*---------------------------------------------------------------------------
** Commands "wear" and "unwear"
**
**   "wear <item>"
**      Wears the item if it's carried and the player has enough
**      free hands.
**   "unwear <item>"
**      Unwears the item.
*/

public varargs mixed fwear (string arg, int inh)
{
  mixed   rc;
  mapping cdata;
  string  msg, passive;

  passive = (query_verb() == "wear") ? "worn"
            : query_verb() == "unwear" ? "unworn"
            : query_verb()+"ed";

  if (!arg) {
    notify_fail(query_verb()+" what?\n", NOTIFY_NOT_OBJ);
    return 0;
  }

  // Parse the commandline and match the given objects.
  if (!(cdata = getCData("wear", arg)))
    return 0;

  // If this object was meant, try to wear it.
  if (-1 != member(cdata["what"], THIS))
  {
    rc = Wear(TP, (({mixed})THIS->QueryArmourType() ? EWF_WEAPON : 0));
    if (stringp(rc))
      cdata["msg"] += rc;
    else
      switch (rc) {
      case EQ_OK:
        cdata["handled"] += ({ THIS });
        break;
      case EQ_ALREADY:
        cdata["already"] += ({ THIS });
        break;
      case EQ_FORBID:
        cdata["nothandled"] += ({ THIS });
        break;
      case EQ_NOHANDS:
        cdata["fewhands"] += ({ THIS });
        break;
      case EQ_SMALL:
        cdata["small"] += ({ THIS });
        break;
      case EQ_LARGE:
        cdata["large"] += ({ THIS });
        break;
      case EQ_ERROR:
      default:
        raise_error(sprintf("%O: Illegal or bogus object: rc %O\n", THIS, rc));
      }

    msg =  cdata["msg"]
          +implode_list(cdata["already"], "You already "+query_verb()+" ")
          +implode_list(cdata["nothandled"], "You can't "+query_verb()+" ")
          +implode_list( cdata["fewhands"]
                     , "You don't have enough free hands to "+query_verb()+" ")
          +capitalize(implode_list(cdata["small"]
                     , "", " "+(sizeof(cdata["small"]) > 1 ? "are" : "is")
                              +" too small for you to be "+passive+"."))
          +capitalize(implode_list(cdata["large"]
                     , "", " "+(sizeof(cdata["large"]) > 1 ? "are" : "is")
                              +" too big for you to be "+passive+"."))
          +implode_list(cdata["handled"], "You "+query_verb()+" ")
    ;
    notify_fail(msg, NOTIFY_DO_ALL);
  }
  TP->SetCmdData("/std/equipment:wear", cdata);
  return inh ? cdata : 0;
}


public varargs mixed funwear (string arg, int inh)
{
  mixed   rc;
  mapping cdata;
  string  msg;

  if (!arg) {
    notify_fail(query_verb()+" what?\n", NOTIFY_NOT_OBJ);
    return 0;
  }

  // Parse the commandline and match the given objects.
  if (!(cdata = getCData("unwear", arg)))
    return 0;

  // If this object was meant, try to wear it.
  if (-1 != member(cdata["what"], THIS))
  {
    rc = Unwear();
    if (stringp(rc))
      cdata["msg"] += rc;
    else
      switch (rc) {
      case EQ_OK:
        cdata["handled"] += ({ THIS });
        break;
      case EQ_ALREADY:
        cdata["already"] += ({ THIS });
        break;
      case EQ_FORBID:
        cdata["nothandled"] += ({ THIS });
        break;
      case EQ_NOHANDS:
      case EQ_ERROR:
      default:
        raise_error(sprintf("%O:Illegal or bogus object: rc %O\n", THIS, rc));
      }

    msg =  cdata["msg"]
          +implode_list( cdata["already"]
                       , "You don't "+(stringp(pWearData[EQD_DO])
                                       ? pWearData[EQD_DO]
                                       : pWearData[EQD_DO][0])+" "
                       )
          +implode_list(cdata["nothandled"], "You can't "+query_verb()+" ")
          +implode_list(cdata["handled"], "You "+query_verb()+" ")
    ;
    notify_fail(msg, NOTIFY_DO_ALL);
  }
  TP->SetCmdData("/std/equipment:unwear", cdata);
  return inh ? cdata : 0;
}

/*---------------------------------------------------------------------------
** Commands "wield" and "unwield"
**
**   "wield <weapon>"
**      Wields the weapon if it's carried and the player has enough
**      free hands.
**   "unwield <weapon>"
**      Unwields the weapon.
*/

public varargs mixed fwield (string arg, int inh)
{
  mixed   rc;
  mapping cdata;
  string  msg;

  if (!arg) {
    notify_fail(query_verb()+" what?\n", NOTIFY_NOT_OBJ);
    return 0;
  }

  // Parse the commandline and match the given objects.
  if (!(cdata = getCData("wield", arg)))
    return 0;

  // If this object was meant, try to wield it.
  if (-1 != member(cdata["what"], THIS))
  {
    rc = Wield(TP, (({mixed})THIS->QueryWeaponType() ? EWF_WEAPON : 0));
    if (stringp(rc))
      cdata["msg"] += rc;
    else
      switch (rc) {
      case EQ_OK:
        cdata["handled"] += ({ THIS });
        break;
      case EQ_ALREADY:
        cdata["already"] += ({ THIS });
        break;
      case EQ_FORBID:
        cdata["nothandled"] += ({ THIS });
        break;
      case EQ_NOHANDS:
        cdata["fewhands"] += ({ THIS });
        break;
      case EQ_SMALL:
        cdata["small"] += ({ THIS });
        break;
      case EQ_LARGE:
        cdata["large"] += ({ THIS });
        break;
      case EQ_ERROR:
      default:
        raise_error(sprintf("%O: Illegal or bogus object: rc %O\n", THIS, rc));
      }

    msg =  cdata["msg"]
          +implode_list(cdata["already"], "You already "+query_verb()+" ")
          +implode_list(cdata["nothandled"], "You can't "+query_verb()+" ")
          +implode_list( cdata["fewhands"]
                       , "You don't have enough free hands to "+query_verb()+" ")
          +capitalize(implode_list(cdata["small"]
                     , "", " "+(sizeof(cdata["small"]) > 1 ? "are" : "is")
                              +" too small for you to be "+query_verb()+"ed."))
          +capitalize(implode_list(cdata["large"]
                     , "", " "+(sizeof(cdata["large"]) > 1 ? "are" : "is")
                              +" too big for you to be "+query_verb()+"ed."))
          +implode_list(cdata["handled"], "You "+query_verb()+" ")
    ;
    notify_fail(msg, NOTIFY_DO_ALL);
  }
  TP->SetCmdData("/std/equipment:wield", cdata);
  return inh ? cdata : 0;
}


public varargs mixed funwield (string arg, int inh)
{
  mixed   rc;
  mapping cdata;
  string  msg;

  if (!arg) {
    notify_fail(query_verb()+" what?\n", NOTIFY_NOT_OBJ);
    return 0;
  }

  // Parse the commandline and match the given objects.
  if (!(cdata = getCData("unwield", arg)))
    return 0;

  // If this object was meant, try to wear it.
  if (-1 != member(cdata["what"], THIS))
  {
    rc = Unwield();
    if (stringp(rc))
      cdata["msg"] += rc;
    else
      switch (rc) {
      case EQ_OK:
        cdata["handled"] += ({ THIS });
        break;
      case EQ_ALREADY:
        cdata["already"] += ({ THIS });
        break;
      case EQ_FORBID:
        cdata["nothandled"] += ({ THIS });
        break;
      case EQ_NOHANDS:
      case EQ_ERROR:
      default:
        raise_error(sprintf("%O: Illegal or bogus object: rc %O\n", THIS, rc));
      }

    msg =  cdata["msg"]
          +implode_list( cdata["already"]
                       , "You don't "+(stringp(pWieldData[EQD_DO])
                                       ? pWieldData[EQD_DO]
                                       : pWieldData[EQD_DO][0])+" "
                       )
          +implode_list(cdata["nothandled"], "You can't "+query_verb()+" ")
          +implode_list(cdata["handled"], "You "+query_verb()+" ")
    ;
    notify_fail(msg, NOTIFY_DO_ALL);
  }
  TP->SetCmdData("/std/equipment:unwield", cdata);
  return inh ? cdata : 0;
}

/***************************************************************************/
