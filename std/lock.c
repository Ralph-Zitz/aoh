/* /std/lock
**
** The basic mechanic to keep something closed or even locked.
**
**  10-Jun-93 [Mateese]
**  10-Aug-94 [Mateese] Added some utility functions.
**  23-Apr-96 [Mateese] Ghosts can't handle the lock.
*/

#include <config.h>
#include <properties.h>
#include <lock.h>
#include <search.h>
#include <classes.h>

#define TP   this_player()
#define THIS this_object()

public varargs mixed Query(string name); // from /std/base
public varargs string Short(string what);    // from /std/thing/description

private nosave string *Pkeyids;
private nosave mixed Pkeycode;
private nosave int Pstate;
private nosave int Ppickchance = 50;

/*-------------------------------------------------------------------------*/

public string *SetKeyIds (mixed id) {
  if (id)
    THIS->AddClassAdj(CA_LOCKABLE);
  else
    THIS->RemoveClassAdj(CA_LOCKABLE);
  Pkeyids = id ? (pointerp(id) ? id : ({ id })) : 0;
  if (Pkeyids)
  {
    Pkeyids -= ({ "", 0 });
    if (!sizeof(Pkeyids))
      Pkeyids = 0;
  }
  return Pkeyids;
}
public string *QueryKeyIds () { return Pkeyids; }

public int SetLockState (int state) {
  mixed texte;
  texte = ({ "open", "closed", ({ "locked", "closed" }) });
  THIS->RemoveAdjective(texte[Pstate]);
  THIS->AddAdjective(texte[state]);
  return Pstate = state;
}
public int QueryLockState ()        { return Pstate; }

public string LockStateText () {
  return ({ "open", "closed", "locked" })[QueryLockState()];
}

public mixed SetKeyCode (mixed c) { return Pkeycode = c; }
public mixed QueryKeyCode () { return Pkeycode; }

public int SetPickChance (int i) { return Ppickchance = i; }
public int QueryPickChance () { return Ppickchance; }

/*-------------------------------------------------------------------------*/

public mixed LockDescription(string str)
{
  int p;
  string msg;

  if (!Query(P_KEYIDS))
    return "You don't see a lock there.\n";
  p = Query(P_PICK_CHANCE);
  if (p <= 0)
    msg = "The lock is a wonderful example of the art of locksmithing.\n";
  else
    switch (p)
    {
    case 1..25:
      msg = "The lock looks really strong. ";
      break;
    case 26..50:
      msg = "The lock looks strong. Picking it would be quite a feat.\n";
      break;
    case 51..75:
      msg = "The lock looks hard enough to keep adventurers outside.\n";
      break;
    case 76..80:
      msg = "The lock certainly has seen better times.\n";
      break;
    case 81..99:
      msg = "A clunky, cheap lock. ";
      break;
    default:
      msg = "The lock would give a locksmith nightmares.\n";
      break;
    }
  return msg+"It is "+LockStateText()+".\n";
}

/*-------------------------------------------------------------------------*/
// Filter stub for _filter_key().
// Return true if the <idstr> matches for <key>.

static int _filter_keyids (string idstr, object key) {
  return ({int})key->id(idstr);
}

/*-------------------------------------------------------------------------*/
// int ValidKey (object key)
//
// Return true if the <key> has one of the defined Pkeyids.

