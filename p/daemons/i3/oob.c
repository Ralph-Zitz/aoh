//
// Wunderland Mudlib
//
// global/daemon/i3/oob.c  --  Process OOB requests
//
// $Log: oob.c,v $
// Revision 1.9  2002/08/21 15:32:38  Fiona
// uses RAISE_ERROR() macro now
//
// Revision 1.8  2002/08/16 12:33:16  Fiona
// Fixed timeout mechanism (Nostradamus@AgeOfHeros)
//

#pragma strong_types
#pragma no_clone
#pragma no_inherit
#pragma no_shadow

inherit __DIR__ "assert";

#define NEED_PROTOTYPES
#include <socket.h>
#undef DEBUG_I3
#include <i3.h>

static nosave object  listening_sock;
static nosave mapping serv_cb;
static nosave mapping mudname; // socket to mudname converter
#define MN_NAME    0
#define MN_INOU    1

static nosave mapping conns; // key is mudname
#define CONIN      0  // conns mapping index
#define CONOUT     1

#define CON_FLAGS  0  // see CF_*
#define CON_KEY    1  // key if outbound connection
#define CON_SOCK   2  // the socket
#define CON_DATA   3  // pending outgoing data
#define CON_CB     4  // callback for outgoing data
#define CON_TIME   5  // last usage time
#define CON_INIT   6  // initializing time of connection

#define CF_KEY     1  // set if keyed connection
#define CF_START   2  // set if startup finished
#define CF_DIR     4  // current direction, we may send if set

/***** The data structure of conns:
 *
 * conns = ([ "Wunderland": ({ ... }); ({ ... }), ... ])
 *
 * Each mud has exactly one entry in conns. The mapping has to values,
 * CONIN and CONOUT which are arrays. We may have two connections with each
 * mud, one incoming and one outgoing. This means who was initiator of the
 * connection, not current communication direction on that connection.
 * We may drop outgoing connection after we finished. This is nessesary to
 * prevent deadlocks with other muds.
 *
 * Each connection array looks like this:
 * conns[mud, CONIN] =
 *   ({ CON_FLAGS, CON_KEY, CON_SOCK, CON_DATA, CON_CB, CON_TIME })
 *
 * CON_DATA and CON_CB are arrays with the data to be send and associated
 * error callback functions.
 * CON_DATA = ({ ({ data 1 }), ({ data 2 }), ... })
 * CON_CB   = ({ cb for data 1, cb for data 2, ... })
 *
 * The mapping's value is NULL if the particular connection does not exist.
 */

public void ServiceQuery() {
  I3->RegisterService("!oob", 0, 0, 0);
  if (listening_sock) I3->change_state(I3_OOB_GEN);
}

