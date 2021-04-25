//
// Wunderland Mudlib
//
// global/daemon/i3/emote.c  --  EmoteTo handler
//
// $Log: emote.c,v $
// Revision 1.2  2002/08/06 10:35:34  Fiona
// clean up
//

#pragma strong_types
#pragma no_clone
#pragma no_shadow
#pragma no_inherit

#include <properties.h>
#include <msgclass.h>

#undef DEBUG_I3
#include <i3.h>

static void receive(mixed data);

public void ServiceQuery() {
  I3->RegisterService("emoteto", "emoteto", #'receive /*'*/, ASSERT_EMOTETO);
}

public void DaemonShutdown() {
  // nothing here
}

static string myname, repl;

static void receive(mixed data) {
  string name;
  object ob;

  //////
  // lib dependent: send appropriate communication event

  name = lower_case(data[D_USER]);
  if (ob = find_player(name)) {
    if (ob->QueryProp(P_INVIS)) {
      SENDI3ERR(data, "unk-user", capitalize(data[D_USER]) +
        " is not available.");
      return;
    }
    string user = sprintf("%s@%s", capitalize(data[O_USER]), data[O_MUD]);
    msg_object(ob, CMSG_GENERIC,
	       sprintf("*%s\n",
		       implode( explode( data[T_MESS], "$N" ), user ) ) );

  }
  else
  {
    SENDI3ERR(data, "unk-user", capitalize(data[D_USER]) +
      " is unknown.");
  }
}

public void send_emoteto(string who, string where, string msg, object act) {
  if (strstr(msg, "$N") == -1)
    msg = "$N " + msg;
  if (msg[<1] == '\n')
    msg = msg[0..<2];
  SENDI3(({ "emoteto",       // SERVICE
            5,               // HOPS
            MUDNAME,         // O_MUD
            getuid(act),     // O_USER
            where,           // D_MUD
            lower_case(who), // D_USER
            capitalize(act->Query(P_REALNAME)),  // E_O_VISNAME
            msg              // E_MESS
    }));
}
