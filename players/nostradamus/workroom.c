#define NAME "Nostradamus"
/* The default workroom for new wizards. It's copied to their homedir */

#ifndef NAME
#define NAME "Someone"
#endif

#include <stdrooms.h>
#include <properties.h>
#include <msgclass.h>
#include <mxp.h>
#include <config.h>
#include <daemons.h>
#include <inetd.h>
#include <driver/regexp.h>

inherit "/std/room";

static int my_cmd(string s);
static int my_efun(string s);
static int my_reg(string s);
static int my_update_hosts(string s, mapping hosts, string * indices, string * done);

public varargs void create () {
  ::create();
  Set(P_INT_SHORT, "The workroom of " NAME);
  Set(P_INT_LONG,
     "This is the workroom of " NAME " 😃.\n"
     "Nothing " + MXPTAG("b") + "to" + MXPTAG("/b") + " see here, move along.\n"
  );
  AddRoomCmd("hosts", #'my_cmd /*'*/);
  AddRoomCmd("run", #'my_update_hosts /*'*/);
  AddRoomCmd("efun", #'my_efun /*'*/);
  AddRoomCmd("reg", #'my_reg /*'*/);
  AddExit("out", STARTROOM);
}

static int my_reg(string s) {
   string mat = "https?:\/\/(www\.)?[-a-zA-Z0-9@:%._\+~#=]{1,256}\.[a-zA-Z0-9()]{1,6}\\b([-a-zA-Z0-9()@:%_\+.~#?&//=]*)";
   mixed res;
   if (!s)
     return notify_fail("Illegal URL to match!\n", NOTIFY_ILL_ARG);
   if ((res = regmatch(s, mat, RE_PCRE|RE_GLOBAL)) && stringp(res) && sizeof(res) > 0)
     msg_write(CMSG_GENERIC, "Matched: " + MXPTAG("a href=\"" + res + "\"") + res + MXPTAG("/a")+ "\n");
   else
     return notify_fail(sprintf("Failed to match the URL: %s\n", s), NOTIFY_ILL_ARG);
   return 1;
}

static void my_test(string s) {
  if (s) {
    write(s);
  }
}

static int my_efun(string s) {
  python_exec(s, #'my_test);
  return 1;
}

static int my_cmd(string s) {
  string msg;
  msg = ({string})INET_D->send_packet(s,
                                  ([
                                    "REQ": "query",
                                    "DATA": "hosts",
                                    "SND": ({string})this_player()->Query(P_REALNAME)
                                   ]), 1);
  if (stringp(msg))
    msg_write(CMSG_GENERIC, msg);
  else
    msg_write(CMSG_GENERIC, "Message sent!");
  return 1;
}

static int my_update_hosts(string s, mapping hosts = ([]), string * indices = ({}), string * done = ({})) {
  this_player()->RegisterCmdFun(({this_player(), "print_prompt"}), s);
  return 1;
#if 0
  int i;
  string index, resp;
  string * exp_resp;

  if (!sizeof(hosts)) {
    hosts = ({mapping})INET_D->query(I2RQ_HOSTS);
    indices = sort_array(m_indices(hosts), #'> /*'*/);
    if (sizeof(hosts) == sizeof(done))
       return 0;
  }
  while (remove_call_out(#'my_update_hosts /*'*/) != -1);
  i = sizeof(indices) - 1;
  if (i < 0)
    return 0;
  index = indices[i];
  if (sizeof(indices) > 1)
    indices = indices[..<2];
  else
    indices = ({});
  call_out(#'my_update_hosts, 2, 0, m_delete(hosts, index), indices, done += ({index}));

  // Process each mud from here on out
  resp = ({string})INET_D->send_packet(index,
          ([
            "REQ": "query",
            "DATA": "hosts",
            "SND": ({string})this_player()->Query(P_REALNAME)
          ]), 1);
  if (stringp(resp) && sizeof(resp)) {
      exp_resp = explode(resp, "\n");
  }
  return 1;
#endif
}