public int ValidKey (object key) {
  return sizeof(filter (Pkeyids, #'_filter_keyids /*'*/, key)) != 0;
}

/*-------------------------------------------------------------------------*/
// mixed TurnLock (int unlock, object key, void | mixed keycode)
//
// Lock (unlock == 0) or unlock (unlock != 1) this lock with the
// given <key> using the given <keycode>.
// The keycode defaults to that of the key.
// If this function changes the state of the lock, it does this
// by calling SetLockState().
// Results:
//   0 = LOCK_OK      : locking successful
//       LOCK_NOCHANGE: lock was already (un)locked.
//       LOCK_NOTLOCK : lock is not lockable (i.e.: is open)
//       LOCK_KEYWRONG: key doesn't fit
//       LOCK_KEYDENY : key denies operation
//       <string>     : Denial message from the key.

public varargs mixed TurnLock (int unlock, object key, mixed keycode) {
  mixed rc;
  int state;
  state = unlock ? LOCK_CLOSED : LOCK_LOCKED;
  if (!Pkeyids || Pstate == LOCK_OPEN)
    return LOCK_NOTLOCK;
  if (!keycode) keycode = ({mixed})key->Query(P_KEYCODE);
  if (Pkeycode != keycode || !ValidKey(key))
    return LOCK_KEYWRONG;
  if (!(rc = ({mixed})key->Query(unlock ? P_NOUNLOCK : P_NOLOCK))) {
    if (Pstate == state)
      return LOCK_NOCHANGE;
    SetLockState (state);
  }
  else if (!stringp(rc))
    rc = LOCK_KEYDENY;
  return rc;
}

/*-------------------------------------------------------------------------*/
// object *FindKey (mixed key)
//
// Search in the players inventory for objects which match <key> and are
// valid keys according to Pkeyids.
// Result:
//   An array of the found valid key objects, empty if no key fit.
//   0 if 'key' could not be found at all.

public object *FindKey (mixed key) {
  object *rc;
  rc = ({object *})TP->Search(key, SEARCH_INV);
  return sizeof(rc) ? filter (rc, #'ValidKey /*'*/) : 0;
}

/*-------------------------------------------------------------------------*/
// int | object* ParseArgs (string arg, int mandatory, int where)
//
// Parse a commandline for a '<thing>' or '<thing> with <key>' pattern
// and validize that '<thing>' is this object (search according to <where>).
// If '<key>' is specified, all matching keys in the players are selected,
// else all keys.
// In <mandatory> mode, the player must have a (matching) key.
//
// Results:
//         0: An error occured. An approbiate notify_fail() has been issued.
//   <array>: Valid keys found (may be empty).

public varargs mixed ParseArgs (string arg, int mandatory, int where) {
  string what, skey;
  mixed keys;
  object obj;
  int i;

  if (!arg || !environment() || (mandatory && !Pkeyids)) return 0;

  if (!mandatory) {
    if (sscanf (arg, "%s with %s", what, skey) == 2) {
      arg = what;
      if (Pkeyids) {
        keys = FindKey (skey);
        if (!sizeof(keys)) {
          if (!keys)
            notify_fail ("You have no '"+skey+"'.\n", NOTIFY_NOT_VALID);
          else
            notify_fail ("The "+skey+" doesn't fit.\n", NOTIFY_NOT_VALID);
          return 0;
        }
      }
      else keys = ({});
    }
    else if (Pkeyids)
      keys = FindKey(Pkeyids) || ({});
    else
      keys = ({});
  }
  else {
    if (sscanf (arg, "%s with %s", what, skey) == 2) arg = what;
    else skey = "key";

    keys = FindKey (skey);
    if (!sizeof(keys)) {
      if (!keys)
        notify_fail ("You have no '"+skey+"'.\n", NOTIFY_NOT_VALID);
      else
        notify_fail ("The "+skey+" doesn't fit.\n", NOTIFY_NOT_VALID);
      return 0;
    }
  }

  if (!arg
   || (obj = ({object})TP->Search (arg, where|SEARCH_OBJECT|WizSearch())) != THIS
     )
  {
    if (!arg)
      arg = capitalize(query_verb())+" what?";
    else if (obj)
    {
      arg = ({string})obj->Short();
      if (!arg || "" == arg)
        arg = capitalize(query_verb())+" what?";
      else
        arg = "You can't "+query_verb()+" "+arg+".";
    }
    else {
      arg = "You see no "+arg+" here.";
      if ((i = ({int})TP->CantSee()) < 0) arg += " It's too dark.";
      else if (i > 0)                   arg += " It's too bright.";
    }
    return notify_fail (arg+"\n", NOTIFY_NOT_OBJ);
  }
  return keys;
}

/*-------------------------------------------------------------------------*/
// int TryLocking (object *keys, int unlock)
//
// Try to lock (<unlock> == 0) or to unlock (<unlock> != 0) the lock
// with one of the given <keys>.
// Except for key-denial-messages, no own messages are issued.
//
// Results:
//   <int>: The index of the successfully used key.
//      -1: Failure.

public varargs int TryLocking (object *keys, int unlock) {
  int i;
  mixed rc;
  for (i = 0; i < sizeof(keys); i++) {
    rc = TurnLock (unlock, keys[i]);
    if (stringp(rc)) write (rc);
    else if (rc == LOCK_OK) break;
  }
  if (i >= sizeof(keys)) return -1;
  return i;
}

/*-------------------------------------------------------------------------*/

public string ModifyInvShort (string text) {
  if (QueryLockState() != LOCK_OPEN)
    return text;
  if (text[0] != 'a')
  {
    string last;
    if (-1 != member("!?.", text[<1]))
    {
      last = text[<1..<1];
      text = text[0..<2];
    }
    else
      last = "";
    return text+" (open)"+last;
  }
  return "an open "+strip_a(text);
}

public string ModifyLong (string text) {
  return text+"It is "+LockStateText()+".\n";
}

/*-------------------------------------------------------------------------*/

public void init () {
  add_action ("fopen", "open");
  add_action ("fopen", "close");
  add_action ("flock", "lock");
  add_action ("flock", "unlock");
}

/*-------------------------------------------------------------------------*/
public varargs int DoActionFor (int action, int type, object *keys, int kix)

// Do an appropriate user-notifying action for an operation of the lock.
// <action> and <type> denote the operation, <keys> is the keys array in use,
// <kix> the index of the used key.
//
// Valid actions are:
//   LOCK_DO_OPEN, LOCK_DO_CLOSE, LOCK_DO_LOCK, LOCK_DO_UNLOCK
//
// Valid types are:
//   LOCK_ACT_NOKEY   : the player has no keys.
//   LOCK_ACT_WRONGKEY: the player has no matching key.
//   LOCK_ACT_LOCK    : the object is (un)locked with <keys>[<kix>].
//   LOCK_ACT_OPEN    : the object is opened/closed.
//   LOCK_ACT_ALREADY : No actions need to be performed.
//
// Result is the return code:
//   1: Abort action with 1
//   0: Abort action with 0
//  -1: Continue action.

{
  int rc;
  string sh, verb;

  sh = strip_a(Query(P_SHORT));

  switch (type) {
    case LOCK_ACT_NOKEY:
      if (action == LOCK_DO_OPEN || action == LOCK_DO_CLOSE)
        write("The "+sh+" is locked.\n");
      else
        write("You have no key.\n");
      rc = 1;
      break;

    case LOCK_ACT_WRONGKEY:
      if (action == LOCK_DO_LOCK)
        verb = "lock";
      else
        verb = "unlock";
      write(
"You fail to "+verb+" the "+sh+" - "
+(sizeof(keys) > 1 ? "no key fits" : "the key doesn't fit") + ".\n"
                  );
      rc = 1;
      break;

    case LOCK_ACT_LOCK:
      if (action == LOCK_DO_LOCK)
        verb = "lock";
      else
        verb = "unlock";
      write("You "+verb+" the "+sh+" with "
            +({string})keys[kix]->Short()+".\n");
      show( capitalize(({string})TP->QueryName())+" "+verb+"s "
           +Short()+" with "+({string})keys[kix]->Short()+".\n"
          , "You hear a cranking noise.\n"
          );
      show_room(THIS, capitalize(({string})TP->QueryName())+" "+verb+"s "
           +Short()+" with "+({string})keys[kix]->Short()+".\n"
          , "You hear a cranking noise.\n"
          );
      rc = -1;
      break;

    case LOCK_ACT_OPEN:
      if (action == LOCK_DO_OPEN)
        verb = "open";
      else
        verb = "close";
      write ("You "+verb+" the "+sh+".\n");
      show(capitalize(({string})TP->QueryName())+" "+verb+"s "+Short()+".\n");
      show_room(THIS, capitalize(({string})TP->QueryName())+" "+verb+"s "+Short()+".\n");
      rc = (LOCK_DO_OPEN == action) ? 1 : -1;
      break;

    case LOCK_ACT_ALREADY:
      switch(action) {
      case LOCK_DO_LOCK:
        write("It is already locked.\n");
        break;
      case LOCK_DO_UNLOCK:
        write("It is already unlocked.\n");
        break;
      case LOCK_DO_OPEN:
        write("It is already open.\n");
        break;
      default:
        write("It is already closed.\n");
      }
      rc = 1;
      break;
  }

  return rc;
}

/*-------------------------------------------------------------------------
** Commands "open" and "close"
**
**   "open <object>"
**      Opens the lock. If it is locked, and the player has the right key,
**      it is unlocked first.
**   "open <object> with <key>":
**      Opens the lock. If it is locked, and the player specified the
**      right key, it is unlocked first.
**   "close <object>"
**   "close <object> with <key>"
**      Closes the lock.
**
** <where> determines where the object is searched. Default is SEARCH_ENV_INV.
*/

public varargs int fopen (string str, int where) {
  object *keys;
  mixed rc;
  int i;

  if (!(rc = ParseArgs (str, 0, where)))
    return 0;

  if (LSTATE_GHOST == ({int})TP->Query(P_LSTATE))
  {
    write("You can't do that in your present state.\n");
    return 1;
  }

  keys = rc;

  if (query_verb() == "open") {
    switch (QueryLockState()) {
      case LOCK_LOCKED:
        if (!sizeof(keys)) {
          rc = DoActionFor(LOCK_DO_OPEN, LOCK_ACT_NOKEY);
          if (rc >= 0) return rc;
        }
        if ((i = TryLocking (keys, 1)) < 0) {
          rc = DoActionFor(LOCK_DO_OPEN, LOCK_ACT_WRONGKEY);
          if (rc >= 0) return rc;
        }
        rc = DoActionFor(LOCK_DO_OPEN, LOCK_ACT_LOCK, keys, i);
        SetLockState(LOCK_CLOSED);
        if (rc >= 0) return rc;
        /* FALL THROUGH */

      case LOCK_CLOSED:
        rc = DoActionFor(LOCK_DO_OPEN, LOCK_ACT_OPEN);
        SetLockState (LOCK_OPEN);
        if (rc >= 0) return rc;
        break;

      case LOCK_OPEN:
        rc = DoActionFor(LOCK_DO_OPEN, LOCK_ACT_ALREADY);
        if (rc >= 0) return rc;
        break;
    }
  }
  else { /* close */
    switch (QueryLockState()) {
      case LOCK_OPEN:
        rc = DoActionFor(LOCK_DO_CLOSE, LOCK_ACT_OPEN);
        SetLockState (LOCK_CLOSED);
        if (rc >= 0) return rc;
        break;

      case LOCK_LOCKED:
      case LOCK_CLOSED:
        rc = DoActionFor(LOCK_DO_CLOSE, LOCK_ACT_ALREADY);
        if (rc >= 0) return rc;
        break;
    }
  }
  return 1;
}

/*-------------------------------------------------------------------------
** Commands "lock" and "unlock"
**
**   "lock <object>"
**      Locks the lock if the player has the right key.
**      If it is open, it is closed first.
**   "lock <object> with <key>"
**      Locks the lock if the player has specified the right key.
**      If it is open, it is closed first.
**   "unlock <object>"
**      Unlocks the lock if the player has the right key.
**   "unlock <object> with <key>"
**      Unlocks the lock if the player has specified the right key.
**
** <where> determines where the object is searched. Default is SEARCH_ENV_INV.
*/

public varargs int flock (string str, int where) {
  object *keys;
  mixed rc;
  int i;

  // This notify_fail() is for nil arg-strings, and attempts
  // to (un)lock a lockless object.
  notify_fail(capitalize(query_verb())+" what?\n", NOTIFY_NOT_OBJ);

  if (!(rc = ParseArgs (str, 1, where))) return 0;

  if (LSTATE_GHOST == ({int})TP->Query(P_LSTATE))
  {
    write("You can't do that in your present state.\n");
    return 1;
  }

  keys = rc;

  if (query_verb() == "lock") {
    switch (QueryLockState()) {
      case LOCK_OPEN:
        rc = DoActionFor(LOCK_DO_LOCK, LOCK_ACT_OPEN);
        SetLockState(LOCK_CLOSED);
        if (rc >= 0) return rc;
        /* FALL THROUGH */

      case LOCK_CLOSED:
        if (!sizeof(keys)) {
          rc = DoActionFor(LOCK_DO_LOCK, LOCK_ACT_NOKEY);
          if (rc >= 0) return rc;
        }
        if ((i = TryLocking (keys, 0)) < 0) {
          rc = DoActionFor(LOCK_DO_LOCK, LOCK_ACT_WRONGKEY);
          if (rc >= 0) return rc;
        }
        rc = DoActionFor(LOCK_DO_LOCK, LOCK_ACT_LOCK, keys, i);
        SetLockState (LOCK_LOCKED);
        if (rc >= 0) return rc;
        break;

      case LOCK_LOCKED:
        rc = DoActionFor(LOCK_DO_LOCK, LOCK_ACT_ALREADY);
        if (rc >= 0) return rc;
        break;
    }
  }
  else {  /* "unlock" */
    switch (QueryLockState()) {
      case LOCK_LOCKED:
        if (!sizeof(keys)) {
          rc = DoActionFor(LOCK_DO_UNLOCK, LOCK_ACT_NOKEY);
          if (rc >= 0) return rc;
        }
        if ((i = TryLocking (keys, 1)) < 0) {
          rc = DoActionFor(LOCK_DO_UNLOCK, LOCK_ACT_WRONGKEY);
          if (rc >= 0) return rc;
        }
        rc = DoActionFor(LOCK_DO_UNLOCK, LOCK_ACT_LOCK, keys, i);
        SetLockState (LOCK_CLOSED);
        if (rc >= 0) return rc;
        break;

      case LOCK_CLOSED:
      case LOCK_OPEN:
        rc = DoActionFor(LOCK_DO_UNLOCK, LOCK_ACT_ALREADY);
        if (rc >= 0) return rc;
        break;
    }
  }
  return 1;
}

/*-------------------------------------------------------------------------*/

public void create () {
  THIS->AddAdjective("open");
  THIS->AddSubDetail("lock", #'LockDescription);
}

/*************************************************************************/
