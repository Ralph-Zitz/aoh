#include "../bincmd.h"

inherit BASICXCMD;

static object msgto;

QueryAbrev() { return "xms"; }

/* we need to destruct ourself, because of input_to */
QueryDestruct() { return 0; }

void XMsgSay(string str) {
  if(str=="."||str=="**") {
    tell_object( cloner,"[End of message].\n");
    say("[End of message]\n");
    remove();
  } else {
    say(str+"\n");
    input_to("XMsgSay");
  }
}

void XMsgTell(string str) {
  if(str=="."||str=="**") {
    tell_object(cloner, "[End of message]\n");
    tell_object(msgto, "[End of message]\n");
    remove();
  } else {
    tell_object(msgto, str+"\n");
    input_to("XMsgTell");
  }
}

void XMsgShout(string str) {
  if(str=="."||str=="**") {
    tell_object( cloner,"[End of message]\n");
    shout("[End of message]\n");
    remove();
  } else {
    shout(str+"\n");
    input_to("XMsgShout");
  }
}

int main( string str ) {
  string tmp;

  if ( str == "?" )
    return notify_fail( "xms(g) [to <object>|all]\n"), 0;

  if ( ! str ) {
    tell_object( cloner, "Send message into room.\n" );
    tell_object( cloner, "End message with \".\" or \"**\".\n");
    say("Message from "+capitalize(getuid(cloner))+":\n");
    input_to("XMsgSay");
    return 1;
  } 
  else if ( sscanf( str, "to %s", tmp ) ) {
    if ( msgto = XFindObj( tmp ) ) {
      PrintShort("Send message to: ", msgto);
      tell_object(cloner,"End message with \".\" or \"**\".\n");
      tell_object(msgto, "Message from "+capitalize(getuid(cloner))+" to you:\n");
      input_to("XMsgTell");
    }
    return 1;
  } else if(str=="all") {
    tell_object( cloner, "Send message to all players.\n" );
    tell_object( cloner, "End message with \".\" or \"**\".\n" );
    shout( "Message from "+capitalize(getuid(cloner))+" to all:\n");
    input_to("XMsgShout");
    return 1;
  }
  return 0;
}



