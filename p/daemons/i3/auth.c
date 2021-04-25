//
// Wunderland Mudlib
//
// global/daemon/i3/auth.c  --  Authenticate OOB requests
//
// $Log: auth.c,v $
// Revision 1.3  2002/08/06 08:30:02  Fiona
// type corrected
//

#pragma strong_types
#pragma no_clone
#pragma no_inherit
#pragma no_shadow

#undef DEBUG_I3
#include <i3.h>

static nosave mapping session_keys = ([:3]);

#define AUTH_HOST     0
#define AUTH_KEY      1
#define AUTH_TIME     2

static void handle(mixed* data);
static void get_key(mixed* data);

/////
// Main handling functions

public void ServiceQuery() {
  I3->RegisterService("auth", "auth-mud-req",   #'handle,  ASSERT_AUTH);
  I3->RegisterService(0,      "auth-mud-reply", #'get_key, ASSERT_AUTH_REP);

  I3->RegisterService(0,      "oob-req",        #'handle,  ASSERT_AUTH);
}

public void DaemonShutdown() {
  // nothing here
}

static void handle(mixed* data) {
  int key;

  if (data[SERVICE] == "auth-mud-req") key = random(__INT_MAX__-1)+1;

  session_keys[data[O_MUD], AUTH_HOST] =
    I3_TOOL("mudlist")->query_mud_info(data[O_MUD])[MLI_IP_ADDR];
  session_keys[data[O_MUD], AUTH_KEY]  = key;
  session_keys[data[O_MUD], AUTH_TIME] = time() + 600;

  while (remove_call_out("flush_keys") >= 0);
  call_out("flush_keys", 602);

  if (!key) {
    DEBUGI3("oob request received");
    return;
  }

  // don't log/output session keys (only for debugging)
# ifdef SHOW_KEYS
    DEBUGI3("key for " + data[O_MUD] + ": " + key);
# else
    DEBUGI3("key for " + data[O_MUD]);
# endif

  SENDI3(({ "auth-mud-reply", // SERVICE
            5,                // HOPS
            MUDNAME,          // O_MUD
            0,
            data[O_MUD],      // D_MUD
            0,
            key               // A_KEY
    }));
}
#define SHOW_KEYS

static void get_key(mixed* data) {
  // don't log/output session keys (only for debugging)
# ifdef SHOW_KEYS
    DEBUGI3("got key from " + data[O_MUD] + ": " + data[A_KEY]);
# else
    DEBUGI3("got key from " + data[O_MUD]);
# endif
  I3_TOOL("oob")->got_oob_key(data[O_MUD], data[A_KEY]);
}

public void send_auth_req(string target, int key_wanted) {
  string service;
  // if (object_name(previous_object()) != I3_TOOL("oob")) return;
  if (key_wanted) service = "auth-mud-req";
  else            service = "oob-req";
  SENDI3(({ service,  // SERVICE
            5,        // HOPS
            MUDNAME,  // O_MUD
            0,
            target,   // D_MUD
            0
    }));
}

static void flush_keys() {
  string mud, host;
  int t, key, my_t;

  t = time();
  foreach (mud, host, key, my_t : session_keys) {
    if (my_t < t) m_delete(session_keys, mud);
  }
}

// positiv = key
// 0       = oob-req granted
// -1      = forbidden
public int query_auth_key(string mud) {
  flush_keys();
  if (object_name(previous_object()) != I3_TOOL("oob")) return -1;
  if (!member(session_keys, mud)) return -1;
  return session_keys[mud, AUTH_KEY];
}

// return mudname for ip address if mud is expected to connect to oob
// return 0 otherwise
public string query_oob_pend(string host) {
  string mud, ip;

  flush_keys();
  if (object_name(previous_object()) != I3_TOOL("oob")) return 0;
  foreach (mud, ip : session_keys)
    if (ip == host) return mud;
  return 0;
}
