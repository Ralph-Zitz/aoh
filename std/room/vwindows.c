/*--------------------------------------------------------------------------
 * /std/room/vwindows.c -- The virtual windows
 *
 * Copyright (C) 1997 by One Step Beyond.
 * All Rights Reserved.
 * Permission granted for non-commercial use.
 *--------------------------------------------------------------------------
 * PV_WINDOW_CHECKFUN:
 *  This can eiter be a string, an array or a closure. Beeing a string the
 *  function is called with call_other( this_object, <string>, ...), a
 *  closure is called with funcall and an array is always evaluated with
 *  apply enable you to give extra paramters. The array either be in the form
 *  ({ <stringfun>, ... }), ({ <object>, <stringfun>, ... }) or
 *  ({ <closure>, ... }), given extra parameters will be appended to the call
 *
 *  The checkfun itself has to return WINDOW_CHECK_OK if the action is
 *  permitted and WINDOW_CHECK_FAIL or a string/array if it is denied.
 *  string will be written to the current user, string array will be written
 *  to user([0]) and room([1]), it is possible to give string-arrays for [0]
 *  and [1], then MMSG_SEE is used.
 *
 *  The function is called with the following paramters:
 *    check_fun( string action, string v_id );
 *   with action beeing on of the WINDOW_ACTION_-values and v_id the windows
 *   vitem id. Check is always done for this_player()
 *
 *  Checkfuns are called only if the action would take place, which means
 *  that if someone tries to open an already open window he will get the
 *  normal failure message and the checkfun won't be called.
 *
 * NOTES:
 *  The window syncs during AddWindow with the other room is there is one
 *  which is already loaded and has a matching window. Synced are the state
 *  and the transparency
 *
 * KNOWN BUGS:
 *  The window always gets dirty from both sides and once cleaned, both
 *  sides are cleaned, not caring about the side the player is standing.
 *  It would be possible to adjust, but imho that would be way too
 *  realistic and anoy players. [Mica, 15Jul97]
 *
 * CHANGES:
 *  05/26/98 Mica PV_WINDOW_OTHERLONG is now shown always if defined, only
 *                if it is empty the long description of the target room is
 *                shown (PV_WINDOW_PATH)
 *
 * TODO:: Windows commands should only work if the player is able to see
 * TODO::  the window
 * TODO:: reset() issues wrong PV_WINDOW_SHORT to the other room
 * TODO:: vwindows should catch errors for not existing PV_WINDOW_PATH
 */

#include <properties.h>
#include <msgclass.h>
#include <moving.h>
#include <macros.h>
#include <nightday.h>
#include <vitems.h>

/* --------------------------------------------------------------------------
 * global variables
 * --------------------------------------------------------------------------
 */

private object lastowin;
private string lasto_id;

/* --------------------------------------------------------------------------
 * prototypes
 * --------------------------------------------------------------------------
 */

public varargs mixed Query(string pn, int sc);

public mixed VQuery( string id, string prop );
public mixed VSet( string id, string prop, mixed val );
public varargs mixed GetVItem( string str, mixed prop );
public mapping ids_n_ads( mixed det );
public varargs mixed AddVItem( mapping det );

/* --------------------------------------------------------------------------
 * helpfuns for output
 * --------------------------------------------------------------------------
 */

public string WindowLastShort( string v_id ) {
  string * ads;

  ads = VQuery( v_id, P_LAST_ADS );
  return (sizeof(ads)?(implode(ads, " ")+" "):"")+VQuery( v_id, P_LAST_ID );
}

public string WindowShort( string v_id ) {
  return VQuery( v_id, PV_WINDOW_SHORT );
}

public string OWindowShort() {
  return ({string})lastowin->VQuery( lasto_id, PV_WINDOW_SHORT );
}

/* --------------------------------------------------------------------------
 * helpfuns for window sync
 * --------------------------------------------------------------------------
 */

static void filter_window_find_otherwindow( string v_id, mapping d_data,
                                     string o_id ) {
  if ( d_data[PV_WINDOW_PATH] == object_name( this_object() ) )
    o_id = v_id;
}

