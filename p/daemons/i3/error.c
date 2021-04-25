//
// Wunderland Mudlib
//
// global/daemon/i3/error.c  --  Error handler
//
// $Log: error.c,v $
// Revision 1.7  2003/06/19 11:32:21  Fiona
// Pass chan-user errors to ucache module
//

#pragma strong_types
#pragma no_clone
#pragma no_inherit
#pragma no_shadow

inherit __DIR__ "assert";

//#include <defines.h>

#undef DEBUG_I3
#include <i3.h>
#include <string.h>

static void handle(mixed* data, string mud, object conn, int dir, int io);

/////
// Main handling functions

public void ServiceQuery() {
  I3->RegisterService("!error", "error", #'handle, ASSERT_ERROR);
}

public void OOBService() {
  I3_TOOL("oob")->RegisterOOB("error", #'handle, ASSERT_ERROR);
}

public void DaemonShutdown() {
  // nothing here
}

// Additional Arguments (mud, ...) used for OOB only
static void handle(mixed* data, string mud, object conn, int dir, int io) {
  string err, target, *strs;
  object pl;
  mixed* ass;

  if (data[D_USER]) pl = find_player(data[D_USER]);
  if (pl && !interactive(pl)) pl = 0;

  if (data[ERR_DATA]) {
    if (!sizeof(data[ERR_DATA])) err = "datasize too small";
    else if (!stringp(data[ERR_DATA][SERVICE])) err = "SERVICE not string";
    else {
      if (mud) ass = I3_TOOL("oob")->QueryService(data[ERR_DATA][SERVICE]);
      else     ass = I3->QueryService(data[ERR_DATA][SERVICE]);

      if (!ass) err = "Unknown SERV "+data[SERVICE];
      if (ass[1]) err = assert(data[ERR_DATA], ass[1]);
    }
    if (err) {
      err = "ERR_DATA: " + err;
      DEBUGI3(err);
      LOGI3("assertion", err + " " + MIXED2STR(data[ERR_DATA]));
      data[ERR_DATA] = 0;
    }
  }

  switch (data[ERR_TYPE]) {
    // Router errors
    case "unk-dst":     err = "Unknown destination";      break;
    case "not-imp":     err = "Service not implemented";  break;
    case "unk-type":    err = "Unknown packet type";      break;
    case "unk-src":     err = "Unknown source";           break;
    case "bad-pkt":     err = "Bad packet format";        break;
    case "bad-proto":   err = "Protocol error";           break;
    case "not-allowed": err = "Action not allowed";       break;

    // this is undocumented but sent by the router (Karm@Nightfall)
    case "bad-name":    err = "Mud name or password wrong"; break;

    // Only the following errors may be sent by muds.
    // Some muds do send the others also, though.
    // case "bad-pkt":
    // case "unk-type":

    case "unk-channel":
      err = "Unknown channel";
      break;

    case "unk-user":
      if (pl && data[ERR_DATA]) {

        if (data[ERR_DATA][SERVICE] == "finger-req")
          target = data[ERR_DATA][F_USER];

        else if (data[ERR_DATA][SERVICE] == "mail") {
          strs = data[ERR_DATA][MAIL_TO][MUDNAME];
          strs = filter(strs, #'stringp);
          strs = map(strs, (: "'"+$1+"'" :));
          target = L_STRING->countup(strs, 1)[1..<2];
        }

        // general targetted request failed (not for oob)
        else if (!mud && sizeof(data[ERR_DATA]) > D_USER)
          target = data[ERR_DATA][D_USER];
      }

      if (stringp(target) && sizeof(target))
        err = "Player '" + capitalize(target) + "' absent or unknown";
      else err = "Player unknown";
      break;

    default:
      err = "Unknown error";
      break;
  }

  if (pl) {
    data[ERR_DESCR] = FILTER_TXT(data[ERR_DESCR], 1);
    MSG_TO_PLAYER(pl, data[O_MUD] + ": "+err+".\n" + "("+data[ERR_DESCR]+")");
    return;
  }

  if (data[ERR_DATA] && data[ERR_DATA][SERVICE] == "chan-user-req") {
    if (I3_TOOL("ucache")->got_error(data, err)) return;
  }

  DEBUGI3(err);
  LOGI3(data[ERR_TYPE], err + " " + MIXED2STR(data));
}
