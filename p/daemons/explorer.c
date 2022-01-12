/*
 *	obj/explorer.c
 *
 * 23-12-94 Pepel created
 * 08-01-95 Pepel introduced rewards
 * 09-01-95 Pepel separated the rooms list int /obj/explorerooms.ic
 * 10-01-95 Pepel renamed GetRoomIndex() to SetExplored(), which is more
 *		  appropriate for what it does. Accordingly changed the
 *		  invocation in player.c.
 *		  Also implemented some sanity checking.
 * 12-04-95 Pepel added check if room files exist
 *
 * The control object for the explorer quest.
 * Strongly inspired by the explorer system in TubMUD.
 *
 * Should be integrated into the QUESTMASTER object some day(?)
 *
 * Q: So, now, what is the magic that I need to put into my
 *    room to set the explorer flag?
 * A: Basically, the room needs to call this_player()->SetExplored()
 *    To ease the simple exploration of rooms and details (i.e.: entering
 *    a room or scrutinizing a detail counting as \'act of exploration\'),
 *    Mateese has added the P_EXPLORABLE property to the /std/room.
 *    In the room\'s create simply do:
 *	SetExplorable(1)	to make the room explorable
 *	SetExplorable("zorch")	to make the detail explorable
 *				(look at zorch)
 *
 *    These two functins save you the bother (and the mud the memory
 *    usage :-) of programming special functions into otherwise simple
 *    rooms.
 *    An other possibility is, to invoke the SetExplored() in a function
 *    that is invoked to handle a command.
 *
 *    Example:
 *	pull_lever() {
 *	    this_player()->SetExplored();
 *	    write("as you pull the lever, you hear a clicking sound\n");
 *	    [..]
 *	    return 1;
 *	}
 *
 *   Another example (obsoleted by SetExplorable("detail-name")):
 *	create() {
 *	    [..]
 *	    AddDetail("picture",
 *		      lambda(0,
 *			     ({ #', ,
 *				({ #'call_other,
 *				    ({ #'this_player }),
 *				    "SetExplored"
 *				}),
 *				"A picture of Pepel exploring the World\n"
 *			     })
 *			    );
 *	    [..]
 *	}
 * Although the second example looks a bit complicated, it is really
 * simple. You need not to understand it, you just can copy it into
 * your code.
 */


#define ME this_object()
#define PREV(x) previous_object(x)
#define PO previous_object()
#define TP this_player()
/* the purpose of the next three macros is to avoid the hash-tick notation,
 * which would confuse emacs c-mode indent engine _sigh_
 */
#define LSF(x) symbol_function(x, ME)	/* lfun */
#define ESF(x) symbol_function(x)	/* efun */
#define L(x) lambda(0, x)

#define INACTIVE "inactive"

nomask void log(string s) {
  CHANNEL_D->SendTell("error", object_name(), s, 1);
}
#define LOG(cond, str) if (cond) log(str);

private nosave mapping rooms;

/* --- */

private void check_file(string|string *x) {
  if ("" == x) return;
  if (!x || !stringp(x)
   || (x[0..sizeof(INACTIVE)-1] != INACTIVE && file_size(x+".c") < 0))
    log("Warning: No File for ``"+x+"''\n");
}

private void check_files() {	/* do the specified files exist? */
  map_indices(rooms, #'check_file /*'*/);
}

/* create() and SetExplored() are the two only functions that are
 * required for the explorer mechanism to work. the other functions
 * are for the wizards comfort and curiosity
 */
void create() {
  int *indizes;
  int i;

  /* the list of rooms and rewards is contained in a separate file */

#include "/p/daemons/explorerooms.ic"

  /* sanity checking */
  /* are all indizes unique ? */
  indizes = sort_array(m_values(rooms), #'>/*'*/);
  for (i = 0; i < sizeof(indizes)-1 ; i++)
    if (indizes[i] == indizes[i+1])
      log("Error: Room Index "+indizes[i]+" defined twice\n");

  /* have numbers been allocated sequentially ? */
  if (indizes[i] >= sizeof(indizes)) {
    log("Error: room indizes have not been allocated sequentially, "
        + indizes[i] + " should be " + sizeof(indizes) + "\n");
    /* try to find out which one is missing */
    for (i = 0; i < sizeof(indizes)-1 ; i++)
      if (indizes[i] != i) write(indizes[i] + " should be " + i + "\n");
  }

  call_out(#'check_files /*'*/, 0);
}

/* keep it simple :-) */

nomask varargs string SetExplored(string e,int delay) {
  string n;
  mixed x;
  int b;

  n = explode(object_name(PREV(1)), "#")[0];
  b = rooms[n]; x = rooms[n, 1];
  if (TP == PO && ! test_bit(e, b)) {
    /* first time */
    if (intp(x)) PO->AddXP(x,delay); else funcall(x);
    /* Just log it for the test rooms */
    LOG(b < 2, sprintf("%O explores %s #%d, reward %O", TP, n, b, x));
  } else {
    /* deja vu */
    /* condition 0 == b will log all rooms that try to set
     * an explored bit but are not in the list.
     */
    LOG((0 == b), sprintf("%O has a deja-vu from %s #%d", TP, n, b));
  }
  return set_bit(e, b);
}

/* --- */

int clean_up(int ref) { destruct(ME); return ME && 1; } /* we are stateless , thus can easily save mem */

/* --- */

/* some functions for comfort, statistics and curious people */
int CountBits(string s) {
  int i, j;
  for (i = 0; i < sizeof(s||"") * 6; i++)
    j += test_bit(s||"", i);
  return j;
}

int TotalEP() {
  return sizeof(rooms);
}
