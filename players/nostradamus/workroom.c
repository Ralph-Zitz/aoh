#define NAME "Nostradamus"
/* The default workroom for new wizards. It's copied to their homedir */

#ifndef NAME
#define NAME "Someone"
#endif

#include <stdrooms.h>
#include <properties.h>
#include <msgclass.h>
#include <mxp.h>

inherit "/std/room";

static int my_cmd(string s);
static int my_efun(string s);

public varargs void create () {
  ::create();
  Set(P_INT_SHORT, "The workroom of " NAME);
  Set(P_INT_LONG,
     "This is the workroom of " NAME " 😃.\n"
     "Nothing " + MXPTAG("b") + "to" + MXPTAG("/b") + " see here, move along.\n"
  );
  AddRoomCmd("hosts", #'my_cmd /*'*/);
  AddRoomCmd("efun", #'my_efun /*'*/);
  AddExit("out", STARTROOM);
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
  msg = ({string})"/p/daemons/inetd"->send_packet(s,
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
