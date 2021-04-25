/*---------------------------------------------------------------------------
** std/hull/moving.c
**
** Handle the (re)moving of and into/outof the hull.
**
** 12-Sep-1993 [Mateese]
** 02-Jan-1994 [Mateese] Changed clean_up()-handling.
**---------------------------------------------------------------------------
*/

#include <object.h>
#include <moving.h>
#include <room.h>

#include "/std/object.d"

#define ENV   environment
#define THIS  this_object()
#define PREV  previous_object()

privat static closure oldQueryTPort;
privat static closure oldPrevLeave;
privat static closure oldAllowEnter;
privat static closure oldNotifyLeave;
privat static closure oldNotifyEnter;

//---------------------------------------------------------------------------
public int QueryTPort ()

// Query the teleport permission

{
  if (isLearner(PREV)) return TPORT_BOTH;
  return query(P_TPort);
}

//---------------------------------------------------------------------------
public int PreventLeave(mixed dest, int method, mixed extra)

// Check if something may leave the room

{
  if (method == M_TELEPORT && (Query(P_TPort) & TPORT_OUT))
    return ME_NO_LEAVE;
  if (!MayAddWeight(-(int)PREV->Query(P_Weight)))
    return ME_TOO_LIGHT;
  return ME_OK; /* == 0 */
}

//---------------------------------------------------------------------------
public int AllowEnter(int method, mixed extra)

// Check if something may enter the room.

{
  if (method == M_TELEPORT && (Query(P_TPort) & TPORT_IN))
    return ME_NO_LEAVE;
  if (!MayAddWeight((int)PREV->Query(P_Weight)))
    return ME_TOO_HEAVY;
  return ME_OK; /* == 0 */
}

//---------------------------------------------------------------------------
public void NotifyLeave(mixed to, int method, mixed extra)

// Handle that something left the room.

{
  AddIntLight(-(int)PREV->Query(P_Light));
  AddIntWeight(-(int)PREV->Query(P_Weight));
}

//---------------------------------------------------------------------------
public void NotifyEnter(mixed from, int method, mixed extra)

// Handle that something entered the room.

{
  AddIntLight((int)PREV->Query(P_Light));
  AddIntWeight((int)PREV->Query(P_Weight));
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
// ref = -1:
//   Call clean_up() with argument -1 in each contained object.
//   If no object remains, selfdestruct.
// ref = 0:
//   If the object has no environment and doesn't contain objects,
//   selfdestruct after calling remove() in all contained objects
//   (a deep_inventory() is used for it).
// ref = 1:
//   If the object has no enviroment, clean up as for ref=-1, else stay.
// ref > 1:
//   Stay, but try again next time.

{
  object *inv;

  // TODO: set inv to just those objects no recreatable by the room.
  //       Also, do not selfdestruct if there are items depending on the room.
  inv = all_inventory();

  switch(ref) {
    case 0:
      if (environment() || sizeof(inv))
        return 1;
      filter_objects(deep_inventory(), "remove");
      break;
    case 1:
      if (environment())
        return 1;
      /* FALL THROUGH */
    case -1:
      filter_objects(inv, "clean_up", -1);
      inv -= ({ 0 });
      if (sizeof(inv))
        return 1;
      break;
    default:
      return 1;
  }
  remove();
  return THIS && 1;
}

/***************************************************************************/