static string window_find_otherwindow( string v_id, object ob ) {
  mixed o_id;
  string path;

  if ( ! path = VQuery( v_id, PV_WINDOW_PATH ) )
    return 0;

  o_id = VQuery( v_id, PV_WINDOW_OTHERID );
  ob = load_object( path );

  if ( ( stringp( o_id ) ) &&
       ( ({string})ob->VQuery( o_id, PV_WINDOW_PATH ) != object_name( this_object() ) ) )
    o_id = -1;

  if ( ( intp( o_id ) ) && ( o_id == -1 ) ) {
    o_id = 0;
    walk_mapping( filter_indices( ({mapping})ob->Query( P_VITEMS ), "VQuery", ob,
                                  PV_WINDOW_ISWINDOW ),
                  SF( filter_window_find_otherwindow ), & o_id );
  }

  VSet( v_id, PV_WINDOW_OTHERID, o_id );
  return o_id;
}

static void window_set_otherwindow( string v_id, string prop, mixed val ) {
  string o_id;
  object ob;

  if ( ! stringp( o_id = window_find_otherwindow( v_id, & ob ) ) )
    return;

  ob->VSet( o_id, prop, val );
}

static void window_set_from_otherwindow( string v_id, string prop ) {
  string o_id;
  object ob;

  if ( ! stringp( o_id = window_find_otherwindow( v_id, & ob ) ) )
    return;

  VSet( v_id, prop, ({mixed})ob->VQuery( o_id, prop ) );
}

static void window_msg_otherwindow( string v_id, mixed msg ) {
  string o_id;
  object ob;
  int i;

  if ( ! stringp( o_id = window_find_otherwindow( v_id, & ob ) ) )
    return;

  /* we need that for the OWindowShort() function */
  lasto_id = o_id;
  lastowin = ob;

  if ( stringp( msg ) )
    msg_room( ob, CMSG_ROOM, process_string(msg) );
  else if ( pointerp( msg ) ) {
    for( i = 0; i < sizeof( msg ); i ++ )
      msg[i] = process_string(msg[i]);

    msg_room( ob, CMSG_ROOM|MMSG_SEE, msg );
  }
}

/* --------------------------------------------------------------------------
 * helpfun for PV_WINDOW_CHECKFUN
 *  returns 0 for action permitted and 1 for action denied and issues the
 *  appropriate messasges.
 * --------------------------------------------------------------------------
 */

static int window_call_checkfun( string action, string v_id ) {
  mixed fun;
  mixed res;

  if ( ! fun = VQuery( v_id, PV_WINDOW_CHECKFUN ) )
    return 0; /* no fun means always permitted */

  /* handle the various formats */
  if ( closurep( fun ) && to_object( fun ) )
    res = funcall( fun, action, v_id );
  else if ( stringp( fun ) )
    res = ({mixed})call_other( this_object(), fun, action, v_id );
  else if ( pointerp( fun ) ) {
    if ( ( sizeof( fun ) > 1 ) &&
         ( objectp( fun[0] ) ) &&
         ( stringp( fun[1] ) ) )
      res = apply( SF(call_other), fun[0], fun[1], action, v_id, fun[2..] );
    else if ( stringp( fun[0] ) )
      res = apply( SF(call_other), this_object(), fun[0], action,
                   v_id, fun[1..] );
    else if ( closurep( fun[0] ) )
      res = apply( fun[0], action, v_id, fun[1..] );
  }

  if ( ! res )
    return 0; /* we are permitted */

  if ( pointerp( res ) && sizeof( res ) ) {
    if ( pointerp( res[0] ) )
      msg_write( CMSG_GENERIC|MMSG_SEE, res[0] );
    else
      msg_write( CMSG_GENERIC, res[0] );

    if ( sizeof( res ) > 1 ) {
      if ( pointerp( res[0] ) )
        msg_say( CMSG_GENERIC|MMSG_SEE, res[1] );
      else
        msg_say( CMSG_GENERIC, res[1] );
    }
  }
  else if ( stringp( res ) )
    msg_write( CMSG_GENERIC, res );
  else
    msg_write( CMSG_GENERIC, "You can't do that right now.\n" );

  return 1;
}

/* --------------------------------------------------------------------------
 * WindowLong - returns the windows long description
 * --------------------------------------------------------------------------
 */

