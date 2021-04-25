/* My workroom.
** The description will change as the room will get more and more battered
** from me working here :)
**
**   14-Jan-92 [m] First version
**   28-Jan-92 [m] Magic field now switchable.
**   23-Apr-92 [m] Realworkroom got real workroom :)
**   08-Jun-92 [m] Adapted for native mode
**   13-Aug-94 [m] Control-Panel eingebaut.
*/

#include "/players/mateese/mydefs.h"

#include <properties.h>
#include <rooms.h>
#include <stdrooms.h>
#include <moving.h>

inherit "/std/room";

static string fieldon, fieldoff;
static int field;
static object home;

/*---------------------------------------------------------------------------
** Give status of field.
*/

_do_field(string key) { 
  return field ? fieldon : fieldoff;
}

/*---------------------------------------------------------------------------
** Handle Exits
*/

fdown() {
    this_player()->move(MR+"dispatch", M_GO, "down");
    return 1;
}

fwest() {
    if (ME->query_field()) {
      if (!PL_WLEVEL) {
        write ("You can't pass the security field.\n");
        say (PL_NAME+" tries to pass the field, but fails.\n");
        return 1;
      }
      write ("The security field opens to let you pass.\n");
    }
    this_player()->move(MR+"home", M_GO, "west");
    return 1;
}

ftestareal() {
  object areal;
  areal = resolve_file("./room/testareal")->mkroom(1,1);
  if (areal)
  {
    write("The workroom folds up before your eyes...\n");
    if (ME_OK != PL->move(areal, M_TELEPORT))
      write("... then unfolds again.\n");
  }
  else
    write("You can't enter the areal.\n");
  return 1;
}

fscatter() {
  write("The workroom vanishes before your eyes...\n");
  this_player()->move(M+"room/scatter", M_TELEPORT);
  return 1;
}

/*---------------------------------------------------------------------------
** Initialization
*/

create() {
  ::create();
  fieldon = "A magic field, keeping catastrophic events inside.\n";
  fieldoff = "A magic field, separating the workroom from the home.\n";
  field = 0;

  SetIntShort ("Mateese's Workroom");
  SetIntLong (
"You are in Mateese's Workroom.\n"+
"The walls show some deep carves and dark burned stains, and your view\n"+
"is not quite clear due to the magic fields wobbling through the air.\n"+
"Alas, you can recognize a small shelter in one corner, and the floor is\n"+
"not that material.\n"+
"At one side of the room is a small control panel.\n"+
"To the west a magic field 'secure's the outside from the room, and\n"+
"through a dimensional rift leads a way to the 'testareal'.\n"
          );
  AddDetail (({"dimensional rift", "rift"})
            , "The rift leads to a small areal in the void.\n");
  AddDetail ("shelter","A small but sturdy shelter.\n");
  AddDetail ( ({"panel", "control", "control panel"})
            , "Black, quiet, with open wires, this does not seem functional.\n"
            );
  AddDetail ( ({ "magic field", "field", "doorway" }), #'_do_field);
  Set(P_TPORT, TPORT_OUT);
  Set(P_INDOORS, 1);
  AddExit("west", #'fwest);
  AddExit("testareal",#'ftestareal);
/*
  AddExit ("scatter",#'fscatter);
*/
  AddExit("down", #'fdown);
  AddRoomCmd ( ({ "secure", "unsecure" }), "fsecure");

  call_other (MR+"home", "???");
  home = find_object (MR+"home");
}

/*---------------------------------------------------------------------------
** Fieldhandling; partly for use by my home
*/

fsecure(str) {
  if (str || !PL_WLEVEL) return 0;
  set_field (query_verb() == "secure");
  home->set_field (query_verb() == "secure");
  return 1;
}

query_field() { return field; }

set_field(flag) {
  if (field == flag) return;
  if (flag) tell_room (ME, "The magic field strengthens.\n");
  else tell_room (ME, "The magic field weakens.\n");
  field = flag;
}

/*=========================================================================*/





