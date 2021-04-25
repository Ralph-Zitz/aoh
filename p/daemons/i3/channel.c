//
// Wunderland Mudlib
//
// global/daemon/i3/channel.c  --  Channel handler
//
// $Log: channel.c,v $
// Revision 1.9  2003/09/23 10:37:05  Fiona
// Sinnlose casts entfernt
//

#pragma strong_types
#pragma no_clone
#pragma no_shadow
#pragma no_inherit

#include <daemons.h>
#include <ansi.h>
#include <properties.h>
#include <string.h>
#include <channel.h>
#include <driver/regexp.h>

#undef DEBUG_I3
#include <i3.h>

mapping channels;           // list of all i3 channels
int     chanlist_id;        // router-id of that list
mapping chans;              // list of listened to channels

nosave string  sendername;  // helper for name()
nosave mapping p_chans;     // list of prefered channels
nosave int     state;       // state of i3-connection. 1 = listening

// data formats:
// channels: ([ channel_name: ({ CLST_HOST, CLST_TYPE }), ... ])
// chans: ([ channel_name: prio; description; listen level; send level, ... ])

private int  add_channel(string chan, mixed* chani, int setup);
private void del_channel(string chan);
static void  chanlist(mixed* data);
static void  rec_msg(mixed data);
static void  rec_emote(mixed data, mixed temote);
static void  rec_t_emote(mixed data);
static void  send_channel_listen(string channel, int doit);
static void  who_reply(mixed data);
static void  who_req(mixed data);
static string substitute_name(string m, int g, int e);

void create() { //OK
  int reload;
  string chan;
  mixed* chani;

  if (p_chans) return;

  seteuid(getuid());
  efun::restore_object(I3_CHANNEL_SAVE);

  // prefered channels
  // they have a prio, a description and read/send (wiz)levels
  p_chans = ([ "imud_gossip": 15; "Englischer Kanal auf Intermud3";     0;  0,
               "imud_code"  : 17; "Programmierer Forum auf Intermud3"; 10; 10,
               "german"     : 17; "Deutscher Kanal auf Intermud3";      0;  0,
               "dcode"      : 17; "obsolet";10; 10,
               "g-code"     : 17; "Deutsche Programmierer auf Intermud3";10; 10,
               "bofh"       :  7; "Bastard Operator From Hell (i3)";    0;  0,
               "dutch"      :  3; "Niederlaender reden hier (i3)";      1;  1,
  ]);

  if (!channels) channels = ([]);
  chans = m_allocate(0, 4);

  if (chans && find_object(I3)) {
    catch(reload = I3->query_state() & I3_CHANLIST_OK);
  }
  if (reload) {
    state = 1;
    foreach (chan, chani : channels) add_channel(chan, chani, 0);
  }
  else chans = m_allocate(0, 4);
}

