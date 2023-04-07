/* -------------------------------------------------------------------------
 * /std/player/message.c -- The player message handler.
 *
 * Copyright (C) 1997 by One Step Beyond.
 * All Rights Reserved.
 * Permission granted for non-commercial use.
 * -------------------------------------------------------------------------
 * KNOWN BUGS:
 *  - subsequent MMSG_MORE-writes are not mored correctly. This can be
 *    solved by using RegisterCmdFun to call buffer_more_handler.
 * CHANGES
 *  98/07/05 [Mica] implemented MMSG_DISCARD, filter_message for CMSG_ROOM
 */

#pragma no_range_check

#include <msgclass.h>
#include <mxp.h>
#include <newmxp.h>
#include <properties.h>
#include <secure/wizlevels.h>
#include <daemons.h>
#include <macros.h>
#include <driver/regexp.h>
#include <driver/input_to.h>
#include <driver/driver_info.h>
#include <driver/telnet.h>

/* the max. size of the editing-buffer */
#define MAX_BUFFER 5000

/* the max. size of the user-definable class-dep. history */
#define MAX_MSG_HISTORY 20

/* -------------------------------------------------------------------------
 * Prototypes
 * -------------------------------------------------------------------------
 */

public varargs mixed Query( string str, int sc );            /* std/base */
public varargs mixed Set(string pn, mixed arg, int sc);      /* std/base */
private void buffer_message( string msg, int class, int indent ); /* fwd */
static varargs void buffer_more_handler( string arg, int left );  /* fwd */
public varargs void receive( string msg, int class, int indent, int time );
public varargs int CanSee( object env );       /* std/living/description */
void print_eor();                              /* std/player/telnet_neg  */
void print_ga();                               /* std/player/telnet_neg  */
public mixed query_terminal();                 /* std/player/telnet_neg  */
int set_prompt_iacga(int i);                   /* std/player/telnet_neg  */
int query_prompt_iacga();                      /* std/player/telnet_neg  */
public mixed QueryMXP();                       /* std/player/mxp         */
protected string process_mxp(string msg, mapping attributes);
private string calc_item_id(int arg, mapping attributes);
/* -------------------------------------------------------------------------
 * Global vars - saved
 * -------------------------------------------------------------------------
 */

public string pPrompt;              /* How the prompt shall look like */
public string tty;                  /* terminal type */
public int pPageWidth;              /* page width */
public int * pClassMuffs;           /* 'earmuffs' for message classes */
public mapping pClassColors;        /* the class colors, pairs of fg,hg */
public int pNoBuffering;            /* no buffering at all, user setable */
public int pNewChannelStyle;        /* this changed the look of the channels */
public int * pClassHistory;         /* which message classes should be buf'd */

/* -------------------------------------------------------------------------
 * Global vars - not saved
 * -------------------------------------------------------------------------
 */

nosave int nobuf;                   /* no buffering */
nosave string terminal;             /* the actual terminal used by client */
nosave string incmsg;               /* Incomplete message to print */
nosave int promptpend;              /* prompt pending flag */
nosave mixed ** msgbuf;             /* buffer for output */
nosave int editing;                 /* editing flag for user editors */
nosave mapping pColorTranslations;  /* the color map including class colors */

private nosave mapping mxp_tag_names = ([
    VT_MXP_ROOM_NAME:    "rname";      0,
    VT_MXP_ROOM_DESC:    "rdesc";      0,
    VT_MXP_ROOM_EXITS:   "rexits";     0,
    VT_MXP_ROOM_NUM:     "rnum";       0,
    VT_MXP_ROOM_EXP:     "rexpire";    0,
    VT_MXP_PROMPT:       "prompt";     0,
    VT_MXP_EX:           "ex";         0,
    VT_MXP_VERSION:      "version";    0,
    VT_MXP_SUPPORT:      "support";    0,
    VT_MXP_IROOMCONTENT: "ircontent";  #'calc_item_id, /*'*/
    VT_MXP_LROOMCONTENT: "lrcontent";  #'calc_item_id, /*'*/
    VT_MXP_IINVENTORY:   "iinventory"; #'calc_item_id, /*'*/
]);