static string WindowLong( string what, string v_id ) {
   string desc, path;

   if ( window_call_checkfun( WINDOW_ACTION_LOOK, v_id ) )
     return ""; /* need to return an empty string here */

   /* redefine what to something to what the user looked at */
   what = WindowLastShort( v_id );

   if ( VQuery( v_id, P_LAST_PREP ) == "through" ) {
     if ( ( ! VQuery( v_id, PV_WINDOW_TRANS ) ) &&
          ( VQuery( v_id, PV_WINDOW_STATE ) == WINDOW_STATE_CLOSED ) ) {
       desc = "The "+what+" is way too dirty to look through.\n";
     }
     else {
       if ( desc = VQuery( v_id, PV_WINDOW_OTHERLONG ) )
         desc = "Looking through the "+what+" you see:\n" + desc;
       else {
         path = VQuery( v_id, PV_WINDOW_PATH );
         if ( path ) {
           if ( catch( desc = ({string})load_object(path)->GetIntDesc() ) )
             desc =
               "Looking through the "+what+
               " you notify a wrongness in space.\n";
           else
             desc = "Looking through the "+what+" you see:\n" + desc;
         } else
           desc = "Looking through the "+what+" you see nothing.\n";
       }
     }
   }
   else {
     desc = VQuery( v_id, PV_WINDOW_LONG );
     if(-1 != member("\n .!?",desc[<1])) desc[<1]=' ';

     switch( VQuery( v_id, PV_WINDOW_STATE ) ) {
     case WINDOW_STATE_SMASHED:
       desc += "The "+what+" is smashed.\n";
       break;
     case WINDOW_STATE_OPEN:
       desc += "The "+what+" is open.\n";
       break;
     default:
       desc += "The "+what+" is closed.\n";
       break;
     }
   }

   return desc;
}

/* --------------------------------------------------------------------------
 * cmd_clean - clean command
 * --------------------------------------------------------------------------
 */

public int cmd_clean( string str ) {
  string v_id;

  notify_fail("Clean what?\n", NOTIFY_ILL_ARG);
  if( ! str )
    return 0;

  if ( ( ! stringp ( v_id = GetVItem( str, 2 ) ) ) ||
       ( ! VQuery( v_id, PV_WINDOW_ISWINDOW ) ) )
    return 0; /* no vitem or not one of our windows */

  if ( VQuery( v_id, PV_WINDOW_STATE ) == WINDOW_STATE_SMASHED ) {
    msg_write(CMSG_GENERIC,
              "Now, why do you want to clean a broken "+WindowLastShort(v_id)+
              "+?\n" );
    return 1;
  }

  if ( VQuery( v_id, PV_WINDOW_TRANS ) ) {
    msg_write(CMSG_GENERIC,
              "The glass of the "+WindowLastShort(v_id)+" looks just fine. "
              "There is no need to further clean it.\n");
    return 1;
  }

  if ( window_call_checkfun( WINDOW_ACTION_CLEAN, v_id ) )
    return 1;

  msg_say(CMSG_GENERIC,
          CAP(NAME(TP))+" cleans the "+WindowShort(v_id)+
          ". As "+HESHE(TP)+" is finished with "
          "it, the glass of the "+WindowShort(v_id)+" is clean and "
          "shiny again.\n");
  msg_write(CMSG_GENERIC,
            "You clean the glass of the "+WindowLastShort(v_id)+". After some "
            "scrubbing, you are able to see through the "+WindowLastShort(v_id)+
            " window again.\n");
  VSet( v_id, PV_WINDOW_TRANS,1);
  window_set_otherwindow( v_id, PV_WINDOW_TRANS, 1 );
  window_msg_otherwindow( v_id, ({ "You suddenly notice that the @@OWindowShort@@ is clean.\n" }) );

  return 1;
}

/* --------------------------------------------------------------------------
 * cmd_close - close command
 * --------------------------------------------------------------------------
 */

public int cmd_close( string str ) {
  string v_id;

  notify_fail("Close what?\n", NOTIFY_ILL_ARG);
  if ( ! str )
    return 0;

  if ( ( ! stringp ( v_id = GetVItem( str, 2 ) ) ) ||
       ( ! VQuery( v_id, PV_WINDOW_ISWINDOW ) ) )
    return 0; /* no vitem or not one of our windows */

  if ( VQuery( v_id, PV_WINDOW_STATE ) == WINDOW_STATE_CLOSED ) {
    msg_write(CMSG_GENERIC,"The "+WindowLastShort(v_id)+" is not open.\n");
    return 1;
  }

  if ( VQuery( v_id, PV_WINDOW_STATE ) == WINDOW_STATE_SMASHED ) {
    msg_write(CMSG_GENERIC,
              "The "+WindowLastShort(v_id)+" is smashed. There is nothing left, "
              "you could close.\n");
    return 1;
  }

  if ( window_call_checkfun( WINDOW_ACTION_CLOSE, v_id ) )
    return 1;

  msg_write(CMSG_GENERIC,"You close the "+WindowLastShort(v_id)+".\n");
  msg_say(CMSG_ROOM,CAP(NAME(TP))+" closes the "+WindowShort(v_id)+".\n");
  VSet( v_id, PV_WINDOW_STATE, WINDOW_STATE_CLOSED );
  window_set_otherwindow( v_id, PV_WINDOW_STATE, WINDOW_STATE_CLOSED );
  window_msg_otherwindow( v_id,
                          ({ "You notice that the @@OWindowShort@@ closed.\n",
                             "You hear a noice as if something closed.\n"}) );
  return 1;
}

