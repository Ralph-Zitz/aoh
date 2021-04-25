//
// Wunderland Mudlib
//
// global/daemon/i3/ucache.c  --  Username cache handler
//
// $Log: ucache.c,v $
// Revision 1.2  2003/06/19 11:30:42  Fiona
// First try to have full ucache and chan-user functionalety
//

#pragma strong_types
#pragma no_clone
#pragma no_inherit
#pragma no_shadow

#include <properties.h>
#undef DEBUG_I3
#include <i3.h>

mapping userinfo; // ([ "id@mud" : Name; Mud; Gender, ... ])
mixed* pending;   // ({ ({ request, request, ... })
                  // with request = ({ time, "id@mud", callback, data })

#define UI_NAME    0
#define UI_MUD     1
#define UI_GENDER  2
#define UI_TIME    3

#define PEND_TIME  0
#define PEND_ID    1
#define PEND_CB    2
#define PEND_DATA  3

#define U_TIMEOUT 60
#ifndef FINGEROBJ
#define FINGEROBJ "/obj/player_dummy"
#endif

/////
// Main handling functions

static void  user_req(mixed data);
static void  user_reply(mixed data);
static void clear_pending(int clear_all);

void create() {
  userinfo = ([ :4 ]);
  pending = ({});
}

public void ServiceQuery() {
  I3->RegisterService("ucache", "ucache-update", #'user_reply /*'*/, ASSERT_USER_REPLY);
  I3->RegisterService(0, "chan-user-req",        #'user_req /*'*/,   ASSERT_USER_REQ);
  I3->RegisterService(0, "chan-user-reply",      #'user_reply /*'*/, ASSERT_USER_REPLY);
}

public void DaemonShutdown() {
  clear_pending(1);
}

int remove() {
  DaemonShutdown();
  return 1;
}

//////
// get user info for targetted emotes

static void user_req(mixed data) {
  int gender;
  string user, uname;

  user = data[CU_REQ_USER];
  user = lower_case(trim(user));

  gender = FINGEROBJ->QueryProperty(user, P_GENDER);
  if (gender < 0) {
    SENDI3ERR(data, "unk-user", "Unknown user.");
    return;
  }

  switch (gender) {
    case GENDER_MALE:   gender = CU_GENDER_MALE; break;
    case GENDER_FEMALE: gender = CU_GENDER_FEMALE; break;
    default:            gender = CU_GENDER_NEUTER; break;
  }

  uname = FINGEROBJ->QueryProperty(user, P_NAME);

  SENDI3(({ "chan-user-reply", // SERVICE
            5,                 // HOPS
            MUDNAME,           // O_MUD
            0,
            data[O_MUD],       // D_MUD
            0,
            user,              // CU_REQ_USER
            uname,             // CU_VISNAME
            gender             // CU_GENDER
    }));
}

static void user_reply(mixed data) {
  string id;
  int i, f;

  DEBUGI3(MIXED2STR(({ data[CU_VISNAME], data[CU_GENDER] })));

  id = lower_case(data[CU_REQ_USER]) + "@" + data[O_MUD];

  // accept updates only for existing entries
  if (!data[D_MUD] && !member(userinfo, id)) return;

  userinfo[id, UI_NAME]   = data[CU_VISNAME];
  userinfo[id, UI_MUD]    = data[O_MUD];
  userinfo[id, UI_GENDER] = data[CU_GENDER];

  for (i = sizeof(pending); i--;) {
    if (pending[i][PEND_ID] != id) continue;
    funcall(pending[i][PEND_CB], id, pending[i][PEND_DATA],
      userinfo[id, UI_NAME], userinfo[id, UI_MUD], userinfo[id, UI_GENDER]);
    pending[i..i] = ({});
    f = 1;
  }

  if (f) {
    userinfo[id, UI_TIME] = time();
    clear_pending(0);
  }

}

void send_chan_user_req(string targetid, string targetmud) {
  SENDI3(({ "chan-user-req",   // SERVICE
            5,                 // HOPS
            MUDNAME,           // O_MUD
            0,
            targetmud,         // D_MUD
            0,
            targetid           // CU_REQ_USER
    }));
}

// We got an error for our request. See if we could generate a
// readable Message for a player from it, else let the error
// handle log the error
int got_error(mixed* data, string errmsg) {
  string id;
  int i, f;

  id = lower_case(data[ERR_DATA][CU_REQ_USER]) + "@" + data[ERR_DATA][D_MUD];

  for (i = sizeof(pending); i--;) {
    if (pending[i][PEND_ID] != id) continue;

    funcall(pending[i][PEND_CB], id, pending[i][PEND_DATA], errmsg, 0, data);
    pending[i..i] = ({});
    f = 1;
  }

  if (f) {
    userinfo[id, UI_TIME] = time();
    clear_pending(0);
  }

  switch (data[ERR_TYPE]) {
    case "unk-dst":
    case "not-imp":
    case "unk-user":
      return 1;
  }
  return 0;
}

// Callback 'cb' is called in three different forms:
//
// CB(id, data, user_visname, mudname, gender)        // all o.k.
// CB(id, data, error_message, 0, error_packet_data)  // user-unk etc
// CB(id, data, error_message, 0, 0)                  // time out

public int get_chan_user(string id, mixed data, closure cb) {
  string* x;

  if (userinfo[id, UI_NAME]) {
    userinfo[id, UI_TIME] = time();
    funcall(cb, id, data, userinfo[id, UI_NAME],
      userinfo[id, UI_MUD], userinfo[id, UI_GENDER]);
    return 1;
  }

  x = efun::explode(id, "@");
  send_chan_user_req(x[0], x[1]);
  pending += ({ ({ time(), id, cb, data }) });
  clear_pending(0);

  return 0;
}

static void clear_pending(int clear_all) {
  int i, j, next;

  next = -1;
  if (clear_all) {
    i = sizeof(pending) - 1;
  } else {
    for (i = sizeof(pending); i--;) {
      if (pending[i][PEND_TIME] + U_TIMEOUT < time()) break;
      next = pending[i][PEND_TIME] - time() + U_TIMEOUT + 2;
    }
  }

  for (j = i; j >= 0; --j) {
    funcall(pending[j][PEND_CB], pending[j][PEND_ID],
      pending[j][PEND_DATA], "Timeout", 0, 0);
  }
  pending[0..i] = ({});

  j = i = remove_call_out("clear_pending");
  while (i >= 0) i = remove_call_out("clear_pending");

  if (j != -1) next = min(next, j);

  if (next >= 0) call_out("clear_pending", next, 0);
}

// Clear userinfo, just keep the 50 last used
void reset() {
  string* ids;

  if (sizeof(userinfo) < 50) return;
  ids = m_indices(userinfo);
  ids = sort_array(ids, (: userinfo[$1, UI_TIME] < userinfo[$2, UI_TIME] :));
  ids = ids[50..];
  map(ids, (: m_delete(userinfo, $1) :));
}