/* -------------------------------------------------------------------------
 * Builtin properties
 * -------------------------------------------------------------------------
 */

public int SetEditing(int f) { return editing=f; }
public int QueryEditing() {
  return ( editing ||
	   objectp(query_input_pending(this_object())) ||
	   objectp(query_editing(this_object())) );
}

/* ---------------------------------------------------------------------- */

public int SetNoBuffering( int n ) { return pNoBuffering = n; }
public int QueryNoBuffering() { return pNoBuffering; }

/* ---------------------------------------------------------------------- */

public int SetNewChannelStyle( int n ) { return pNewChannelStyle = n; }
public int QueryNewChannelStyle() { return pNewChannelStyle; }

/* ---------------------------------------------------------------------- */

public string SetPrompt(string s) {

    return pPrompt = s;
}
public string QueryPrompt() { return pPrompt||"> "; }

/* ---------------------------------------------------------------------- */

public string SetTTY(string str) {
  if (member(({"dumb","vt100","ansi"}),(str = lower_case(str||"")))==-1)
    return 0;
  return tty = str;
}
public string QueryTTY(string str) { return tty; }

public string SetTerminal(string str) {
  mixed term = query_terminal();
  if (pointerp(term) && sizeof(term) > 1 && sizeof(term[1]) > 0)
    terminal = term[1][0];
  else
    terminal = str;
  if (terminal && sizeof(terminal) && strstr(terminal, "mudlet") == 0)
    set_prompt_iacga(1);
  return terminal;
}
public string QueryTerminal() { return terminal; }

/* ---------------------------------------------------------------------- */

public int QueryPageWidth() { return pPageWidth ? pPageWidth : 80; }
public int SetPageWidth( int w ) { return pPageWidth = w; }

/* ---------------------------------------------------------------------- */

/* handling for msg_class earmuffs */

public int * SetClassMuffs( int * m ) {
  if ( pointerp(m) )
    return pClassMuffs = m + ({});
  else
    return pClassMuffs = 0;
}

public int * QueryClassMuffs() { return pClassMuffs?pClassMuffs+({}):0; }

public void AddClassMuff( int class ) {
  if ( pClassMuffs ) {
    if ( member( pClassMuffs, class ) == -1 )
      pClassMuffs += ({ class });
  }
  else
    pClassMuffs = ({ class });
}

public void RemoveClassMuff( int class ) {
  if ( pClassMuffs )
    pClassMuffs -= ({ class });

  if ( ! sizeof( pClassMuffs ) )
    pClassMuffs = 0;
}

/* ---------------------------------------------------------------------- */

/* handling for msg_class dep. buffering */

public int * SetClassHistory( int * m ) {
  if ( pointerp(m) )
    return pClassHistory = m + ({});
  else
    return pClassHistory = 0;
}

public int * QueryClassHistory() { return pClassHistory?pClassHistory+({}):0; }

/* ---------------------------------------------------------------------- */

/* builtin property for message class coloring */

public mapping SetClassColors( mapping m ) {
  if ( mappingp( m ) ) {
    pClassColors = m;
    return pClassColors + ([]);
  }
  else
    return pClassColors = 0;
}

public mapping QueryClassColors() { return pClassColors?pClassColors+([]):0; }

public varargs void AddClassColor( int class, int fg, int bg, int hlt ) {
  int * tx;

  tx = ({ fg, bg, hlt });

  if ( pClassColors )
    pClassColors[class] = tx;
  else
    pClassColors = ([ class : tx ]);
}

public void RemoveClassColor( int class ) {
  if ( pClassColors )
    m_delete( pClassColors, class );
}

/* --------------------------------------------------------------------------
 * SendMsgHistory - writes the contents of P_MSG_HISTORY (not builtin)
 *  num : the last num messages
 *  cfilter : optional filter
 * --------------------------------------------------------------------------
 */