/* --------------------------------------------------------------------------
 * cmd_open - open command
 * --------------------------------------------------------------------------
 */

public int cmd_open( string str ) {
  string v_id;

  notify_fail("Open what?\n",NOTIFY_ILL_ARG);
  if ( ! str )
    return 0;

  if ( ( ! stringp ( v_id = GetVItem( str, 2 ) ) ) ||
       ( ! VQuery( v_id, PV_WINDOW_ISWINDOW ) ) )
    return 0; /* no vitem or not one of our windows */

  if ( VQuery( v_id, PV_WINDOW_STATE ) == WINDOW_STATE_OPEN ) {
    msg_write(CMSG_GENERIC,"The "+WindowLastShort(v_id)+" is already open.\n");
    return 1;
  }

  if ( VQuery( v_id, PV_WINDOW_STATE ) == WINDOW_STATE_SMASHED ) {
    msg_write(CMSG_GENERIC,
              "The "+WindowLastShort(v_id)+" is smashed and there is nothing left "
              "you could open.\n");
    return 1;
  }

  if ( window_call_checkfun( WINDOW_ACTION_OPEN, v_id ) )
    return 1;

  msg_write(CMSG_GENERIC,"You open the "+WindowLastShort(v_id)+".\n");
  msg_say(CMSG_ROOM,CAP(NAME(TP))+" opens the "+WindowShort(v_id)+".\n");
  VSet( v_id, PV_WINDOW_STATE, WINDOW_STATE_OPEN );
  window_set_otherwindow( v_id, PV_WINDOW_STATE, WINDOW_STATE_OPEN );
  window_msg_otherwindow( v_id,
                          ({ "You notice that the @@OWindowShort@@ opened.\n",
                             "You hear a noice as if something opened.\n"}) );

  return 1;
}

/* --------------------------------------------------------------------------
 * cmd_climbenter - climb/enter command
 * --------------------------------------------------------------------------
 */

public int cmd_climbenter( string str ) {
  string v_id, mes1, mes2;
  string * strs, path;
  object ob;

  if ( query_verb() == "climb" ) {
    if ( Query( P_INDOORS ) )
      notify_fail("Climb out of what?\n", NOTIFY_ILL_ARG );
    else
      notify_fail("Climb into what?\n", NOTIFY_ILL_ARG );

    if ( ! str )
      return 0;

    strs = explode( norm_id( str ), " " );
    if ( sizeof( strs ) < 2 )
      return 0;
    if ( ( strs[0] == "through" ) ||
         ( ( strs[0] == "into" ) &&
           ( ! Query( P_INDOORS ) ) ) )
      str = implode( strs[1..], " " );
    else if ( ( sizeof( strs ) >= 3 ) &&
              ( strs[0] == "out" ) &&
              ( strs[1] == "of" ) &&
              ( Query( P_INDOORS ) ) )
      str = implode( strs[2..], " " );
    else
      return 0;
  }
  else {
    notify_fail( "Enter what?\n", NOTIFY_ILL_ARG );
    if ( ! str )
      return 0;
  }

  if ( ( ! stringp ( v_id = GetVItem( str, 2 ) ) ) ||
       ( ! VQuery( v_id, PV_WINDOW_ISWINDOW ) ) )
    return 0; /* no vitem or not one of our windows */

  if ( VQuery( v_id, PV_WINDOW_STATE ) == WINDOW_STATE_CLOSED ) {
    msg_write(CMSG_STATUS,
              "Funny idea. Why don't you open the "+WindowLastShort(v_id)+" first?\n");
    return 1;
  }

  if ( window_call_checkfun( WINDOW_ACTION_CLIMB, v_id ) )
    return 1;

  path = VQuery( v_id, PV_WINDOW_PATH );
  if ( ! path ) {
    msg_write( CMSG_GENERIC,
               "Looking closer at the room behind the "+WindowLastShort(v_id)+
               " you see that "
               "it is not a real room. You decide not to go there.\n" );
    return 1;
  }

  if ( ! ob = load_object( path ) ) {
    msg_write( CMSG_GENERIC,
               "Your sensitive mind notices a wrongness in space.\n" );
    return 1;
  }

  if ( Query( P_INDOORS ) )
    mes1 = "out of";
  else
    mes1 = "into";

  if ( ({int})ob->Query( P_INDOORS ) )
    mes2 = "in through";
  else
    mes2 = "out of";

  /* TODO:  WindowShort is wrong, needs to be the short of the other side
   * TODO:: for the other side message
   */
  TP->move( ob, M_SPECIAL,
            ({ "climbs "+mes1+" the "+WindowShort(v_id),
                 "climbs "+mes2+" the "+WindowShort(v_id),
                 "climb "+mes1+" the "+WindowLastShort(v_id)
                 }) );
  return 1;
}

