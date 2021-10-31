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

static int my_efun(string s);
static int my_reg(string s);
static void my_test(string s);

public varargs void create () {
  ::create();
  Set(P_INT_SHORT, "The workroom of " NAME);
  Set(P_INT_LONG,
     "This is the workroom of " NAME " 😃.\n"
     "Nothing " + MXPTAG("b") + "to" + MXPTAG("/b") + " see here, move along.\n"
  );
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
  python_exec(s, #'my_test /*'*/);
  return 1;
}