private int cfilter_helpfun( mixed * entry, int * cfilter ) {
  if ( ( ! pointerp( cfilter ) )
       || ( member( cfilter, entry[1] ) != -1 ) )
    return 1;
  else
    return 0;
}

public varargs void SendMsgHistory( int num, int * cfilter ) {
  mixed * tx;

  tx = filter( Query( P_MSG_HISTORY )||({}),
                     SF( cfilter_helpfun ), cfilter );

  if ( ! sizeof( tx ) ) {
    msg_object( this_object(), CMSG_GENERIC,
                "Your history buffer is currently empty.\n" );
    return;
  }

  if ( ( num >= sizeof( tx ) ) || ( num <= 0 ) )
    num = 0;
  else
    num = sizeof( tx ) - num;

  msg_object( this_object(), CMSG_GENERIC|MMSG_MORE,
              "--- History.\n" );

  for( ;num < sizeof( tx ); num++ )
    apply( SF( receive ), tx[num] );

  msg_object( this_object(), CMSG_GENERIC|MMSG_MORE,
              "--- End of history.\n" );
}

/* --------------------------------------------------------------------------
 * Colour translations for the different terminal types
 * NOTE: this is not a property to prevent xprops messing the users coloring
 * --------------------------------------------------------------------------
 */

public mapping GetAllColorTranslations() {
  return pColorTranslations ? copy(pColorTranslations) : 0;
}

public mapping GetColorTranslation( int class ) {
  mapping rc;

  if ( ! mappingp( pColorTranslations ) ) return ([]);
  else if ( ! mappingp( pColorTranslations[tty] ) ) {
    if ( ! pColorTranslations["dumb"] )
      return ([]);
    else
      rc = pColorTranslations["dumb"];
  }
  else
    rc = pColorTranslations[tty];

  /* and add RESET for current class */
  rc["RESET"] = rc["cmsg_"+(STRING_CMSG[class&MMSG_MASKOUT])];

  return rc;
}

/* -------------------------------------------------------------------------
 * no buffering handling for converse-mode etc.
 *  this is _no_ property! that is the reason for the different naming
 *  it checks both the player setable value pNoBuffering and the internal
 *  nobuf
 * -------------------------------------------------------------------------
 */

public int set_no_buffering( int n ) { return nobuf = n; }
public int query_no_buffering() { return (nobuf||pNoBuffering); }

/* ------------------------------------------------------------------------
 * add_class_tokens (private)
 *  adds the class %^-tokens, class has to be masked out with MMSG_MASKOUT
 *  takes care about \n at the end of the string
 * ------------------------------------------------------------------------
 */

private string add_class_tokens( string msg, int class ) {
  if ( sizeof(msg) && msg[<1] == '\n' )
    return
      "%^cmsg_"+STRING_CMSG[class]+"%^" +
      msg[0..<2] +
      "%^cmsg_generic%^" +
      "\n";
  else
    return
      "%^cmsg_"+STRING_CMSG[class]+"%^" +
      msg +
      "%^cmsg_generic%^";
}

/* ------------------------------------------------------------------------
 * translate (private)
 *  this function 'translates' the color macros and returns the result
 * ------------------------------------------------------------------------
 */

private varargs string translate( mixed msg, int class, int indent ) {
  mapping trans;

  /* if we get called with an array, indent is unused and we add MMSG_NOWRAP
   * (hack to speed it up for the buffer more handler)
   */
  if ( pointerp( msg ) ) {
    class = msg[1]&MMSG_MASKOUT;
    msg = add_class_tokens( msg[0], class );
    class |= MMSG_NOWRAP;
  }

  /* support the identically translate message-modifier here */
  if ( class & MMSG_IDENT_TRANS )
    trans = ({mapping})COLOUR_D->QueryIdentTrans();
  else
    trans = GetColorTranslation( class );

  /* pass it through terminal_colour for macros expansion and wrapping */
  msg = terminal_colour( msg,
			 trans,
			 ( class & MMSG_NOWRAP ) ?
			 0 :
			 ( Query(P_PAGEWIDTH) ),
			 ( class & MMSG_NOWRAP ) ?
			 0 :
			 indent );

  return msg;
}

