//
// Wunderland Mudlib
//
// global/daemon/i3/finger.c  --  Finger handler
//
// $Log: finger.c,v $
// Revision 1.4  2003/09/15 11:23:46  Fiona
// Corrected problem with nameless remote players
//

#pragma strong_types
#pragma no_clone
#pragma no_shadow
#pragma no_inherit

#include <properties.h>
#include <secure/wizlevels.h>
#include <driver/time.h>
#include <daemons.h>
#include <msgclass.h>
#include <ansi.h>

#undef DEBUG_I3
#include <i3.h>
#ifndef FINGEROBJ
#define FINGEROBJ "/obj/player_dummy"
#endif

static void finger(mixed data);
static void answer(mixed data);
private mixed finger_infos(string str);
private string mudos_terminal_colour(string str, mapping ext, int w, int i);

/////
// Main handling functions

public void ServiceQuery() {
  I3->RegisterService("finger", "finger-req",   #'finger, ASSERT_FINGER_REQ);
  I3->RegisterService(0,        "finger-reply", #'answer, ASSERT_FINGER_REPLY);
}

public void DaemonShutdown() {
  // nothing here
}

static void finger(mixed data) {
  mixed *ret;

  ret = finger_infos(lower_case(trim(data[F_VISNAME])));

  if (!ret) {
    SENDI3ERR(data, "unk-user", "Unknown user.");
    return;
  }

  SENDI3(({ "finger-reply", // SERVICE
            5,              // HOPS
            MUDNAME,        // O_MUD
            0,
            data[O_MUD],    // D_MUD
            data[O_USER]    // D_USER
            }) + ret        // see finger_infos() below
    );
}

static void answer(mixed data) {
  object ob;
  string plan, full, out;
  mapping clr;

  if (!(ob = find_player(lower_case(data[D_USER])))) return;
  if (ob->QueryProp(P_TTY) == "ansi") clr = PINKFISH_2_ANSI;
  else clr = PINKFISH_2_DUMB;

  out = "Finger information for " + FILTER_TXT(data[F_VISNAME], 1) +
    " from " + data[O_MUD]+":\n";

  if (data[F_TITLE] && data[F_REALNAME])
    full = data[F_TITLE] + " (" + data[F_REALNAME] + ")";
  else if (data[F_REALNAME])
    full = data[F_VISNAME] + " (" + data[F_REALNAME] + ")";
  else if (data[F_TITLE])
    full = data[F_TITLE];
  else if (data[F_VISNAME])
    full = data[F_VISNAME];
  else full = "<Someone>";
  full = mudos_terminal_colour(full, clr, 75, 0);
  full = "| "+ implode(efun::explode(full, "\n"), "\n| ");
  if (full[<3..] == "\n| ") full = full[0..<3];
  else full += "\n";
  out += FILTER_TXT(full, 0);

  if (data[F_EMAIL])
    out += "| Email-Address: " + FILTER_TXT(data[F_EMAIL], 1) + "\n";

  if (data[F_IDLE] == -1) {
    if (data[F_LOGOUT])
      out += "| Last Logout: " + FILTER_TXT(data[F_LOGOUT], 1) + "\n";
    else
      out += "| Not logged in.\n";
  } else {
    if (data[F_LOGOUT]) {
      if (data[F_IDLE] >= 60) out += "| Last Login: " +
        FILTER_TXT(data[F_LOGOUT], 1) +
        sprintf(" (idle %d seconds)\n", data[F_IDLE]);
      else out += "| Last Login: " + FILTER_TXT(data[F_LOGOUT], 1) + "\n";
    }
    else {
      if (data[F_IDLE] >= 60)
        out += "| Idle: " + sprintf(" %d seconds\n", data[F_IDLE]);
      else
        out += "| Logged in.\n";
    }
  }

  if (data[F_LEVEL])
    out += "| Level: " + FILTER_TXT(data[F_LEVEL], 1) + "\n";

  if (data[F_IP_NAME])
    out += "| Logged in from : " + FILTER_TXT(data[F_IP_NAME], 1) + "\n";
  if (data[F_EXTRA]) {
    plan = mudos_terminal_colour(FILTER_TXT(data[F_EXTRA], 0), clr, 75, 0);
    plan = regreplace(plan, "\n", "\n| \\1", 1);
    if (plan[<3..] == "\n| ") plan = plan[0..<3];
    else if (plan[<1] != '\n') plan += "\n";
    out += "| "+plan;
  }
  msg_object(ob, CMSG_GENERIC|MMSG_MORE, out);
}

public void send_finger_request(string who, string where, string myid) {
//  if (object_name(previous_object()) != IMUD_CONNECT) return;
  SENDI3(({ "finger-req", // SERVICE
            5,            // HOPS
            MUDNAME,      // O_MUD
            myid,         // O_USER
            where,        // D_MUD
            0,
            who           // F_USER
    }));
}

private string mudos_terminal_colour(string str, mapping ext, int w, int i) {
  return terminal_colour("%^"+implode(efun::explode(str, "%^")-({""}), "%^%^"),
    ext, w, i);
}

//////
// Mudlib dependent functions

private mixed finger_infos(string str) {
  object ob;
  int idle, i, j, *t;
  string nam, title, pre, plan, level, loginout;

  nam = FINGEROBJ->QueryProperty(str, P_NAME);
  if (!stringp(nam)) return 0;

  title = FINGEROBJ->QueryProperty(str, P_TITLE);
  pre   = FINGEROBJ->QueryProperty(str, P_PRESAY);
  i     = FINGEROBJ->QueryProperty(str, P_NETDEAD);

  if (!stringp(title)) title = "";
  else if (title[0] != ',') title = " " + title;
  if (!stringp(pre) || !sizeof(pre)) pre = "";
  else pre += " ";
  title = pre + nam + title;

  ob = find_player(str);

  if (ob && interactive(ob) && !ob->QueryProp(P_INVIS)) idle = query_idle(ob);
  else idle = -1;

  if (i) {
    t = localtime(i);
    loginout = sprintf("%02d.%02d.%02d %02d:%02d",
      t[TM_MDAY], t[TM_MON]+1, t[TM_YEAR], t[TM_HOUR], t[TM_MIN]);
  }

//  in = FINGEROBJ->QueryProperty(str, P_GENDER) == GENDER_FEMALE ? "in)" : ")";
  i = __MASTER_OBJECT__->query_user_level(str);
  if (i <= 3) {
    j = FINGEROBJ->QueryProperty(str, P_LEVEL);
    if      (i < 1)  level = j+" (Player)";
    else if (i == 1) level = j+" (Ancient)";
    else if (i == 2) level = j+" (Wizard)";
    else             level = j+" (Illuminati)";
  }
  if (IS_WIZARD(str))
    plan = read_file("/players/" + str + "/.plan");
  if (stringp(plan) && sizeof(plan)) plan = "Plan:\n" + plan;

  return ({ nam,      // F_VISNAME
            title,    // F_TITLE
            0,        // F_REALNAME
            0,        // F_EMAIL
            loginout, // F_LOGOUT
            idle,     // F_IDLE
            0,        // F_IP_NAME
            level,    // F_LEVEL
            plan      // F_EXTRA
         });
}
