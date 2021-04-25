//---------------------------------------------------------------------------
// room/restrictions.c
//
// The light&weight handling and restrictions of a room.
//---------------------------------------------------------------------------

#include <config.h>
#include <rooms.h>
#include <moving.h>
#include <properties.h>
#include <messages.h>
#include <search.h>
#include <nightday.h>
#include <msgclass.h>
#include <weather.h>
#include <vitems.h>
#include <skills.h>
#include <secure/wizlevels.h>
#include <features.h>

#define PRE   previous_object()
#define THIS  this_object()
#define TP    this_player()

public mixed Query(string arg); // std/base
public mixed VQuery( string id, string prop );  // std/room/vitems
public int SetBright(int b);  // forward or cross-inherited
varargs object *GetViewers (mixed excl);

//--- global variables ------------------------------------------------------

private nosave mixed Pserver;
private nosave int Pindoors, Ptport, Pmaxweight, Psunbright, Pmaxinv;
private nosave int Pintlight, Pintbright, Pintweight, autolight;
private nosave mapping PmagicProt;

public void create() {
  Pindoors = 1;
  Pserver = NIGHTDAY;
  Ptport = TPORT_BOTH;
  Psunbright = MAX_SUNBRIGHT;
  Pmaxweight = 1<<30;
  autolight = (MAX_SUNBRIGHT * 75) / 100;;
  PmagicProt = ([]);
}

//--- light and weather handling --------------------------------------------

public int SetIndoors(int i) {
  if (autolight && !i)
    autolight = 0;
  return Pindoors = i;
}
public int QueryIndoors()    { return Pindoors; }

public string QueryServer () { return Pserver; }
public string SetServer (mixed sv) {
  if (objectp(sv))
    sv = object_name(sv);
  return Pserver = sv;
}

public varargs mixed QueryOutdoors () {
  return QueryIndoors() ? 0 : Pserver;
}
public mixed SetOutdoors(mixed i) {
  if (i && intp(i))
    i = NIGHTDAY;
  SetIndoors(!i);
  return SetServer(i);
}

