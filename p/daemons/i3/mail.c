//
// Wunderland Mudlib
//
// global/daemon/i3/mail.c  --  The OOB mail service
//
// $Log: mail.c,v $
// Revision 1.5  2002/09/16 13:08:36  Fiona
// Error in bounce-code fixed (Gawain@Regenbogen)
//
// Revision 1.4  2002/08/06 17:36:24  Fiona
// bounce mails on error
//

#pragma strong_types
#pragma no_clone
#pragma no_inherit
#pragma no_shadow

#undef DEBUG_I3
#include <i3.h>
#include <daemons.h>
#include <secure/udp_mail.h>
#include <string.h>

mixed* mails;

static void mail_in (mixed* d, string m, object c, int r, int io, int k);
static void mail_ack(mixed* d, string m, object c, int r, int io, int k);
static string check_mail_types(mixed* data);
static void mail_cb(int id, string err);
static int getid();

public void ServiceQuery() {
  I3->RegisterService("mail", 0, 0, 0);
}

public void OOBService() {
  I3_TOOL("oob")->RegisterOOB("mail",           #'mail_in,  ASSERT_MAIL);
  I3_TOOL("oob")->RegisterOOB("mail-ack",       #'mail_ack, ASSERT_MAIL_ACK);
}

//////
// Services

static void mail_in(mixed* data, string mud, object conn, int dir,
    int io, int k) {
  string *recips, rec, sender, *fails, err;
  object r;

  if (io) DEBUGI3("mail_in: Warning: Got mail req while in output state");

  err = check_mail_types(data);
  if (err) {
    DEBUGI3("mail_in: " + err);
    // Sending error packet... should we?
    I3_TOOL("oob")->send_oob_reply(mud, io, ({
        "error",           // SERVICE
        5,                 // HOPS
        MUDNAME,           // O_MUD
        0,                 // O_USER
        mud,               // D_MUD
        data[MAIL_O_USER], // D_USER
        "bad-pkt",         // ERR_TYPE
        err,               // ERR_DESCR
        data               // ERR_DATA
      }));
    return;
  }

  recips  = data[MAIL_TO][MUDNAME] || ({});
  recips += data[MAIL_CC][MUDNAME] || ({});
  recips += data[MAIL_BCC];
  recips  = filter(recips, #'stringp);
  recips  = map(recips, #'lower_case);

  sender = FILTER_TXT(capitalize(data[MAIL_O_USER]) + "@" +
    capitalize(mud), 1);
  data[MAIL_SUBJECT] = FILTER_TXT(data[MAIL_SUBJECT], 1);
  if (!k) data[MAIL_CONTENT] =
    "[Mud "+mud+" hat sich nicht authentifiziert]\n\n" + data[MAIL_CONTENT];
  fails = ({});

  foreach (rec : recips) {
    if (!LOCAL_MAILER->query_recipient_ok(rec)) {
      fails += ({ rec });
      continue;
    }
    // We don't distinguish sending and receiving time
    LOCAL_MAILER->deliver_mail(rec, sender,
      data[MAIL_SUBJECT], data[MAIL_CONTENT]);

    r = find_player(rec);
    if (r) tell_object(r, "Du hast neue Post!\n");
  }

  I3_TOOL("oob")->send_oob_reply(mud, io, ({
      "mail-ack",                // SERVICE
      ([ data[MAIL_ID]: fails ]) // MAIL_ACK
    }));
}

static void mail_ack(mixed* data, string mud, object conn, int dir,
    int io, int k) {
  int i, n, id, all;
  string *fails, ok, err, *oks;
  object pl;

  foreach (id, fails : data[MAIL_ACK]) {
    err = 0;
    if (!intp(id)) err = "mail_ack: id not number (ignoring)";
    else if (!pointerp(fails)) err = "mail_ack: fails not array (ignoring)";
    else if (sizeof(filter(fails, (: !stringp($1) :))))
      err = "mail_ack: fails entry not string (ignoring)";

    if (err) {
      DEBUGI3(err);
      continue;
    }

    for (n = sizeof(mails); i < n; ++i)
      if (id == mails[i][1][MAIL_ID]) break;

    if (i == n) {
      DEBUGI3("mail_ack: id not found");
      return;
    }
    if (mud != mails[i][0]) {
      DEBUGI3("mail_ack: wrong mud for this id");
      return;
    }


    pl = find_player(mails[i][1][MAIL_O_USER]);

    // Some muds just put the failed username in the fails array, some
    // also put a reason into it. Lima (which has by far the most muds
    // with mail service) returns something like:
    // ({ "'qwert123' is not a user on Lima Bean." })

    oks   = mails[i][1][MAIL_TO][mud] || ({});
    oks  += mails[i][1][MAIL_CC][mud] || ({});
    oks  += mails[i][1][MAIL_BCC];
    oks   = map(oks, #'lower_case);
    all   = sizeof(oks);

    fails = map(fails, #'lower_case);

    ok  = "Mailer@" + mud + ":";

    if (sizeof(fails) == all) {
      // That's allways the case because we send only to one user at a time
      fails -= oks;
      oks = map(oks, (: "'"+capitalize($1)+"'" :));
      err = " Zustellung an " + L_STRING->countup(oks) + " erfolglos.";
      if (sizeof(fails)) err += " (" + implode(fails, " ") + ")";
    }

    else {
      oks -= fails;

      // works only if mud put usernames in the array
      if (sizeof(oks) + sizeof(fails) == all) {
        if (sizeof(oks)) {
          oks = map(oks, (: "'"+capitalize($1)+"'" :));
          ok += " Zustellung an " + L_STRING->countup(oks) + " erfolgreich.";
        }
        if (sizeof(fails)) {
          oks = map(fails, (: "'"+capitalize($1)+"'" :));
          err = " Zustellung an " + L_STRING->countup(fails) + " erfolglos.";
        }
      }

      // ok, Lima format, just report the failures
      else {
        err = " Zustellung(en) erfolglos: " + implode(fails, " ");
      }
    }

    if (!err) {
      if (pl) MSG_TO_PLAYER(pl, ok);
    }
    else {
      // bounce mail
      if (pl) MSG_TO_PLAYER(pl, ok + err);

      if (!LOCAL_MAILER->query_recipient_ok(mails[i][1][MAIL_O_USER])) {
        DEBUGI3("mail_ack: cannot bounce mail to " + mails[i][1][MAIL_O_USER]);
      }
      else LOCAL_MAILER->deliver_mail(mails[i][1][MAIL_O_USER],
        "Mailer@Wunderland", "Bounced Mail",
        "Grund:" + err + "\n\nDie Original-Nachricht folgt:\n" +
        "Titel: " + mails[i][1][MAIL_SUBJECT] + "\n\n" +
        mails[i][1][MAIL_CONTENT]);
    }

    mails[i..i] = ({}); // delete entry

  } /* end foreach mailid */
}

//////
// Sending Mail

public void send_mail(string mud, string user, string sub,
    string txt, string who) {
  object pl;
  mixed *mail;
  int id;
  int k;

//  if (object_name(previous_object()) != IMUD_CONNECT) return;

  // if destination does not feature auth service, just try without
  k = I3_TOOL("mudlist")->query_mud_info(mud)[MLI_SERVICES]["auth"];
  if (!k) {
    pl = find_player(who);
    if (pl) tell_object(pl, "Sende unauthentifiziert an "+mud+"\n");
  }

  user = lower_case(user);
  who  = lower_case(who);
  id   = getid();
  mail = ({ "mail",                // SERVICE
            id,                    // MAIL_ID
            who,                   // MAIL_O_USER
            ([ mud: ({ user }) ]), // MAIL_TO
            ([ ]),                 // MAIL_CC
            ({ }),                 // MAIL_BCC
            time(),                // MAIL_TIME
            sub,                   // MAIL_SUBJECT
            txt                    // MAIL_CONTENT
    });

  if (!mails) mails =  ({ ({ mud, mail }) });
  else        mails += ({ ({ mud, mail }) });

  I3_TOOL("oob")->send_oob_to(mud, k, mail,
    lambda( ({ 'err }), ({ #'mail_cb, id, 'err }) ));
}

// error handler
static void mail_cb(int id, string err) {
  int i, n;
  object pl;

  for (n = sizeof(mails); i < n; ++i)
    if (id == mails[i][1][MAIL_ID]) break;

  if (i == n) return;

  pl = find_player(mails[i][1][MAIL_O_USER]);

  if (pl)
    MSG_TO_PLAYER(pl, "Mailer@Wunderland: Zustellung schug fehl ("+err+")");

  // bounce mail
  if (!LOCAL_MAILER->query_recipient_ok(mails[i][1][MAIL_O_USER])) {
    DEBUGI3("mail_ack: cannot bounce mail to " + mails[i][1][MAIL_O_USER]);
  }
  LOCAL_MAILER->deliver_mail(mails[i][1][MAIL_O_USER],
    "Mailer@Wunderland", "Bounced Mail",
    "Grund:" + err + "\n\nDie Original-Nachricht folgt:\n" +
    "Titel: " + mails[i][1][MAIL_SUBJECT] + "\n\n" +
    mails[i][1][MAIL_CONTENT]);

  mails[i..i] = ({});
}

// ID is sending time
// If a mail with that ID exists already, just increase the ID
static int getid() {
  int i, n, id;
  id = time();

  // works because mails are chronologically sorted
  for (n = sizeof(mails); i < n; ++i) {
    if (id == mails[i][1][MAIL_ID]) ++id;
  }
  return id;
}

// checks types of recipients data structures
static string check_mail_types(mixed* data) {
  foreach (string mud, string* rec : data[MAIL_TO]) {
    if (!stringp(mud)) return "key of MAIL_TO not string";
    if (!pointerp(rec)) return "value of MAIL_TO not array";
    if (sizeof(filter(rec, (: !stringp($1) :))))
      return "element in MAIL_TO value not string";
  }

  foreach (string mud, string* rec : data[MAIL_CC]) {
    if (!stringp(mud)) return "key of MAIL_CC not string";
    if (!pointerp(rec)) return "value of MAIL_CC not array";
    if (sizeof(filter(rec, (: !stringp($1) :))))
      return "element in MAIL_CC value not string";
  }

  if (sizeof(filter(data[MAIL_BCC], (: !stringp($1) :))))
    return "element in MAIL_BCC value not string";
}
