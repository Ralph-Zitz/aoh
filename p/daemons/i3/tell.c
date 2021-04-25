//
// Wunderland Mudlib
//
// global/daemon/i3/tell.c  --  Tell handler
//
// $Log: tell.c,v $
// Revision 1.3  2002/10/11 14:05:00  Fiona
// filter incoming messages
//
// Revision 1.2  2002/08/06 10:48:58  Fiona
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
  I3->RegisterService("tell", "tell", #'receive, ASSERT_TELL);
}

public void DaemonShutdown() {
  // nothing here
}

static void receive(mixed data) {
  object ob;
  string name;

  name = lower_case(data[D_USER]);

  if (ob = find_player(name)) {

    if (ob->QueryProp(P_INVIS)) {
      SENDI3ERR(data, "unk-user", capitalize(data[D_USER]) +
        " is not here.");
      return;
    }

    msg_object(ob, CMSG_TELL, sprintf("%s@%s tells you: %s\n",
                                      capitalize(data[O_USER]),
                                      capitalize(data[O_MUD]),
                                      data[T_MESS]));
    ob->Set( P_TELL_REPLYTO, ({ data[O_USER] + "@" + data[O_MUD] }) );
  }
  else
  {
    SENDI3ERR(data, "unk-user", capitalize(data[D_USER]) +
      " is not here.");
  }
}

void send_tell(string who, string where, string msg, object act) {
//  if (object_name(previous_object()) != IMUD_CONNECT) return;
  SENDI3(({ "tell",          // SERVICE
            5,               // HOPS
            MUDNAME,         // O_MUD
            getuid(act),     // O_USER
            where,           // D_MUD
            lower_case(who), // D_USER
            capitalize(act->Query(P_REALNAME)),  // T_O_VISNAME
            msg              // T_MESS
     }));
}
