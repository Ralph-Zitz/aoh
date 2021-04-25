#define NAME "Nostradamus"
/* The default workroom for new wizards. It's copied to their homedir */

#ifndef NAME
#define NAME "Someone"
#endif

#include <stdrooms.h>
#include <properties.h>
#include <msgclass.h>

inherit "/std/room";

static int my_cmd(string s);

public varargs void create () {
  ::create();
  Set(P_INT_SHORT, "The workroom of " NAME);
  Set(P_INT_LONG,
     "This is the workroom of " NAME " 😃.\n"
     "Since " NAME " is just a new wizard, it is completely empty, but "
     "nevertheless quite comfortable.\n"
  );
  AddRoomCmd("hosts", #'my_cmd);
  AddExit("out", STARTROOM);
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