/* ------------------------------------------------------------------------
 * receive
 *  this actually buffers the message or tells it to the user
 * ------------------------------------------------------------------------
 */

public varargs void receive( string msg, int class, int indent, int time ) {
  /* we have an old incomplete message, add it, we don't care about the
   * msgclass of incomplete msgs, it gets the class of the next complete.
   */

  if ( incmsg ) {
    msg = incmsg + msg;
    incmsg = 0;
  }

  /* we do not collect strings without \n's for MMSG_NOWRAP, MMSG_DIRECT
   * or MMSG_MORE
   */

  if ( ! ( class & (MMSG_NOWRAP|MMSG_DIRECT|MMSG_MORE) ) ) {
    if ( msg[<1] != '\n' ) {
      string * strs;

      strs = explode( msg, "\n" );
      incmsg = strs[<1];
      promptpend = 1; /* ensure it is printed */
      msg = implode(strs[0..<2], "\n");
      if ( msg == "" )
        return; /* might be empty now */
    }
  }

  /* no buffer or tell the message based on class and user status,
   * for MMSG_MORE flush immediatly based on status
   */
  if ( class & MMSG_MORE ) {
    buffer_message( msg, class, indent );
    if ( ( ! Query( P_EDITING ) ) &&
         ( ( sizeof( msgbuf ) < Query( P_PAGESIZE ) - 1 ) ||
           ( ! sizeof( Query( P_ENEMIES ) ) ) ) )
      buffer_more_handler( 0 );
  }
  else if ( ( ! ( class & MMSG_DIRECT ) ) &&
            ( ! query_no_buffering() ) &&
            ( Query( P_EDITING ) ) )
    buffer_message( msg, class, indent );
  else {
    /* no class coloring for ident translations
     */
    if ( ! ( class & MMSG_IDENT_TRANS ) ) {
      /* class color handling for ansi
       */
      msg = add_class_tokens( msg, class & MMSG_MASKOUT );
    }

    /* if time has been given, add it in front and adjust indent */
    if ( time ) {
      msg = "<" + ctime(time)[11..18] + "> " + msg;
      indent = 11;
    }

    msg = translate( msg, class, indent );
    msg = process_mxp(msg, ([]));
    efun::tell_object(this_object(), msg);
  }
}


/* -------------------------------------------------------------------------
 * prettyfile - shortens path to ~&+ synta
 *  When 'short' is given only the current dir is returned
 * -------------------------------------------------------------------------
 * TODO: prettyfile should be moved to secure/kernel (better name :)
 */

protected varargs string prettyfile(string str,int short) {
  string res,name;

  if (!str) return 0;

  if (str[0..2] == DPATH)
    res = "+"+str[3..<1];
  else if (str[0..2] == PPATH)
    res = "&"+str[3..<1];
  else if (str[0..8] == WPATH) {
    res = "~"+str[9..<1];
    name = getuid();
    if (res[1..sizeof(name)]==name)
      res = "~"+res[sizeof(name)+1..];
  }
  else res = str;

  if (short)
    res = explode(res,"/")[<1];

  return res;
}

/* -------------------------------------------------------------------------
 * parse_prompt - (private) expands the user-defined prompt
 * -------------------------------------------------------------------------
 */

