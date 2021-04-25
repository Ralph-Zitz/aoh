// This file is part of UNItopia Mudlib (based on Wunderland Mudlib).
// ------------------------------------------------------------------
// File:        /i/player/telnet_neg.c
// Description: Telnet-Negotiations
// Author:      Fiona@Wunderland
//
// This telnet implementation is 'unconditionally compliant'
// to RFC 1143 and so follows all rules of the telnet protocol RFC 854.
// The driver itself is not quite compliant.
//
// $Log: telnet_neg.c,v $
// Revision 1.33  2019/09/22 14:12:33  myonara
// CHARSET angepasst (Gnomi)
//
// Revision 1.32  2019/04/03 20:57:30  gnomi
// CHARSET-Request korrigiert.
//
// Revision 1.31  2019/03/30 15:53:03  gnomi
// Einstellungsmenue fuer Zeichenkodierung.
//
// Revision 1.30  2019/03/29 15:50:42  gnomi
// Implementierung von TELOPT_CHARSET.
//
// Revision 1.29  2019/03/29 15:49:48  gnomi
// Anpassungen fuer neuen Driver.
//
// Revision 1.28  2018/02/10 15:56:06  myonara
// query_telnet_client erweitert
//
// Revision 1.27  2018/02/09 21:44:43  gnomi
// MXP-Unterstuetzung in eigene Datei ausgelagert.
// GMCP-Protokoll eingebaut.
//
// Revision 1.26  2018/02/08 20:36:24  gnomi
// MSSP-Unterstuetzung.
//
// Revision 1.25  2018/01/25 22:15:40  gnomi
// MXP-Unterstuetzung.
//
// Revision 1.24  2018/01/20 10:19:38  myonara
// Andere CodingPAges (Gnomi?)
//
// Revision 1.23  2017/06/17 13:09:48  myonara
// tintin++ in PING_WORKAROUND_CLIENTS ergaenzt und positiv getestet.
//
// Revision 1.22  2017/04/12 14:46:12  gnomi
// PING-Workaround auf kildclient ausgeweitet.
//
// Revision 1.21  2015/08/26 21:40:28  gnomi
// Anpassungen an LDMud 3.5.0
//
// Revision 1.20  2015/05/01 16:32:21  myonara
// query_telnet_Client um Terminalgroesse erweitert
//
// Revision 1.19  2015/03/17 11:26:56  myonara
// neue Adminfunktion query_telnet_client
//
// Revision 1.18  2014/12/23 16:04:55  gnomi
// Doppelte Modifier korrigiert.
//
// Revision 1.17  2012-11-16 17:50:14  gnomi
// Telnet-Ping beim MushClient via TType vornehmen.
//
// Revision 1.16  2009-06-17 08:08:47  gnomi
// Typo.
//
// Revision 1.15  2009-06-17 07:51:41  gnomi
// Zwei Variablen nosave gemacht.
//
// Revision 1.14  2009-04-21 11:53:36  gnomi
// Noecho-Flag merken, fuer den Fall, dass ein DO LINEMODE kommt.
//
// Revision 1.13  2009-04-15 13:38:51  gnomi
// EOR via add_message senden, damit's in einem Paket ankommt.
// (TF hat sonst Probleme damit.)
//
// Revision 1.12  2008-04-02 08:20:21  gnomi
// dumptelnegs und query_telnet static gemacht, da darueber u.U. auch sensible
// Informationen ueber den Spieler und seinen Rechner ausgegeben werden.
//
// Revision 1.11  2007/10/24 21:38:14  gnomi
// Funktion um das EOR-Prompt-Protokoll auszuschalten.
//
// Revision 1.10  2007/09/01 23:29:23  gnomi
// Prompt-Ausgabe ueber receive_message_low, Prompt auch via
// sefun::input_to empfangen.
//
// Revision 1.9  2007/08/30 20:27:08  gnomi
// set_active_prompt nur dann aufrufen, wenn EOR nicht aktiviert ist.
// Ping-Befehl eingebaut.
//
// Revision 1.8  2007/08/20 18:16:55  gnomi
// Bugfix zum vorherigen Patch.
//
// Revision 1.7  2007/08/13 20:59:44  gnomi
// NOECHO-Workaround anders implementiert und ganz aus der normalen
// Telnet-Maschine rausgenommen. sb_ttype korrigiert.
//
// Revision 1.6  2007/08/11 23:26:22  gnomi
// Workaround fuer fehlerhafte MUD-Clients, die keine Antwort auf ECHO-Requests
// geben. LINEMODE LIT_ECHO deaktiviert. MCCP-Unterstuetzung eingebaut.
//
// Revision 1.5  2007/08/11 16:09:59  gnomi
// Implementation aus dem Wunderland eingebaut.
//

#pragma no_range_check
#include <driver/input_to.h>            // INPUT_*
#include <driver/telnet.h>
#include <driver/configuration.h>
#include <secure/config.h>
#include <secure/wizlevels.h>
#include <driver/driver_info.h>
#include <driver/debug_message.h>
#include <properties.h>
#include <msgclass.h>

// ******************** Telnet State Machine ********************
//
// The following code is the implementation of the telnet state
// machine as suggested with RFC 1143.
//
// The machine is used with three functions:
//   set_telnet()    requests a change in the telnet state
//   set_callback()  sets our prefered telnet states and a callback
//                   function which is called on telnet state changes
//   query_telnet()  query state and sb info
//
// The driver communicates with the engine through the H_TELNET_NEG
// and got_telnet().
//
// Do this in logon() to turn IAC quoting on:
//   set_connection_charset(({255})*32, 1);
//
// Do this in logon() to start telnet negotiation:
//   set_telnet(WILL, TELOPT_ECHO);
//   set_telnet(WONT, TELOPT_ECHO);
// alternatively you could do the following, which is not as robust
//   set_telnet(WILL, TELOPT_EOR);

#define PO              previous_object()
// Access to mapping ts
#define TS_STATE        0 // option state (yes, no, want yes, want no, Q)
#define TS_SB           1 // sb infos (option specific)
#define TS_R_AGREE      2 // preference or decision callback (remote state)
#define TS_L_AGREE      3 // preference or decision callback (local state)
#define TS_CB           4 // option state change callback (yes, no)
#define TS_SBCB         5 // option sb callback
#define TS_SIZE         6

// Each option could be in one of the following four states. There
// is also a one element queue for each option. Mind that each option
// could have different states for the lokal and the remote system.
// Six bits of TS_STATE are used.
#define NO           0x00 // option is deactivated
#define YES          0x01 // option is activated
#define WANT_NO      0x02 // option is activated, negotiating to switch off
#define WANT_YES     0x03 // option is deactivated, negotiating to switch on
#define Q_EMPTY      0x00 // no entry in queue
#define Q_OPPOSITE   0x04 // request to toggle state in queue
#define REJECTED     0x08 // option denied, don't retry

// State and queue on the remote side (DO + DONT) (bits 0-3)
#define Q_REMOTE(x)       ((x)  &  0x03)
#define S_REMOTE(x, y)    (((x) & ~0x03) | (y))
#define Q_REMOTEQ(x)      ((x)  &  0x04)
#define S_REMOTEQ(x, y)   (((x) & ~0x04) | (y))
#define Q_REMOTER(x)      ((x)  &  0x08)
#define S_REMOTER(x, y)   (((x) & ~0x08) | (y))
// State and queue on this side (WILL + WONT) (bits 4-7)
#define Q_LOCAL(x)        (((x) &  0x30) >> 4)
#define S_LOCAL(x,y)      (((x) & ~0x30) | ((y) << 4))
#define Q_LOCALQ(x)       (((x) &  0x40) >> 4)
#define S_LOCALQ(x, y)    (((x) & ~0x40) | ((y) << 4))
#define Q_LOCALR(x)       (((x) &  0x80) >> 4)
#define S_LOCALR(x, y)    (((x) & ~0x80) | ((y) << 4))

// To have everything in one place we have one special key in ts
#define TS_EXTRA       -1 // key
#define TSE_STATE       0 // input_to's INPUT_NOECHO and/or INPUT_CHARMODE
#define TSE_TELNETNEG   1 // 1: client answered a negotiation
                          // 0: didn't try any negotation yet
			  // -1: client didn't answer
#define TSE_LOG         2 // negotiation log
#define TSE_ENCODING    3 // negotiated encoding

#define ECHO_WORKAROUND
#define CLIENT_PING_WORKAROUND

nosave mapping ts = ([:TS_SIZE]); // Complete telnet negotation state

#ifdef CLIENT_PING_WORKAROUND
#define PING_WORKAROUND_CLIENTS "mushclient", "kildclient", "tintin++"
nosave int needs_ping_workaround;
#endif

private string client_encoding; // Configured charset

protected void update_encoding();
static void ping_no_answer(object ob);

