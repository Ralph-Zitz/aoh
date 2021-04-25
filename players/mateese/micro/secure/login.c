/*---------------------------------------------------------------------------
** /secure/login
**
** Accept a new login, check the password and start the player.
**
** 04-Sep-93 [Mateese]
** 02-Oct-93 [Mateese] Passwords are checked for minimal security.
** 10-Jan-94 [Mateese] Checks for reserved or banished names.
**                     Now able to connect to a new player object on demand.
** 08-Jun-94 [Mateese] Typing the name ahead prevents the etc/WELCOME from
**                     being printed.
** 21-Feb-95 [Mateese] The loaded player object is renamed to the players name
**                     Forgot to set the wizard bit.
**---------------------------------------------------------------------------
*/

#include "/i/player.h"
#include "/i/sys/info.h"

#define THIS     this_object()
#define TP       this_player()
#define TI       this_interactive()

#define STDPLAYER   "/std/player"  // The initial player object.
#define STDWIZARD   "/std/wizard"  // The initial wizard object.
#define MAX_GUESTS  5              // Max number of guests.

static string name;
static string password;
static string obname;   // Name of the player object.
static int wlevel;      // Wizard level of the player.
static int guestno;     // >0: player is guest #<guestno>
                        // =0: player is just a player
                        // <0: player is a new player
static int enforce;
static int checkedshut;

//---------------------------------------------------------------------------
nomask void create ()

{
  if (explode(file_name(THIS), "#")[0] != LOGIN)
  {
    raise_error("Illegal attempt.\n");
    destruct (THIS);
  }
  enforce = 0;
  checkedshut = 0;
}

nomask int prevent_shadow (object shadow) { return 1; }

//---------------------------------------------------------------------------
static nomask logon ()

// Logon a new player.
//
// This function is called by the gamedriver to log on a new player.
// It should return 1 to confirm that the function executed properly.
//
// As this function is called from within the poll for incoming messages,
// it shouldn't do much in order to to allow quick responses.

{
  input_to("logon2");           // let the user be able to type ahead.
  call_out("PrintWelcome", 3);  // Print the title screen
  call_out("TimeOut", 240);     // Terminate login after 4 minutes.
  enforce = 0;
  checkedshut = 0;
  return 1;
}

//---------------------------------------------------------------------------
static void TimeOut ()

// Login timed out - destruct login object.

{
  if (interactive(THIS)) write("Time out\n");
  destruct(THIS);
}

//---------------------------------------------------------------------------
static int CheckShut ()

// Check the shutdown object and print any diagnostics.
// Return True if the login may be done.

{ object shut;
  int    tleft;

  if (!interactive(THIS))  // Connection lost on login
    return 0;

  if (checkedshut++)
    return 1;

  if (!(shut = find_living(SHUTNAME)) || !shut->QueryShutProgress())
    shut = 0;
  else if ((tleft = (int)shut->QueryTimeLeft()) < 60)
  {
    write ("\nSorry, "+MUDNAME+" is shutting down.\nReason: "
          +(string)shut->QueryReason()
          +"\nPlease try again in a few minutes.\n\n");
    return 0;
  }

  if (shut && tleft <= 600)
  {
    tleft = (tleft + 59) / 60;
    write ("\nWARNING: "+MUDNAME+" is shutting down in "
          +tleft+" minute"+(tleft > 1 ? "s" : "")+ ".\n"
          +"Reason: "+(string)shut->QueryReason()+"\n\n"
          );
  }
  return 1;
}

//---------------------------------------------------------------------------
static void PrintWelcome ()

// Print the title screen and ask for the name.

{

  if (!interactive(THIS))  // Connection lost on login
  {
    destruct(THIS);
    return;
  }

  write("Gamedriver LPMud "+__VERSION__+" (Amylaar)\n"
       +"Mudlib "+MUDNAME+" v"+LIBVERSION+"\n"
       );

  cat ("/etc/WELCOME");

  if (!CheckShut())
  {
    destruct(THIS);
    return;
  }

  write("Precede your name with a '%' to enforce a fresh player object.\n");
  write("What is your name: ");
}