public mixed FilterWeatherData (object client, mixed *data, int newstate) {
  // Outdoor room pass the weather data unchanged. In players, the
  // given CDESC will override any existing WDESC.
  if (!Query(P_INDOORS))
    return data;

  // If there are windows in this (indoors) room, annulate the CDESC
  // so that the player will see the WDESC.
  // TODO: The window weather stuff should also check doors. But otoh, we
  // TODO:: have lots of implicite doors (aka 'exits')... hmm.
  if (sizeof(filter_indices( Query( P_VITEMS ), #'VQuery /*'*/, PV_WINDOW_ISWINDOW )))
  {
    data[WD_CDESC] = 0;
    return data;
  }

  // Indoor rooms with no windows block any message.
  return 0;
}

public mixed * QueryDayData () {
  mixed server;
  mixed *data;

  server = QueryOutdoors();
  if (!server || catch(data = ({mixed *})server->QueryStateData()))
    return 0;
  return data;
}

public int QueryDayState () {
  mixed server;
  int data;

  server = QueryOutdoors();
  if (!server || catch(data = ({int})server->QueryState()))
    return 0;
  return data;
}

public string QueryDayDesc () {
  mixed server;
  string data;

  server = QueryOutdoors();
  if (!server || catch(data = ({string})server->QueryStateDesc()))
    return "";
  return data;
}

public int QuerySunBright() { return Psunbright; }
public int SetSunBright(int s) { return Psunbright = s; }

public int QuerySunLight() {
  mixed in;
  return (in = QueryOutdoors())
           ? (QuerySunBright() * ({int})in->QuerySunLight()) / MAX_SUNBRIGHT
           : 0;
}

public void light_from_inside(int l) {
  filter_objects(all_inventory() - ({ PRE }), "light_from_outside", l);
  Pintlight += l;
}

public void emit_intlight(int l) {
  filter_objects(all_inventory(), "light_from_outside", l);
  Pintlight += l;
}

public varargs int QueryIntLight(int nosun) {
  return nosun ? autolight+Pintlight : autolight+Pintlight + QuerySunLight();
}
public varargs int SetIntLight(int l, int nosun) {
  autolight = 0;
  emit_intlight(l -= nosun ? Pintlight : Pintlight + QuerySunLight() );
  Pintbright += l;
  return Pintlight;
}
public int AddIntLight(int l) {
  autolight = 0;
  light_from_inside(l);
  return Pintlight;
}

/* Nasty hack to circumvent inheritance problems */
static void _add_int_light (int l) {
  Pintlight += l;
}

public int QueryBright()    { return Pintbright; }
public int SetBright(int l) {
  autolight = 0;
  emit_intlight(l - Pintbright);
  return Pintbright = l;
}

//--- weight handling -------------------------------------------------------

public int QueryMaxInv()    { return Pmaxinv; }
public int SetMaxInv(int i) { return Pmaxinv = (i >= 0) ? i : 0; }

public int QueryWeightContent() { return Pintweight; }
public int QueryWeightContents() { return Pintweight; }

public int QueryMaxWeight()    { return Pmaxweight; }
public int SetMaxWeight(int w) {
  return w >= Pmaxweight
         ? Pmaxweight = w
         : ( w <= Pintweight ? Pmaxweight = Pintweight
                             : Pmaxweight = w
           );
}

public void AddIntWeight (int w) {
  Pintweight += w;
}

// Compatibility
public int AddWeight (int w) {
  AddIntWeight(w);
  return ({int})this_object()->QueryWeight();
}

public int MayAddWeight(int w) {
  return QueryMaxWeight() >= w + Pintweight;
}

public int MayAddItem() {
  int i;
  i = QueryMaxInv();
  return i < 1 || sizeof(all_inventory(THIS)) < i;
}

//--- Magic restrictions and modifications ---------------------------------

public mapping QueryMagicProtection() {
  return PmagicProt;
}
public mapping SetMagicProtection(mapping map) {
  if (mappingp(map))
    PmagicProt = copy(map);
  return QueryMagicProtection();
}
public string AddMagicProtection(int type, string message) {
  PmagicProt[type] = message;
  return PmagicProt[type];
}
public varargs mapping ForbidAllMagic(string str) {
  if (!stringp(str))
    str = "Your present location doesn't allow you the use of magic *sigh*.\n";
  SetMagicProtection(([ST_ALL:str]));
  return QueryMagicProtection();
}
public string QueryIsMagicForbidden(int type) {
  return PmagicProt && (PmagicProt[type] || PmagicProt[ST_ALL]);
}

//--- moving restrictions and notifications ---------------------------------

public int QueryTPort() {
  return (IS_IMMORTAL(TP) && !({int})TP->Query(P_NOWIZ)) ? TPORT_BOTH : Ptport;
}
public int SetTPort(int t) { return Ptport = t; }

public int prevent_leave(mixed dest, int method, mixed extra) {
  if(method == M_TELEPORT && !(QueryTPort() & TPORT_OUT)) return ME_NO_LEAVE;
  return ME_NO_PREVENT;
}

public int allow_enter(int method, mixed extra) {
  if(method == M_TELEPORT && !(QueryTPort() & TPORT_IN)) return ME_NO_ENTER;
  return (MayAddItem() && MayAddWeight(({int})PRE->QueryWeight()))
         ? ME_OK : ME_TOO_HEAVY;
}

public void IssueLeaveMessages(object pl, mixed to, int method, mixed extra) {
  string movemsg, altmsg, moveextra, altextra;
  mixed msgout;
  object *rej;
  int invis_player;

  invis_player = interactive(pl) && ({int})pl->QueryInvis();

  if (!invis_player && (!({string})pl->Short() || ({string})pl->Short() == "")) return;

  if (invis_player)
    movemsg = "("+capitalize(({string})pl->QueryRealName())+") ";
  else
    movemsg = capitalize((({string})pl->QueryName()||({string})pl->Short()))+" ";
  altmsg = living(pl) ? "Someone " : "Something ";

  moveextra = "";
  altextra = "";
  if (stringp(extra))
    moveextra = " "+extra;
  else if(pointerp(extra) && sizeof(extra)) {
    if (stringp(extra[0]))
      moveextra = " "+extra[0];
    else {
      moveextra = " "+extra[0][0];
      altextra = " "+extra[0][1];
    }
  }

  switch(method) {
  case M_GO:
    msgout = ({mixed})pl->QueryMsgOut() || "leaves";
    movemsg += (stringp(msgout) ? msgout : msgout[0]) + moveextra;
    altmsg = (pointerp(msgout) ? msgout[1] : (altmsg+"disappears"))+altextra;
    break;
  case M_TELEPORT:
    msgout = ({mixed})pl->QueryMMsgOut() || "leaves";
    if (moveextra != "")
      movemsg += moveextra[1..];
    else
      movemsg += (stringp(msgout) ? msgout : msgout[0]) + moveextra;
    if (altextra != "")
      altmsg += altextra[1..];
    else
      altmsg = (pointerp(msgout) ? msgout[1] : (altmsg+"disappears"))+altextra;
    break;
  case M_SPECIAL:
    movemsg += moveextra[1..];
    altmsg += altextra[1..];
    break;
  default:
    return;
  }
  if(member(".!?", movemsg[<1]) == -1)
    movemsg += ".";
  if(member(".!?", altmsg[<1]) == -1)
    altmsg += ".";

  if (invis_player) {
    rej = filter_objects(all_inventory(THIS), "QueryIsLiving");
    rej -= filter(rej, #'query_user_level /*'*/);
    if (!query_user_level(pl))
      rej -= filter_objects(rej, "Query", P_SEE_INVIS);
  }
  else
    rej = ({});
  show_room(THIS, movemsg+"\n", altmsg+"\n", rej+({ pl }));
}

public void notify_leave(mixed to, int method, mixed extra) {
  emit_intlight(method==M_DESTRUCT ? -({int})PRE->QueryBright() : -({int})PRE->QueryLight() );
  PRE->light_from_outside(method==M_DESTRUCT ? -QueryBright() : -QueryIntLight());
  AddIntWeight(method==M_DESTRUCT ? -({int})PRE->QueryWeight(1) : -({int})PRE->QueryWeight());
  if (living(PRE))
    map(GetViewers(PRE), (: ({void})$1->RoomRemovePlayer(PRE) :));
  IssueLeaveMessages(PRE, to, method, extra );
}

public void IssueEnterMessages(object pl, mixed from, int method, mixed extra){
  mixed msgin;
  string movemsg, altmsg, moveextra, altextra;
  int    invis_player, dpos;
  object *rej;

  invis_player = interactive(pl) && ({int})pl->QueryInvis();

  if (!invis_player && (!({string})pl->Short() || ({string})pl->Short() == "")) return;

  moveextra = "";
  altextra = "";
  if (stringp(extra))
    moveextra = " "+extra;
  else if(pointerp(extra) && sizeof(extra) > 1) {
    if (stringp(extra[1]))
      moveextra = " "+extra[1];
    else {
      moveextra = " "+extra[1][0];
      altextra = " "+extra[1][1];
    }
  }

  if (invis_player)
    movemsg = "("+capitalize(({string})pl->QueryRealName())+") ";
  else
    movemsg = capitalize((({string})pl->QueryName()||({string})pl->Short()))+" ";
  altmsg = living(pl) ? "Someone " : "Something ";

  switch(method) {
  case M_GO:
    msgin = ({mixed})pl->QueryMsgIn() || "arrives";
    movemsg += stringp(msgin) ? msgin : msgin[0];
    altmsg = pointerp(msgin) && sizeof(msgin) > 1 ? msgin[1] : (altmsg+"arrives");
    if(stringp(extra))
      if ( ( dpos = member( COMPASS_DIRECTIONS, extra ) ) != -1 ) {
        movemsg += ( " from " + COMPASS_DIRECTIONS[(dpos+4)%sizeof(COMPASS_DIRECTIONS)] );
      } else {
        switch(extra) {
          case "up":        movemsg += " from below";     break;
          case "down":      movemsg += " from above";     break;
        }
      }
    else {
      movemsg += moveextra;
      altmsg += altextra;
    }
    break;
  case M_TELEPORT:
    msgin = ({mixed})pl->QueryMMsgIn() || "comes into existance";
    if (moveextra != "")
      movemsg += moveextra[1..];
    else
      movemsg += stringp(msgin) ? msgin : msgin[0];
    if (altextra != "")
      altmsg += altextra[1..];
    else
      altmsg = pointerp(msgin) ? msgin[1] : (altmsg+"comes into existance");
    break;
  case M_SPECIAL:
    movemsg += moveextra[1..];
    altmsg += altextra[1..];
    break;
  default:
    return;
  }
  if(member(".!?", movemsg[<1]) == -1)
    movemsg += ".";
  if(member(".!?", altmsg[<1]) == -1)
    altmsg += ".";

  if (invis_player) {
    rej = filter_objects(all_inventory(THIS), "QueryIsLiving");
    rej -= filter(rej, #'query_user_level /*'*/);
    if (!query_user_level(pl))
      rej -= filter_objects(rej, "Query", P_SEE_INVIS);
  }
  else
    rej = ({});
  show_room(THIS, movemsg+"\n", altmsg+"\n", rej+({ pl }));
}

public void notify_enter(mixed from, int method, mixed extra) {
  mixed in, oldin;

  PRE->light_from_outside(QueryIntLight()-({int})PRE->QueryLight());
  emit_intlight(({int})PRE->QueryLight());
  AddIntWeight(({int})PRE->QueryWeight());

  oldin = from && ({mixed})from->QueryOutdoors();
  if ((in = QueryOutdoors()) && in != oldin && ({int})in->IsMyPlayer(PRE))
    tell_object(PRE, ({string})in->QueryStateDesc());
  if (living(PRE)) {
    map(GetViewers(PRE), (: ({void})$1->RoomAddPlayer(PRE) :));
    PRE->RoomPlayers();
  }
  IssueEnterMessages(PRE, from, method, extra);

  // We call the RelayCall function with the argument skill
  // to forward the call to the skill modules itself
  // Looks complicated but is not.
  // Do it onlyif secrets are defined...maybe change this TODO
  if (({mapping})THIS->Query(P_ROOM_SECRETS) )
    call_other(SKILLSERVER,"RelayCall",SK_DETECT,
                "EnterRoom",TP,THIS,method);

}

//--- find something or more in the room -------------------------------------
#if __VERSION__ > "3.5.0"
static int equippedByMe(object obj) {
#else
static int * equippedByMe(object obj) {
#endif
  return obj && (({object})obj->Query(P_EQUIPPED) == THIS);
}

public object * filterWornWielded (object * objs) {
  objs -= filter_objects(objs, "QueryProp", P_WIELDED);
  objs -= filter_objects(objs, "QueryProp", P_WORN);
  objs -= filter(objs, #'equippedByMe /*'*/);
  return objs;
}

public varargs mixed Locate (mixed what, int mode, object * also) {
  return locate(THIS, what, mode, also);
}

public varargs int CanSeeHere (object who)
{
  return Query(P_CANSEE);
}

/*---------------------------------------------------------------------------
** object *GetViewers (void | object excl | object *excl)
**   Return an array of all livings in the container which are able to see.
**   <excl> are excluded if given.
*/

int _view_filter (object elem) {
  return (living(elem) && ({int})elem->CanSee());
}

varargs object *GetViewers (mixed excl) {
  if (!excl) excl = ({});
  else if (!pointerp(excl)) excl = ({ excl });
  return filter(all_inventory(THIS)-excl, #'_view_filter /*'*/);
}

/*---------------------------------------------------------------------------
** object *display (string msg, void | object excl | object *excl )
**   Display <msg> to all livings in the container which are able to see.
**   <excl> are excluded if given.
**   Return the array of actual recipients.
**
** object *display (string *msg, void | object excl | object *excl )
**   <msg> is an array of two strings. The first is told to all livings in
**   the container which are able to see, the second to all which aren't able
**   to see.
**   Return the array of all recipients.
*/

int _display_filter (object rec, mixed msg) {
  return 0;
}

#if __VERSION__ > "3.5.0"
varargs object *display (string|mixed * msg, mixed excl) {
#else
varargs object *display (mixed * msg, mixed excl) {
#endif
  object * excl2;
  if (!excl) excl = ({});
  else if (!pointerp(excl)) excl = ({ excl });
  if (stringp(msg)) msg = ({ msg, 0 });
  excl2 = filter(all_inventory(THIS)-excl, #'_display_filter /*'*/, msg);
  msg_room( THIS, CMSG_ROOM|MMSG_SEE, msg, excl+excl2 );
  return GetViewers(excl+excl2)+excl2;
}
