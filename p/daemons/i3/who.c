//
// Wunderland Mudlib
//
// global/daemon/i3/who.c  --  Wholist handler
//
// $Log: who.c,v $
// Revision 1.6  2003/03/24 08:19:08  Fiona
// Flagbuf didnt clean correctly; timeout time was too long (Fuchur)
//

#pragma strong_types
#pragma no_clone
#pragma no_shadow
#pragma no_inherit

inherit __DIR__ "assert";

#include <properties.h>
#include <secure/wizlevels.h>
#include <ansi.h>
#include <msgclass.h>

#undef DEBUG_I3
#include <i3.h>

// ([ uid: ({ send_time, flag, d_mud, send_time2, flag2, d_mud2... }) ])
nosave mapping flagbuf;
// Flags
#define SHORT     1
#define HELP      2 // not supported
#define ALPHA     4
#define VERTICAL  8
#define RACE     16 // not supported

private mixed* who_data();
private mixed* make_who_data(object user);
private string make_line(mixed* data);
private int    sorter_fun(object a, object b);

static void wholist(mixed data);
static void answer(mixed data);

public void ServiceQuery() {
  I3->RegisterService("who", "who-req",   #'wholist, ASSERT_WHOLIST_QUERY);
  I3->RegisterService(0    , "who-reply", #'answer,  ASSERT_WHOLIST);
}

public void DaemonShutdown() {
  // nothing here
}

// Send a who request
// The flags are buffered and used to build the list from the reply packet.
// Flags are put on a user specific fifo list.
void send_who_request(string where) {
  string uid = getuid(this_player());

  SENDI3(({ "who-req",             // SERVICE
            5,                     // HOPS
            MUDNAME,               // O_MUD
            uid,                   // O_USER
            where,                 // D_MUD
            0                      // D_USER
    }));
}

static void wholist(mixed data) {
  SENDI3(({ "who-reply",  // SERVICE
            5,            // HOPS
            MUDNAME,      // O_MUD
            0,
            data[O_MUD],  // D_MUD
            data[O_USER], // D_USER
            who_data()    // W_DATA
    }));
}

static void answer(mixed data) {
  object pl;
  string err;
  int max, i;

  max = sizeof(data[W_DATA]);
  for (i = 0; i < max; ++i) {
    err = assert(data[W_DATA][i], ASSERT_WHOLIST_CONTENT);
    if (err) {
      LOGI3("answer", err + " " + MIXED2STR(data[W_DATA]));
      DEBUGI3(err);
      return;
    }
  }

  data[D_USER] = lower_case(data[D_USER]);
  pl = find_player(data[D_USER]);
  if (!pl) return;
  else {
    string s;
    s = sprintf("[%s] %d users connected\n%-78'-'s\n",
                data[O_MUD],
                sizeof(data[W_DATA]), "");
    s += implode(map(data[W_DATA], #'make_line /*'*/ ), "\n" );
    s += sprintf( "\n%-78'-'s\n", "" );
    msg_object(pl, CMSG_GENERIC|MMSG_MORE, s);
  }
}
/////
// Mudlib specific functions

// collect infos of our players

private string make_line( mixed * data ) {
  if (sizeof(data) > 2 && intp(data[1]))
    return sprintf("%-15s%-10d %s", data[0]||"", data[1], data[2]||"" );
  else
    return sprintf("%-15s%-10s %s", data[0]||"", data[1], data[2]||"" );
}

private mixed * make_who_data( object user ) {
  string visname;
  string title;

  if ( ( user->QueryIsPlayer() ) && ( ! user->Query( P_INVIS ) ) ) {
    title = user->Short();
    visname = capitalize( user->Query(P_REALNAME) );
    return ({ visname, query_idle(user), title });
  }
  else
    return 0;
}

private mixed *who_data() {
  mixed *ret = filter(users(), (: !$1->QueryProp(P_INVIS) :));
  sort_array(&ret, #'sorter_fun /*'*/);
  ret = map(ret, #'make_who_data /*'*/) - ({0});
  return ret;
}

private int sorter_fun(object a, object b) {
  int al, bl;

  al = __MASTER_OBJECT__->query_user_level(a);
  bl = __MASTER_OBJECT__->query_user_level(b);

  if (al != bl) return al > bl;
  if (al >= 10) return getuid(a) > getuid(b);

  al = a->QueryProp(P_LEVEL);
  bl = b->QueryProp(P_LEVEL);

  if (al != bl) return al > bl;
  return getuid(a) > getuid(b);
}