public void OOBService() {
  I3_TOOL("oob")->RegisterOOB("oob-begin", #'oob_startup,   ASSERT_OOB_BEGIN);
  I3_TOOL("oob")->RegisterOOB("oob-end",   #'oob_dirchange, ASSERT_OOB_END);
}

//////
// general management functions
//

void create() {
  string file;

  if (serv_cb) return;
  serv_cb = ([]);
  conns   = ([:2]);
  mudname = ([:2]);
  foreach (file : get_dir(I3_TOOL("*.c"))) {
    catch(I3_TOOL(file)->OOBService());
  }

  if (this_player()) call_out("start_oob", 3);
  else call_out("start_oob", LOAD_DELAY+3); // after I3 daemon
}

void RegisterOOB(string type, closure cb, mixed* ass) {
  object ob;
  string obn;

  ob  = to_object(cb);
  obn = object_name(ob);
  if (ob != ME && (ob != previous_object() || strstr(obn, I3_TOOLDIR)))
    RAISE_ERROR("privilege violation: RegisterOOB()\n");

  if (!pointerp(ass)) DEBUGI3("assertions missing for " + type);

  serv_cb[type] = ({ cb, obn, ass });

  DEBUGI3("registered " + type);
}

// Set up listening socket
static void start_oob() {
  if (!listening_sock) {

    if (I3->query_state() & I3_DISABLED) return;

    listening_sock = SOCKETD->New(TCP_LISTEN, 0, PORT_OOB,
      #'listen_cb, SOCKET_MUDMODE);
    if (!listening_sock)
      DEBUGI3("creating of OOB sock failed");
    else
      DEBUGI3("OOB sock created");
  }
}

// Handles the listening itself
static void listen_cb(object sock, int action, mixed d1, mixed d2, mixed d3) {
  object conn;
  switch (action) {
    case SOCKET_READY:
      I3->change_state(I3_OOB_GEN);
      return;
    case SOCKET_ACCEPT:
      conn = sock->Accept(#'conn_cb);
      if (!conn) DEBUGI3("Could not establish incomming connection");
      return;
    case SOCKET_CLOSE:
      I3->change_state(I3_OOB_GEN, 1);
      call_out("start_oob", RECON_DELAY, 0);
      return;
    case SOCKET_ERROR:
      LOGI3("listen_cb", "SOCKET_ERROR: " +
        MIXED2STR(d1) + " " + MIXED2STR(d2) + " " +
        MIXED2STR(d3));
      return;
    case SOCKET_NOPEND:
      return;
  }
  LOGI3("listen_cb", "Unknown action: " +
    MIXED2STR(action) + " " + MIXED2STR(d1) + " " +
    MIXED2STR(d2) + " " + MIXED2STR(d3));
}

// caches socket to mudname conversion
static string fetch_mudname(object sock, int inout) {
  string mud;
  mixed *i1, *i2;
  mud   = mudname[sock, MN_NAME];
  inout = mudname[sock, MN_INOU];
  if (mud) return mud;
  foreach (mud, i1, i2 : conns)
    if (pointerp(i1) && i1[CON_SOCK] == sock) {
      mudname[sock, MN_NAME] = mud;
      mudname[sock, MN_INOU] = inout = CONIN;
      return mud;
    }
    if (pointerp(i2) && i2[CON_SOCK] == sock) {
      mudname[sock, MN_NAME] = mud;
      mudname[sock, MN_INOU] = inout = CONOUT;
      return mud;
    }
  return 0;
}

static void register_connection(object sock, string mud) {
  // This handles incoming connections only
  // The specs don't specify what should happen if we have already
  // a connection to this mud. We decided to do as follows:
  // The old connection is dropped, pending data is transfered
  // to the new connection.
  mixed* info;
  object oldsock;

  info = conns[mud, CONIN];
  if (!info) {
    info = allocate(7);
    info[CON_DATA] = ({});
    info[CON_CB]   = ({});
  }

  oldsock = info[CON_SOCK];
  if (oldsock) {
    DEBUGI3("register_connection: dropping old connection");
    m_delete(mudname, oldsock);
    oldsock->Close();
    info[CON_FLAGS] = 0;
  }

  info[CON_SOCK]         = sock;
  info[CON_TIME]         = time();
  info[CON_INIT]         = time();
  conns[mud, CONIN]      = info;
  mudname[sock, MN_NAME] = mud;
  mudname[sock, MN_INOU] = CONIN;
}

static void conn_cb(object sock, int action, mixed d1, mixed d2, mixed d3) {
  string mud;
  int inout;
  mixed x;

  mud = fetch_mudname(sock, &inout);
  // if mud is still unknown it is a newly accepted connection

  switch (action) {
    case SOCKET_READY:

      // if mud is known, it is an outbound connection
      if (mud) {
        if (inout == CONIN) {
          DEBUGI3("conn_cb: connection type mismatch");
          sock->Close();
          return;
        }
        DEBUGI3("conn_cb: sending startup to "+mud);
        sock->Write(
          ({ "oob-begin",                                // SERVICE
             MUDNAME,                                    // OOB_O_MUD
             !!(conns[mud, CONOUT][CON_FLAGS] & CF_KEY), // OOB_AUTH_TYPE
             conns[mud, CONOUT][CON_KEY]                 // OOB_AUTH_KEY
          }));
        conns[mud, CONOUT][CON_TIME] = time();
        conn_timeout();
        return;
      }

      // We accept incoming connections only from the same IP as the mud
      // itself is running with. It's not part of the specification,
      // but rids us off strange people connecting to our service.
      //
      // If you want to check connections just with the oob-begin
      // packets define LOOSE_AUTH
      if (inout == CONOUT) {
        DEBUGI3("conn_cb: connection type mismatch 2");
        sock->Close();
        return;
      }
#     ifndef LOOSE_AUTH
        mud = I3_TOOL("auth")->query_oob_pend(d2); // d2 = ip-address
        if (!mud) {
          DEBUGI3("conn_cb: dropping unexpected connection from "+d2);
          LOGI3("conn_cb", "dropping unexpected connection from "+d2);
          sock->Close();
          return;
        }
        register_connection(sock, mud);
        conns[mud, CONIN][CON_TIME] = time();
        conn_timeout();
#     endif
      return;

    case SOCKET_READ:
#     ifndef LOOSE_AUTH
        if (!mud) {
          DEBUGI3("conn_cb: READ: couldn't find associated mud");
          sock->Close();
          return;
        }
#     endif
      conns[mud, inout][CON_TIME] = time();
      conn_timeout();
      return oob_service(d1, mud, sock,
        conns[mud, inout][CON_FLAGS] & CF_DIR, inout);

    case SOCKET_CLOSE:
      if (!mud) {
        DEBUGI3("conn_cb: CLOSE: couldn't find associated mud");
        return;
      }
      x = conns[mud, inout];
      m_delete(mudname, sock);
      conns[mud, inout] = 0;
      if (!pointerp(conns[mud, CONIN]) && !pointerp(conns[mud, CONOUT]))
        m_delete(conns, mud);

      if (x) {
        foreach (mixed* data : x[CON_DATA]) {
          // TODO: (re)send data
        }
        foreach (closure c : x[CON_CB])
          funcall(c, "Verbindungsabbuch");
      }
      return;

    case SOCKET_WRITTEN:
      if (!mud) {
        DEBUGI3("conn_cb: WRITTEN: couldn't find associated mud");
        sock->Close();
        return;
      }
      conns[mud, inout][CON_TIME] = time();
      conn_timeout();
      return;

    case SOCKET_ERROR:
      DEBUGI3("conn_cb: SOCKET_ERROR: " +
        MIXED2STR(d1) + " " + MIXED2STR(d2) + " " +
        MIXED2STR(d3));
      LOGI3("conn_cb", "SOCKET_ERROR: " +
        MIXED2STR(d1) + " " + MIXED2STR(d2) + " " +
        MIXED2STR(d3));
      foreach (x : conns[mud, inout][CON_CB])
        funcall(x, "Verbindungsfehler: " + (d3 || d1));

      sock->Close();
      // SOCKET_CLOSE is not called in every case...
      x = conns[mud, inout];
      m_delete(mudname, sock);
      conns[mud, inout] = 0;
      if (!pointerp(conns[mud, CONIN]) && !pointerp(conns[mud, CONOUT]))
        m_delete(conns, mud);

      if (x) {
        foreach (mixed* data : x[CON_DATA]) {
          // TODO: (re)send data
        }
        foreach (closure c : x[CON_CB])
          funcall(c, "Verbindungsabbuch");
      }
      return;
  }
  LOGI3("conn_cb", "Unknown action: " +
    MIXED2STR(action) + " " + MIXED2STR(d1) + " " +
    MIXED2STR(d2) + " " + MIXED2STR(d3));
}

// drop all connections which are inactive for more than HOLDTIME seconds
static void conn_timeout() {
  string mud;
  int t, wait, call;
  object sock;
  mixed *i1, *i2, i;
  closure x;

  foreach (mud, i1, i2 : conns) {
    foreach (i : ({ i1, i2 })) {
      if (!pointerp(i)) continue;
      t = i[CON_TIME] + OOB_HOLDTIME;
      if (!t) continue;
      t -= time();
      if (t <= 0) {
        DEBUGI3("conn_timeout: " + mud);
        i[CON_TIME] = -1;
        foreach (x : i[CON_CB])
          funcall(x, "Timeout, closing connection");
        sock = i[CON_SOCK];
        if (sock) sock->Close();
      }
      else if (wait > t || !wait) wait = t;
    }
  }

  call = find_call_out("conn_timeout");
  if (wait == call) return;
  if (call != -1) while (remove_call_out("conn_timeout") != -1);
  if (wait > 0) call_out("conn_timeout", wait);
}

int remove() {
  string mud;
  mixed *i1, *i2;

  if (find_object(I3)) I3->change_state(I3_OOB_GEN, 1);
  if (listening_sock) listening_sock->Close();
  foreach (mud, i1, i2 : conns) {
    if (pointerp(i1) && i1[CON_SOCK]) i1[CON_SOCK]->Close();
    if (pointerp(i2) && i2[CON_SOCK]) i2[CON_SOCK]->Close();
  }
  destruct(this_object());
  return 1;
}

string QueryService(string type) {
  mixed* x;
  x = serv_cb[type];
  if (!x) return 0;
  // NEVER return the closures itself, would break security
  return deep_copy(serv_cb[type][1..]);
}

//////
// Services
//

static void oob_service(mixed* data, string mymud, object conn, int dir,
    int inout) {
  string err, mud;
  mixed* handler;
  int auth;

  if (mymud) mud = mymud;
  else       mud = conn->State()[S_HOST];

  if (!pointerp(data)) err = "data not array (from "+mud+")";
  else if (sizeof(data) < 2)   err = "datasize too small (from "+mud+")";
  else if (!stringp(data[SERVICE])) err = "SERVICE not string (from "+mud+")";
  else if (!(handler = serv_cb[data[SERVICE]]))
    err = "Unknown SERV "+data[SERVICE]+" (from "+mud+")";
  else {
    if (handler[2]) err = assert(data, handler[2]);
    if (err) {
      err = data[SERVICE] + ": " + err + " (from "+mud+")";
      DEBUGI3("assertion: " + err);
      LOGI3("assertion", err + " " + MIXED2STR(data));
      conn->Close();
      return;
    }
  }
  if (!err && data[SERVICE] != "oob-begin" && mymud
    && !(conns[mymud, inout][CON_FLAGS] & CF_START)) {
    err = "first packet not oob-begin";
  }

  if (err) {
    DEBUGI3(err);
    LOGI3("oob_service", err + " " + MIXED2STR(data));
    conn->Close();
    return;
  }

  if (!to_object(handler[0])) {
    DEBUGI3("Handler destroyed... reloading...");
    m_delete(serv_cb, data[SERVICE]);
    catch(call_other(handler[1], "OOBService"));
    handler = serv_cb[data[SERVICE]];
    if (!handler) {
      LOGI3("call_service", "Fatal: " + data[SERVICE] +
        "handler unavailable.");
      DEBUGI3("Fatal: Handler still unavailable.");
      conn->Close();
      return;
    }
  }

  DEBUGI3("Handling " + data[SERVICE] + " on " + (inout?"CO":"CI") +
    (dir?"_out":"_in") + " (for "+mud+")");
  auth = conns[mymud, inout][CON_FLAGS] & CF_KEY;
  funcall(handler[0], data, mymud, conn, !!dir, inout, auth);
}

//////
// Send something over OOB
//

public void send_oob_reply(string mud, int io, mixed* data) {
  mixed *info;
  object po;
  if (extern_call() && (po = previous_object()) && po != ME) {
    if (strstr(object_name(po), I3_TOOLDIR))
      RAISE_ERROR("privilege violation: send_oob_reply()\n");
  }

  info = conns[mud, io];
  if (!pointerp(info) || !info[CON_SOCK]) {
    DEBUGI3("send_oob_reply: " + (io?"OUT":"IN") +
      " connection to " + mud + " vanished");
    return;
  }
  info[CON_SOCK]->Write(data);
  info[CON_TIME] = time();
  DEBUGI3("send_oob_reply: sending on " + (io?"OUT":"IN") +
      " connection to " + mud);
}

// This function opens a OOB connection to the target mud
// If connection already exists, it is reused.
public void send_oob_to(string mud, int key_needed, mixed* data, closure cb) {
  object po;
  if (extern_call() && (po = previous_object()) && po != ME) {
    if (strstr(object_name(po), I3_TOOLDIR))
      RAISE_ERROR("privilege violation: send_oob_to()\n");
  }

  if (!pointerp(conns[mud, CONOUT])) {
    I3_TOOL("auth")->send_auth_req(mud, key_needed);
    conns[mud, CONOUT]             = allocate(7);
    conns[mud, CONOUT][CON_FLAGS]  = key_needed ? CF_KEY : 0;
    conns[mud, CONOUT][CON_DATA]   = ({ data });
    conns[mud, CONOUT][CON_CB]     = ({ cb });
    conns[mud, CONOUT][CON_FLAGS] |= CF_DIR;
    conns[mud, CONOUT][CON_INIT]   = time();

    // If we use keyless OOB we don't know when the destination
    // mud will have gotten our oob_req via the network. So we just
    // wait for some WAITTIME and try to establish a connection then
    if (!key_needed) call_out("got_oob_key", OOB_WAITTIME, mud, 0);

    call_out("check_out_conn", OOB_TIMEOUT + 2, mud);

    return;
  }

  // TODO: check for key_needed

  conns[mud, CONOUT][CON_DATA] += ({ data });
  conns[mud, CONOUT][CON_CB]   += ({ cb });
  send_pending_data(mud);
}

// checks if the outgoing connection to mud succeeded within
// the time limits. It just checks for the startup. If other party
// does not answer to "end-oob" this is handled by HOLDTIME and
// conn_timeout() instead.
static void check_out_conn(string mud) {
  mixed* info;
  closure x;
  object sock;

  info = conns[mud, CONOUT];
  if (!info) return;
  if (info[CON_FLAGS] & CF_START) return;
  if (info[CON_INIT] + OOB_TIMEOUT > time()) return;

  info[CON_TIME] = -1;
  foreach (x : info[CON_CB])
    funcall(x, "Timeout 2, closing connection");
  sock = info[CON_SOCK];
  if (sock) sock->Close();
}

// Either we got an oob key or we dont even want one
// The oob connection itself is openend here
void got_oob_key(string mud, int key) {
  string ip, po;
  object sock;
  int port;

  po = object_name(previous_object());
  if (po != I3_TOOL("auth") && po != I3_TOOL("oob")) return;
  if (!pointerp(conns[mud, CONOUT])) {
    DEBUGI3("got_oob_key: unexpected from " + mud);
    return;
  }
  conns[mud, CONOUT][CON_KEY] = key;
  if (conns[mud, CONOUT][CON_SOCK])
    DEBUGI3("got_oob_key: got new key for " + mud);
  else {
    ip   = I3_TOOL("mudlist")->query_mud_info(mud)[MLI_IP_ADDR];
    port = I3_TOOL("mudlist")->query_mud_info(mud)[MLI_PORT_TCP];
    if (!ip || !port) {
      DEBUGI3("got_oob_key: got invalid connection data");
      return;
    }
    DEBUGI3("got_oob_key: opening OOB to " + ip + ":" + port);
    sock = SOCKETD->New(TCP_OPEN, ip, port, #'conn_cb /*'*/, SOCKET_MUDMODE);
    conns[mud, CONOUT][CON_SOCK] = sock;
    mudname[sock, MN_NAME] = mud;
    mudname[sock, MN_INOU] = CONOUT;
  }
}

static void send_pending_data(string mud) {
  object sock;
  closure* cbs;
  mixed* data;
  int i;

  for (i = CONIN; i <= CONOUT; i++) {
    if (!pointerp(conns[mud, i])) continue;

    if (!(conns[mud, i][CON_FLAGS] & CF_START)) continue; // wait
    if (!(conns[mud, i][CON_FLAGS] & CF_DIR)) {
      // TODO: If we waited too long, drop connection
      // and retry (on CONOUT only)
      continue; // wait
    }

    sock = conns[mud, i][CON_SOCK];
    if (!sock) {
      DEBUGI3("send_pending_data: vanished sock for " + mud);
      conns[mud, i] = 0;
      if (!pointerp(conns[mud, CONIN]) && !pointerp(conns[mud, CONOUT]))
        m_delete(conns, mud);
      continue;;
    }

    cbs  = conns[mud, i][CON_CB];
    data = conns[mud, i][CON_DATA];

    DEBUGI3("send_pending_data: sending to "+mud);
    while (sizeof(data)) {
      if (sock->State()[S_STATE] != S_CONNECTED) {
        DEBUGI3("send_pending_data: connection vanished for " + mud);
        break;
      }
      sock->Write(data[0]);
      data = data[1..];
      cbs  = cbs[1..];
    }

    conns[mud, i][CON_CB]   = cbs;
    conns[mud, i][CON_DATA] = data;

    if (sizeof(data) || !sock) {
      // TODO: do something?
      continue;
    }
    DEBUGI3("send_pending_data: reversing direction");

    // reverse communication direction
    conns[mud, i][CON_FLAGS] &= ~CF_DIR;
    sock->Write( ({
        "oob-end", // SERVICE
        MUDNAME    // OOB_O_MUD
      }));
  }
}

//////
// OOB begin pakets are handled here
//

static void oob_startup(mixed* data, string mud, object conn, int dir,
    int io, int keyed) {
  int key;

# ifdef LOOSE_AUTH
  if (!mud && !dir) {
    mud = data[OOB_O_MUD];
    if (I3_TOOL("auth")->query_auth_key(mud) < 0) {
      mud = conn->State()[S_HOST] + " (" + mud +")";
      DEBUGI3("oob_startup: dropping unexpected connection from "+mud);
      LOGI3("oob_startup", "dropping unexpected connection from "+mud);
      conn->Close();
      return;
    }
    register_connection(conn, mud);
  }
  else
# endif
  if (data[OOB_O_MUD] != mud) {
    DEBUGI3("oob_startup: wrong oob_o_mud '" + data[OOB_O_MUD]+
      "' for " + mud);
    conn->Close();
    return;
  }

  if (conns[mud, io][CON_FLAGS] & CF_START) {
    DEBUGI3("oob_startup: ignoring repeated paket");
    return;
  }

  // Incoming OOB connection. Send oob-begin reply if key is ok
  // disconnect otherwise
  if (io == CONIN) {
    key = I3_TOOL("auth")->query_auth_key(mud);
    if (key < 0) {
      DEBUGI3("oob_startup: dropping unexpected connection from "+mud);
      LOGI3("oob_startup", "dropping unexpected connection from "+mud);
      conn->Close();
      return;
    }
    if (key && !data[OOB_AUTH_TYPE]) {
      DEBUGI3("oob_startup: no auth for connection type from "+mud);
      LOGI3("oob_startup", "no auth for connection type from "+mud);
      conn->Close();
      return;
    }
    if (!data[OOB_AUTH_TYPE]) conns[mud, io][CON_FLAGS] &= ~CF_KEY;
    else {
      if (data[OOB_AUTH_KEY] != key) {
        DEBUGI3("oob_startup: wrong key for session from "+mud);
        conn->Close();
        return;
      }
      conns[mud, io][CON_FLAGS] |= CF_KEY;
    }
    conn->Write( ({ "oob-begin", // SERVICE
                    MUDNAME,     // OOB_O_MUD
                    0,
                    0
      }));

    conns[mud, io][CON_FLAGS] |= CF_START;
    return;
  }

  // Outgoing OOB connection. Other mud acknoledges our key, so we
  // could start to transmit data afterwards
  conns[mud, io][CON_FLAGS] |= CF_START;
  send_pending_data(mud);
}

static void oob_dirchange(mixed* data, string mud, object conn, int dir,
    int io, int keyed) {
  if (data[OOB_O_MUD] != mud) {
    DEBUGI3("oob_dirchange: wrong oob_o_mud '" + data[OOB_O_MUD]+
      "' for " + mud);
    conn->Close();
    return;
  }

  if (dir) {
    DEBUGI3("oob_dirchange: ignoring repeated paket");
    return;
  }
  if (io == CONOUT && !sizeof(conns[mud, io][CON_DATA])) {
    DEBUGI3("oob_dirchange: dropping connection");
    conns[mud, io][CON_SOCK]->Close();
    return;
  }
  conns[mud, io][CON_FLAGS] |= CF_DIR;
  send_pending_data(mud);
}