private string parse_prompt(string what) {
  if ( member( what , '$' ) != -1 ) {
    string * tx;
    int i;

    /* first of all tokenize the prompt */
    tx = regexplode( what,
                     "\\$h|\\$H|\\$s|\\$S|\\$x|\\$t|\\$T|\\$n"
                     "|\\$p|\\$P|\\$d|\\$c|\\$e", RE_TRADITIONAL) - ({ "" });

    for( i = 0; i < sizeof( tx ); i ++ ) {
      switch( tx[i] ) {
      case "$h":
        tx[i] = to_string( Query(P_HP) );
        break;
      case "$H":
        tx[i] = to_string( Query(P_MAX_HP) );
        break;
      case "$s":
        tx[i] = to_string( Query(P_SP) );
        break;
      case "$S":
        tx[i] = to_string( Query(P_MAX_SP) );
        break;
      case "$x":
        tx[i] = to_string( Query(P_XP) );
        break;
      case "$t":
        tx[i] = ctime(time())[<13..<9];
        break;
      case "$T":
        tx[i] = ctime(time())[<13..<6];
        break;
      case "$n":
        tx[i] = "\n";
      }
      if ( IS_IMMORTAL( this_object() ) ) {
        switch( tx[i] ) {
        case "$P":
          tx[i] = Query(P_CURRENTDIR);
          break;
        case "$p":
          tx[i] = prettyfile(Query(P_CURRENTDIR));
          break;
        case "$d":
          tx[i] = prettyfile(Query(P_CURRENTDIR),1);
          break;
        case "$e":
          tx[i] = prettyfile(object_name(environment( this_object() )));
          break;
        case "$c":
          tx[i] = to_string((driver_info(DI_CURRENT_RUNTIME_LIMITS)[0])-get_eval_cost());
          break;
        }
      }
    }

    what = implode( tx, "" );
  }

  if ( what[<1] != ' ' )
    what+=" ";

  return what;
}

/* -------------------------------------------------------------------------
 * print_prompt - print out the prompt
 *  This function is called by the gd (set_prompt closure) and we call it
 *  outselves during heartbeat for pending prompts.
 * -------------------------------------------------------------------------
 */

public varargs mixed print_prompt() {
  mixed value;

  if ( ( msgbuf ) &&
       ( ( sizeof( msgbuf ) < Query( P_PAGESIZE ) - 1 ) ||
         ( ! sizeof( Query( P_ENEMIES ) ) ) ) ) {
    buffer_more_handler( 0 );
  }
  else {
    promptpend = 0;

    /* query our prompt value */
    value = Query( P_PROMPT );

    /* it maybe a closure, evaluate it with funcall */
    if (closurep(value))
      value = funcall(value);

    /* if we now have a prompt-string, process it to expand the macros,
     * else set an error prompt
     */
    if (stringp(value))
      value = parse_prompt(value);
    else
      value = "*Error> ";

    /* we might have an incomplete message, add it in front of the prompt */
    if ( incmsg ) {
      value = incmsg + "\n" + value;
      incmsg = 0;
    }
    value = process_mxp(MSG_PROMPT(value), ([]));
    /* translate the color macros and do the output */
    value = translate(value, CMSG_GENERIC, 0);
    if (query_prompt_iacga()) {
        binary_message(to_bytes(value, "UTF-8"), 1);
        print_ga();
    }
    else {
        efun::tell_object(this_object(), value);
        print_eor();
    }
  }

  /* the gd expects string return value for prompt closure, but we return
   * 0. That results in no prompt beeing printed.
   */
  return 0;
}

/* -------------------------------------------------------------------------
 * the buffered message more functions
 * -------------------------------------------------------------------------
 */