/* --------------------------------------------------------------------------
 * cmd_smash - smash command
 * --------------------------------------------------------------------------
 */

public int cmd_smash( string str ) {
  string v_id;

  notify_fail("Smash what?\n",NOTIFY_ILL_ARG);
  if ( ! str )
    return 0;

  if ( ( ! stringp ( v_id = GetVItem( str, 2 ) ) ) ||
       ( ! VQuery( v_id, PV_WINDOW_ISWINDOW ) ) )
    return 0; /* no vitem or not one of our windows */

  if ( VQuery( v_id, PV_WINDOW_STATE ) == WINDOW_STATE_OPEN ) {
    msg_write(CMSG_STATUS,
              "The "+WindowLastShort(v_id)+" is already open. There is no need to smash it.\n");
    return 1;
  }

  if ( VQuery( v_id, PV_WINDOW_STATE ) == WINDOW_STATE_SMASHED ) {
    msg_write(CMSG_STATUS,"The "+WindowLastShort(v_id)+" is already smashed.\n");
    return 1;
  }

  if ( window_call_checkfun( WINDOW_ACTION_SMASH, v_id ) )
    return 1;

  msg_say(CMSG_ROOM,
          CAP(NAME(TP))+" smashes the "+WindowShort(v_id)+". Shards of "
          "the glass are falling outside.\n");

  msg_write(CMSG_STATUS,"You smash the glass.\n");
  VSet( v_id, PV_WINDOW_STATE, WINDOW_STATE_SMASHED );
  window_set_otherwindow( v_id, PV_WINDOW_STATE, WINDOW_STATE_SMASHED );
  window_msg_otherwindow( v_id,
                          ({ "You notice that someone smashed the @@OWindowShort@@.\n",
                             "You hear a noice of something breaking.\n"}) );

  return 1;
}

/* --------------------------------------------------------------------------
 * AddWindow - the user interface
 * --------------------------------------------------------------------------
 */

varargs string AddWindow( mixed id, mixed desc, mixed path,
                       mapping props ) {
  string v_id, * temp;
  mapping wdata;

  /* check some arguments to make usage more flexible */

  if ( mappingp( id ) ) {
    props = id;
    desc = 0;
    path = 0;
    id = props[PV_WINDOW_SHORT];
    if ( ! stringp( id ) )
      raise_error( object_name( this_object() )+
                   ": Bad argument 1 to AddWindow - PV_WINDOW_SHORT "
                   "missing.\n" );
  }
  else if ( ! stringp( id ) )
    raise_error( object_name( this_object() )+
                 ": Bad argument 1 to AddWindow.\n" );

  if ( mappingp( desc ) ) {
    props = desc;
    desc = 0;
    path = 0;
  }
  else if ( mappingp( path ) ) {
    props = path;
    path = 0;
  }
  else if ( ! mappingp( props ) )
    props = 0;

  /* if there is no desc, make a primitive one */
  if ( ! desc )
    desc = "This is a window. You may open or close it.\n";

  /* check if we have been called with string id, add ids and ads from it */
  if ( mappingp( props ) && props[PV_WINDOW_SHORT] )
    wdata = ([]);
  else if ( ! wdata = ids_n_ads( id ) )
    return 0;

  /* store our state etc. as properties at the vitem */
  wdata +=
    ([
      P_LONG : SF( WindowLong ),
      P_PREPS : ({ "through" }),
      PV_WINDOW_SHORT : id,
      PV_WINDOW_LONG : desc,
      PV_WINDOW_PATH : path,
      PV_WINDOW_STATE : WINDOW_STATE_CLOSED,
      PV_WINDOW_TRANS : 0,
      PV_WINDOW_OTHERID : -1, /* we init this one at first request */
      PV_WINDOW_ISWINDOW : 1  /* we identify based on that */
     ]);

  /* there might be additional properties, add them */
  if ( mappingp( props ) ) {
    /* remove some values that are not to be set by the user */
    m_delete( props, PV_WINDOW_ISWINDOW );
    m_delete( props, PV_WINDOW_OTHERID );

    /* do special handling for P_ADS, P_IDS and P_PREPS */
    if ( temp = props[P_IDS] ) {
      wdata[P_IDS] += temp;
      m_delete( props, P_IDS );
    }
    if ( temp = props[P_ADS] ) {
      wdata[P_ADS] += temp;
      m_delete( props, P_ADS );
    }
    if ( temp = props[P_PREPS] ) {
      wdata[P_PREPS] += temp;
      m_delete( props, P_PREPS );
    }

    /* add the rest */
    wdata += props;
  }

  /* clean up the window short in case someone used an article */
  wdata[PV_WINDOW_SHORT] = strip_a( wdata[PV_WINDOW_SHORT] );

  /* resolve the given if given */
  if ( wdata[PV_WINDOW_PATH] )
    wdata[PV_WINDOW_PATH] = resolve_file( wdata[PV_WINDOW_PATH] );

  if ( ! v_id = AddVItem( wdata ) )
    return 0;

  /* sync us if the other room is already loaded */
  if ( wdata[PV_WINDOW_PATH] && find_object( wdata[PV_WINDOW_PATH] ) ) {
    window_set_from_otherwindow( v_id, PV_WINDOW_STATE );
    window_set_from_otherwindow( v_id, PV_WINDOW_TRANS );
  }

  return v_id;
}