// TODO: it's not quite complete
private string telnet_to_text(int command, int option, int* args) {
  string d_txt;
  int i, j;

  d_txt = TELCMD2STRING(IAC) + " " +
    TELCMD2STRING(command) + " " + TELOPT2STRING(option);
  if (args && sizeof(args)) {
    if (command == SB && option == TELOPT_LINEMODE) {
      switch (args[0]) {
        case LM_MODE:
          if (sizeof(args) > 1) {
            d_txt += " MODE" +
              (args[1] & MODE_EDIT ? " EDIT" : " NOEDIT") +
              (args[1] & MODE_TRAPSIG ? " TRAPSIG" : "") +
              (args[1] & MODE_SOFT_TAB ? " SOFT_TAB" : "") +
              (args[1] & MODE_LIT_ECHO ? " LIT_ECHO" : "") +
              (args[1] & MODE_ACK ? " ACK" : "");
            if (sizeof(args) == 2) return d_txt;
            args = args[2..];
          }
          break;

        case WILL: case WONT: case DO: case DONT:
          d_txt += " " + TELCMD2STRING(args[0]);
          args = args[1..];
          if (sizeof(args) > 0 && args[0] == LM_FORWARDMASK) {
            d_txt += " FORWARDMASK";
            args = args[1..];
          }
          break;

        case LM_SLC:
          j = sizeof(args) - 2;
          d_txt += " SLC";
          for (i = 1; i < j; i += 3) {
            d_txt += "\n          ";
            d_txt += sprintf("%-6s %-23s %02x",
              SLC2STRING(args[i]),
               SLC_FLAGNAMES[args[i+1] & SLC_LEVELBITS] +
               (args[i+1] & SLC_FLUSHOUT ? " FOUT" : "") +
               (args[i+1] & SLC_FLUSHIN  ? " FIN"  : "") +
               (args[i+1] & SLC_ACK      ? " ACK"      : ""),
               args[i+2]);
          }
          if (i > j) return d_txt;
          args = args[i-3..]; // dump rest (is error)
          break;

      }

    }
    if (command == SB && option == TELOPT_CHARSET) {
      d_txt += " " + CS_FLAGNAME(args[0]);
      if (sizeof(args) > 1) d_txt += " (" +
        implode(map(args[1..], (: sprintf("%02x", $1) :)), ",") + ")";
    } else
    if (command == SB && option != TELOPT_NAWS && option != TELOPT_LINEMODE) {
      d_txt += " " + TELQUAL2STRING(args[0]);
      if (sizeof(args) > 1) d_txt += " (" +
        implode(map(args[1..], (: sprintf("%02x", $1) :)), ",") + ")";
    }
    else if (sizeof(args)) d_txt +=
      " (" + implode(map(args, (: sprintf("%02x", $1) :)), ",") + ")";
  }
  return d_txt;
}

// All telnet negotations are sent through this function
private int send(int* x) {
  string log;
  int *y, i, j;

  if (x[1] != SB) y = x[3..];
  else {
    y = x[3..<3];
    j = sizeof(y) - 1;
    for (i = 0; i < j; ++i) { // undo 0xff quoting
      if (y[i] == IAC && y[i+1] == 0xff) {
        y[i..i+1] = ({ 0xff });
        --j;
      }
    }
  }

  log = ts[TS_EXTRA, TSE_LOG];
  if (sizeof(log) > 4000) log = "..." + log[<3800..];
  log += "sent " + telnet_to_text(x[1], x[2], y) +"\n";
  ts[TS_EXTRA, TSE_LOG] = log;

  return efun::binary_message(x);
}

/*
FUNKTION: set_callback
DEKLARATION: protected void set_callback(int opt, mixed r_a_cb, mixed l_a_cb, closure change_cb, closure sb_cb)
BESCHREIBUNG:
Diese Funktion setzt die Praeferenzen und Callbacks fuer bestimmte Telnet-Optionen.

Parameter:
    int opt            Die Telnet-Option (eine der TELOPT-Defines).

    mixed r_a_cb       Das ist der Wunsch, wie wir den Kommunikationspartner
                       gern haetten. Entweder DO, DONT oder eine
		       Closure, die aufgerufen wird, wenn wir eine
		       entsprechende Anfrage erhalten. Die Closure
		       erhaelt zwei Parameter, der erste ist der
		       Zustand (WILL, WONT, DO, DONT) und die zweite
		       ist die Telnet-Option, und sollte 0 liefern,
		       wenn es nicht unterstuetzt wird (also ein
		       WONT oder DONT verlangt wird), 1 anderenfalls.

    mixed l_a_cb       Das ist der Zustand, den wir selbst gern haetten.
                       Entweder WILL, WONT oder eine Closure.

    closure change_cb  Diese Closure wird bei Aenderung dieser Option
                       lokal oder entfernt aufgerufen. Die Closure
		       erhaelt zwei Parameter, der erste ist der
		       Zustand (WILL, WONT, DO, DONT) und die zweite
		       ist die Telnet-Option.

    closure sb_cb      Diese Closure wird fuer Subnegotiations (SB TELOPT)
                       aufgerufen, sie erhaelt als ersten Parameter das SB,
		       als zweiten die Telnet-Option und als dritten
		       Parameter ein Byte-Array mit den zusaetzlichen Daten.

GRUPPEN: telnet
*/
protected void set_callback(int opt, mixed r_a_cb, mixed l_a_cb,
    closure change_cb, closure sb_cb) {

  if (r_a_cb == DO) r_a_cb = 1;
  else if (r_a_cb == DONT) r_a_cb = 0;
  if (l_a_cb == WILL) l_a_cb = 1;
  else if (l_a_cb == WONT) l_a_cb = 0;

  ts[opt, TS_R_AGREE] = r_a_cb;
  ts[opt, TS_L_AGREE] = l_a_cb;
  ts[opt, TS_CB] = change_cb;
  ts[opt, TS_SBCB] = sb_cb;
}

/*
FUNKTION: set_telnet
DEKLARATION: nomask static int set_telnet(int command, int option)
BESCHREIBUNG:
Diese Funktion verlangt einen bestimmten Telnetzustand.
'command' ist entweder DO, DONT (fuer die Gegenseite)
                  oder WILL, WONT (fuer unseren Zustand),
'option' eine der Telnet-Optionen (TELOPT-Defines).

Die Funktion liefert 0, wenn keine Aenderung stattfindet (entweder
wir sind bereits in diesem Zustand oder wir haben eine entsprechende
Anforderung schon gesendet oder unsere Anforderung wurde schon einmal
abgelehnt), 1 anderenfalls.
GRUPPEN: telnet
*/
nomask static int set_telnet(int command, int option) {
  int state, ok;

  state = ts[option, TS_STATE];

#ifdef ECHO_WORKAROUND
  // Workaround fuer TELOPT_ECHO, da einige
  // MUD-Clients (Mushclient z.B.) es nicht bestaetigen.
  // Wir muessen es daher einfach immer senden.
  if(option == TELOPT_ECHO)
  {
    // Immer senden.
    if(Q_LOCAL(state) != (command==WILL?WANT_YES:WANT_NO))
      send(({IAC, command, option}));
    state = S_LOCAL(state, command==WILL?WANT_YES:WANT_NO);
    ok = 1;
  }
  else
#endif
  switch (command) {
    case DO:
      if (Q_REMOTER(state) == REJECTED) break;
      switch (Q_REMOTE(state)) {
        case YES:
          break; // ignore, it's already enabled
        case NO:
          state = S_REMOTE(state, WANT_YES);
          send(({ IAC, DO, option }));
          ok = 1;
          break;
        case WANT_NO:
          if (Q_REMOTEQ(state) == Q_EMPTY) {
            state = S_REMOTEQ(state, Q_OPPOSITE);
            ok = 1;
          } else {
	    // ignore, request sent already
          }
          break;
        case WANT_YES:
          if (Q_REMOTEQ(state) == Q_EMPTY) {
	    // ignore, request sent already
          } else {
            state = S_REMOTEQ(state, Q_EMPTY);
            ok = 1;
          }
          break;
      }
      break;

    case DONT:
      switch (Q_REMOTE(state)) {
        case NO:
          break; // ignore, it's already disabled
        case YES:
          state = S_REMOTE(state, WANT_NO);
          send(({ IAC, DONT, option }));
          ok = 1;
          break;
        case WANT_NO:
          if (Q_REMOTEQ(state) == Q_EMPTY) {
	    // ignore, request sent already
          } else {
            state = S_REMOTEQ(state, Q_EMPTY);
            ok = 1;
          }
          break;
        case WANT_YES:
          if (Q_REMOTEQ(state) == Q_EMPTY) {
            state = S_REMOTEQ(state, Q_OPPOSITE);
            ok = 1;
          } else {
	    // ignore, request sent already
          }
          break;
      }
      break;

    case WILL:
      if (Q_LOCALR(state) == REJECTED) break;
      switch (Q_LOCAL(state)) {
        case NO:
          state = S_LOCAL(state, WANT_YES);
          send(({ IAC, WILL, option }));
          ok = 1;
          break;
        case YES:
          break; // ignore, it's already enabled
        case WANT_NO:
          if (Q_LOCALQ(state) == Q_EMPTY) {
            state = S_LOCALQ(state, Q_OPPOSITE);
            ok = 1;
          } else {
	    // ignore, request sent already
          }
          break;
        case WANT_YES:
          if (Q_LOCALQ(state) == Q_EMPTY) {
	    // ignore, request sent already
          } else {
              state = S_LOCALQ(state, Q_EMPTY);
            ok = 1;
          }
          break;
      }
      break;

    case WONT:
      switch (Q_LOCAL(state)) {
        case NO:
          break; // ignore, it's already disabled
        case YES:
          state = S_LOCAL(state, WANT_NO);
          send(({ IAC, WONT, option }));
          ok = 1;
          break;
        case WANT_NO:
          if (Q_LOCALQ(state) == Q_EMPTY) {
	    // ignore, request sent already
          } else {
              state = S_LOCALQ(state, Q_EMPTY);
            ok = 1;
          }
          break;
        case WANT_YES:
          if (Q_LOCALQ(state) == Q_EMPTY) {
            state = S_LOCALQ(state, Q_OPPOSITE);
            ok = 1;
          } else {
	    // ignore, request sent already
          }
          break;
      }
      break;
  }
  ts[option, TS_STATE] = state;
  return ok;
}

