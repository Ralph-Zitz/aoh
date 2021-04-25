/*
** This is the temporary files where all ships will move to, when
** they are on their journey from one port to the other.
** File:   /room/ocean.c
** Author: Thragor@Nightfall, 11aug95
** Update:
*/

#include <moving.h>

#define SHIP "/obj/lib/ship.c"

inherit "/std/room";

public varargs void create()
{
  (::create());
  SetIntShort("The Wide Ocean");
  SetIntLong("You're on the wide blue ocean.\n");
  SetIndoors(0);
}

public int allow_enter(int method,mixed extra)
{
  if (member(inherit_list(previous_object()),SHIP)==-1)
    return ME_NO_ENTER;
  return ME_OK;
}