public void ServiceQuery() { // OK
  I3->RegisterService("channel", "channel-m", #'rec_msg /*'*/,     ASSERT_CHAN_MSG);
  I3->RegisterService(0, "channel-e",         #'rec_emote /*'*/,   ASSERT_CHAN_EMOTE);
  I3->RegisterService(0, "channel-t",         #'rec_t_emote /*'*/, ASSERT_CHAN_TEMOTE);
  I3->RegisterService(0, "chan-who-reply",    #'who_reply /*'*/,   ASSERT_WHO_REPLY);
  I3->RegisterService(0, "chan-who-req",      #'who_req /*'*/,     ASSERT_WHO_REQ);
  I3->RegisterService(0, "chanlist-reply",    #'chanlist /*'*/,    ASSERT_CHANLIST);
  state = 0;
}

public void DaemonShutdown() { // OK
  // nothing here
}

//////
// channel list management

// Theres a bug in the implementation of the router. chanlist is not done
// on diffs but we get a complete list every time. We have to find out
// if some channel has been deleted on our own

static void chanlist(mixed *data) { // OK
  string chan;
  string* channames;
  mixed* chani;
  int i, setup;

  chanlist_id = data[CLST_ID];
  setup = !(I3->query_state() & I3_CHANLIST_OK);

# if 0 // code for diff-mode
  foreach (chan, chani : data[CLST_LST]) {
    if (chani) {
      channels[chan] = chani;
      add_channel(chan, chani, setup);
    }
    else {
      del_channel(chan);
      m_delete(channels, chan);
    }
  }
# else // code for actual implementation of the router (28. Sep 2001)

  channames = m_indices(channels);
  channels  = ([]);

  foreach (chan, chani : data[CLST_LST]) {
    channels[chan] = chani;
    i = member(channames, chan);
    if (i >= 0) channames[i] = 0;
    add_channel(chan, chani, setup);
    if (!setup) {
      DEBUGI3("new channel: " + chan);
      LOGI3("new channel", chan);
    }
  }

  foreach (chan : channames) {
    if (!chan) continue;
    if (!setup) {
      DEBUGI3("channel removed: " + chan);
      LOGI3("channel removed", chan);
    }
    del_channel(chan);
  }
#endif
  efun::save_object(I3_CHANNEL_SAVE);
  if (setup) I3->change_state(I3_CHANLIST_OK);
  if (!state) state = 1;
}

// helper function
private void del_channel(string chan) { // OK
  if (member(chans, chan)) {
    CHANNEL_D->RemoveChannel(chan, this_object());
    m_delete(chans, chan);
  }
}

// helper function
private int add_channel(string chan, mixed* chani, int setup) { // OK
  object ci;
  int i;
  string *match;

  if (!setup && state && member(chans, chan)) {
    DEBUGI3("already listening");
    return -5;
  }

  // get selectively banned channels only
  if (chani[CLST_TYPE] != CLST_TYPE_S_BANNED) {
    return -4;
  }
  if (sizeof(match = regexplode(chan, "[^-a-zA-Z0-9_]", RE_TRADITIONAL)) > 1
    || (sizeof(match) == 1 && sizeof(match[0]) == 0)) {
    DEBUGI3("special chars in channame");
    return -3;
  }
  if (sizeof(regexplode(chani[CLST_HOST], "[^ -~]", RE_TRADITIONAL)) > 1) {
    DEBUGI3("special chars in hostname");
    return -3;
  }
  if (member(p_chans, chan)) {
    chans[chan, 0] = p_chans[chan, 0];
    chans[chan, 1] = p_chans[chan, 1];
    chans[chan, 2] = p_chans[chan, 2];
    chans[chan, 3] = p_chans[chan, 3];
  }
  else {
    // If only prefered channels are wanted... uncomment following line
    //return 0;
    chans[chan, 0] = 4;
    chans[chan, 1] = "Intermud3 Channel (" + chani[CLST_HOST] + ")";
    chans[chan, 2] = 10;
    chans[chan, 3] = 10;
  }
  i = CHANNEL_D->AddChannel(chan,
                            ({ this_object(), 0 }),
                            this_object());
  if (i > 0) send_channel_listen(chan, 1);
  else if (i == -1) {
    if (!setup || !state) send_channel_listen(chan, 1);
    else {
      // channel exists already, check if we are the owner
      string c = (({mixed *})CHANNEL_D->ChannelData(chan))[CHANNEL_CREATOR];
      ci = find_object(c);
      if (ci && object_name(ci) == object_name())
        send_channel_listen(chan, 1);
      else {
        del_channel(chan);
        send_channel_listen(chan, 0);
      }
    }
  }
  else  {
    del_channel(chan);
    send_channel_listen(chan, 0);
  }
  return i;
}

//////
// some general functions

static void send_channel_listen(string channel, int doit) { // OK
  string router;
  router = I3->query_router()[0];
  DEBUGI3("Listening to channel '"+channel+"': "+doit);
  SENDI3(({ "channel-listen",  // SERVICE
            5,                 // HOPS
            MUDNAME,           // O_MUD
            0,
            router,            // D_MUD
            0,
            channel,           // C_CHANNEL
            doit               // CLISTEN
    }));
}

void send_channel_add(string channel, string user, int flag) { // OK
  string router;

  //return; // allow only privileged user

  router = I3->query_router()[0];
  SENDI3(({ "channel-add",     // SERVICE
            5,                 // HOPS
            MUDNAME,           // O_MUD
            user,              // U_USER
            router,            // D_MUD
            0,
            channel,           // C_CHANNEL
            flag               // CA_TYPE
    }));
}

void send_channel_remove(string channel, string user) { // OK
  string router;

  //return; // allow only privileged user

  router = I3->query_router()[0];
  SENDI3(({ "channel-remove",  // SERVICE
            5,                 // HOPS
            MUDNAME,           // O_MUD
            user,              // U_USER
            router,            // D_MUD
            0,
            channel            // C_CHANNEL
    }));
}

int remove() { // OK
  foreach (string chan : chans) del_channel(chan);
  destruct(ME);
  return 1;
}

//////
// channel messages

static void rec_msg(mixed data) { // OK
  string chan, msg;

  DEBUGI3("Msg on '"+data[C_CHANNEL]+"' from "+capitalize(data[O_USER]));
  if (member(chans, data[C_CHANNEL]) < 0) return;

  data[O_MUD] = FILTER_TXT(data[O_MUD], 1);
  sendername  = capitalize( FILTER_TXT( data[C_VISNAME] , 1 ) );
  chan        = data[C_CHANNEL];
  msg         = trim(FILTER_TXT(data[C_MESSAGE], 1));
  msg         = terminal_colour(msg, PINKFISH_2_DUMB);

//  if (lower_case(MUDNAME) != lower_case(data[O_MUD]))
//    sendername += "@" + capitalize(data[O_MUD]);

  CHANNEL_D->SendTell( chan,
  		       sprintf("%s@%s", sendername, data[O_MUD]),
  		       msg );
  sendername = 0;
}

// temote is only used internally from rec_t_emote
static void rec_emote(mixed data, mixed temote) {
  string msg, *tmp;
//  string msg, chan, *tmp;
  int e_in_middle, gemote;

  DEBUGI3("Emote on '"+data[C_CHANNEL]+"' from "+capitalize(data[O_USER]));
  if (member(chans, data[C_CHANNEL]) < 0) return;

  data[O_MUD] = FILTER_TXT(data[O_MUD], 1);
//  chan        = capitalize(data[C_CHANNEL]);
  msg         = trim(FILTER_TXT(data[C_MESSAGE], 1));
  msg         = terminal_colour(msg, PINKFISH_2_DUMB);

  sendername = capitalize(FILTER_TXT(data[C_VISNAME], 1));
  if (lower_case(MUDNAME) != lower_case(data[O_MUD]))
    sendername += "@" + capitalize(data[O_MUD]);

# ifdef __PCRE__
  if (!temote) {
    // If the sending mud did not use $N but the player's name, we
    // substitude it back to $N. Some muds use 'name' some 'name@mud'.
    if (tmp = regmatch(msg, "(?i)^" +
        FILTER_TXT(data[C_VISNAME], 1) + "(@" + data[O_MUD] + ")?\\b(.*)",
        RE_PCRE))
      msg = "$N" + tmp[2];
  }
# else
  // Do it with regreplace() or something. But that is hard, because you
  // don't have a 'ignore case' mode.
# endif
  msg = substitute_name(msg, &gemote, &e_in_middle);
  if (temote) {
    temote[2] = msg;
    temote[1] = substitute_name(temote[1], gemote, e_in_middle);
  }
#if 0
  if (e_in_middle)
    CHANNEL_D->SendToChannel(chan, this_object(),
      SAY, msg, 0, temote);
  else
    CHANNEL_D->SendToChannel(chan, this_object(),
      gemote?GEMOTE:EMOTE, msg, 0, temote);
#endif
  sendername = 0;
}

static void rec_t_emote(mixed data) {
  string vic, msg;
  mixed* temote;
  object t;

  vic = capitalize(FILTER_TXT(data[CT_T_VISNAME], 1));

  // If the emote is targetted at us or we can find the targetted
  // player, prepare channeld's temote array here
  if (lower_case(MUDNAME) == lower_case(data[CT_T_MUD])
      && (t = find_player(lower_case(data[CT_T_USER])))) {

    msg = trim(FILTER_TXT(data[CT_MESS_T], 1));
    msg = terminal_colour(msg, PINKFISH_2_DUMB);

    // temote = ({ t_object, t_txt, s_txt })
    temote = allocate(3);
    temote[0] = t;
    temote[1] = msg;
  }
  else
    vic +=  "@" + capitalize(data[CT_T_MUD]);

  // prepare emote
  data[C_MESSAGE] = implode(efun::explode(data[CT_MESS], "$O"), vic);
  data[C_VISNAME] = data[CT_O_VISNAME];

  data[C_MESSAGE+1..] = ({});
  rec_emote(data, temote);
}

static void send_channel_m(string name, string chan, string msg) { // OK
  msg = implode(efun::explode(msg, "%^"), "%%^^"); // pinkfish escape

  SENDI3(({ "channel-m",       // SERVICE
            5,                 // HOPS
            MUDNAME,           // O_MUD
            lower_case(name),  // O_USER
            0,
            0,
            chan,              // C_CHANNEL
            name,              // C_VISNAME
            msg                // C_MESSAGE
    }));
}

static void send_channel_e(string name, string chan, string msg) { // OK
  msg = implode(efun::explode(msg, "%^"), "%%^^"); // pinkfish escape

  SENDI3(({ "channel-e",       // SERVICE
            5,                 // HOPS
            MUDNAME,           // O_MUD
            lower_case(name),  // O_USER
            0,
            0,
            chan,              // C_CHANNEL
            name,              // C_VISNAME
            msg                // C_MESSAGE
    }));
}

static void send_channel_t(string name, string chan, string msg, string tuser,
  string tuserid, string tmud, string tmsg) { // OK
  msg  = implode(efun::explode(msg,  "%^"), "%%^^"); // pinkfish escape
  tmsg = implode(efun::explode(tmsg, "%^"), "%%^^"); // pinkfish escape

  SENDI3(({ "channel-t",         // SERVICE
            5,                   // HOPS
            MUDNAME,             // O_MUD
            lower_case(name),    // O_USER
            0,
            0,
            chan,                // C_CHANNEL
            tmud,                // CT_T_MUD
            lower_case(tuserid), // CT_T_USER
            msg,                 // CT_MESS
            tmsg,                // CT_MESS_T
            name,                // CT_O_VISNAME
            tuser                // CT_T_VISNAME
    }));
}


//////
// channel who
static void who_reply(mixed data) {
  object ob;

  ob = find_player(lower_case(data[D_USER]));
  if (!ob) return;

  data[CWHO] = filter(data[CWHO], #'stringp /*'*/);
  if (!sizeof(data[CWHO])) {
    tell_object(ob, "Niemand hoert auf '" + data[C_CHANNEL] +
      "' im " + data[O_MUD] + ".\n");
  }
  else {
    tell_object(ob, "Auf '" + data[C_CHANNEL] +  "' lauschen im "
      + data[O_MUD] + ": " + L_STRING->countup(data[CWHO]) + ".\n");
  }
}

static void who_req(mixed data) { // OK
  object pl;
  mixed who;
  int i;

  if (member(chans, data[C_CHANNEL]) > 0) {
    who = filter(CHANNEL_D->ChannelListeners(data[C_CHANNEL]),
                 (: !$1->Query(P_INVIS) :));
    who = implode(map(who, (: $1->Query(P_REALNAME) :) ), " ");
#if 0
    who = CHANNEL_D->GetUserIDs(capitalize(data[C_CHANNEL]), this_object());
#endif
    if (stringp(who)) {
      who = efun::explode(who, " ");
      for (i=sizeof(who); i--;) {
        pl = find_player(who[i]);
        if (!pl) {
          who[i] = 0;
        }
        else who[i] = pl->Query(P_NAME); //name(RAW);
        if (!stringp(who[i])) who[i] = 0;
      }
      who -= ({ 0 });
    }
    else who = ({});
  }
  else who = ({});

  SENDI3(({ "chan-who-reply", // SERVICE
            5,                // HOPS
            MUDNAME,          // O_MUD
            0,
            data[O_MUD],      // D_MUD
            data[O_USER],     // D_USER
            data[C_CHANNEL],  // C_CHANNEL
            who               // CWHO
    }));
  return;
}

void send_chan_who_req(string chan, string targetmud, string userid) { // OK
//  if (object_name(previous_object()) != IMUD_CONNECT) return;

  SENDI3(({ "chan-who-req",     // SERVICE
            5,                  // HOPS
            MUDNAME,            // O_MUD
            userid,             // O_USER
            targetmud,          // D_MUD
            0,
            chan                // C_CHANNEL
    }));
}

//////
// some general functions

int query_chanlist_id() {
  return chanlist_id;
}

mapping query_channels() {
  return deep_copy(channels);
}

string* query_listened_chans() {
  return m_indices(chans);
}

// This function tries to substitude $N with sendername intelligently.
// Genitiv emotes are recognised as such and the gemote flag is set.
// If the messages does not start with the $N, e_in_middle is set.
static string substitute_name(string msg, int gemote, int e_in_middle) {
  string* tmp;

  tmp = efun::explode(msg, "$N");

  if (sizeof(tmp) > 1) {

    if (tmp[1][0..1] == "s ") {
      tmp[1] = tmp[1][1..];
      gemote = 1;
    }
    else if (tmp[1][0..2] == "'s ") {
      tmp[1] = tmp[1][2..];
      gemote = 1;
    }

    if (sizeof(tmp[0]) || tmp[1][0]!=' ') {
      e_in_middle = 1;
      // The 'from' is removed because the originator's name starts
      // allways the line with our channeld
      if (tmp[<1] == ")" && tmp[<2][<7..] == " (from ") {
        tmp[<2] = tmp[<2][0..<8];
        tmp[<1..<1] = ({});
      }
      msg = implode(tmp, sendername);
    }
    else {
      if (sizeof(tmp) > 2) {
        if (tmp[<1] == ")" && tmp[<2][<7..] == " (from ") {
          tmp[<2] = tmp[<2][0..<8];
          tmp[<1..<1] = ({});
        }
        msg = implode(tmp[1..], sendername);
      }
      else msg = tmp[1];
    }
  }
  else {
    /* nothing */
  }
  return msg;
}

//////
// strongly mudlib dependant helper functions

// outbound messages get here first (from channeld)
public int ChannelSend(string chan, mixed sname, mixed msg, int cmd, mixed* temote) {
  if (!previous_object() || load_name(previous_object())!=CHANNEL_D)
    RAISE_ERROR(sprintf("Unerlaubtes PO [%O] fuer ChannelSend()\n",
      previous_object()));

  if (member(chans, chan) < 0) return 0;

  if (pointerp(msg)) msg = msg[1];       // normal text
  if (pointerp(sname)) sname = sname[0]; // visname
#if 0
  if (cmd == VEMOTE && pointerp(temote) && pointerp(temote[0])) {
    send_channel_t(sname, chan, msg, temote[0][0], temote[0][3], temote[0][1],
      temote[1]);
  }
  else if (cmd == EMOTE || cmd == VEMOTE) {
    send_channel_e(sname, chan, "$N " + msg);
  }
  else if (cmd == GEMOTE) {
    send_channel_e(sname, chan, "$N's " + msg);
  }
  else {
    send_channel_m(sname, chan, msg);
  }
#endif
  return 1;
}

nomask void NotifyChannelChange(string name, mixed *users) { // OK
  if (state != 1)
    return;

  if (member(chans, name) < 0)
    return;

  if (!pointerp(users))
    return;
  if (!sizeof(users - ({ 0, this_object() })))
    send_channel_listen(name, 0);
  else
    send_channel_listen(name, 1);
}
