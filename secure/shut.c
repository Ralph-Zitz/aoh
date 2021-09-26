/* /secure/shut
**
** Shutdown the Mud.
** This is the old code from Armageddon, put into a separate demon to make
** more robust. The 'transport to shop' facility is still with Armageddon
** itself.
** Program and messages modified by Tares (08/91)
** Messages re-edited by Dogbowl (08/91)
** Added the transport to /room/shut_shop by Tares (05/92)
** Quickly patched for native mode. (Mateese, 07/92)
*/

#include <secure/config.h>
#include <secure/wizlevels.h>

#define LOGFILE  "GAME_LOG"

#define PO previous_object()
#define TO this_object()
#define TP this_player()
#define TI this_interactive()

#ifdef DEBUG
#define DTELL(a) dtell("nostradamus", a)
#else
#define DTELL(a)
#endif

#define NO_EMBELLISHMENT
#define NO_TRANSPORT_OFFER

/*------------------------------------------------------------------------*/

string	* random_messages;	  /* Random apocalypse messages */
string  * progress_messages;	/* Progress apocalypse messages */
int	shut_in_progress;  	      /* Is a shutdown in progress? */
int 	transport_offer;        /* Already given a transport offer to players? (shouted) */
int     ok_random_msgs;       /* Are the random messages ok? */
string  reason;
int     time_left;
int     time_left_set;
object  armageddon;           /* The armageddon-NPC */

void shut(int minutes);
void do_shout(string str);

/*------------------------------------------------------------------------*/

void create() {
  seteuid(getuid(TO));

  if (-1 != member(object_name(TO), '#')
   || object_name(TO)+".c" != __FILE__
     ) {
    log_file (LOGFILE,
 ctime(time())+": shut: Illegal attempt!\n"
+"  this:        "+object_name(TO)+" ["+getuid(TO)+"]\n"
+"  previous:    "+(PO ? object_name(PO)+" ["+getuid(PO)+"]" : "<none>")+"\n"
+"  player:      "+(TP ? object_name(TP)+" ["+getuid(TP)+"]" : "<none>")+"\n"
+"  interactive: "+(TI ? object_name(TI)+" ["+getuid(TI)+"]" : "<none>")+"\n\n"
             );
    destruct (TO);
    return;
  }

  reason = "Unknown.";

  random_messages = ({
"The curtain of the sky is torn away by a flash in the sky. The heavens\nare a blaze.\n",
"There are several earthquakes all over the world!\n",
"The ground you stand on heaves and rolls under your feet.\nYou are thrown to the ground.\n",
"There is thunder and lightning in the sky!\n",
"You hear rolling thunder.\n"
                    });

  progress_messages = ({
" has started the Apocalypse!\n",
"You hear a low rumble echo through " MUDNAME ".\n",
"For a second everything goes black and a tremendous explosion is heard.\n",
"There is a big thunder and a dim blue light all over the world.\n",
"You see little electric flashes on your skin...\n",
"The air becomes thin as the landscape explodes into flames.\n",
"The inhabitants of " MUDNAME " begin to slowly fade away.\n",
(random(100) >= 75
   ? "Violet clouds raise in the sky. It begins to rain... BLOOD!\n"
   : "A harsh voice shouts: 'Computer! Freeze program!'\n"),
                       });

  shut_in_progress = 0;
  enable_commands();
  set_heart_beat(1);
}

/*-------------------------------------------------------------------------*/

int clean_up(int ref) { return 0; }
varargs int remove(int flag)
{
  if (flag < 0) /* Protection against mudclean */
    return 0;
  if (armageddon)
    armageddon->remove();
  destruct(this_object());
  return this_object() && 1;
}

/*-------------------------------------------------------------------------*/

object Armageddon() {
  if (armageddon)
    return armageddon;
  armageddon = find_living("armageddon");
  if (!armageddon)
    armageddon = clone_object("/obj/armageddon");
  armageddon->move_to_loc();
  return armageddon;
}

void slow_shut (int minutes) {
  int i, delay, seconds;
  string who;

  if (minutes <= 10)
    return shut(minutes);

  i = remove_call_out("cont_shutting");
  if (i > 0) {
    write("There is already a shutdown in process, " + i + " seconds.\n");
    return;
  }
  if (!intp(minutes)) {
    write("Bad argument\n");
    return;
  }
  if (minutes <= 0) {
    write("No time given\n");
    return;
  }

  Armageddon()->Shut();

  if (this_player()) who = capitalize(({string})this_player()->QueryRealName());
  else if (object_name(previous_object()) == "/secure/master")
    who = "The Gamedriver";
  else who = 0;

  /* shout((who || "Someone")+progress_messages[0]+""); */
  catch(log_file ("GAME_LOG", ctime(time())+" "
                        +(who || (object_name(previous_object())
                                  +" ["+getuid(previous_object())+"]"
                                 )
                         )
                        +" started Armageddon ("+minutes+" minutes)\n"
           ));

  shut_in_progress = 1;
  ok_random_msgs = 0;
  seconds = minutes * 60;
  time_left = seconds;
  time_left_set = time();
  delay = seconds * 3 / 4 - 10;
  call_out("cont_shutting", seconds - delay, delay);
}