// Request information on an option. sb has to be called by reference.
static int query_telnet(int option, mixed sb) {
  if (!intp(option) || option < 0 || !member(ts, option)) {
    sb = 0;
    return 0;
  }
  sb = copy(ts[option, TS_SB]);
  return ts[option, TS_STATE];
}

static int has_telnet_option(int option, int remote)
{
    return remote ? (Q_REMOTE(ts[option, TS_STATE]) == YES) : (Q_LOCAL(ts[option, TS_STATE]) == YES);
}

// Telnet protocoll violations end up here
private void tel_error(string err) {
  string log;

  log = ts[TS_EXTRA, TSE_LOG];
  if (sizeof(log) > 4000) log = "..." + log[<3800..];
  log += "ERR " + err + "\n";
  ts[TS_EXTRA, TSE_LOG] = log;
}

// Full negotiation tries to set all our preferences
// Options with closures as preference are not actively enabled.
private void start_telnetsrv();
private void start_telnetneg() {
  int opt, *options;

  ts[TS_EXTRA, TSE_TELNETNEG] = 1;

  options = m_indices(ts) - ({ TS_EXTRA });
  foreach (opt : options) {
    if (ts[opt, TS_R_AGREE] == 1) set_telnet(DO, opt);
    if (ts[opt, TS_L_AGREE] == 1) set_telnet(WILL, opt);
  }

  start_telnetsrv();
}

// Got telnet negotations from the driver
void got_telnet(int command, int option, int *optargs) {
  int state;
  mixed agree;
  string log;

  if (getuid(PO) != ROOTID) return;

  log = ts[TS_EXTRA, TSE_LOG];
  if (sizeof(log) > 4000) log = "..." + log[<3800..];
  log += "got  " + telnet_to_text(command, option, optargs) + "\n";
  ts[TS_EXTRA, TSE_LOG] = log;

  state = ts[option, TS_STATE];
#ifdef ECHO_WORKAROUND
  // Workaround fuer ECHO wegen buggy MUD-Clients
  if(option == TELOPT_ECHO)
  {
    // Wir machen nix.
    funcall(ts[option, TS_CB], command, option);
  }
  else
#endif
  switch (command) {
    case WILL:
      switch (Q_REMOTE(state)) {
        case NO:
          agree = ts[option, TS_R_AGREE];
          if (closurep(agree)) agree = funcall(agree, command, option);
          if (agree) {
            state = S_REMOTE(state, YES);
            ts[option, TS_STATE] = state;
            send(({ IAC, DO, option }));
            agree = ts[option, TS_CB];
            if (closurep(agree)) funcall(agree, command, option);
          } else {
            send(({ IAC, DONT, option }));
          }
          break;
        case YES:
          break; // ignore
        case WANT_NO:
          if (Q_REMOTEQ(state) == Q_EMPTY) {
            tel_error("DONT answered by WILL");
            state = S_REMOTE(state, NO);
          } else {
            tel_error("DONT answered by WILL");
            state = S_REMOTE(state, YES);
            state = S_REMOTEQ(state, Q_EMPTY);
          }
          ts[option, TS_STATE] = state;
          break;
        case WANT_YES:
          state = S_REMOTE(state, YES);
          ts[option, TS_STATE] = state;
          agree = ts[option, TS_CB];
          if (closurep(agree)) funcall(agree, command, option);
          if (Q_REMOTEQ(state) != Q_EMPTY) {
            state = S_REMOTE(state, WANT_NO);
            state = S_REMOTEQ(state, Q_EMPTY);
            ts[option, TS_STATE] = state;
            send(({ IAC, DONT, option }));
          }
          break;
      }
      break;

    case WONT:
      switch (Q_REMOTE(state)) {
        case NO:
          break; // ignore
        case YES:
          state = S_REMOTE(state, NO);
          ts[option, TS_STATE] = state;
          send(({ IAC, DONT, option }));
          agree = ts[option, TS_CB];
          if (closurep(agree)) agree = funcall(agree, command, option);
          break;
        case WANT_NO:
          state = S_REMOTE(state, NO);
          ts[option, TS_STATE] = state;
          agree = ts[option, TS_CB];
          if (closurep(agree)) funcall(agree, command, option);
          if (Q_REMOTEQ(state) != Q_EMPTY) {
            state = S_REMOTE(state, WANT_YES);
            state = S_REMOTEQ(state, Q_EMPTY);
            ts[option, TS_STATE] = state;
            send(({ IAC, DO, option }));
          }
          break;
        case WANT_YES:
          if (option != TELOPT_TM)
            // *sigh* TM is not as all other options are
	          state = S_REMOTER(state, REJECTED);
          state = S_REMOTE(state, NO);
          state = S_REMOTEQ(state, Q_EMPTY);
          ts[option, TS_STATE] = state;
          agree = ts[option, TS_CB];
          if (closurep(agree)) funcall(agree, command, option);
          break;
      }
      break;

    case DO:
      switch (Q_LOCAL(state)) {
        case NO:
          agree = ts[option, TS_L_AGREE];
          if (closurep(agree)) agree = funcall(agree, command, option);
          if (agree) {
            state = S_LOCAL(state, YES);
            ts[option, TS_STATE] = state;
            send(({ IAC, WILL, option }));
            agree = ts[option, TS_CB];
            if (closurep(agree)) funcall(agree, command, option);
          } else {
            send(({ IAC, WONT, option }));
          }
          break;
        case YES:
          break; // ignore
        case WANT_NO:
          if (Q_LOCALQ(state) == Q_EMPTY) {
            tel_error("WONT answered by DO");
            state = S_LOCAL(state, NO);
          } else {
            tel_error("WONT answered by DO");
            state = S_LOCAL(state, YES);
            state = S_LOCALQ(state, Q_EMPTY);
          }
          ts[option, TS_STATE] = state;
          break;
        case WANT_YES:
          state = S_LOCAL(state, YES);
          ts[option, TS_STATE] = state;
          agree = ts[option, TS_CB];
          if (closurep(agree)) funcall(agree, command, option);
          if (Q_LOCALQ(state) != Q_EMPTY) {
            state = S_LOCAL(state, WANT_NO);
            state = S_LOCALQ(state, Q_EMPTY);
            ts[option, TS_STATE] = state;
	    send(({ IAC, WONT, option }));
          }
          break;
      }
      break;

    case DONT:
      switch (Q_LOCAL(state)) {
        case NO:
          break; // ignore
        case YES:
          state = S_LOCAL(state, NO);
          ts[option, TS_STATE] = state;
          send(({ IAC, WONT, option }));
          agree = ts[option, TS_CB];
          if (closurep(agree)) agree = funcall(agree, command, option);
          break;
        case WANT_NO:
          state = S_LOCAL(state, NO);
          ts[option, TS_STATE] = state;
          agree = ts[option, TS_CB];
          if (closurep(agree)) funcall(agree, command, option);
          if (Q_LOCALQ(state) != Q_EMPTY) {
            state = S_LOCAL(state, WANT_YES);
            state = S_LOCALQ(state, Q_EMPTY);
            ts[option, TS_STATE] = state;
            send(({ IAC, WILL, option }));
          }
          break;
        case WANT_YES:
          state = S_LOCAL(state, NO);
          state = S_LOCALQ(state, Q_EMPTY);
          state = S_LOCALR(state, REJECTED);
          ts[option, TS_STATE] = state;
          agree = ts[option, TS_CB];
          if (closurep(agree)) agree = funcall(agree, command, option);
          break;
      }
      break;

  case SB:
    agree = ts[option, TS_SBCB];
    if (closurep(agree)) funcall(agree, command, option, optargs);
    break;

  }

  // Do full negotation only if client can telnetneg, means we got some
  // telnetneg message from it. If we never got one we assume it can't
  // do it. The negotiation is triggered by one single telnetneg request
  // in the login process.
  if (ts[TS_EXTRA, TSE_TELNETNEG]<=0) start_telnetneg();
}

// ******************** End Telnet State Machine ********************

// This function is used to transfer the telnet state engine's state
// from one object to an other, used for player renew and so on
//
// The transfer has to be done before the exec(E), so that both objects
// work with the same copy of ts
mapping transfer_ts(mapping old_ts) {
  int opt;
  if (!previous_object() || getuid(previous_object()) != ROOTID) return 0;
  if (!old_ts) return ts;

  // use callbacks of THIS object
  foreach (opt : ts) {
    if (opt < 0) continue;
    old_ts[opt, TS_R_AGREE] = ts[opt, TS_R_AGREE];
    old_ts[opt, TS_L_AGREE] = ts[opt, TS_L_AGREE];
    old_ts[opt, TS_CB]      = ts[opt, TS_CB];
    old_ts[opt, TS_SBCB]    = ts[opt, TS_SBCB];
  }

  ts = old_ts;

#ifdef CLIENT_PING_WORKAROUND
  if(member(ts, TELOPT_TTYPE) &&
     sizeof(ts[TELOPT_TTYPE, TS_SB])>1 &&
     sizeof(ts[TELOPT_TTYPE, TS_SB][1]) &&
     member(([ PING_WORKAROUND_CLIENTS ]), ts[TELOPT_TTYPE, TS_SB][1][0]))
       needs_ping_workaround = 1;
#endif

  return 0;
}

