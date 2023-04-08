#define NAME "Nostradamus"
/* The default workroom for new wizards. It's copied to their homedir */

#ifndef NAME
#define NAME "Someone"
#endif

#include <stdrooms.h>
#include <macros.h>
#include <msgclass.h>
#include <mxp.h>
#include <newmxp.h>
#include <config.h>
#include <daemons.h>
#include <inetd.h>
#include <driver/regexp.h>

inherit "/std/room";

static int my_efun(string s);
static int my_reg(string s);
static void my_test(string s);
static int my_skill(string s);
public int ApproxValue(int x, int y0, int app, int dcc);
public varargs int LearnAttr(string aname, string skill, int app, int dcc, int sc);

public varargs void create () {
    ::create();
    Set(P_INT_SHORT, "The workroom of " NAME);
    Set(P_INT_LONG,
            "This is the workroom of " NAME " 😃.\n"
            "Nothing to see here, move along.\n"
       );
    AddRoomCmd("efun", #'my_efun /*'*/);
    AddRoomCmd("reg", #'my_reg /*'*/);
    AddRoomCmd("sk", #'my_skill /*'*/);
    AddExit("out", STARTROOM);
}

static int my_reg(string s) {
    string mat = "https?:\/\/(www\.)?[-a-zA-Z0-9@:%._\+~#=]{1,256}\.[a-zA-Z0-9()]{1,6}\\b([-a-zA-Z0-9()@:%_\+.~#?&//=]*)";
    mixed res;
    if (!s)
        return notify_fail("Illegal URL to match!\n", NOTIFY_ILL_ARG);
    // TODO - re-implement
    if ((res = regmatch(s, mat, RE_PCRE|RE_GLOBAL)) && stringp(res) && sizeof(res) > 0)
        msg_write(CMSG_GENERIC, "Matched: " + MXPTAG("a href=\"" + res + "\"") + res + MXPTAG("/a")+ "\n");
    else
        return notify_fail(sprintf("Failed to match the URL: %s\n", s), NOTIFY_ILL_ARG);
    return 1;
}

static int my_efun(string s) {
    send_discord(({string})this_player()->QueryName() + ": " + s);
    return 1;
}

static int my_skill(string s) {
    int i = 1;
    //  for (int u = 0; u < 500; u++) {
    //    i += LearnAttr("skills", "climbing", 1000, i, 0);
    i = ApproxValue(1, 1, 1000, 300);
    msg_write(CMSG_GENERIC, sprintf("Skill-level: %d\n", i));
    //  }
    return 1;
}

public varargs int LearnAttr(string aname, string skill, int app, int dcc, int sc) {
    int i;
    if (!m_contains(&i, ({mapping})TP->QueryAttr(aname), skill))
        return notify_fail("Skill is unknown\n", NOTIFY_ILL_ARG);
    i += (app - i + dcc + 1) / dcc;
    return i;
    //return SetAttr(aname, i, sc);
}

public int ApproxValue(int x, int y0, int app, int dcc) {
    float fx, fapp, fdcc, t1, t2;

    if(y0 == app) return x;
    fx   = to_float(x);
    fapp = to_float(app);
    fdcc = to_float(dcc);
    t1    = fapp + fdcc / 2.0;
    t2    = t1 - t1 * exp(fx / dcc - log(1.0 - y0 / t1));
    printf("fx=%f, fapp=%f, fdcc=%f, t1=%f, t2=%f\n",fx,fapp,fdcc,t1,t2);
    if(y0 < app) return t2 < fapp ? to_int(-t2) : app;
    else         return t2 > fapp ? to_int(t2) : app;
}