//---------------------------------------------------------------------------
static nomask logon2 (string str)

// User wants to enter the name. Check for validity and ask for password.

{ int newOne;
  int tleft;
  int retry;
  string cfile;
  mixed *data;
  object shut;

  if (!interactive(THIS))
  {
    destruct(THIS);
    return;
  }

  if (str && str[0] == '%') {
    enforce = 1;
    str = str[1..];
  }

  if (!str || str == "")
  {
    destruct(THIS);
    return;
  }

  if (remove_call_out("PrintWelcome") >= 0)
    if (!CheckShut())
    {
      destruct(THIS);
      return;
    }

  name = lower_case(str);
  if (!validName(name)) {
    write("\nWhat is your name: ");
    input_to("logon2");
    return;
  }

  data = SECUREINFO->Info(name);
  retry = 0;
  if (data[SINFO_LEVEL] < 0 && data[SINFO_LEVEL] != SILEVEL_ILLEGAL) {
    write("Sorry, that name is reserved.\n");
    retry = 1;
  }
  else if (data[SINFO_BANISH]) {
    write("Sorry, that character is banished.\n");
    retry = 1;
  }
  if (retry) {
    write("\nWhat is your name: ");
    input_to("logon2");
    return;
  }

  wlevel = SECUREINFO->Wizlevel(name, 1);

  if (wlevel < ADMIN_LVL && file_size("/adm/etc/closed/ADMIN") > 0)
    cfile = "/adm/etc/closed/ADMIN";
  else if (wlevel < ARCH_LVL && file_size("/adm/etc/closed/ARCH") > 0)
    cfile = "/adm/etc/closed/ARCH";
  else if (wlevel < WIZARD_LVL && file_size("/adm/etc/closed/WIZARD") > 0)
    cfile = "/adm/etc/closed/WIZARD";
  else if (wlevel < LEARNER_LVL && file_size("/adm/etc/closed/LEARNER") > 0)
    cfile = "/adm/etc/closed/LEARNER";
  else
    cfile = 0;

  if (cfile) {  // Game closed
    cat(cfile);
    destruct(THIS);
    return;
  }

  guestno = 0;
  if (name == "guest") {
    load_guest();
    return;
  }

  if (data[SINFO_LEVEL] == SILEVEL_ILLEGAL) { // A new player logged on.
    write ("\nWelcome to "+MUDNAME+", "+capitalize(name)
          +"!\nPlease enter a password: ");
    input_to ("logon_new", 1);
    obname = STDPLAYER;
  }
  else {
    if (data[SINFO_LEVEL] >= LEARNER_LVL)
      obname = STDWIZARD;
    else
      obname = STDPLAYER;
    write ("\nWelcome back, "+capitalize(name)+".\n");
    if (SECUREINFO->CheckPassword(name, "dummy") < 0) {
      write ("You have NO PASSWORD!\n");
      load_player();
      return;
    }
    write("Password: ");
    input_to ("logon_old", 1);
  }
}

//---------------------------------------------------------------------------
static nomask validName (string name)

// Check that a name is valid. Allow only lowercase letters.

{
  int i, length;

  length = strlen(name);
  if (length > 11) {
    write("That name is too long. Choose a shorter one.\n");
    return 0;
  }
  i = 0;
  for (i = 0; i < length; i++) {
    if (name[i] < 'a' || name[i] > 'z') {
      write("Invalid characters in name: "+name+"\n");
      write("Use only alphabetical characters.\n");
      return 0;
    }
  }
  return 1;
}

//---------------------------------------------------------------------------
static nomask logon_old (string str)

// Called to verify the password of an existing player.

{
  write("\n");
  if (!SECUREINFO->CheckPassword(name, str))
  {
    write("Wrong password!\n");
    destruct(THIS);
    return;
  }
  load_player(0);
}