// ******************** Telnet Server Engine ********************
//
// The following code is the telnet server using the telnet state machine.
// It has to follow the some basic rules that cannot be assured by the
// state machine to be complient to RFC 854. The most important is to
// enable only options which are fully implemented.
//
// State changes are requested by the driver with the H_NOECHO hook
// and set_noecho().
//
// Here is defined how the mud will act in the negotiation.

#define NOECHO_AFTER_CHARMODE
#undef GET_ALL_TTYPES

nosave int* tm_t;
nosave int pending_noecho;
nosave int last_noecho;

// Bits used for the charmode and noecho state of the connection
// Bits 0 + 1 used for TSE_NOECHO (set for noecho mode)
// Bits 2 + 3 used for TSE_SGA_CHAR (set for charmode using SGA)
// Bits 4 + 5 used for TSE_LM_CHAR (set for charmode using LINEMODE)
// each representing the state with NO, YES, WANT_NO and WANT_YES
#define Q_TSE_NOECHO      ((ts[TS_EXTRA, TSE_STATE])  &  0x03)
#define S_TSE_NOECHO(y)   (ts[TS_EXTRA, TSE_STATE] = \
                            ((ts[TS_EXTRA, TSE_STATE]) & ~0x03) | (y))
#define Q_TSE_SGA_CHAR    (((ts[TS_EXTRA, TSE_STATE]) &  0x0c) >> 2)
#define S_TSE_SGA_CHAR(y) (ts[TS_EXTRA, TSE_STATE] = \
                            ((ts[TS_EXTRA, TSE_STATE]) & ~0x0c) | ((y) << 2))
#define Q_TSE_LM_CHAR     (((ts[TS_EXTRA, TSE_STATE]) &  0x30) >> 4)
#define S_TSE_LM_CHAR(y)  (ts[TS_EXTRA, TSE_STATE] = \
                            ((ts[TS_EXTRA, TSE_STATE]) & ~0x30) | ((y) << 4))

void send_telopt_tm();

// Change the NOECHO and CHARMODE state, called indirectly thru input_to()
void set_noecho(int flag) {
  // Security check needs H_NOECHO to be specified as follows,
  // with lfun in the master:
  //
  //   set_driver_hook(H_NOECHO, #'noecho_hook);
  //
  //   void noecho_hook(int flag, int ob) {
  //     if (ob) ob->set_noecho(flag);
  //   }
  int i, j;

  // Called by start_telnetsrv or the driver hook.
  if (extern_call() && object_name(previous_object()) != __MASTER_OBJECT__)
    return;

  if (!ts[TS_EXTRA, TSE_TELNETNEG])
  {
    // Try one.
    ts[TS_EXTRA, TSE_TELNETNEG] = -1;
    send_telopt_tm();
    pending_noecho = flag;
    return;
  }
  else if(ts[TS_EXTRA, TSE_TELNETNEG]<0)
  {
    pending_noecho = flag;
    return;
  }

  flag &= 255;
  last_noecho = flag;

  i = Q_TSE_NOECHO;

  // Check if a change in NOECHO is requested
  if (flag & INPUT_NOECHO) {
    if (i == NO || i == WANT_NO) {
      S_TSE_NOECHO(WANT_YES);
      // If CHARMODE and NOECHO are requested (as usual), enter NOECHO
      // only if we could enter CHARMODE, else the player would be blind.
#ifdef NOECHO_AFTER_CHARMODE
      if (!(flag & INPUT_CHARMODE)) {
        ts[TS_EXTRA, TSE_LOG] += "* trying to get NOECHO\n";
        set_telnet(WILL, TELOPT_ECHO);
      }
      else
        ts[TS_EXTRA, TSE_LOG] += "* waiting for CHARMODE to activate NOECHO\n";
#else
      ts[TS_EXTRA, TSE_LOG] += "* trying to get NOECHO\n";
      set_telnet(WILL, TELOPT_ECHO);
#endif
    }
  } else {
    if (i == YES || i == WANT_YES) {
      ts[TS_EXTRA, TSE_LOG] += "* trying to leave NOECHO\n";
      S_TSE_NOECHO(WANT_NO);
      set_telnet(WONT, TELOPT_ECHO);
    }
  }

  i = Q_TSE_SGA_CHAR;
  j = Q_TSE_LM_CHAR;

  // Check if a change in CHARMODE is requested
  // When the client knows LINEMODE, use it. If not use old (sga) charmode.
  if (flag & INPUT_CHARMODE) {
    if (i != YES && j != YES && i != WANT_YES && j != WANT_YES) {
      ts[TS_EXTRA, TSE_LOG] += "* trying to get CHARMODE\n";

      if (Q_REMOTE(ts[TELOPT_LINEMODE, TS_STATE] == YES)) {
        S_TSE_LM_CHAR(WANT_YES);
        send(({ IAC, SB, TELOPT_LINEMODE, LM_MODE,
          MODE_SOFT_TAB, IAC, SE }));
      } else {
        S_TSE_SGA_CHAR(WANT_YES);
        set_telnet(WILL, TELOPT_SGA);
        set_telnet(DO, TELOPT_SGA);
      }
    }
  } else {
    if (i == YES || j == YES || i == WANT_YES || j == WANT_YES) {
      ts[TS_EXTRA, TSE_LOG] += "* trying to leave CHARMODE\n";
      // check both flags independently
      if (j == YES || j == WANT_YES) {
        S_TSE_LM_CHAR(WANT_NO);
        send(({ IAC, SB, TELOPT_LINEMODE, LM_MODE,
          MODE_EDIT | MODE_SOFT_TAB, IAC, SE }));
      }
      if (i == YES || i == WANT_YES) {
        S_TSE_SGA_CHAR(WANT_NO);
        set_telnet(WONT, TELOPT_SGA);
        set_telnet(DONT, TELOPT_SGA);
      }
    }
  }
}

private void start_telnetsrv()
{
  ts[TS_EXTRA, TSE_LOG] += "* Start Telnet Server\n";

   if(pending_noecho)
   {
  ts[TS_EXTRA, TSE_LOG] += "* Pending\n";
     set_noecho(pending_noecho);
     pending_noecho = 0;
   }
}

void print_eor() {
  int state = Q_LOCAL(ts[TELOPT_EOR, TS_STATE]);
  if (state == YES || state == WANT_NO)
	  efun::binary_message(({ IAC, EOR }), 1);
}

// H_PRINT_PROMPT driver hook.
void print_prompt(mixed str)
{
  int state;

  if (extern_call() && object_name(previous_object()) != __MASTER_OBJECT__
    && this_player() != this_object())
    return;
  efun::tell_object( this_object(), str);

  state = Q_LOCAL(ts[TELOPT_EOR, TS_STATE]);
  if (state == YES || state == WANT_NO)
	  efun::binary_message(({ IAC, EOR }), 1);
  else if(sizeof(str) && str[<1]!='\n')
    efun::tell_object( this_object(), "\n");

}

protected void set_eor_protokoll(int act)
{
  int state = Q_LOCAL(ts[TELOPT_EOR, TS_STATE]);

  if((state == YES || state == WANT_NO) == !act)
	  set_telnet(act?WILL:WONT, TELOPT_EOR);
}

//
// The callback functions
//

// neg_*() are called to decide if we say 'no' or 'yes' to an incoming
// option change request

private int neg_sga(int command, int option) {
  // Remote SGA   is allowed for old charmode.
  // Local SGA    is allowed for noecho mode (due to the SGA/ECHO ambiguity,
  //              see RFC 857). It is also allowed if we are in old charmode
  //              because some clients think it's needed.
  int i;

  if (command == DO) {
    i = Q_TSE_NOECHO;
    if (i == WANT_YES || i == YES) return 1;
  }

  // else WILL
  i = Q_TSE_SGA_CHAR;
  if (i == WANT_YES || i == YES) return 1;
  return 0;
}

private int neg_echo(int command, int option) {
  // Remote ECHO  is never allowed (why should we like to get our own text)
  // Local ECHO   is allowed for noecho mode

  // only called for DO
  int i;
  i = Q_TSE_NOECHO;
  if (i == WANT_YES || i == YES) return 1;
  return 0;
}

private int neg_bin(int command, int option) {
  // We like to have it enabled, but we don't want to have it enabled
  // with start_telnetneg().

  // BINARY actively enabled only it the TTYPE is "xterm".

  // Some clients cant even answer a BINARY request properly (they have
  // to embed a '\0' into the stream). The RFC says, that it is needed
  // for 8 bit data path. Most clients just use this anyway and send
  // for example german umlauts. Strictly it's forbidden, though. Some
  // clients (BSD telnet for example) do honor this state and need
  // BINARY to send umlauts.
  return 1;
}

