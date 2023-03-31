/* -------------------------------------------------------------------------
 * /d/beyond/std/sy_street.c -- The basic room for streets at Sykorath
 *
 * Copyright (C) 1997 by One Step Beyond.
 * All Rights Reserved.
 * Permission granted for non-commercial use.
 * -------------------------------------------------------------------------
 * This room uses some additional properties:
 *
 * P_SKULL_BRIGHTNESS    : the amount of light added during night
 *
 * P_SKULL_NIGHT_DETAIL  : description of the skull detail during night
 * P_SKULL_DAY_DETAIL    : description of the skull during day
 *
 * P_SKULL_NIGHT_MSG     : message written to the room when the skulls are 
 *                         lit.
 * P_SKULL_DAY_MSG       : message written to the room when the skulls are
 *                         unlit.
 *
 * P_SKULL_NIGHT_EXTRA   : Addon to the rooms P_INT_LONG during night
 * P_SKULL_DAY_EXTRA     : Addon to the rooms P_INT_LONG during day
 *
 * All properties can be set to 0 which means no desc/detail/message
 * -------------------------------------------------------------------------
 * 21 jan 98 - Magictcs - added special entry messages from time to time
 */

#include <properties.h>
#include <msgclass.h>
#include <nightday.h>
#include <macros.h>
#include <sykorath.h>

inherit BASEROOM;

// animalmsgs while moving (time delayed)
nosave string * animal_msg = ({
  "A small mouse appears, looks at you and disappears in the next "
    "mouse-hole.\n",
  "A small bat flies around your head.\n",
  "A small reddish looking rat scurries across the floor.\n",
  "A tiny rat crosses the road and disappears between some houses.\n",
  "A feather dances in the wind and vanishes in the sky.\n",
  "A snake crawls out of a hole, tries to bite into your feet and vanishes "
    "int the hole again.\n",

  // add more here

  });

//*******************************
// prints the message to the room
//*******************************
void print_animal_msg(int num) {
  show_room(TO,animal_msg[num]);
}

//******************************************
// show entry msg if someone enters the room
//******************************************
void notify_enter(object oldenv,int method,mixed extra) {
  ::notify_enter(oldenv,method,extra);
  if (!interactive(PO)) return;         // only when interactive
  if (!random(30)) {
    // then send a time delayed msg (3 to 13 seconds delay)
    call_out("print_animal_msg",random(10)+3,random(sizeof(animal_msg)));
  }
}

/* -------------------------------------------------------------------------
 * Bright is modified by skulls too
 * -------------------------------------------------------------------------
 */

int QueryBright() {
  if ( !({int})NIGHTDAY->IsDay() )
    return (::QueryBright())+Query(P_SKULL_BRIGHTNESS);
  else
    return (::QueryBright());
}

/* -------------------------------------------------------------------------
 * skull vitem long desc
 * -------------------------------------------------------------------------
 */

string skull_vitem_long() {
  if ( !({int})NIGHTDAY->IsDay() )
    return Query( P_SKULL_NIGHT_DETAIL );
  else
    return Query( P_SKULL_DAY_DETAIL );
}

/* -------------------------------------------------------------------------
 * skull extra int long addon
 * -------------------------------------------------------------------------
 */

string skull_extra_int_long() {
  if ( !({int})NIGHTDAY->IsDay() )
    return Query( P_SKULL_NIGHT_EXTRA )||"";
  else
    return Query( P_SKULL_DAY_EXTRA )||"";
}

/* -------------------------------------------------------------------------
 * Initialize us
 * -------------------------------------------------------------------------
 */

public varargs void create() {
  ::create();
  
  AddVItem( ([ 
    P_IDS:  ({"skull","lamp"}),
    P_ADS:  ({"skull"}),
    P_LONG : #'skull_vitem_long ]) );

  SetExtraEntry( "skull_extra", P_INT_LONG, #'skull_extra_int_long );

  Set( P_SKULL_NIGHT_MSG,
    "A shadowy, translucent figure levitates in and utters a word and "
    "the lamp suddenly burns with a dark green light. Then the figure "
    "levitates out to the next lamp.\n");

  Set( P_SKULL_DAY_MSG,
    "A shadowy, translucent figure floats past the lamp and you feel "
    "a sudden bust of wind and the lamp is dark.\n" );

  Set( P_SKULL_NIGHT_EXTRA,
    "A skull lamp, impaled on the wall, emits some dark green light.\n" );

  Set( P_SKULL_DAY_EXTRA,
    "There is a skull lamp impaled on the wall.\n" );

  Set( P_SKULL_NIGHT_DETAIL,
    "The shape of that skull looks very much like that of a human. The "
    "whole bottom jaw is missing, and some dust has accumulated during "
    "the long time it has been impaled on this wall. In its dead "
    "eyesockets two candles are glowing in an eerie dark green light. "
    "The shadows dance in the flickering light of the candles. Looking "
    "around, you see that all things cast two almost black shadows, "
    "which seem to be surrounded by that green light. The street is "
    "somehow lit by those lamps, but the night still fights that light "
    "and it does not penetrate far into that darkness.\n" );

  Set( P_SKULL_DAY_DETAIL,
    "The skull obviously belonged once to a human. The whole bottom jaw "
    "fell off long ago, and rain streaked dust has accumulated across "
    "the face over time. Each eyesocket is filled with a candle that is "
    "lit at night.\n" );

  Set( P_SKULL_BRIGHTNESS, 30 );
}

/* -------------------------------------------------------------------------
 * Filter the weather data for skull on/off
 * -------------------------------------------------------------------------
 */
mixed FilterWeatherData (object client, mixed *data, int newstate) {
  string msg;

  if(newstate==ND_TWILIGHT) {
    msg=Query(P_SKULL_NIGHT_MSG);
    if(msg) msg_object(client,CMSG_ROOM,msg);
  }
  else if(newstate==ND_SUNRISE) {
    msg=Query(P_SKULL_DAY_MSG);
    if(msg) msg_object(client,CMSG_ROOM,msg);
  }

  return ::FilterWeatherData(client,data,newstate);
}