void shut(int minutes) {
  int i;
  string who;

  i = remove_call_out("cont_shutting");
  if (i > 0) {
    write("There is already a shutdown in process, " + i + " seconds.\n");
    return;
  }

  if (!intp(minutes)) {
    write("Bad argument\n");
    return;
  }

  if (minutes <= 0) {
    write("No time given\n");
    return;
  }

  Armageddon()->Shut();

  if (this_player()) who = capitalize(({string})this_player()->QueryName());
  else if (object_name(previous_object()) == "/secure/master")
    who = "The Gamedriver";
  else who = 0;

#ifndef NO_EMBELLISHMENT
  shout((who || "Someone")+progress_messages[0]+"");
#endif
  catch(log_file ("GAME_LOG", ctime(time())+" "
                        +(who || (object_name(previous_object())
                                  +" ["+getuid(previous_object())+"]"
                                 )
                         )
                        +" started Armageddon ("+minutes+" minutes)\n"
           ));
  shut_in_progress = 1;
  ok_random_msgs = (minutes <= 5);
  time_left = minutes * 60;
  time_left_set = time();
  call_out("cont_shutting", 0, minutes * 60);
}

/*-------------------------------------------------------------------------*/
static void cont_shutting(int seconds) {
  string delay;
  int    new_delay;

  Armageddon();

  if (seconds <= 0) {
    do_shout("I'am going to destruct the world now!\n");
    do_shout("It will be recreated in several minutes.\n");
    catch(log_file ("GAME_LOG", ctime(time())+" Armageddon shut down the game\n"));
    shutdown();
    call_out("do_shout", 5, "I have been thwarted for now, but i will return.\n");
    remove();
    return;
  }

#ifndef NO_TRANSPORT_OFFER
  if (seconds <= 300 && !transport_offer) {
    do_shout("Last chance to flee!\n");
    transport_offer = 1;
  }
  if (transport_offer)
  {
    do_shout("TELL me if you need a trip to the shop!\n");
    Armageddon()->TransportOk();
  }
#endif

  new_delay = seconds * 3 / 4 - 10;
  time_left = seconds;
  time_left_set = time();
  call_out("cont_shutting", seconds - new_delay, new_delay);
  delay = "";
  if (seconds > 3599) {
    delay = seconds / 3600 + " hours ";
    seconds = seconds % 3600;
  }
  if (seconds > 59) {
    delay += seconds / 60 + " minutes ";
    seconds = seconds % 60;
  }
  if (seconds != 0) {
    delay += seconds + " seconds ";
  }
  do_shout(delay + "till the Apocalypse.\n");
#ifndef NO_EMBELLISHMENT
  if(time_left < 600 && shut_in_progress < sizeof(progress_messages)) {
    call_out("efunshout", random(3), progress_messages[shut_in_progress]);
    shut_in_progress++;
  }
#endif
  ok_random_msgs = (new_delay <= 300);
}

/*-------------------------------------------------------------------------*/
void stopshut() {
  remove_call_out("cont_shutting");
  if (!shut_in_progress) return;
  shut_in_progress = 0;
  if (this_player()) {
    catch(log_file("GAME_LOG",({string})this_player()->QueryRealName()+
                        " stopped Armageddon "+ctime(time())+"\n"));
  }
  shout("Armageddon has been stopped. You are save ... for now.\n");
  remove();
}

/*-------------------------------------------------------------------------*/
void heart_beat() {
#ifndef NO_EMBELLISHMENT
  if(shut_in_progress && ok_random_msgs) {
    /* random apocalypse messages */
    if(random(100) < 5) {
      shout(random_messages[random(sizeof(random_messages))]);
    }
  }
#endif
  set_heart_beat(1);
}

/*-------------------------------------------------------------------------*/
void do_shout(string str) {
  shout("Armageddon shouts: " + str);
  if (this_player()) tell_object(this_player(), "Armageddon shouts: "+str);
}

/*-------------------------------------------------------------------------*/
int query_progress() {
  return shut_in_progress;
}

string query_reason() { return reason; }
string set_reason(string arg) { return reason = arg; }
int query_time_left() { return time_left - (time() - time_left_set); }

/*-------------------------------------------------------------------------*/
/* To call 'shout' via call_out(), even reaching the player which started
** the shutdown
*/
void efunshout(string str) {
  shout(str);
  if (this_player()) tell_object(this_player(), str);
}

/*========================================================================*/
