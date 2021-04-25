inherit "std/thing";
#include <properties.h>
#include <secure/wizlevels.h>
#include <config.h>
#include <secure/udp.h>

#define TP this_player()

static int count;

create() {
  seteuid(getuid());
  (::create());
  SetShort("Intermud Command Tool");
  SetLong("\
This is a tool for using intermud commands.\n\
The following commands can be used:\n\
muds\n\
iwho <mudname>\n\
ifinger <player>@<mud>\n\
itell <player>@<mud> <msg>\n\
imsg <channel> <msg>\n\
imote <channel> <msg>\n\
(end of command list)\n");
  SetIds(({"tool","intermud","netcmds"}));
  SetAds(({"intermud","command"}));
  SetValue(0);
}


init()
{
  (::init());
  add_action("cmd_muds","muds");
  add_action("cmd_who","iwho");
  add_action("cmd_finger","ifinger");
  add_action("cmd_tell","itell");
  add_action("cmd_chanmsg","imsg");
  add_action("cmd_chanemote","imote");
  count = 0;
}

/*-------------------------------------------------------------------------
**  Stringformatting
*/

string mkstr (int len) {
  string rc, pattern;
  int actlen;

  if (len <= 0) return "";
  pattern = "          ";
  rc = pattern;
  actlen = strlen(pattern);
  while (actlen < len) rc += rc, actlen *= 2;
  return rc[0..len-1]; 
}

string ladjust (string str, int len) {
  int actlen;

  if (!str || len <= 0) return 0;
  if ((actlen = strlen(str)) >= len) return str;
  return str+mkstr(len-actlen);
}

string radjust (string str, int len) {
  int actlen;

  if (!str || len <= 0) return 0;
  if ((actlen = strlen(str)) >= len) return str;
  return mkstr(len-actlen)+str;
}

int cmd_muds() {
    mapping hosts;
    int i;
    mixed muds;

    muds = ladjust("Mudname", 20) + "  Status   Last access";
    write(muds + "\n"+
"-----------------------------------------------------------------"[0..strlen(muds)] + "\n");
    muds = sort_array(m_indices(hosts = INETD->query("hosts")), #'>);
    for(i = 0; i < sizeof(muds); i++) {
	write(ladjust(hosts[muds[i]][HOST_NAME], 20) + "  " +
	(hosts[muds[i]][HOST_STATUS] ?
	    hosts[muds[i]][HOST_STATUS] > 0 ?
		"UP       " + ctime(hosts[muds[i]][HOST_STATUS])[4..15] :
		"DOWN     " + ctime(-hosts[muds[i]][HOST_STATUS])[4..15]
	    : "UNKNOWN  Never accessed.") +
	"\n");
    }
    return 1;
}


int cmd_who(string str) {
    string s;
    if (str) {
	if (s = INETD->send_udp(str,
				([ REQUEST: "who",
				 SENDER: this_player()->query_real_name() ]),
				1))
	    write(s);
	else
	    write("Request transmitted.\n");
    }
    return 1;
}


int cmd_finger(string arg) {
    string s, who, mud;

    if (arg && sscanf(arg, "%s@%s", who, mud) == 2) {
	if (s = INETD->send_udp(mud, ([
	    REQUEST: "finger",
	    SENDER: this_player()->query_real_name(),
	    DATA: who
	]), 1))
	    write(s);
	else
	    write("Request transmitted.\n");
    }
    else
	write("Local not yet implemented\n");
    return 1;
}


cmd_tell(str) {
    string s, who, msg;
    if (!str || sscanf(str, "%s %s", who, msg) != 2)
	write("Tell what ?\n");
    else {
	/* This part handles the intermud tell. */
	object ob;
	int i;
	string mudname, user;
	if (sscanf(who, "%s@%s", user, mudname) == 2) {
	    if (s = INETD->send_udp(mudname, ([
		REQUEST: "tell",
		RECIPIENT: user,
		SENDER: this_player()->query_real_name(),
		DATA: msg
	    ]), 1))
	        write(s);
	    else
		write("Message transmitted.\n");
	    return 1;

	/* End of intermud tell routine. */

	}
	write("Local tell not covered by this command\n");
    }
    return 1;
}

cmd_chanemote(str)
{
    string ch, msg;
    if (!str || sscanf(str, "%s %s", ch, msg) != 2)
	write("Say on which channel what ?\n");
    SendChannelMsg(ch, msg, this_player()->query_real_name(), 1);
    return 1;
}

cmd_chanmsg(str)
{
    string ch, msg;
    if (!str || sscanf(str, "%s %s", ch, msg) != 2)
	write("Say on which channel what ?\n");
    SendChannelMsg(ch, msg, this_player()->query_real_name(), 0);
    return 1;
}

void SendChannelMsg(string ch, string msg, string wh, int is_emote)
{
    mapping data, hosts;
    string *muds;
    int i;

    data = ([
	    REQUEST: "channel",
	    SENDER: wh,
	    "CHANNEL": ch,
	    DATA: msg
    ]);
    if (is_emote)
	data["EMOTE"] = 1;
    for(i = sizeof(muds = m_indices(hosts = INETD->query("hosts")));
		i--; ) {
	    /*
	     * Don't send it to ourselves and check that the receiver
	     * recognizes "channel" requests. Normally this checking is done
	     * by the inetd, but we don't want it throwing out
	     * 'invalid command' messages for muds that don't use channels
	     * every time we send an intermud message.
	     */
	if ( /* muds[i] != lower_case(MUDNAME) && */
	    (member(hosts[muds[i]][HOST_COMMANDS], "*") != -1 ||
	    member(hosts[muds[i]][HOST_COMMANDS], "channel") != -1))
	    INETD->send_udp(muds[i], data);
    }
}