private int neg_tm(int command, int option) {
  int *t, i;

  // reset state immediately
  if (command == DO || command == DONT) {
    i = ts[option, TS_STATE];
    i = S_LOCAL(i, NO);
    i = S_REMOTE(i, NO);
    i = S_REMOTER(i, 0);
    ts[option, TS_STATE] = i;
  }

  if (command == WILL || command == WONT) {
    i = ts[option, TS_STATE];
    i = S_LOCAL(i, NO);
    i = S_REMOTE(i, NO);
    i = S_REMOTER(i, 0);
    ts[option, TS_STATE] = i;

    // got an answer
    if(tm_t)
    {
      t = utime();
      i = (t[0]-tm_t[0])*1000 + (t[1]-tm_t[1])/1000;
      tm_t = 0;
      ts[TS_EXTRA, TSE_LOG] += "* Ping answer: "+i+"ms.\n";

      // i -= 11; // interne Verzoegerung (muss man messen!)
      if (i < 1) i = 1;
      while (remove_call_out(#'ping_no_answer /*'*/) != -1);
      tell_object(this_object(), "Nostradamus tells you: "
      "Data needs "+i+" ms from you to the mud and back.\n");
    }
  }
  return 1;
}

// start_*() and cb_*() are called on/after succesfull option changes
// We do the appropriate things then.

private void start_sb(int command, int option) {
  // If we got a WILL, request subnegotiation
  if (command == WILL) send(({ IAC, SB, option, TELQUAL_SEND, IAC, SE }));
}

static void start_cs(int command, int option)
{
  // The client requests our charset list.
  if (command == DO && !pointerp(ts[option, TS_SB]))
  {
    send(({ IAC, SB, TELOPT_CHARSET, CS_REQUEST }) + (to_array(" UTF-8 ISO-8859-15 ISO-8859-1 windows-1252 US-ASCII") - ({ 0 })) + ({ IAC, SE }));
    ts[option, TS_SB] = 1; // Indicate, that we have sent our list.
  }
}

private void start_lm(int command, int option) {
  if (command != WILL) return;
  if (!ts[option, TS_SB]) ts[option, TS_SB] = allocate(LM_SLC+1);

  send(({ IAC, SB, TELOPT_LINEMODE, LM_MODE,
    ((last_noecho & INPUT_CHARMODE)?0:MODE_EDIT) | MODE_SOFT_TAB, IAC, SE }));
  ts[option, TS_SB][LM_MODE] = MODE_EDIT | MODE_SOFT_TAB;

  // Flush on every control character
  // remember to prepend 0xff with IACs
  send(({ IAC, SB, TELOPT_LINEMODE, DO,
    LM_FORWARDMASK, IAC, 0xff, IAC, 0xff, IAC, 0xff, IAC, 0xff, IAC, SE }));
  ts[option, TS_SB][LM_FORWARDMASK] = WANT_YES;

}

private void start_eor(int command, int option) {
  // If we are allowed to use EOR whilst displaying a possible prompt
  // line, mark is as such. This happens usually only at the login.
  if (command == DO) efun::binary_message(({ IAC, EOR }));
}

private int neg_mccp(int command, int option)
{
  if(command == DO)
	  return 1;
  return 0;
}

#ifdef __MCCP__
private void start_mccp(int command, int option)
{
  if(command == DO)
#if __EFUN_DEFINED__(start_mccp_compress)
	  start_mccp_compress(option);
#else
	  configure_interactive(this_object(), IC_MCCP, option);
#endif /* __EFUN_DEFINED__(start_mccp_compress) */
  else if(command == DONT)
  {
#if __EFUN_DEFINED__(end_mccp_compress)
	  if(query_mccp())
	    end_mccp_compress();
#else
	  if(interactive_info(this_object(), IC_MCCP))
	    configure_interactive(this_object(), IC_MCCP, 0);
#endif /* __EFUN_DEFINED__(end_mccp_compress) */
	  else if(option == TELOPT_COMPRESS2)
	    set_telnet(WILL, TELOPT_COMPRESS);
  }
}
#endif /* __MCCP__ */

#ifdef __TLS__
protected void tls_finished()
{
    // We do nothing here, but it may be overriden to check the certificate.
}

private void tls_callback(int error, object me)
{
    if (!error)
        tls_finished();
}

private void sb_tls(int command, int option, int* optargs)
{
    if(optargs[0] != TELQUAL_SEND)
        return;

    if(Q_REMOTE(ts[TELOPT_STARTTLS, TS_STATE] == YES))
    {
        call_out(function void() {
            send(({ IAC, SB, option, TELQUAL_SEND, IAC, SE }));
            tls_init_connection(this_object(), #'tls_callback /*'*/);
        }, 0);
    }
    else if(tls_query_connection_state())
    {
        call_out(function void() {
            send(({ IAC, SB, option, TELQUAL_SEND, IAC, SE }));
            tls_deinit_connection();
        }, 0);
    }
}

private void start_auth(int command, int option)
{
    if (command == WILL)
        send(({ IAC, SB, option, AUTH_SB_SEND,
                    AUTH_METHOD_SSL, AUTH_WHO_CLIENT_TO_SERVER|AUTH_HOW_ONE_WAY,
                IAC, SE }));
}

private void sb_auth(int command, int option, int* optargs)
{
    switch (optargs[0])
    {
        case AUTH_SB_IS:
            if (Q_REMOTE(ts[option, TS_STATE]) != YES)
                return;

            if (optargs[1] != AUTH_METHOD_SSL ||
                optargs[2] != AUTH_WHO_CLIENT_TO_SERVER|AUTH_HOW_ONE_WAY ||
                optargs[3] != AUTH_SSL_START)
            {
                set_telnet(DONT, option);
                return;
            }

            call_out(function void()
            {
                send(({ IAC, SB, option, AUTH_SB_REPLY,
                            AUTH_METHOD_SSL, AUTH_WHO_CLIENT_TO_SERVER|AUTH_HOW_ONE_WAY,
                            AUTH_SSL_ACCEPT,
                        IAC, SE }));
                tls_init_connection(this_object(), #'tls_callback /*'*/);
            }, 0);
            break;
    }
}
#endif

private void cb_sga(int command, int option) {
  switch (command) {
    case WILL:
      if (Q_TSE_SGA_CHAR == WANT_YES) {
        S_TSE_SGA_CHAR(YES);
        ts[TS_EXTRA, TSE_LOG] += "* CHARMODE established\n";
#ifdef NOECHO_AFTER_CHARMODE
        if (Q_TSE_NOECHO == WANT_YES) {
          ts[TS_EXTRA, TSE_LOG] += "* trying to get NOECHO\n";
          set_telnet(WILL, TELOPT_ECHO);
        }
#endif
      }
      break;
    case WONT:
      switch (Q_TSE_SGA_CHAR) {
        case WANT_NO:
          ts[TS_EXTRA, TSE_LOG] += "* CHARMODE left\n";
          S_TSE_SGA_CHAR(NO);
	  break;
        case WANT_YES:
          ts[TS_EXTRA, TSE_LOG] += "* CHARMODE denied\n";
	  break;
        case YES:
          ts[TS_EXTRA, TSE_LOG] += "* CHARMODE forcefully left\n";
          S_TSE_SGA_CHAR(WANT_YES);
	  break;
      }
      break;
  }
}

private void cb_echo(int command, int option) {
  switch (command) {
    case DO:
      if (Q_TSE_NOECHO == WANT_YES) {
        S_TSE_NOECHO(YES);
        ts[TS_EXTRA, TSE_LOG] += "* NOECHO established\n";
      }
      break;
    case DONT:
      switch (Q_TSE_NOECHO) {
        case WANT_NO:
          ts[TS_EXTRA, TSE_LOG] += "* NOECHO mode left\n";
          S_TSE_NOECHO(NO);
	  break;
        case WANT_YES:
          ts[TS_EXTRA, TSE_LOG] += "* NOECHO denied\n";
	  break;
        case YES:
          ts[TS_EXTRA, TSE_LOG] += "* NOECHO forcefully left\n";
          S_TSE_NOECHO(WANT_YES);
	  break;
      }
      break;
  }
}

// sb_*() handle the different incoming subnegotiations

private void sb_status(int command, int option, int* optargs) {
  int *ret, opt, state, s, end;

  if (sizeof(optargs) != 1) return;
  if (optargs[0] != TELQUAL_SEND) return;

  ret = ({});
  foreach (opt : ts) {
    if (opt < 0) continue;
    state = ts[opt, TS_STATE];
    s = Q_LOCAL(state);
    if (s == YES || s == WANT_NO) ret += ({ WILL, opt });
    s = Q_REMOTE(state);
    if (s == YES || s == WANT_NO) ret += ({ DO, opt });
  }

  // some driver include the string end '\0'
  end = sizeof(to_array("")) + 1;

  if (ts[TELOPT_TSPEED, TS_SB])
    ret += ({ SB, TELOPT_TSPEED, TELQUAL_IS }) +
	    to_array(ts[TELOPT_TSPEED, TS_SB][0]+","+
	    ts[TELOPT_TSPEED, TS_SB][1])[0..<end] + ({ -SE });

  if (ts[TELOPT_XDISPLOC, TS_SB])
    ret += ({ SB, TELOPT_XDISPLOC, TELQUAL_IS }) +
	    to_array(ts[TELOPT_XDISPLOC, TS_SB])[0..<end] + ({ -SE });

  if (ts[TELOPT_TTYPE, TS_SB])
    ret += ({ SB, TELOPT_TTYPE, TELQUAL_IS }) +
	    to_array(ts[TELOPT_TTYPE, TS_SB][1][0])[0..<end] + ({ -SE });

  if (ts[TELOPT_NAWS, TS_SB])
    ret += ({ SB, TELOPT_NAWS, ts[TELOPT_NAWS, TS_SB][0]/0x100,
		    ts[TELOPT_NAWS, TS_SB][0]%0x100,
		    ts[TELOPT_NAWS, TS_SB][1]/0x100,
		    ts[TELOPT_NAWS, TS_SB][1]%0x100, -SE });

  if (ts[TELOPT_LINEMODE, TS_SB]) {
    ret += ({ SB, TELOPT_LINEMODE, LM_MODE,
            ts[TELOPT_LINEMODE, TS_SB][LM_MODE], -SE });
    if (ts[TELOPT_LINEMODE, TS_SB][LM_FORWARDMASK] == YES)
      ret += ({ SB, TELOPT_LINEMODE, DO, LM_FORWARDMASK, 0, 0x24, -SE });
    if (sizeof(ts[TELOPT_LINEMODE, TS_SB][LM_SLC])) {
      ret += ({ SB, TELOPT_LINEMODE, LM_SLC });
      foreach (int i, int j, int k : ts[TELOPT_LINEMODE, TS_SB][LM_SLC])
        ret += ({ i, j, k });
      ret += ({ -SE });
    }
  }

  // We dont send SB ENVIRON or SB NEWENV

  // quote IAC and SE
  // we use -SE for unquotable SEs
  for (s = sizeof(ret); s--;) {
    if (ret[s] == IAC) ret[s..s] = ({ IAC, IAC });
    if (ret[s] ==  SE) ret[s..s] = ({ SE, SE });
    if (ret[s] == -SE) ret[s]    = SE;
  }

  send(({ IAC, SB, option, TELQUAL_IS }) + ret + ({ IAC, SE }));

}

private void sb_naws(int command, int option, int* optargs) {
  int cols, lines, *old;

  if (sizeof(optargs) != 4) return tel_error("SB NAWS with wrong size");

  cols  = optargs[1] + optargs[0] * 256;
  lines = optargs[3] + optargs[2] * 256;
  old = ts[option, TS_SB];
  ts[option, TS_SB] = ({ cols, lines });

  ts[TS_EXTRA, TSE_LOG] +=
    "     Window size: " + cols + " cols, " + lines + " lines\n";

  if(!pointerp(old) || old[0] != cols || old[1] != lines) {
    this_object()->receive(sprintf("window_size height:%d x width:%d\n", lines, cols),
                           CMSG_GENERIC|MMSG_DIRECT);
    this_object()->Set(P_PAGEWIDTH, cols);
    this_object()->Set(P_PAGESIZE, lines-2);
  }
//     this_object()->notify("window_size", this_object(), cols, lines);
}

private void sb_xdisp(int command, int option, int* optargs) {
  string value;
  int len;

  if (sizeof(optargs) < 2) return;
  if (optargs[0] != TELQUAL_IS) return;
  value = to_string(filter(optargs[1..], (: $1 >= ' ' && $1 <= '~' :)));
  len = sizeof(value);

  if (sizeof(value) != sizeof(optargs)-1)
    tel_error("SB XDISPLOC contained bad characters");
  if (!len)
    return tel_error("SB XDISPLOC empty, ignoring");

  ts[option, TS_SB] = value;
  ts[TS_EXTRA, TSE_LOG] += "     XDISPLOC is " + value + "\n";
}

private void sb_ttype(int command, int option, int* optargs) {
#ifdef GET_ALL_TTYPES
  int len, i;
#else
  int len;
#endif
  string value;
  mixed* all;

  if (sizeof(optargs) < 2) return;
  if (optargs[0] != TELQUAL_IS) return;
  value = to_string(filter(optargs[1..], (: $1 >= ' ' && $1 <= '~' :)));
  len = sizeof(value);

  if (sizeof(value) != sizeof(optargs)-1)
    tel_error("SB TTYPE contained bad characters");
  if (!len)
    return tel_error("SB TTYPE empty, ignoring");
  value = lower_case(value);
  ts[TS_EXTRA, TSE_LOG] += "     TTYPE is " + value + "\n";

  all = ts[option, TS_SB];
  if (!all) {
    all = ({ 0, ({}) });
    // inform mudlib
//    SetProp(P_TTY_TYPE, value);
  }

#ifdef GET_ALL_TTYPES
  // Clients may provide different terminal types which are synonyms to
  // the native one (RFC 930). It is also possible to have a client which
  // switches terminal emulation along with the given (non synonymous)
  // terminal type names (RFC 1091). All these types are collected if
  // GET_ALL_TTYPES is defined.
  //
  // The drawback is, that the cycling thru all terminal emulations takes
  // some time, especially on slow links. MS windows 2000's and XP's
  // telnet have an emulation with key on-off sequences rather that chars.
  // Commands typed while TTYPE is negotiated may be wrong with that clients.

  i = member(all[1], value);
  if (i < 0) {
    all[1] += ({ value });
    // get next TTYPE
    if (Q_REMOTE(ts[option, TS_STATE]) == YES) start_sb(WILL, option);
  } else {
    // switch back to first
    if (i == 0) {
      // ok, initial type re-established
      ts[TS_EXTRA, TSE_LOG] += "     TTYPE in client set\n";
      return;
    }
    switch (++all[0]) {
      case 1:
        if (Q_REMOTE(ts[option, TS_STATE]) == YES) start_sb(WILL, option);
        break; // just retry
      case 2:
        tel_error("could not get initial TTYPE, may be rfc930 client");
        if (value != "vtnt") break;
        tel_error("trying trick to restart TTYPE list on windows client");
        set_telnet(DONT, TELOPT_TTYPE);
        set_telnet(DO, TELOPT_TTYPE);
        break;
      case 3:
        tel_error("giving up on TTYPE");
        break;
    }
  }
#else
  if (member(all[1], value) < 0)
    all[1] += ({ value });
#endif

  ts[option, TS_SB] = all;

  // Some clients cannot answer BINARY correctly. Xterm should be
  // able to. Also most clients ignore this option and just use a 8 bit
  // data path; only BSD telnet is known to need BINARY in order to
  // send umlauts.
  if (lower_case(value) == "xterm" || lower_case(value) == "xterm-256color")
    set_telnet(DO, TELOPT_BINARY);

#ifdef CLIENT_PING_WORKAROUND
  if(needs_ping_workaround && tm_t)
  {
    int *t = utime();
    int  d = (t[0]-tm_t[0])*1000 + (t[1]-tm_t[1])/1000;

    tm_t = 0;
    ts[TS_EXTRA, TSE_LOG] += "* Ping (via TTYPE) answer: "+d+"ms.\n";

    if(d < 1) d = 1;
    tell_object(this_object(), "Nostradamus tells you: "
      "Data needs "+d+" ms from you to the mud and back.\n");
//    this_object()->notify("ping", this_object(), d);
  }
  else if(member(([PING_WORKAROUND_CLIENTS]), value))
    needs_ping_workaround = 1;
#endif

}

private void sb_tspeed(int command, int option, int* optargs) {
  int i, j, os;

  os = sizeof(optargs);
  if (os < 2) return;
  if (optargs[0] != TELQUAL_IS) return;

  for (i = 1; i < os; ++i) {
    if (optargs[i]==',') break;
    if (optargs[i] < '0' || optargs[i] > '9')
      return tel_error("SB TSPEED invalid character");
  }

  if (i == 1) return tel_error("SB TSPEED empty transmit speed");
  if (i == os) return tel_error("SB TSPEED no comma found");

  for (j = i+1; j < os; ++j) {
    if (optargs[j] < '0' || optargs[j] > '9')
      return tel_error("SB TSPEED invalid character");
  }

  if (i+1 >= os) return tel_error("SB TSPEED empty receive speed");

  ts[option, TS_SB] = ({ to_int(to_string(optargs[1..i-1])),
                         to_int(to_string(optargs[i+1..])) });

  ts[TS_EXTRA, TSE_LOG] +=
    "     Transmit speed " + ts[option, TS_SB][0] +
    ", receive speed " + ts[option, TS_SB][1] + "\n";

}

static void sb_charset(int command, int option, int* optargs)
{
    debug_message("sb_charset\n");
    if (!sizeof(optargs))
        return;

    switch (optargs[0])
    {
        case CS_REQUEST:
        {
            int pos = 1;
            string *charsets, *icharsets;

            if (ts[option, TS_SB] == 1)
            {
                // If we have sent our list, we must reject now.
                send(({ IAC, SB, TELOPT_CHARSET, CS_REJECTED, IAC, SE }));
            }

            // We got a list from the server.
            if (optargs[1] == '[')
            {
                pos = member(optargs, ']');
                if (pos < 0)
                {
                    send(({ IAC, SB, TELOPT_CHARSET, CS_REJECTED, IAC, SE }));
                    break;
                }

                pos += 2;
            }

#if __VERSION__ > "3.5.2"
            charsets = explode(to_bytes(optargs[pos+1..]), to_bytes(optargs[pos..pos])) - ({ to_bytes(({})) });
            icharsets = map(charsets, (: lower_case(to_text($1, "ascii")) :));
#else
            charsets = explode(to_string(optargs[pos+1..]), to_string(optargs[pos..pos])) - ({ "" });
            icharsets = map(charsets, (: lower_case($1) :));
#endif
            ts[option, TS_SB] = charsets;

            // We prefer UTF-8
            if ((pos = member(icharsets, "utf-8")) >= 0)
            {
                ts[TS_EXTRA, TSE_ENCODING] = "UTF-8";
                send(({ IAC, SB, TELOPT_CHARSET, CS_ACCEPTED }) + to_array(charsets[pos]) + ({ IAC, SE }));
            }
            else if (sizeof(icharsets))
            {
                ts[TS_EXTRA, TSE_ENCODING] = icharsets[0];
                send(({ IAC, SB, TELOPT_CHARSET, CS_ACCEPTED }) + to_array(charsets[0]) + ({ IAC, SE }));
            }
            else
            {
                ts[TS_EXTRA, TSE_ENCODING] = 0;

                send(({ IAC, SB, TELOPT_CHARSET, CS_REJECTED, IAC, SE }));
                update_encoding();
                break;
            }

            update_encoding();
            ts[TS_EXTRA, TSE_LOG] += "     Charset: " + ts[TS_EXTRA, TSE_ENCODING] + "\n";

            break;
        }

        case CS_ACCEPTED:
            ts[TS_EXTRA, TSE_ENCODING] = to_string(optargs[1..]);
            update_encoding();
            ts[TS_EXTRA, TSE_LOG] += "     Charset: " + to_string(optargs[1..])+ "\n";
            break;
    }
}

private void sb_env(int command, int option, int* optargs) {
  int i, j, os;
  mixed mix;
  mapping env;
  string s;

  os = sizeof(optargs);
  if (os < 2) return;
  if (optargs[0] != TELQUAL_IS && optargs[0] != TELQUAL_INFO) return;

  mix = ({});
  for (i = j = 1; j < os; ++j) {
    if (optargs[j] == ENV_ESC) {
      ++j;
      continue;
    }
    if (optargs[j] <= ENV_USERVAR) {
      if (i != j) {
        s = to_string(filter(optargs[i..j-1], #'>= /*'*/, ' '));
        if (sizeof(s) != j-i)
          tel_error("SB ENV contained bad characters");
        mix += ({ s });
      }
      mix += ({ optargs[j] });
      i = j + 1;
      continue;
    }
  }
  if (i < j-1) {
    s = to_string(filter(optargs[i..j-1], #'>= /*'*/, ' '));
    if (sizeof(s) != j-i)
      tel_error("SB ENV contained bad characters");
    mix += ({ s });
  }

  // Due to some weird implementation mistakes (see RFC 1571)
  // we can't be shure if VALUE or VAR are valid or swapped.
  // So we don't distuingush VAR and USERVAR and assume that every
  // variable's name is followed by its value.

  s = 0;
  env = ([]);
  os = sizeof(mix);
  for (i = 0; i < os; ++i) {
    if (!s && stringp(mix[i])) s = mix[i];
    else if (s) {
      if (stringp(mix[i])) {
        env[s] = mix[i];
        s = 0;
      } else if (!stringp(mix[i-1])) {
        env[s] = 1; // defined but no value assigned
        s = 0;
      }
    }
  }

  ts[option, TS_SB] = env;

  foreach (s, mix : env) {
    ts[TS_EXTRA, TSE_LOG] += "     Environment " + s +" => " +
      (stringp(mix) ? mix : "defined") + "\n";
  }

}

private void sb_line(int command, int option, int* optargs) {
  // Maybe LINEMODE should be turned of on errors, but then... I never saw any.
  int state, *xx, i, j;

  if (!ts[option, TS_SB]) ts[option, TS_SB] = allocate(LM_SLC+1);

  if (sizeof(optargs) < 2) return;
  switch (optargs[0]) {
    case LM_MODE:
      if (!(optargs[1] & MODE_ACK))
        return tel_error("SB LINEMODE MODE with no ACK set (ignoring)");

      if (optargs[1] & MODE_EDIT) switch (Q_TSE_LM_CHAR) {
        case YES:
          return tel_error("SB LINEMODE MODE EDIT unexpected");
        case WANT_YES:
          return tel_error("SB LINEMODE MODE EDIT invalid answer");
        case WANT_NO:
          S_TSE_LM_CHAR(NO);
          ts[TS_EXTRA, TSE_LOG] += "* CHARMODE left\n";
          break;
      }
      else switch (Q_TSE_LM_CHAR) {
        case NO:
          return tel_error("SB LINEMODE MODE EDIT unexpected");
        case WANT_NO:
          return tel_error("SB LINEMODE MODE EDIT invalid answer");
        case WANT_YES:
          S_TSE_LM_CHAR(YES);
          ts[TS_EXTRA, TSE_LOG] += "* CHARMODE established\n";
#ifdef NOECHO_AFTER_CHARMODE
          if (Q_TSE_NOECHO == WANT_YES) {
            ts[TS_EXTRA, TSE_LOG] += "* trying to get NOECHO\n";
            set_telnet(WILL, TELOPT_ECHO);
          }
#endif
          break;
      }
      ts[option, TS_SB][LM_MODE] = optargs[1] & ~MODE_ACK;

      break;

    // server does not use forwardmask
    case DO:
    case DONT:
      if (optargs[1] != LM_FORWARDMASK) {
        return tel_error("SB LINEMODE " + TELCMD2STRING(optargs[0]) +
          sprintf(" %02x unknown (ignored)", optargs[1]));
      }
      return tel_error("SB LINEMODE DO/DONT FORWARDMASK not "
        "allowed for client");
      return;
    // client is requested to use forwardmask
    case WILL:
      if (optargs[1] != LM_FORWARDMASK) {
        return tel_error("SB LINEMODE " + TELCMD2STRING(optargs[0]) +
          sprintf(" %02x unknown (ignored)", optargs[1]));
      }
      state = ts[option, TS_SB][LM_FORWARDMASK];
      switch (state) {
        case NO:
          tel_error("got spontaneous WILL FORWARDMASK (denied)");
          break;
        case YES:
          // is already set, ignore
          break;
        case WANT_NO:
          tel_error("got WILL on DONT");
          break;
        case WANT_YES:
          ts[option, TS_SB][LM_FORWARDMASK] = YES;
          break;
      }
      return;
    case WONT:
      if (optargs[1] != LM_FORWARDMASK) {
        return tel_error("SB LINEMODE " + TELCMD2STRING(optargs[0]) +
          sprintf(" %02x unknown (ignored)", optargs[1]));
      }
      state = ts[option, TS_SB][LM_FORWARDMASK];
      switch (state) {
        case YES:
          send(({ IAC, SB, option, DONT, LM_FORWARDMASK, IAC, SE }));
          ts[option, TS_SB][LM_FORWARDMASK] = NO;
          break;
        case NO:
          // is already set, ignore
          break;
        case WANT_YES:
        case WANT_NO:
          ts[option, TS_SB][LM_FORWARDMASK] = NO;
          break;
      }
      return;

    case LM_SLC:
        if (sizeof(optargs) < 4) return tel_error("SB too short");
        if (!ts[option, TS_SB][LM_SLC]) ts[option, TS_SB][LM_SLC] = ([ :2 ]);
        xx = ({ IAC, SB, TELOPT_LINEMODE, LM_SLC });

        // Request for our preferences?
        // All the rest (especially the cursor movement) we would like
        // to be escape sequences, so no SLCs for that
        if (optargs[1] == 0 && optargs[2] == SLC_DEFAULT && optargs[3] == 0) {
          for (i = SLC_SYNCH; i <= SLC_SUSP; ++i)
            xx += ({ i, SLC_NOSUPPORT, 0 });
          xx += ({ SLC_EC,    SLC_DEFAULT,   127,
                   SLC_EL,    SLC_DEFAULT,    21,
                   SLC_EW,    SLC_DEFAULT,    23,
                   SLC_XON,   SLC_NOSUPPORT,   0,
                   SLC_XOFF,  SLC_NOSUPPORT,   0,
                   SLC_FORW1, SLC_NOSUPPORT,   0,
                   SLC_FORW2, SLC_NOSUPPORT,   0, IAC, SE });
          send(xx);
          break;
        }

        // Answer to client's preferences
        // We dont want any of the session control keys (Synch - Susp)
        // Everything else is just acknoledged
        optargs = optargs[1..];
        j = sizeof(optargs) - 3;
        for (i = 0; i < j; i+=3) {
          if (optargs[i+1] & SLC_ACK) {
            m_add(ts[option, TS_SB][LM_SLC], optargs[i],
              optargs[i+1] & ~SLC_ACK, optargs[i+2]);
            continue;
          }
          if (optargs[i+1] == SLC_NOSUPPORT) continue;
          if (optargs[i] > SLC_SUSP) {
            m_add(ts[option, TS_SB][LM_SLC], optargs[i],
              optargs[i+1] & ~SLC_ACK, optargs[i+2]);
            xx += ({ optargs[i], optargs[i+1] | SLC_ACK, optargs[i+2] });
          } else
            xx += ({ optargs[i], SLC_NOSUPPORT, 0 });
        }
        i = sizeof(xx);
        if (i > 4) {
          for (; --i;) if (xx[i] == 0xff) xx[i..i] = ({ IAC, 0xff });
          send(xx + ({ IAC, SE }));
        }

        return;
  }
}

private void start_mxp(int command, int option)
{
    if (command != WILL)
        return;

    ts[TS_EXTRA, TSE_LOG] += "* Sending MXP initialization\n";
    send(({ IAC, SB, TELOPT_MXP, IAC, SE }));
    efun::binary_message(to_array("\e[7z")); // Lock to locked mode for now.

    this_object()->init_mxp();
}

private void start_gmcp(int command, int option)
{
    if (command != DO)
        return;

    this_object()->init_gmcp();
}

private void sb_gmcp(int command, int option, int* optargs)
{
    this_object()->receive_gmcp(to_string(optargs));
}

#define MSSP_VAR 1
#define MSSP_VAL 2
private void send_mssp_var(string name, string value)
{
    efun::binary_message(({ MSSP_VAR }), 1);
    efun::binary_message(to_array(name), 1);
    efun::binary_message(({ MSSP_VAL }), 1);
    efun::binary_message(to_array(value), 1);
}

private void start_mssp(int command, int option)
{
    if (command != DO)
        return;

    efun::binary_message(({IAC, SB, TELOPT_MSSP}), 1);

    // Required
    send_mssp_var("NAME", MUDNAME);
    send_mssp_var("PLAYERS", to_string(sizeof(users())));
    send_mssp_var("UPTIME", to_string(__BOOT_TIME__));

    // Optional
    send_mssp_var("HOSTNAME", "aoh.zitz.dk");

    int *ports = driver_info(DI_MUD_PORTS);
    efun::binary_message(({ MSSP_VAR }), 1);
    efun::binary_message(to_array("PORT"), 1);
    foreach (int port: ports[1..] + ports[0..0])
    {
        efun::binary_message(({ MSSP_VAL }), 1);
        efun::binary_message(to_array(to_string(port)), 1);
    }

    if (sizeof(GAMESMGR))
      send_mssp_var("CONTACT", GAMESMGR);
    send_mssp_var("LANGUAGE", "English");
    send_mssp_var("CODEBASE", "OSB");
    send_mssp_var("FAMILY", "LPMud");
    send_mssp_var("INTERMUD", "I2");
    send_mssp_var("INTERMUD", "I3");
    send_mssp_var("CREATED", "2020");
    send_mssp_var("LOCATION", "DK");
    send_mssp_var("GENRE", "Fantasy");
    send_mssp_var("SUBGENRE", "Medieval Fantasy");
    send_mssp_var("GAMEPLAY", "Adventure");
    send_mssp_var("STATUS", "Alpha");
    send_mssp_var("GAMESYSTEM", "Custom");
    send_mssp_var("ANSI", "1");
    send_mssp_var("MCCP", "1");
    send_mssp_var("VT100", "1");
    send_mssp_var("XTERM 256 COLORS", "1");
    send_mssp_var("GMCP", "1");
    send_mssp_var("MXP", "1");
    send_mssp_var("PAY TO PLAY", "0");
    send_mssp_var("PAY FOR PERKS", "0");
    send_mssp_var("HIRING BUILDERS", "1");
    send_mssp_var("HIRING CODERS", "1");

    efun::binary_message(({IAC, SE}), 1);
}

void create() {
  ts[TS_EXTRA, TSE_LOG] = "";

  // set how we would like the options' states
  set_callback(TELOPT_NAWS,      DO,               WONT,             0,                 #'sb_naws    /*'*/);
  set_callback(TELOPT_STATUS,    DONT,             WILL,             0,                 #'sb_status  /*'*/);
  set_callback(TELOPT_TTYPE,     DO,               WONT,             #'start_sb /*'*/,  #'sb_ttype)  /*'*/;
  set_callback(TELOPT_TSPEED,    DO,               WONT,             #'start_sb /*'*/,  #'sb_tspeed  /*'*/);
  set_callback(TELOPT_NEWENV,    DO,               WONT,             #'start_sb /*'*/,  #'sb_env     /*'*/);
  set_callback(TELOPT_ENVIRON,   DO,               WONT,             #'start_sb /*'*/,  #'sb_env     /*'*/);
  set_callback(TELOPT_XDISPLOC,  DO,               WONT,             #'start_sb /*'*/,  #'sb_xdisp   /*'*/);
  set_callback(TELOPT_CHARSET,   DO,               WILL,             #'start_cs /*'*/,  #'sb_charset /*'*/);

  set_callback(TELOPT_EOR,       DONT,             WILL,             #'start_eor /*'*/, 0);
  set_callback(TELOPT_LINEMODE,  DO,               WONT,             #'start_lm /*'*/,  #'sb_line /*'*/);
  set_callback(TELOPT_TM ,       #'neg_tm /*'*/,   #'neg_tm /*'*/,   #'neg_tm /*'*/,    0);
  set_callback(TELOPT_BINARY,    #'neg_bin /*'*/,  #'neg_bin /*'*/,  0,                 0);

  set_callback(TELOPT_SGA,       #'neg_sga /*'*/,  #'neg_sga /*'*/,  #'cb_sga /*'*/,    0);
  set_callback(TELOPT_ECHO,      #'neg_echo /*'*/, WONT,             #'cb_echo /*'*/,   0);
#ifdef __MCCP__
  set_callback(TELOPT_COMPRESS2, DONT,             WILL,             #'start_mccp /*'*/, 0);
  set_callback(TELOPT_COMPRESS,  DONT,             #'neg_mccp /*'*/, #'start_mccp /*'*/, 0);
#endif
#ifdef __TLS__
  if (tls_available()) {
    set_callback(TELOPT_STARTTLS,       DO,        WONT,             0,                  #'sb_tls /*'*/);
    set_callback(TELOPT_AUTHENTICATION, DO,        WONT,             #'start_auth /*'*/, #'sb_auth /*'*/);
  }
#endif
  set_callback(TELOPT_MSSP,      DONT,             WILL,             #'start_mssp /*'*/, 0);
  set_callback(TELOPT_MXP,       DO,               WONT,             #'start_mxp /*'*/,  0);
#if __EFUN_DEFINED__(json_serialize)
  set_callback(TELOPT_GMCP,      DONT,             WILL,             #'start_gmcp /*'*/, #'sb_gmcp /*'*/);
#endif
}

// helper functions, called from mudlib
void send_telopt_tm() {
  if(extern_call())
    tm_t = utime();

#ifdef CLIENT_PING_WORKAROUND
  if(needs_ping_workaround)
    start_sb(WILL, TELOPT_TTYPE);
#endif
  set_telnet(DO, TELOPT_TM);
}

int _dumptelnegs() {
  printf(ts[TS_EXTRA, TSE_LOG]);
  return 1;
}

//#include <parse_com.h>
//#include <message.h>
//#include <level.h>

public mapping GetAllTS() { return ts; }

public string query_telnet_client(int flag_size, int flag_prot)
{
    mixed client;
    mixed sb;
    string size,prot;
    if (!flag_prot)
    {
        prot = "";
    }
    else
    {
        prot = ({int})this_object()->has_mxp()?" MXP":"";
        prot+= ({int})this_object()->has_gmcp()?" GMCP":"";
    }
    if (!flag_size)
    {
        size = "";
    }
    else if (query_telnet(TELOPT_NAWS, &sb) && sizeof(sb)==2)
    {
        size = sprintf(" %dx%d",sb[0],sb[1]);
    }
    else
    {
        size = " ?x?";
    }
    if(query_telnet(TELOPT_TTYPE, &client) && sizeof(client)>1
            && sizeof(client[1]) && stringp(client[1][0]))
        return client[1][0]+size+prot;
    return "Unknown"+size+prot;
}

string query_client_encoding()
{
    if (client_encoding)
        return client_encoding;
    if (ts[TS_EXTRA, TSE_ENCODING])
        return ts[TS_EXTRA, TSE_ENCODING];
    return "ASCII";
}

static string query_set_client_encoding()
{
    return client_encoding;
}

static int set_client_encoding(string enc)
{
    client_encoding = enc;
    update_encoding();
    return 1;
}

protected void update_encoding()
{
#if __VERSION__ > "3.5.2"
    string enc = query_client_encoding();
    if (lower_case(enc)[0..2] != "utf")
        enc = enc + "//TRANSLIT";

    configure_interactive(this_object(), IC_ENCODING, enc);
#endif
}

/*
FUNKTION: has_unicode
DEKLARATION: int has_unicode()
BESCHREIBUNG:
Liefert 1 zurueck, wenn der Spieler in der Lage ist,
Unicode-Zeichen ohne Konvertierung auszugeben.
GRUPPEN: spieler
*/
int has_unicode()
{
    return lower_case(query_client_encoding())[0..2] == "utf";
}

static void ping_answer(string controller, object ob, int ms)
{
  while (remove_call_out(#'ping_no_answer /*'*/) != -1);
#if 0
    ob->delete_controller("notify_ping", #'ping_answer /*'*/);
    this_object()->send_message_to(this_object(),
	MT_NOTIFY, MA_UNKNOWN,
	sprintf("%sPing-Response time%s: %d ms.\n",
	    (ob==this_object())?"Yours ":"",
	    (ob!=this_object())?" for "+ob->query_real_cap_name():"",
      ms));
#endif
}

static void ping_no_answer(object ob)
{
  msg_write(CMSG_GENERIC,
    sprintf("No ping response from %s within 5 seconds.\n",
    (ob != this_object()) ? capitalize(({string})ob->QueryRealName()) : "you"
  ));
}

nomask int ping_command(string str)
{
  object ob;
  str = trim(str||"");
  if (!sizeof(str) || !IS_WIZARD(this_player()))
    ob = this_player();
  else
  {
    ob = find_player(lower_case(str));
    if (!ob)
      return notify_fail(str + " not found.\n");
  }
  call_out(#'ping_no_answer /*'*/, 5, ob);
  ob->send_telopt_tm();
  return 1;
}

protected void add_actions()
{
    add_action("ping_command", "ping");
    add_action("_dumptelnegs", "telneg");
    add_action("set_client_encoding", "encoding");
}
