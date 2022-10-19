//
// Wunderland Mudlib
//
// global/daemon/i3d.c  --  intermud3 daemon
//
// $Log: i3d.c,v $
// Revision 1.18  2003/05/02 07:29:59  Fiona
// uses AUTO_DEBUGGER() now
//
// Revision 1.17  2003/04/24 09:19:10  Fiona
// introduced MUD_TYPE
//
// Revision 1.16  2002/09/18 11:35:49  Fiona
// Fixed state reset on connection problems
//

#pragma strong_types
#pragma no_clone
#pragma no_inherit
#pragma no_shadow

inherit __DIR__ "/i3/assert";

#undef DEBUG_I3
#include <driver/time.h>
#define NEED_PROTOTYPES
#include <i3.h>
#include <socket.h>

mapping muds;
mixed*  routers;

nosave int      state;
nosave object   i3_sock, debugger;
nosave mapping  services;
nosave mapping  serv_cb;

void create() {
  string *fnames, file;

  if (state) return;

  if (AUTO_DEBUGGER(this_player())) debugger = this_player();
  log_error("create", "Start (debugger: " + to_string(debugger) +")");

# if !__EFUN_DEFINED__(send_erq)
    change_state(I3_DISABLED);
# else
    change_state(I3_STARTING);
# endif

  seteuid(getuid());
  efun::restore_object(I3_SAVEFILE);

  // collect all services we are supporting
  services = ([]);
  if (PORT_FTP)  services += ([ "ftp": PORT_FTP ]);
  if (PORT_HTTP) services += ([ "http": PORT_HTTP ]);

  serv_cb = ([]);
  fnames = get_dir(I3_TOOL("*.c"));
  foreach (file : fnames) {
    catch(I3_TOOL(file)->ServiceQuery());
  }

  ME->RegisterService("!startup", "startup-reply", #'r_startup /*'*/, 0);

  if (this_player()) call_out("start_connection", 1, 0);
  else call_out("start_connection", LOAD_DELAY, 0);
}

void RegisterService(string general, string type, closure cb, mixed* ass) {
  object ob;
  string obn;

  if (general) {
    if (general[0] != '!') services[general] = 1;
    else general = general[1..];
    DEBUG("Service: " + general);
  }
  if (!type) return;

  ob  = to_object(cb);
  obn = object_name(ob);
  if (ob != ME && ob != previous_object())
    RAISE_ERROR("privilege violation: RegisterService() (1)\n");
  if (ob != ME && strstr(obn, I3_TOOLDIR))
    RAISE_ERROR("privilege violation: RegisterService() (2)\n");

  serv_cb[type] = ({ cb, obn, ass });

  DEBUG("- registered " + type);
  if (!pointerp(ass)) DEBUG("* assertions missing for " + type);
}

static void start_connection(string prefered_router) {
  string *parts;

  if (state & I3_DISABLED) return;
  while (remove_call_out("start_connection") >= 0);

  if (!i3_sock) {
    if (!prefered_router) prefered_router = "97.107.133.86 8787"; //"150.101.219.57 8080";
//    if (!prefered_router) prefered_router = "45.64.56.66 8787"; //"150.101.219.57 8080";
//    if (!prefered_router) prefered_router = "97.107.133.86 8787"; //"150.101.219.57 8080";

    log_error("start_connection", "opening new socket ("
      + prefered_router + ")");

    parts = efun::explode(trim(prefered_router), " ");

    state &= ~(I3_SOCK_GEN | I3_STARTUP_S | I3_STARTUP_R |
               I3_MUDLIST_OK | I3_CHANLIST_OK | I3_RUNNING);
    change_state(I3_SOCK_GEN);
    i3_sock = SOCKETD->New(TCP_OPEN, parts[0], to_int(parts[1]),
      #'main_cb /*'*/, SOCKET_MUDMODE);
  }
  else log_error("start_connection", "socket still open");
}

static void main_cb(object fd, int action, mixed d1, mixed d2, mixed d3) {
  switch (action) {
    case SOCKET_READY:
      return s_startup();
    case SOCKET_READ:
      return call_service(d1);
    case SOCKET_CLOSE:
      change_state(I3_SOCK_GEN | I3_STARTUP_S | I3_STARTUP_R |
                   I3_MUDLIST_OK | I3_CHANLIST_OK | I3_RUNNING, 1);
      call_out("start_connection", RECON_DELAY, 0);
      return;
    case SOCKET_WRITTEN:
      return;
    case SOCKET_ERROR:
      log_error("main_cb", "SOCKET_ERROR: " +
        MIXED2STR(d1) + " " + MIXED2STR(d2) + " " +
        MIXED2STR(d3));
      return;
  }
  log_error("main_cb", "Unknown action: " +
    MIXED2STR(action) + " " + MIXED2STR(d1) + " " +
    MIXED2STR(d2) + " " + MIXED2STR(d3));
}

static void call_service(mixed* data) {
  string err;
  mixed* handler;

  if (!pointerp(data)) err = "data not array";
  else if (sizeof(data) < D_USER)   err = "datasize too small";
  else if (stringp(data[D_MUD]) && data[D_MUD] != MUDNAME)
    err = "wrong D_MUD";
  else if (!stringp(data[O_MUD]))   err = "O_MUD not string";
  else if (!stringp(data[SERVICE])) err = "SERVICE not string";
  else if (!(handler = serv_cb[data[SERVICE]]))
    err = "Unknown SERV "+data[SERVICE]+" (from "+data[O_MUD]+")";
  else {
    if (handler[2]) err = assert(data, handler[2]);
    if (err) {
      err = data[SERVICE] + ": " + err;
      DEBUG("assertion: " + err);
      log_error("assertion", err + " " + MIXED2STR(data));
      return;
    }
  }

  if (!err && data[O_MUD][0] == '*' && (state & I3_STARTUP_R)) {
    if (data[O_MUD] != routers[0][0]) err = "Unknown Router";
  }

  if (err) {
    DEBUG(err);
    log_error("call_service", err + " " + MIXED2STR(data));
    return;
  }

  if (!to_object(handler[0])) {
    DEBUG("Handler destroyed... reloading...");
    m_delete(serv_cb, data[SERVICE]);
    catch(call_other(handler[1], "ServiceQuery"));
    handler = serv_cb[data[SERVICE]];
    if (!handler) {
      log_error("call_service", "Fatal: " + data[SERVICE] +
        "handler unavailable.");
      DEBUG("Fatal: Handler still unavailable.");
      return;
    }
  }

  DEBUG("Handling " + data[SERVICE]+" (for "+data[O_MUD]+")");
  funcall(handler[0], data);
}

static void s_startup() {
  mixed* s_state;
  if (state & I3_STARTUP_S) return; // hmmm?
  change_state(I3_STARTUP_S);

  s_state = i3_sock->State();
  // "*gjs" is just a guess. The protokoll is very bad here, you have
  // to know the router's name prior to connecting
  routers = ({ ({ /* "*wpr" */ "*dalet" /*, "*Kelly" */, s_state[S_HOST]+" "+s_state[S_PORT] }) });

  send_i3( ({ "startup-req-3", 5, MUDNAME, 0, routers[0][0], 0,
    0,          // enter password from last run HERE if nessesary
    0,          // mudlist_id not working
    0,          // chanlist_id not working
    PORT_MUD,
    PORT_OOB,
    PORT_UDP,
    MUDSHORT+" "+(LIBVERSION_D->MudlibVersion()),
    MUDSHORT,
    "LDMud "+__VERSION__,
    MUD_TYPE,
    MUD_STATUS,
    GAMESMGR,
    services,
    (["banner": "", "url": WEBURL ])
  }));
}

void show_state() {
  DEBUG("state: "
       +(state & I3_DISABLED    ? "DISAB" : "-----")+" "
       +(state & I3_STARTING    ? "START" : "-----")+" "
       +(state & I3_SOCK_GEN    ? "S_GEN" : "-----")+" "
       +(state & I3_STARTUP_S   ? "S_SNT" : "-----")+" "
       +(state & I3_STARTUP_R   ? "S_RCV" : "-----")+" "
       +(state & I3_MUDLIST_OK  ? "MUDLI" : "-----")+" "
       +(state & I3_CHANLIST_OK ? "CHANL" : "-----")+" "
       +(state & I3_RUNNING     ? "*RUN*" : "-----")+" "
       +(state & I3_OOB_GEN     ? "OOB_G" : "-----"));
}

varargs void change_state(int bit, int unset_bit) {
  object po;

  if (extern_call() && (po = previous_object()) && po != ME) {
    if (strstr(object_name(po), I3_TOOLDIR))
      RAISE_ERROR("privilege violation: change_state()\n");
  }

  if (!unset_bit) {
    if (state & bit) DEBUG("state again set:");
    state |= bit;
  }
  else {
    if (!(state & bit)) DEBUG("state was not set:");
    state &= ~bit;
  }
  show_state();
  if (!(state & I3_RUNNING)
      && state & I3_MUDLIST_OK
      && state & I3_CHANLIST_OK) {
    change_state(I3_RUNNING);
    DEBUG("Entering running mode");
    log_error("change_state", "Entering running mode");
  }
}

public int query_state() {
  return state;
}

static void r_startup(mixed* data) {
  mixed* new_router;
  string err;
  int passwd;

  if (!pointerp(data[6]) || !sizeof(data[6]) || !pointerp(data[6][0])
    || sizeof(data[6][0])!=2)  err = "wrong router table received";
  else if (!intp(data[7])) err = "invalid password received";

  if (err) {
    DEBUG(err);
    log_error("r_startup", err + " " + MIXED2STR(data));
    return;
  }

  new_router = data[6];

  passwd = data[7];
  // The password should be memorized in order to reconnect to the router
  // the next time _if our IP address changed_. It's not needed if the mud
  // did not change the host.
  // Write it to a file where 'nobody' could read it.
  log_file("ARCH/I3_PASSWD", ctime() + " received password " + passwd + "\n");

  efun::save_object(I3_SAVEFILE);

  // Get router name, check if router changed
  if (routers[0][1] == new_router[0][1]) routers[0][0] = new_router[0][0];
  else {
    DEBUG("Router change initiated...");
    log_error("r_startup", "Router change " + MIXED2STR(new_router));
    i3_sock->Close();
    i3_sock = 0;
    change_state(I3_SOCK_GEN | I3_STARTUP_S | I3_STARTUP_R |
                 I3_MUDLIST_OK | I3_CHANLIST_OK | I3_RUNNING, 1);
    call_out("start_connection", 1, new_router[0][1]);
    return;
  }

  change_state(I3_STARTUP_R);
}

// ordered destruction
int remove() {
  if (i3_sock) {
    send_i3(({"shutdown",
              5,
              MUDNAME,
              0,
              routers[0][0],
              0,
              3*60
      }));
    i3_sock->Close();
  }
  foreach (string k, mixed val : serv_cb) {
    catch(call_other(serv_cb[k][1], "DaemonShutdown"));
  }
  DEBUG("Cleanup ready... self destruct...");
  destruct(this_object());
  return 1;
}

mixed *QueryService(string type) {
  mixed* x;
  x = serv_cb[type];
  if (!x) return 0;
  // NEVER return the closures itself, would break security
  return deep_copy(serv_cb[type][1..]);
}

// Handler's debugging
void dbg_msg(string x) {
  string obn;
  obn = object_name(previous_object());
  if (strstr(obn, I3_TOOLDIR))
    RAISE_ERROR("privilege violation: dbg_msg()\n");
  obn = regreplace(obn, ".*/", "", 0);
  DEBUG(obn + ": " + x);
}

mixed* query_router() {
  return copy(routers[0]);
}

public int send_i3(mixed data) {
  int s;
  object po;

  if (extern_call() && (po = previous_object()) && po != ME) {
    if (strstr(object_name(po), I3_TOOLDIR))
      RAISE_ERROR("privilege violation: send_i3()\n");
  }

  if (!i3_sock) {
    DEBUG("send_i3: no sock! (restarting)");
    log_error("send_i3", "no sock, restarting connection");
    change_state(I3_SOCK_GEN | I3_STARTUP_S | I3_STARTUP_R |
                 I3_MUDLIST_OK | I3_CHANLIST_OK | I3_RUNNING, 1);
    call_out("start_connection", RECON_DELAY, 0);
    return 0;
  }
  s = i3_sock->State()[S_STATE];
  if (s != S_CONNECTED) {
    DEBUG("send_i3: not connected ("+s+")");
    return -1;
  }
  i3_sock->Write(data);
  return 0;
}

void _debug(int on) {
  if (on) { debugger = this_player(); DEBUG("Ok."); }
  else    { DEBUG("Ok. Have a nice day!"); debugger = 0; }
}

void log_error(string who, string what) {
  object po;
  int *t;
  string ts;

  po = previous_object();

  if (extern_call() && po && po != ME) {
    if (strstr(object_name(po), I3_TOOLDIR))
      RAISE_ERROR("privilege violation: log_error()\n");
    who = regreplace(object_name(previous_object()), ".*/", "", 0)
      + ": " + who;
  }

  t = localtime();
  ts = sprintf("%02d.%02d.%02d, %02d:%02d: ", t[TM_MDAY], t[TM_MON] + 1,
    t[TM_YEAR] % 100, t[TM_HOUR], t[TM_MIN]);

  log_file("INTERMUD3", ts + who + ": " + what +"\n");
}
