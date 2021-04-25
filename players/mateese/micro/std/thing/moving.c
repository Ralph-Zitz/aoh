/*---------------------------------------------------------------------------
** std/thing/moving.c
**
** Handle the moving of a thing.
**
** 07-Sep-1993 [Mateese]
** 02-Jan-1994 [Mateese] Changed clean_up() handling.
** 08-Jun-1994 [Mateese] Adapted for new properties.
**---------------------------------------------------------------------------
*/

#include <base.h>
#include <moving.h>
#include <thing.h>

#include "/std/base.d"
#include "/std/thing/moving.d"

#define ENV   environment
#define THIS  this_object()
#define PREV  previous_object()

private static int nofollow = 0;  // Mutex-Sema for following

//---------------------------------------------------------------------------
public object *AddFollower (object f)

// Add an object to the list of followers.

{ object *followers;

  followers = Query(P_Followers);
  if (!followers)
    followers = ({ f });
  else if (member(followers, f) < 0)
    followers += ({ f });
  return Set(P_Followers, followers);
}

//---------------------------------------------------------------------------
public object *RemoveFollower (object f)

// Remove an object from the list of followers.

{ object *followers;

  followers = Query(P_Followers);
  if (followers)
    followers -= ({ f, 0 });
  return Set(P_Followers, followers);
}

//---------------------------------------------------------------------------
public varargs int move (mixed dest, int method, mixed extra)

// Move this object into <dest> using given <method> (default is M_SILENT).
// <extra> may contain additional information for the preventation and
// notification.
// Return ME_OK if the move was successful, else an error code.
// Before moving this object, the old environment will be asked for permission
// by:
//
//   old_env->PreventLeave(dest, method, extra)
//
// which may return a code != 0 if leaving is not allowed (this code will then
// be returned by move()).
// Second, the new environment will be asked with:
//
//   new_env->AllowEnter(method, extra)
//
// which may return a code != ME_OK if entering is not allowed. If an object
// does not define a allow_enter() function, then moving objects into it will
// not be allowed because a return value of 0 is the standard ME_NO_ENTER
// error code.
// After a successful move, the old environment and the new one will be notifed
// by:
//
//   old_env->NotifyLeave(new_env, method, extra)
//   new_env->NotifyEnter(old_env, method, extra)
//
// The notification functions can then give out moving messages or update
// light and weight etc.
// If all this has been performed, all followers (if any) will also be notifed
// of the move.

{
  mixed err1, err2;
  object oldenv, *followers;
  int i;
                                           !
  switch(method) {
    case M_TAKE:
      if (Query(P_NoTake)) return ME_NO_TAKE;
      break;
    case M_PUT:
    case M_DROP:
      if (Query(P_NoDrop)) return ME_NO_DROP;
      break;
    case M_GIVE:
      if (Query(P_NoGive)) return ME_NO_GIVE;
      break;
  }
  oldenv = ENV();
  if (M_NOCHECK != method) { // In all other cases ask for permissions.

    // Ask if we perhaps are not allowed to leave the current environment.
    // We do not want to catch an error because we have not yet moved the
    // object and so no inconsistency can occur. Just let the error happen
    // so we can fix it. We still can escape with a M_NOCHECK move
    // if we are a player (and somebody helps us).

    if (ENV() && (err1 = ENV()->PreventLeave( dest, method, extra)))
      return err1;

    // Check for permission to enter the new environment. We also do not
    // need to catch an error for the same we reason we did not when asking
    // for permission to leave. Just let the error happen.

    if ((err1 = dest->AllowEnter(method, extra)) != ME_OK)
      return err1 || ME_NO_ENTER;

    if (!THIS) return ME_DESTRUCTED_SELF; // Have we been destructed?
  }

  // if we come to this point everything seams to be o.k. for moving.
  // We also do not want to catch an error for the same reasons as
  // above. Just let it happen.

  move_object(THIS, dest);

  // We now want to notify the environments of the successful move so they
  // can update the weight etc. The problem here is that an error will
  // lead into inconsistency. Especially when notifying the old env we
  // still want to give the new env the chance to update. Therefore we
  // catch possible errors and send a error message.

  err1 = 0;
  if (oldenv && (err1 = catch(oldenv->NotifyLeave(dest, method, extra))))
    MASTER->runtime_error("Error on calling " + to_string(oldenv) +
                          "->NotifyLeave(): " + err1,
                          __FILE__, file_name(), __LINE__);

  if (err2 = catch(dest->NotifyEnter(oldenv, method, extra)))
    MASTER->runtime_error("Error on calling " + to_string(dest) +
                          "->NotifyEnter(): " + err2,
                          __FILE__, file_name(), __LINE__);

  if (err1 || err2) return ME_CAUGHT_ERR;

  // Now tell all our followers about it. Take care of recursions by using
  // an increment/decrement counter.
  followers = Query(P_Followers);
  if (followers)
  {
    nofollow++;
    for (i = sizeof(followers); --i;)
      followers[i]->follow(dest, method, extra);
    nofollow--;
  }

  return ME_OK;
}

//---------------------------------------------------------------------------
public void follow (mixed dest, int method, mixed extra)

// Called by a moving object to let us follow it.

{
  if (!nofollow && ENV(PREV) != ENV())
    move(dest, method, extra);
}

//---------------------------------------------------------------------------
public int clean_up (int ref)

// Give the thing the opportunity to clean up.
//
// Argument:
//   ref: The ref count of the object.
//         > 1: a non swapped blueprint with 'ref' references.
//         = 1: a swapped blueprint, or one with just on ref left.
//         = 0: a clone
//         =-1: the environment wants us to clean_up.
//
// Result:
//   0: don't call clean_up again - either this object wants to stay or
//      did selfdestruct.
//   1: the object stays, but try again next time.
//
// The function calls Query(P_Cleanup, ref) and analyzes the result:
//   <= 0: object selfdestructs if it has no environment.
//    = 1: object will never clean up.
//    > 1: object might clean up later.
//
// ref = -1:
//   Call clean_up() with argument -1 in each contained object.
//   If no object remains, selfdestruct.
// ref = 0:
//   If the object has no environment, selfdestruct after calling remove() in
//   all contained objects (a deep_inventory() is used for it).
// ref = 1:
//   If the object has no enviroment, clean up as for ref=-1, else stay.
// ref > 1:
//   Stay, but try again next time.
//
// Objects carried by (netdead) players are not cleant.

{
  int cflag;

  if (ref > 0)
    return 1;
  cflag = Query(P_Cleanup, ref);
  if (cflag > 0 || (ENV() && query_once_interactive(ENV())) )
    return cflag > 0 ? cflag-1 : 1;

  switch(ref) {
    case 0:
      if (environment() && cflag > -1)
        return 1;
      filter_objects(deep_inventory(), "remove");
      break;
    case 1:
      if (environment() && cflag > -1)
        return 1;
      /* FALL THROUGH */
    case -1:
      filter_objects(all_inventory(), "clean_up", -1);
      if (first_inventory() && cflag > -1)
        return 1;
      break;
    default:
      if (cflag > -1)
        return 1;
  }
  remove();
  return THIS && 1;
}

/***************************************************************************/