/* --------------------------------------------------------------------------
 * init - adds the commands
 * --------------------------------------------------------------------------
 */

public void init() {
  if ( object_name( this_object() ) == "/std/room/vwindows" )
       return;

  add_action( "cmd_open",       "open"  );
  add_action( "cmd_close",      "close" );
  add_action( "cmd_climbenter", "climb" );
  add_action( "cmd_climbenter", "enter" );
  add_action( "cmd_smash",      "smash" );
  add_action( "cmd_clean",      "clean" );
}

/* --------------------------------------------------------------------------
 * reset - reset the windows
 * --------------------------------------------------------------------------
 */

static void reset_window( string v_id, mapping d_data ) {
  mixed val;

  if ( d_data[PV_WINDOW_STATE] != WINDOW_STATE_CLOSED ) {
    if ( d_data[PV_WINDOW_STATE] == WINDOW_STATE_SMASHED ) {
      if ( ! val = VQuery( v_id, PV_WINDOW_REPAIRMSG ) )
        val = ({ "Suddenly you notice that someone snuck in and repaired the "+WindowShort(v_id)+".\n" });
      if ( stringp( val ) )
        val = ({ val });
    }
    else {
      val = ({ "You notice that the "+WindowShort(v_id)+" closed.\n",
               "You hear a noice as if something closed.\n"});
    }

    d_data[PV_WINDOW_STATE] = WINDOW_STATE_CLOSED;
    window_set_otherwindow( v_id, PV_WINDOW_STATE, d_data[PV_WINDOW_STATE] );

    msg_room( this_object(), CMSG_ROOM|MMSG_SEE, val );
    /* TODO: val wrongly uses the window short of this room */
    window_msg_otherwindow( v_id, val );
  }

  if ( ( d_data[PV_WINDOW_TRANS] != WINDOW_ALWAYS_TRANS ) &&
       ( d_data[PV_WINDOW_TRANS] != 0 ) ) {
    if ( ! val = VQuery( v_id, PV_WINDOW_DIRTYMSG ) )
      val = ({ "Suddenly you sense a thick layer of dust on the "+WindowShort(v_id)+".\n" });
    if ( stringp( val ) )
      val = ({ val });

    d_data[PV_WINDOW_TRANS] = 0;
    window_msg_otherwindow( v_id, val );

    msg_room( this_object(), CMSG_ROOM|MMSG_SEE, val );
    /* TODO: val wrongly uses the window short of this room */
    window_set_otherwindow( v_id, PV_WINDOW_TRANS, d_data[PV_WINDOW_TRANS] );
  }
}

public void reset() {
  if ( object_name( this_object() ) == "/std/room/vwindows" )
       return;

  walk_mapping( filter_indices( Query( P_VITEMS ), SF(VQuery),
                                PV_WINDOW_ISWINDOW ), SF( reset_window ) );
}

/* ************************************************************************ */