static varargs void buffer_more_handler( string arg, int left ) {
  int psize, ts;
  string tmp;

  if ( ( ! arg ) || ( arg == "" ) ) {
    /* get a chunk of lines from the buffer */
    psize = Query( P_PAGESIZE );
    if (psize > sizeof(msgbuf) - 1)
      psize = sizeof(msgbuf);
    tmp = implode( map( msgbuf[0..(psize-1)]
                              , #'translate /*'*/ ), "\n" );
    if ( tmp[<1] != '\n' )
      tmp += "\n";

    if (sizeof(msgbuf) > psize)
      msgbuf = msgbuf[psize..];
    else
      msgbuf = msgbuf[sizeof(msgbuf)..];
    ts = sizeof( msgbuf );

    if ( ! sizeof( msgbuf ) )
      msgbuf = 0;
    /* fallthough */
  }
  else if ( arg == "p" ) {
    tmp = implode( map( msgbuf,
                              #'translate /*'*/ ), "\n" );
    if ( tmp[<1] != '\n' )
      tmp += "\n";

    msgbuf = 0;
    /* fallthough */
  }
  else if ( arg == "q" ) {
    tmp = 0;
    msgbuf = 0;
    /* fallthrough */
  }
  else if ( arg == "?" ) {
    efun::tell_object( this_object(),
		       "Accepted commands are:\n"
		       "[CR]    : next page\n"
  		       "p       : print all remaining text\n"
		       "?       : this help\n"
		       "q       : aborts bmore\n" );
    efun::tell_object( this_object(),
		       sprintf( "=== BMore: %d lines left [CR,p,q,?] ",
			        left ) );
    input_to( "buffer_more_handler", 0, left, INPUT_PROMPT );
    if (query_prompt_iacga())
        print_ga();
    else
        print_eor();
    return;
  }
  else {
    efun::tell_object( this_object(),
		       sprintf( "=== BMore: %d lines left [CR,p,q,?] ",
			        left ) );
    input_to( "buffer_more_handler", 0, left, INPUT_PROMPT );
    if (query_prompt_iacga())
      print_ga();
    else
      print_eor();
    return;
  }

  if ( tmp )
    efun::tell_object( this_object(), tmp );

  if ( msgbuf ) {
    efun::tell_object( this_object(),
		       sprintf( "=== BMore: %d lines left [CR,p,q,?] ",
				ts ) );
    input_to( "buffer_more_handler", 0, ts, INPUT_PROMPT );
    if (query_prompt_iacga())
      print_ga();
    else
      print_eor();
    return;
  }
}

/* -------------------------------------------------------------------------
 * buffer_message - this (private) function handles the msgbuf
 * -------------------------------------------------------------------------
 */

/* helpfun to make string/class pairs
 */
private mixed * buffer_add_class( string str, int class ) {
  return ({ str, class });
}

private void buffer_message( string msg, int class, int indent ) {
  string * tx;
  int nc, mc;

  promptpend = 1; /* ensure it is written */

  /* we need to translate identically for indent and wrap
   */
  msg = translate( msg, class|MMSG_IDENT_TRANS, indent );

  /* explode msg's to \n to have a clean line counting for the later more */
  tx = explode( msg, "\n" );
  if ( msg[<1] == '\n' )
    tx = tx[0..<2]; /* this results in one \n too much for us */

  nc = sizeof( tx );
  if ( nc > MAX_BUFFER )
    tx = tx[(nc-MAX_BUFFER)..];

  if ( ! msgbuf ) {
    msgbuf = map( tx, #'buffer_add_class /*'*/,
                        class & MMSG_MASKOUT );
    return;
  }

  mc = sizeof( msgbuf ) + nc;
  if ( mc > MAX_BUFFER )
    msgbuf = msgbuf[(mc-MAX_BUFFER)..];

  msgbuf +=
    map( tx, #'buffer_add_class /*'*/,
               class & MMSG_MASKOUT );
}

/* -------------------------------------------------------------------------
 * heart_beat - gets called by std/player's heart_beat
 *  This flushes the message buffer and prints the prompt if needed
 * -------------------------------------------------------------------------
 */

public void heart_beat() {
  if ( ( ! Query( P_EDITING ) ) &&
       ( promptpend ) )
    print_prompt();
}

/* -------------------------------------------------------------------------
 * receive_msg - the main message handling fun
 * -------------------------------------------------------------------------
 * class is a message class from msgclass.h
 * from is the originating object
 * data is the message data, type depends on class
 *
 * NOTE: IMPORTANT! reply msg_xxx need to take care about MMSG_NOREPLY
 */

public void receive_msg( int class, object from, mixed data ) {
  int indent;
  string msg; /* the string that is to be told the player */
  string tmp;
  mixed * tx;

  if ( ! data ) return; /* invalid call */

  /* handle class-muffs */
  if ( pClassMuffs && ( member( pClassMuffs, class ) != -1 ) )
    return;

  /* Special handling for CMSG_ROOM, needs to be done before the class
   * matching because it influences MMSG_SEE
   */

  if ( ( class & MMSG_MASKOUT ) == CMSG_ROOM ) {
    int newclass;

    if ( ! pointerp( data ) )
      data = ({ data, 0 }); /* need an array to pass a reference */
    else
      data = data + ({}); /* we create a copy here to prevent problems */

    /* if call was successfull we use the returned msgclass,
     * data can be modified because it is passed by reference
     */
    if ( call_resolved( &newclass, environment(), "filter_message", class, data ) )
      class = newclass;

    /* if no MMSG_SEE is given convert the array back to a string */
    if ( ! ( class & MMSG_SEE ) )
      data = data[0];
  }

  /* handle MMSG_DISCARD */
  if ( class & MMSG_DISCARD )
    return;

  /* handle message modifiers */
  if ( ( class & MMSG_SEE ) && ( environment() ) ) {
    if ( CanSee() ) {
      if ( pointerp( data ) && sizeof( data ) )
        data = data[0];
    }
    else {
      if ( pointerp( data ) && sizeof( data ) >= 2 )
        data = data[1];
      else
        data = 0;
    }
  }
  else if ( class & MMSG_HEAR ) {
    if ( pointerp( data ) && sizeof( data ) )
      data = data[0]; /* currently everybody hears everything */
  }
  else if ( class & MMSG_SMELL ) {
    if ( pointerp( data ) && sizeof( data ) )
      data = data[0]; /* currently everybody smells everything */
  }

  if ( ! data ) return; /* no data, for example nothing to see */

  indent = 0; /* default : no indent */
  switch( class & MMSG_MASKOUT ) {
    case CMSG_CHANNEL:
      if ( pointerp( data ) && ( sizeof( data ) == 4 ) ) {
        data[1] = capitalize(data[1]||"");
        if ( pNewChannelStyle ) {
          /* add . if not .?!)( at the end of the data[2] */
          if ( member( ".?!:;,",  data[2][<1]) == -1 )
            data[2] += ".";
            if ( data[3] ) {
            /* nasty hack for emotes on news/title by the player object */
              msg = "["+data[0]+"] "+((data[1]=="")?"":data[1]+" ")+data[2]+"\n";
            }
	          else
	            msg = "["+data[0]+"] "+((data[1]=="")?"":data[1]+": ")+data[2]+"\n";
        }
        else {
          /* handle default channel for players/wizards */
          if ( ( IS_IMMORTAL( this_object() ) ) &&
            ( data[0] == "general" ) &&
            ( data[1] != "" ) )
            tmp = data[1];
          else if ( ( ! IS_IMMORTAL( this_object() ) ) &&
            ( data[0] == "players" ) &&
            ( data[1] != "" ) )
            tmp = data[1];
          else {
            if ( data[3] )
              tmp = data[0]+":"+data[1];
            else {
              if ( data[1] != "" )
                tmp = data[1]+":"+data[0];
              else
                tmp = data[0];
            }
          }
          /* make message depending on emote flag */
          if ( data[3] ) {
            msg = "["+tmp+((data[1]=="")?"":" ")+data[2]+"]\n";
          }
          else
            msg = "["+tmp+"] "+data[2]+"\n";
        }
        indent = 3;
      }
      else
        msg = 0; /* invalid data */
      break;
    case CMSG_TELL:
      if ( ! ( class & MMSG_NOREPLY ) ) {
        if ( from && interactive(from) && !Query( P_INVIS ) ) {
          if ( ( ! ( class & MMSG_DIRECT ) )
            && ( ! query_no_buffering() )
            && ( Query( P_EDITING ) ) ) {
              msg_object( from, CMSG_TELL|MMSG_NOREPLY|MMSG_DIRECT,
                capitalize(getuid(this_object()))+" is currently editing, your tell has been buffered.\n" );
          }
          else if ( tmp = Query( P_AWAY ) ) {
            msg_object( from, CMSG_TELL|MMSG_NOREPLY|MMSG_DIRECT,
              capitalize(getuid(this_object()))+" is away, "+tmp+".\n");
          }
        }
      }
      indent = 3;
      msg = data;
      break;
    case CMSG_SAY:
      indent = 3;
      msg = data;
      break;
    default:
      msg = data;
  }

  if ( stringp( msg ) && ( msg != "" ) ) {
    /* check if we should be buffered */
    if ( pointerp( pClassHistory )
         && ( member( pClassHistory, class&MMSG_MASKOUT ) != -1 ) ) {
      if ( pointerp( tx = Query( P_MSG_HISTORY ) ) ) {
        if ( sizeof( tx ) > MAX_MSG_HISTORY )
          tx = tx[1..];
        tx += ({ ({ msg, class&MMSG_MASKOUT, indent, time() }) });
      }
      else
        tx = ({ ({ msg, class&MMSG_MASKOUT, indent, time() }) });
      Set( P_MSG_HISTORY, tx );
    }

    /* and receive it finally */
    receive( msg, class, indent );
  }
}

/* -------------------------------------------------------------------------
 * compat function Receive
 * -------------------------------------------------------------------------
 */

public varargs void Receive( string str ) {
  /* if there is data, pass it as generic message though the handler */

  if ( str && str != "" )
    receive( str, CMSG_GENERIC|MMSG_DIRECT ); /* do not buffer that */
}

/* -------------------------------------------------------------------------
 * Initialize color translation mappings
 * TODO: InitColourTranslation could be made more efficient
 * -------------------------------------------------------------------------
 */

public void InitColourTranslation() {
  if ( ! mappingp( pColorTranslations ) )
    pColorTranslations = ([]);

  if ( ! pClassColors )
    AddClassColor( -1, 7, 0, 0 ); /* set black on white as default */

  pColorTranslations["dumb"] =
    ({mapping})COLOUR_D->QueryTTYTrans("dumb") +
    ({mapping})COLOUR_D->QueryClassTrans( pClassColors, "dumb" );

  pColorTranslations["vt100"] =
    ({mapping})COLOUR_D->QueryTTYTrans("vt100") +
    ({mapping})COLOUR_D->QueryClassTrans( pClassColors, "vt100" );

  pColorTranslations["ansi"] =
    ({mapping})COLOUR_D->QueryTTYTrans("ansi") +
    ({mapping})COLOUR_D->QueryClassTrans( pClassColors, "ansi" );
}

protected string process_mxp(string msg, mapping attributes) {
    msg = regreplace(msg, VT_ESC "\\[![0-9]+(;[0-9]+)*[st]",
            function string(string sub) {
                if (!QueryMXP())
                    return "";
                int typ_char = sub[<1];
                int is_close;

                if (typ_char == VT_MXP_OPEN_CHAR)
                    is_close = 0;
                else if (typ_char == VT_MXP_CLOSE_CHAR)
                    is_close = 1;
                else
                    return sub;

                int *args = map(explode(sub[3..<2], ";"), #'to_int /*'*/);
                int code = args[0];
                string tag_name = mxp_tag_names[code];
                closure tag_arg_fun = mxp_tag_names[code, 1];

                if (!tag_name)
                    return "";

                string tag_arg = "";
                if (!is_close && sizeof(args) > 1) {
                    tag_arg = funcall(tag_arg_fun, args[1], attributes);
                    if (!tag_arg)
                        return 0;
                }
                string ret = VT_MXP_TEMP_SECURE_MODE
                    + (is_close ? "</" : "<")
                    + tag_name
                    + tag_arg
                    + ">"
                    + VT_MXP_LOCK_LOCKED_MODE;
                return ret;
            },
    RE_GLOBAL|RE_PCRE);
    return msg;
}

private string calc_item_id(int arg, mapping attributes) {
    return " id=\"sword\"";
}

/* -------------------------------------------------------------------------
 * EOF
 * -------------------------------------------------------------------------
 */