//---------------------------------------------------------------------------
static nomask logon_new (string str)

// Called with the password of a new player.
{
  int invalid, l;

  password = str || "";
  if (password == "")
  {
    write ("\nYou have NO PASSWORD!");
    logon_again(password);
    return;
  }

  invalid = (lower_case(password) == lower_case(name));
  for (l = 1; l <= strlen(password); l++)
    str[l-1] = password[<l];

  invalid |= (lower_case(str) == lower_case(name));
  invalid |= (password != "" && strlen(password) < 6);

  if (invalid)
  {
    write("That password is insecure.\nEnter an other password: ");
    input_to("logon_new", 1);
  }
  else
  {
    write("Enter password again: ");
    input_to("logon_again", 1);
  }
}

//---------------------------------------------------------------------------
static nomask logon_again (string str)

// Called with the reenter of the password of a new player.

{
  write("\n");
  if ((str || "") != password) {
    write("Wrong verification!\n");
    destruct(THIS);
    return;
  }
  guestno = -1;
  SECUREINFO->NewPlayer(name, password);
  load_player();
}

//---------------------------------------------------------------------------
static nomask load_guest ()

// Player requested a guest character.

{ object *guests;
  int *flag;
  int oldguest, oldtime, i;

  guests = filter_objects(users(), "id", "guest");
  flag = allocate(MAX_GUESTS);

  // Detect the oldest guest, while removing netdead guests.

  oldtime = time();
  for (i = sizeof(guests); i--;)
  {
    if (interactive(guests[i]))
    {
      flag[guests[i]->Query(P_GuestNo)-1] = 1;
      if (program_time(guests[i]) < oldtime)
      {
        oldtime = program_time(guests[i]);
        oldguest = i;
      }
    }
    else
    {
      MASTER->remove_player(guests[i]);
      guests = guests[0..i-1]+guests[i+1..];
    }
  }

  if (sizeof(guests) == MAX_GUESTS)  // Say goodbye to the oldest one.
  {
    guestno = guests[oldguest]->Query(P_GuestNo);  // This will be reused.
    tell_object(guests[oldguest]
               , "\nSorry, your time is up.\n"
                +"We hope you liked your stay and are going to see you again.\n\n"
               );
    MASTER->remove_player(guests[oldguest]);
  }
  else // Find a free guest
  {
    for (guestno = 1; guestno <= MAX_GUESTS; guestno++)
      if (!flag[guestno-1]) break;
  }
  load_player();
}

//---------------------------------------------------------------------------
static nomask load_player ()

// Finally, load the player.

{
  object obj;
  object blueprint;

  seteuid(name);

  // Test if we are already playing
  if (guestno <= 0 && (obj = find_player(name)))
  {
    write("You are already existing!\n");
    if (enforce) {
      write("Destructing old body...\n");
      MASTER->remove_player(obj);
    }
    else {
      write("Reincarnating old body...\n");
      if (interactive(obj)) remove_interactive(obj);
      exec(obj, THIS);
      obj->Netdead(0); // Restart player
      // TODO: Announce that the player became alive again.
      destruct(THIS);
      return;
    }
  }

  // In case some asshole has moved the blueprint...
  blueprint = find_object(obname);
  if (blueprint && environment(blueprint))
    destruct(blueprint);

  // clone a player object
  write(file_name(THIS)+": Connect to "+obname+"...\n");

  if (catch(obj = clone_object(obname)))
  {
    write("Error on loading "+obname+"\n");
    destruct(THIS);
    raise_error("Error on loading "+obname+"\n");
    return;
  }
  rename_object(obj, guestno ? name+guestno : name);
  exec(obj, THIS);
  if (wlevel > PLAYER_LVL)
    set_is_wizard(obj, 1);
  obj->StartPlayer(name, guestno);  // If this fails, it fails
  destruct(THIS);
}

/***************************************************************************/
