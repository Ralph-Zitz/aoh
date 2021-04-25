/*---------------------------------------------------------------------------
**  /adm/obj/armageddon
**
** The daemon of game crashing :-)
** It takes care of shutting down the game; especially if when the gamedriver
** runs out of memory.
**
** Only the blueprint may exist in the game.
**
** Messages originally written Aug. 1991 by Dogbowl, Nightfall.
**
**   04-Sep-93 [Mateese]
**
** TODO: Make this a true npc, present in the game. Descriptions used
**   in Nightfall:
**   Short         : Armageddon the god of world destruction
**   Long, inactive: He looks like he enjoys to prepare to wipe out all reality.
**   Long, active  : He is firmly concentrated on counting and prepares the
**                   destruction of the world!
** TODO: Possibly let Armageddon rescue players by transporting them to
**       a shop during the last 5 minutes ("Last chance to flee!").
**---------------------------------------------------------------------------
*/

#include "sys/player.h"

#define THIS     this_object()
#define PREV     previous_object()
#define TP       this_player()
#define TI       this_interactive()

static string  *randomMsgs;     // Random apocalypse messages
static string  *progressMsgs;   // Progress apocalypse messages
static int     shutProgress;    // State of the shutdown, 0 for no shutdown
static int     okRandomMsgs;    // Are the random messages ok?
static string  reason;          // Reason given for the shutdown.
static int     timeLeft;        // Time left till shutdown in seconds
static int     timeStamp;       // Time when timeLeft was set.

// A 'shout' that even goes to the player who started Armageddon.

#define SHOUT(str) shout(str); if (TP) tell_object(TP,str)

//---------------------------------------------------------------------------
void create ()

// Setup the god.

{
  object obj;

  seteuid(getuid(THIS));

  // Destruct any other Armageddons in the game
  while (obj = find_living("armageddon"))
  {
    obj->remove();
    if (obj) destruct(obj);
  }
  set_living_name("armageddon");
  enable_commands();
  set_heart_beat(1);

  randomMsgs = ({
"The curtain of the sky is torn away by a flash in the sky. The heavens\nare a blaze.\n",
"There are several earthquakes all over the world!\n",
"The ground you stand on heaves and rolls under your feet.\nYou are thrown to the ground.\n",
"There is thunder and lightning in the sky!\n",
"You hear rolling thunder.\n"
               });

  progressMsgs = ({
/* Name + */ " has started the Apocalypse!\n",
"You hear a low rumble echo through "+MUDNAME+".\n",
"For a second everything goes black and tremendous explosion is heard.\n",
"There is a big thunder and a dim blue light all over the world.\n",
"You see little electric flashes on your skin...\n",
"The air becomes thin as the landscape explodes into flames.\n",
"Violet clouds raise in the sky. It begins to rain... BLOOD!\n"
"The inhabitants of "+MUDNAME+" begin to slowly fade away.\n",
                 });

  shutProgress = 0;
  okRandomMsgs = 0;
}

int prevent_shadow (object shadow) { return 1; }

//---------------------------------------------------------------------------
void heart_beat()

// Output random messages if the shutdown is in progress.

{
  if (shutProgress && okRandomMsgs && random(100) < 5)
  {
    shout(randomMsgs[random(sizeof(randomMsgs))]);
  }
}

//---------------------------------------------------------------------------
// Various queries...

int QueryShutProgress () { return shutProgress; }

int QueryTimeLeft ()
{ int t;

  if (!shutProgress) return 0;
  t = timeLeft - (time()-timeStamp);
  return t < 0 ? 0 : t;
}

string QueryReason () { return reason; }

//---------------------------------------------------------------------------
void doShout (string txt)

// Shout a string (also called via call_out());

{
  SHOUT(txt);
}

//---------------------------------------------------------------------------
void Shutdown (int minutes, string why)

// Start a shutdown.
//
// Arguments:
//   minutes: Number of minutes till the shutdown.
//   why    : Reason for the shutdown.
//
// There will be no messages as long as the shutdown is more than 10 minutes
// in the future.

{
  int i, delay;
  string who;

  i = remove_call_out("contShutting");
  if (i > 0) {
    write("There is already a shutdown in process, "+QueryTimeLeft()+" seconds.\n");
    call_out("contShutting", QueryTimeLeft());
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
  reason = why || "Unknown.";

  if (TP)                             who = capitalize(TP->Query(P_Name)||"Someone");
  else if (file_name(PREV) == MASTER) who = "The Gamedriver";
  else who = 0;

  log_file ("adm/game"
           , stime()+"  "+(who || (file_name(PREV)+" ["+getuid(PREV)+"]"))
                    +" started Armageddon ("+minutes+" minutes)\n"
           );
  if (minutes <= 10)
    SHOUT((who || "Someone")+progressMsgs[0]);
  shutProgress = 1;
  okRandomMsgs = (minutes <= 5);
  timeLeft = minutes * 60;
  timeStamp = time();
  delay = timeLeft * 3 / 4 - 10;
  call_out("contShutting", timeLeft-delay, delay);
}

//---------------------------------------------------------------------------
void contShutting (int seconds)

// Continue the shutdown.
//
// Argument:
//   seconds: The time left at the time of the call.

{ int delay;
  string str;

  if (seconds <= 0) {
    SHOUT("Armageddon shouts: I'am going to destruct the world now!\n");
    SHOUT("The Gamedriver shouts: It will be recreated in several minutes.\n");
    if (random(100) >= 90)
      SHOUT("A harsh voice shouts: Computer, stop program!\n");
    log_file ("adm/game", stime()+"  Armageddon shut down the game\n");
    shutdown();
    call_out("doShout", 5
            , "Armageddon shouts: I have been thwarted for now, but I will return.\n");
    return;
  }

  if (seconds <= 30 && random(100) >= 75)
    SHOUT("For a moment you see nothing but a yellow grid on a black background.\n");

  delay = seconds * 3 / 4 - 10;
  timeLeft = seconds;
  timeStamp = time();
  call_out("contShutting", timeLeft-delay, delay);
  str = "";
  if (seconds > 59) {
    if (seconds > 119) str = seconds / 60 + " minutes ";
    else               str = seconds / 60 + " minute ";
    seconds = seconds % 60;
  }
  if (seconds != 0) {
    if (seconds > 1) str += seconds + " seconds ";
    else             str += seconds + " second ";
  }
  SHOUT("Armageddon shouts: "+str+"till the Apocalypse.\n");
  if (timeLeft <= 600 && shutProgress < sizeof(progressMsgs)) {
    call_out("doShout", random(3), progressMsgs[shutProgress]);
    shutProgress++;
  }
  okRandomMsgs = (timeLeft <= 300);
}

//---------------------------------------------------------------------------
void Stop ()

// Stop a shutdown in progress.

{ string who;

  remove_call_out("contShutting");
  if (!shutProgress) return;
  shutProgress = 0;
  if (TP) who = TP->Query("Name") || "Someone";
  else who = file_name(PREV)+" ["+getuid(PREV)+"]";
  log_file("adm/game",stime()+"  "+who+" stopped Armageddon\n");
  SHOUT("Armageddon has been stopped. You are save ... for now.\n");
  okRandomMsgs = 0;  // Just in case...
  shutProgress = 0;
  destruct (THIS);
}

/***************************************************************************/
