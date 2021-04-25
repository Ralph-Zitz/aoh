//
// Wunderland Mudlib
//
// global/daemon/i3/mudlist.c  --  Mudlist handler
//
// $Log: mudlist.c,v $
// Revision 1.5  2002/10/28 14:03:54  Fiona
// Info ueber Mudtyp ausgeben
//
// Revision 1.4  2002/09/18 12:04:24  Fiona
// report 'unknown' state if i3 is inactive
//

#pragma strong_types
#pragma no_clone
#pragma no_shadow
#pragma no_inherit

inherit __DIR__ "assert";

#undef DEBUG_I3
#include <i3.h>
#include <string.h>
#include <driver/regexp.h>

int     mudlist_id;
mapping muds;
mapping mud_names;

nosave int    update_txt;
nosave string mudlist_text;

static void handle(mixed* data);

void create() {
  seteuid(getuid());
  efun::restore_object(I3_MUDLIST_SAVE);
}

/////
// Main handling functions

public void ServiceQuery() {
  I3->RegisterService("!mudlist", "mudlist", #'handle, ASSERT_MUDLIST);
}

public void DaemonShutdown() {
  // nothing here
}

static void handle(mixed* data) {
  string m_name, m_lname, m_xname, err;
  int initial_run, i;
  mixed* m_info;

  initial_run = !(I3->query_state() & I3_RUNNING);

  if (!(I3->query_state() & I3_MUDLIST_OK)) {
    muds = ([]);
    mud_names = ([]);
    I3->change_state(I3_MUDLIST_OK);
  }

  foreach (m_name, m_info : data[ML_INFO]) {
    m_lname = lower_case(m_name);
    m_xname = regreplace(m_lname, " ", ".", 1);
    if (!m_info) {
      m_delete(muds, m_name);
      m_delete(mud_names, m_xname);
      m_delete(mud_names, m_lname);
      if (!initial_run) DEBUGI3("deleted ("+m_name+")");
    }
    else {
      err = assert(m_info, ASSERT_MUDLIST_ENTRY);
      if (err) {
        LOGI3("handle", err + " " + MIXED2STR(m_info));
        DEBUGI3(err);
        continue;
      }
      i = sizeof(m_info[MLI_SERVICES]);
      m_info[MLI_SERVICES] = filter(m_info[MLI_SERVICES], (: stringp($1) :));
      if (sizeof(m_info[MLI_SERVICES]) != i)
        DEBUGI3("removing wrong service keys for "+m_name);
      muds[m_name] = m_info;
      // check for name clashs?
      mud_names[m_xname] = m_name;
      mud_names[m_lname] = m_name;
      if (!initial_run) {
        if (m_info[MLI_DOWNTIME] < 0) err = "up (";
        else if (m_info[MLI_DOWNTIME] == 0) err = "down indefinite (";
        else err = "down "+m_info[MLI_DOWNTIME]+"s (";
        DEBUGI3(err + m_name + ")");
      }
    }
  }
  mudlist_id = data[ML_MUDLIST_ID];
  update_txt = 1;
  efun::save_object(I3_MUDLIST_SAVE);
  return;
}

/////
// Functions for external data access

// get list of all muds (raw)
mapping query_muds() {
  return deep_copy(muds);
}

string * query_mudnames()
{
  return m_indices(muds);
}

string mud_find(string mud) {
  string *m = regexp(query_mudnames(), "(?i)^"+mud+"$", RE_PCRE);
  if (sizeof(m) < 1)
    return 0;
  return m[0];
}

// get list of all muds (text)
// *not used in Wunderland*
string query_mudlist() {
  int i, all, up;
  mixed idx;
  if (update_txt || !stringp(mudlist_text))
  {
    update_txt = 0;
    idx = sort_array(m_indices(muds), #'<);
    all = sizeof(idx);
    mudlist_text = "";

    for(i = all; i--; ) {
      mudlist_text+=sprintf("%-34s %-15s  %:4d %s\n",
        idx[i], muds[idx[i]][MLI_IP_ADDR], muds[idx[i]][MLI_PORT_PL],
        (muds[idx[i]][MLI_DOWNTIME]<0?"OPEN":"CLOSED"));
      if (muds[idx[i]][MLI_DOWNTIME]<0) up++;
    }

    mudlist_text = "Complete Intermud3 MUD List ("+
      up+"/"+all+" connected):\n"
      "====================================================\n"
      +mudlist_text;
  }
  return mudlist_text;
}

// get mud information (raw)
mixed query_mud_info(string str) {
  if (!stringp(str)) return 0;
  return deep_copy(muds[str]);
}

// get mud information (text)
string query_mudtext(string mud, int extra) {
  mixed tmp;
  string txt, *s;
  int i;

  if (!stringp(mud)) return 0;
  tmp = muds[mud];
  if (!pointerp(tmp)) return 0;

  txt  = sprintf("I3-Information on %s:\n", mud);
  txt += sprintf("%*'='s\n", sizeof(txt), "");
  if (!(I3->query_state() & I3_RUNNING))
    txt += "Present Status : unknown (Intermud 3 inactive)\n";
  else
    txt += sprintf("Present Status : %s\n",
      (tmp[MLI_DOWNTIME]<0?"Connected":(tmp[MLI_DOWNTIME]>0?"Reboot in "+
      tmp[MLI_DOWNTIME]+" Seconds.":"Closed")));
  txt += sprintf("IP-Address (Port): %s (%d)\n", tmp[MLI_IP_ADDR],
    tmp[MLI_PORT_PL]);
  if (extra) {
    txt += sprintf("TCP-Port         : %d\n", tmp[MLI_PORT_TCP]);
    txt += sprintf("UDP-Port         : %d\n", tmp[MLI_PORT_UDP]);
  }
  txt += sprintf("Mudlib           : %s\n", tmp[MLI_MUDLIB]);
  txt += sprintf("Driver           : %s\n", tmp[MLI_DRIVER]);
  txt += sprintf("MUD-Type         : %s\n", tmp[MLI_MUD_TYPE]);
  txt += sprintf("MUD-Status       : %s\n", tmp[MLI_STATE]);
  if (extra) {
    txt += sprintf("Admin-Email      : %s\n", tmp[MLI_EMAIL]);
    s = m_indices(tmp[MLI_SERVICES]);
    for (i=sizeof(s); i--;)
      if (tmp[MLI_SERVICES][s[i]] != 1)
        s[i] += " (" + tmp[MLI_SERVICES][s[i]] + ")";
    s = sort_array(s, #'>);
    txt += L_STRING->break_string(L_STRING->countup(s), 78, "Services         : ");
    if (mappingp(tmp[MLI_OTHER])) {
      s = m_indices(tmp[MLI_OTHER]);
      if (sizeof(s))
        txt += L_STRING->break_string(L_STRING->countup(s), 78, "Extra            : ");
    }
  }
  return txt;
}

// find matching mud(s)
string* expand_mud_name(string mud) {
  string* all;

  if (member(mud_names, mud)) return ({ mud_names[mud] });
  all = m_indices(mud_names);
  all = filter(all, (: !strstr($1, $2) :), mud);
  all = map(all, (: mud_names[$1] :));
  all = unmkmapping(mkmapping(all))[0];
  return all;
}
