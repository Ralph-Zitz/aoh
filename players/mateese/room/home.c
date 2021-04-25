/* Mateese's Home. */

#include "/players/mateese/mydefs.h"

#include <config.h>
#include <properties.h>
#include <rooms.h>
#include <moving.h>
#include <stdrooms.h>

inherit "/std/room";

static string fieldon, fieldoff;
static string dooropen, doorclose;
static int field, door;
#define DOOR_AUTO_TIME 30
static object realwork;

int feast();
int fsouth();
int fdown();
string _door();
string _field();

create() {
  room::create();
  SetIntShort ("Mateese's Home");
  SetIntLong (
"The home of Mateese: a medium sized room, extending further to the west.\n"+
"The place is rather empty, the few stuff on the floor and the walls\n"+
"are dimly lit by cold light.\n"+
"To the east a magic field 'secure's this room from the workroom, while\n"+
"the door south is almost normal.\n"
              );
  AddExit ("west", MR+"homeextend");
  AddExit ("east", #'feast);
  AddExit ("south", #'fsouth);
  AddExit ("down", #'fdown);
  AddDetail (({"wall", "walls"}), 
"The walls between the windows are white naked, except for some strange posters "
"and a picture sketched in charcoal. "
"In one corner a small sign can be seen.\n"
            );
  AddDetail ("floor", 
"The floor is made of wood of the darker type. It feels cool and funny.\n"
            );
  AddDetail ("stuff", 
"Just some stuff, a mattress, blankets, some books, scattered on the floor.\n"
            );
  AddDetail ("posters", "Some really weird posters...\n");
  AddDetail (({"sketch", "charcoal", "picture" }),
"The portrait of an elf, sketched in charcoal. Inviting eyes, blond hair, "
"a cute smile. Below picture, the words 'My QT' were written.\n"
            );
  AddDetail ("sign", "The sign points down as if it shows a direction.\n");
  AddDetail ("computer", "You mean no computer, don't you?.\n");
  AddDetail ("no computer",
"It's not a dust catching thing from a far future.\n"
            );
  AddDetail ("mattress", 
"A mattress as used on trecking tours. Together with the blankets it serves\n"+
"as a quite comfortable bed.\n"
            );
  AddDetail ("blankets", 
"Some blankets, warm and snugly, draped across the mattress.\n"
            );
  AddDetail ("books", "A funny mixture of Comics, SF, Magic and Darkness.\n");
  AddDetail (({"window", "windows"}), 
"Outside you see a vast dark plain, stony and with sparse vegetation.\n"+
"Far at the horizon it is enclosed by rough mountains, which are vaguely\n"
"visible against the dark sky.\n"
            );
  AddDetail (({ "field", "magic field", "doorway" }), #'_field);
  AddDetail ("door", #'_door);
  AddRoomCmd (({"secure","unsecure"}), "fsecure");
  AddRoomCmd (({"open","close"}), "fopen");
  Set(P_INDOORS, 1);
  SetBright (MAX_SUNBRIGHT/4);

  fieldon = "A magic field, keeping catastrophic events outside.\n";
  fieldoff = "A magic field, separating the home from the workroom.\n";
  field = 0;

  dooropen = "The door is open.\n";
  doorclose = "The door is closed.\n";
  door = 0;
  call_other (M+"workroom", "???");
  realwork = find_object (M+"workroom");
}

string _field() { return field ? fieldon : fieldoff; }
string _door() { return door ? dooropen : doorclose; }

set_field(flag) {
  if (field == flag) return;
  if (flag) {
    tell_room (ME, "The magic field strengthens.\n");
  } else {
    tell_room (ME, "The magic field weakens.\n");
  }
  field = flag;
}

int feast() {
    if (ME->query_field()) {
      if (!PL_WLEVEL) {
	write("Visiting Mateese's workroom now won't be healthy for you.\n");
        say (PL_NAME+
             " tries, but at last doesn't dare to enter Mateese's Workroom.\n"
            );
        return 1;
      }
      write ("The security field opens to let you pass.\n");
    }
    this_player()->move(M+"workroom", M_GO, "east");
    return 1;
}


int fsecure(string str) {
  if (str || !PL_WLEVEL) return 0;
  set_field (query_verb() == "secure");
  realwork->set_field (query_verb() == "secure");
  return 1;
}


int fdown() {
    this_player()->move(MR+"dispatch", M_GO, "down");
    return 1;
}

int fsouth() {
  if (!door) {
    write ("The door gets immaterial for a second to let you pass.\n");
    say ("The door gets immaterial for a second to let "+PL_NAME+" pass.\n");
  }
  PL->move(MR+"mylobby", M_GO, "south");
  return 1;
}

int fopen(string str) {
  if (!str || str != "door") return 0;
  if (!PL_WLEVEL) {
    write ("You are not allowed to do that.\n");
    return 1;
  }
  if (query_verb() == "close") {
    if (door) write ("You find no handle.\n");
    else write ("The door is already closed.\n");
    return 1;
  }
  if (door) {
    write ("The door is already open.\n");
    remove_call_out ("door_close");
  } else {
    write ("You open the door.\n");
    say (PL_NAME+" opens the door.\n");
    call_other (MR+"mylobby", "door_opens");
    door = 1;
  }
  call_out ("door_close", DOOR_AUTO_TIME);
  return 1;
}

int fclose(string str) {
  if (!str || str != "door") return 0;
  if (!PL_WLEVEL) {
    write ("You are not allowed to do that.\n");
    return 1;
  }
  if (!door) {
    write ("The door is already closed.\n");
  } else {
    write ("You close the door.\n");
    say (PL_NAME+" closes the door.\n");
    call_other (MR+"mylobby", "door_closes");
    remove_call_out ("door_close");
    door = 0;
  }
  return 1;
}

door_closes() {
  if (!door) return;
  tell_room (ME, "The door closes automagically.\n");
  call_other (MR+"mylobby", "door_closes");
  door = 0;
}

query_lobby_door() { return door; }

player_knocks() {
  object me;
  tell_room (ME, capitalize(PL_RNAME)+" knocks on the door.\n");
  me = find_living("mateese");
  if (me && !present ("mateese", ME))
    tell_object (me, ":-: "+capitalize(PL_RNAME)+" knocks on your door.\n");
}

/* The following both routines are called by some random wiz handling
** the door from the lobby.
*/

lobby_open_door() {
  write ("You open the door.\n");
  say (PL_NAME+" opens the door.\n");
  tell_room (ME, capitalize(PL_RNAME)+" opens the door.\n");
  call_other (MR+"mylobby", "door_opens");
  door = 1;
  call_out ("door_close", DOOR_AUTO_TIME);
}

lobby_close_door() {
  write ("You close the door.\n");
  say (PL_NAME+" closes the door.\n");
  tell_room (ME, capitalize(PL_RNAME)+" closes the door.\n");
  call_other (MR+"mylobby", "door_closes");
  door = 0;
  remove_call_out ("door_close");
}







