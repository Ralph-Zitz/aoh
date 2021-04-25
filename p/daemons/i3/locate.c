//
// Wunderland Mudlib
//
// global/daemon/i3/locate.c  --  Locate handler
//
// $Log: locate.c,v $
// Revision 1.1  2002/08/04 18:14:56  Fiona
// Initial revision
//

#pragma strong_types
#pragma no_clone
#pragma no_shadow
#pragma no_inherit

#include <properties.h>
#include <msgclass.h>

#undef DEBUG_I3
#include <i3.h>

static void locate(mixed *data);
static void answer(mixed *data);

public void ServiceQuery() {
  I3->RegisterService("locate", "locate-req",   #'locate, ASSERT_LOCATE_REQ);
  I3->RegisterService(0,        "locate-reply", #'answer, ASSERT_LOCATE_REPLY);
}

public void DaemonShutdown() {
  // nothing here
}

static void locate(mixed *data) {
  object pl;
  string nam, state;
  int idle;

  // Some muds allow to locate logged off players also. Here we
  // just reply if the player is actually interactive (and visible)

  pl = find_player(trim(lower_case(data[L_REQ_USER])));
  if (!pl || pl->QueryProp(P_INVIS) || !interactive(pl)) return;

  idle  = query_idle(pl);        // mind to check for interactive() first
  nam   = pl->Query(P_NAME);
  state = pl->QueryProp(P_AWAY);

  SENDI3(({ "locate-reply", // SERVICE
            5,              // HOPS
            MUDNAME,        // O_MUD
            0,              // O_USER
            data[O_MUD],    // D_MUD
            data[O_USER],   // D_USER
            MUDNAME,        // L_MUDNAME
            nam,            // L_VISNAME
            idle,           // L_IDLE
            state           // L_STATUS
    }));
}

static void answer(mixed *data) {
  object pl;
  string msg;

  data[D_USER] = lower_case(data[D_USER]);
  pl           = find_player(data[D_USER]);
  if (!pl) return;

  msg = "Found in I3: " + data[L_VISNAME] + "@" + data[L_MUDNAME];
  if (data[L_IDLE] > 0) msg += sprintf(" (idle %d seconds)", data[L_IDLE]);
  if (stringp(data[L_STATUS]) && sizeof(data[L_STATUS]))
    msg += " (status is " + data[L_STATUS] + ")";

  msg_object(pl, CMSG_GENERIC, msg + ".\n" );
}

void send_locate_request(string who, string myid) {
  // if (object_name(previous_object()) != IMUD_CONNECT) return;
  if (!myid) myid = getuid(this_interactive());
  SENDI3(({ "locate-req", // SERVICE
            5,            // HOPS
            MUDNAME,      // O_MUD
            myid,         // O_USER
            0,
            0,
            who           // L_REQ_USER
    }));
}
