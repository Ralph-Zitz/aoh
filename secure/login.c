/*--------------------------------------------------------------------------
 * /secure/login.c -- the login object
 *
 * Copyright (C) 1997 by One Step Beyond.
 * All Rights Reserved.
 * Permission granted for non-commercial use.
 *--------------------------------------------------------------------------
 * The login object builds the connection to a player. It asks for
 * the name of the character and its password, loads the player object
 * and ties the IP connection to the player object.
 * If the character does not exist already, it is created.
 *
 * The charactername 'guest' can be used for guest logins (no password
 * required). At one time, there can be at max MAX_GUESTS in the game.
 *
 * The object prompts for the characters name, but in fact understands
 * several commands at that point:
 *
 *    <name> [<password>] [<options>]
 *    login   <name> [<password>] [<options>]
 *      Login an existing character, or create a new one.
 *
 *    create  <name> [<password>] [<option>]
 *      Create a new character.
 *
 *    restore <name> [<password>] [<options>]
 *      Login an existing character.
 *
 *    help    [<topic>]
 *      Give help about the login commands.
 *
 *    warning
 *      Print the 'Mud package warning' notice by Profezzorn.
 *
 *    quit
 *      Abort the login.
 *
 * The 'options' implemented so far are_
 *    -open  : Password inputs are echoed to avoid problems with broken
 *             clients and firewalls.
 *
 *    -fresh : A new player object is cloned, even after logging in
 *             from netdeath.
 *
 *    -always : Log in a wizard even if the game is closed.
 *
 * Additionally, the login object understands gopher and HTML logins.
 *
 * The player will be greeted with the title screen, which is randomly
 * taken selected among the files /etc/WELCOME* .
 *
 * The login object allows 3 invalid attempts to login, then it will
 * destruct itself.
 *--------------------------------------------------------------------------
 * Logins may be restricted in several ways:
 *
 * - by closing the game: This is done by placing a file into the directory
 *    /etc/closed/. The file contains the "Sorry" message to be printed to
 *    the player, the name of the file determines who will be rejected:
 *
 *      NEWPLAYERS: all new players.
 *      PLAYERS   : all players, new and existing.
 *      WIZARDS   : everybody.
 *
 *    Note that in the last case wizards can enforce a login by specifying
 *    the -always option, but it is not recommended.
 *
 * - by banishing names: Any name for which a savefile /secure/banish/<name>.o
 *    exists, can't be used as character name. To banish a name, use
 *    the /secure/banisher.
 *
 * - by restricting the host: The file /secure/save/RESTRICTED holds a list
 *    of hosts (by numerical ip or by hostname) for which only logins
 *    of existing characters (excluding 'guest') are allowed. New characters
 *    for these hosts must be created from within the game, if at all.
 *    Login attempts from such hosts are logged in /log/RESTRICTED.
 *    The format of /secure/save/RESTRICTED is as following:
 *     - each line describes one host. Empty lines or lines beginning with
 *       the # character are ignored.
 *     - each host must be given as numerical ip address, or as full
 *       domainname.
 *     - each component of the host specification may be wildcared.
 *    Example:
 *      #
 *      # Cincinnati Country Day School and their auxiliary hosts.
 *      #
 *      *.fuse.net
 *      207.40.254.*
 *      *.ccds.cincinnati.oh.us
 *      192.203.249.*
 *      *.eos.net
 *      205.133.149.*
 *      208.16.146.*
 *
 *    To create a character from within the mud, call CreateCharacter():
 *
 *      string CreateCharacter( string name, string initial_password)
 *
 *    This lfun will create the character (unless a character with this name
 *    already exists) and return the success/failure as string message.
 *
 * - by restricting the hosts using /ACCESS.ALLOW: this is similar to
 *    the RESTRICTED mechanism, but much more hairy to use. Better forget
 *    about it for now.
 *--------------------------------------------------------------------------
 * A rough description of the login process:
 *
 * 1. the GD(!) calls the lfun logon() in a clone of this object. logon()
 *    must be fast, therefor it just triggers callouts to PrintWelcome()
 *    and TimeOut() and sets the input_to() to InitialLogin().
 *    PrintWelcome() will print the title screen after a few seconds, to
 *    allow players to skip the title by quickly typing the login command.
 *    The callout to TimeOut() makes sure that noone intentionally or
 *    unintentionally clogs up an IP connection by not logging in.
 * 2. InitialLogin() receives the login command from the player and
 *    branches off into the appropriate functions.
 *    But before, it checks with CheckShut() if the game is about to shut
 *    down. CheckShut() will print an appropriate message and can be
 *    called several times.
 *    InitialLogin() does a few other checks (like for host restrictions
 *    and all such).
 *    Depending on the login command, the control flow diverts like this:
 *      help:
 *        Help() is called, then input_to() is set to InitialLogin().
 *      warning:
 *        PrintWarning() is called, then input_to() is set to InitialLogin().
 *      login of a guest:
 *        StartGuest() is called.
 *      login of existing player:
 *        If the password was given in the command, OldPlayerPwd()
 *        is called directly, else the player is prompted to enter his
 *        password and input_to() is set to OldPlayerPwd().
 *      creation of a new player:
 *        If the password was given in the command, NewPlayerPwd()
 *        is called directly, else the player is prompted to enter his
 *        password and input_to() is set to NewPlayerPwd().
 * 3. OldPlayerPwd() receives the password as input and checks if
 *    it matches the one stored for the selected character name.
 *    On a mismatch, the input_to() is set to InitialLogin(), else the
 *    player is started by StartPlayer().
 * 4. NewPlayerPwd() receives the selected password for the character
 *    to create. It does some simple safety checks on it and on success,
 *    prompts the player to re-enter it for verification (input_to() is
 *    set to VfyPlayerPwd()). On failure, the player is asked for a
 *    better password, the input_to() goes to NewPlayerPwd() again.
 * 5. VfyPlayerPwd() receives the re-entered password and checks for
 *    a match with the initially given one. On mismatch, the password-enter
 *    cycle restarts with NewPlayerPwd().
 *    If the passwords match, the character is created by a call to
 *    CreateCharacter() and then started with StartPlayer().
 * 6. StartGuest() starts a guest character. For this, it counts how many
 *    guests already are in the game (removing netdead guests at the
 *    same time). If the number exceeds MAX_GUESTS, the oldest guest is
 *    gently removed. Then, a guestnumber is selected for the new guest
 *    and the name is set to 'guest<number>'. The character is started
 *    by calling StartPlayer(<number>).
 * 7. StartPlayer() starts the character. It receives a number denoting
 *    whether it is a guest (number > 0) or not.
 *    It cares about destructing old player objects (when necessary),
 *    updating the userinfo in the /secure/master and some other things.
 *    Finally it clones the player object (or reconnects to an old one)
 *    and starts it.
 *--------------------------------------------------------------------------
 * TODO: Connections on the www/gopher port (if defined) should directly
 * TODO:: go to some www/gopher login function, instead of InitialLogin().
 * TODO: Add some sort of 'who' to the login commands.
 * TODO: Extend the gopher/www support.
 * TODO: Merge the GDs ACCESS.ALLOW mechanism into the libs RESTRICTED
 * TODO:: mechanism.
 * TODO: With full autoloading, the -fresh option will be unnecessary as
 * TODO:: there won't be netdead players anymore.
 * TODO: The RESTRICTED should permit automatic timeouts.
 * TODO: The retries count should be checked at the input_to(), not
 * TODO:: at the beginning of the functions.
 *--------------------------------------------------------------------------
 */

#include <secure/config.h>
#include <secure/version.h>
#include <secure/wizlevels.h>
#include <driver/debug_message.h>
#include <driver/telnet.h>

#define STDPLAYER   "/std/player"  // The initial player object.
#define STDWIZARD   "/std/wizard"  // The initial wizard object.
#define MAX_GUESTS  5              // Max number of guests.

#define RESTRICTED  "/secure/save/RESTRICTED" // File denoting the hosts with
                                              // restricted access.

#define CLOSED      "/etc/closed/" // Path where the closed files are.

#define SAVEFILE(x)       SAVEPATH+(x)[0..0]+"/"+(x)
#define SECURESAVEFILE(x) SECURESAVEPATH+(x)[0..0]+"/"+(x)

/*---------------------------------------------------------------------------*/

// The prompt texts
#define INITIAL      "Enter your name, or type 'help': "
#define PASSWORD     "Password: "
#define NEWPASSWORD  "Choose a password for your character: "
#define VFYPASSWORD  "Enter your password again: "

inherit "/std/player/telnet_neg";

/*---------------------------------------------------------------------------*/
/* The variables from the /secure/save (and also /secure/banish) files.
 */

string   name;            // The name (lowercase)
string   password;        // The encrypted pwd, or ""/0 for none.
int      level;           // The wizard level
string * domains;         // The list of domains the wizard is member of.
string   shell;           // Unused yet
string   banished_by;     // For banish-files: name of the banishing wizard
string   reason;          // For banish-files: the reason for banishing
int      promotion_age;   // Age of the promotion to wizard/
int      bRemoved;        // True: Wizard has been auto-removed due to absence.

/*---------------------------------------------------------------------------*/

nosave int retries;       // Number of retries left.
nosave int checkedshut;   // True if already checked for shutdown.
// Login command data
nosave string lname;      // Given name
nosave string lpassword;  // Given password

// Login command options.
nosave int enforce;       // True: clone a new player object in all cases.
nosave int newplayer;     // >0: Create a new player
                          // =0: Connect an existing one.
nosave int doecho;        // True: Echo password
nosave int ignoreclosed;  // True: login wizards even if closed.

//---------------------------------------------------------------------------

// Forward decls.

public void InitialLogin(string input); // Handle the initial login command
public void OldPlayerPwd(string input, int direct);
  // Verify the given password
public void NewPlayerPwd(string input, int direct);
  // Set the pwd for a new player
public void VfyPlayerPwd(string input); // Verify the pwd of a new player
public void StartGuest();               // Start a guest character
public void StartPlayer(int guestno);   // Start a player
protected void GopherLogin(string input);
protected void HtmlLogin(string input);
protected void Help(string *args);         // Print a help command
protected void TimeOut();
protected void PrintWelcome();
private void transfer_gmcp(object player);
private void transfer_mxp(object player);

//---------------------------------------------------------------------------
nomask void create ()
{
  ::create();
  object blueprint;

  if (explode(object_name(this_object()), "#")[0] != LOGIN)
  {
    destruct (this_object());
    raise_error("Illegal attempt.\n");
  }
  enforce = 0;
  checkedshut = 0;
  newplayer = 0;
  doecho = 0;
  ignoreclosed = 0;
  retries = 3;

  // Small neato: if the /sys/secure/version.h is newer than the blueprint,
  // the version/patchlevel of the mudlib changed. Destruct the blueprint
  // then so that the next player will get the right numbers.
  if (-1 != member(object_name(this_object()), '#')
   && (blueprint = find_object(LOGIN))
   && program_time() < file_date("/sys/secure/version.h")
     )
    destruct(blueprint);
}

nomask int prevent_shadow (object shadow) { return 1; }

nomask int clean_up(int ref) { destruct(this_object()); return 0; }

public int IsServiceDaemon() { return 1; }

//---------------------------------------------------------------------------
public string
CreateCharacter(string cname, string cpass)

// Create a new character with name <cname> and password <cpass> (this
// may be empty.
// Result is 'Ok.' if creation succeeded, else a error message.

{
  if (!this_interactive()
   || (!IS_WIZARD(this_interactive()) && this_interactive() != this_object())
     ) {
    log_file("RESTRICTED", sprintf("%s %O attempted to create %O\n"
                                  , ctime(), this_interactive(), cname));
    return "Illegal attempt.";
  }
  cname = lower_case(cname);
  seteuid(ROOTID);
  if (restore_object(SECURESAVEPATH+cname[0..0]+"/"+cname))
    return "Character already exists.";

  name = cname;
  password = crypt(cpass, 0);
  level = 0;
  domains = ({ });
  shell = 0;
  banished_by = 0;
  reason = 0;
  promotion_age = 0;
  bRemoved = 0;
  save_object(SECURESAVEPATH+cname[0..0]+"/"+cname);
  log_file("RESTRICTED", sprintf("%s %O created %O\n"
                                , ctime(), this_interactive(), cname));
  seteuid(0);
  return "Ok.";
}

//---------------------------------------------------------------------------
nomask int logon ()

// Logon a new player.
//
// This function is called by the gamedriver to log on a new player.
// It should return 1 to confirm that the function executed properly.
//
// As this function is called from within the poll for incoming messages,
// it shouldn't do much in order to to allow quick responses.

{
  //start_telnetneg();
  efun::input_to("InitialLogin", 0); // let the user be able to type ahead.
  log_file("ENTER", ctime()[4..15]+" login \t"+query_ip_name()+" \t"
                            +to_string(this_object())+"\n");
  if (query_mud_port() == 2348 && tls_available()) {
    string s = "LDMud " __VERSION__;
    tls_init_connection();
    call_out(#'write /*'*/, 0, "Gamedriver " + s +
             " -- Lib '" MUDNAME "' "+LIBVERSION+"."+LIBREVISION+"."+LIBPATCHLEVEL+"\n");
    call_out(#'PrintWelcome /*'*/, 1);  // Print the title screen
    call_out(#'TimeOut /*'*/, 240);
    enforce = 0;
    newplayer = 0;
    doecho = 0;
    ignoreclosed = 0;
    checkedshut = 0;
    retries = 3;
    return 1;
  }
  if (query_mud_port() != WWW_PORT)
  {
    string s = "LDMud " __VERSION__;
    set_telnet(WILL, TELOPT_ECHO); // Start telnet machine
    set_telnet(WONT, TELOPT_ECHO);
    set_telnet(WILL, TELOPT_EOR);  // Fix for some clients
    write("Gamedriver " + s +
      " -- Lib '" MUDNAME "' "+LIBVERSION+"."+LIBREVISION+"."+LIBPATCHLEVEL+"\n"
       );
     call_out(#'PrintWelcome /*'*/, 1);  // Print the title screen
  }
  call_out(#'TimeOut /*'*/, query_mud_port() == WWW_PORT ? 600 : 240);
    // Terminate login after 4 resp. 10 minutes.
  enforce = 0;
  newplayer = 0;
  doecho = 0;
  ignoreclosed = 0;
  checkedshut = 0;
  retries = 3;
  return 1;
}

//---------------------------------------------------------------------------
protected void TimeOut ()

// Login timed out - destruct login object.

{
  if (interactive(this_object())) write("Time out\n");
  if (tls_available() && tls_query_connection_state(this_object()))
    tls_deinit_connection();
  destruct(this_object());
}

//---------------------------------------------------------------------------
protected void LoginError(string errmsg)

// An internal error occured, described by <errmsg>.
// Print an apology to the player and destruct.

{
  write("Sorry, you found an error in the login object.\n");
  destruct(this_object());
  raise_error(errmsg);
}

//---------------------------------------------------------------------------
protected int CheckShut ()

// Check the shutdown object and print any diagnostics.
// Return True if the login may be done.

{ object shut;
  int    tleft, hours, min;

  if (!interactive(this_object()))  // Connection lost on login
    return 0;

  if (checkedshut++)
    return 1;

  shut = find_object(SHUTDEMON);
  if (shut && ({int})shut->query_progress())
  {
    tleft = ({int})shut->query_time_left();
    if (tleft < 60) {
      write("\nSorry, " MUDNAME " is rebooting.\n"
            +"Please try again in a few minutes.\n\n");
      return 0;
    }
    hours = tleft / 3600;
    min = (tleft / 60) % 60;
    write(sprintf("\nWARNING: " MUDNAME " will reboot in %d:%02d hours.\n"
                  "Reason: %s\n\n", hours, min, ({string})shut->query_reason()));
    return 1;
  }

  return 1;
}

//---------------------------------------------------------------------------
protected int
CheckClosed (string type)

// Check if there is a file CLOSED+<type>.
// If yes, print it and return with 1, else return with 0.

{
  if (0 < file_size(CLOSED+type))
  {
    cat(CLOSED+type);
    return 1;
  }
  return 0;
}

//---------------------------------------------------------------------------
protected int
CheckBanish (string name)

// Check if the <name> is banished.
// If yes, print a message and return 1; else return 0.

{
  if (0 < file_size(BANISHPATH+name))
  {
    write("That name is reserved or banished.\n");
    return 1;
  }
  return 0;
}

//---------------------------------------------------------------------------
protected int
RefusedAccess()

// Check if the ip-name/-number is listed in the file of hosts with restricted
// access, namely RESTRICTED.
// Return true if yes, else 0.
{
  string *list, *login, *entry, line;
  int i, j, k;
  list = explode(read_file(RESTRICTED), "\n") - ({ "", 0  });
  login = explode(lower_case(query_ip_name()), ".");
  j = sizeof(login);
  for (i = sizeof(list); i--; ) {
    line = norm_id(list[i]);
    if (!sizeof(line) || '#' == line[0])
      continue;
    entry = explode(list[i], ".");
    k = sizeof(entry);
    if (j == k) {
      for (k = 0; k < j; k++)
        if (entry[k] != "*" && entry[k] != login[k])
          break;
      if (k == j) // Match!
        return 1;
    }
  }
  if (query_ip_name() == query_ip_number())
    return 0;
  login = explode(lower_case(query_ip_number()), ".");
  j = sizeof(login);
  for (i = sizeof(list); i--; ) {
    entry = explode(list[i], ".");
    k = sizeof(entry);
    if (j == k) {
      for (k = 0; k < j; k++)
        if (entry[k] != "*" && entry[k] != login[k])
          break;
      if (k == j) // Match!
        return 1;
    }
  }
  return 0;
}

//---------------------------------------------------------------------------
protected int
CheckRestricted (string name)

// Check if the login for <name> comes from a restricted host.
// If yes, print a message and return true, else return false;

{
  if (file_size(RESTRICTED) > 0 && RefusedAccess())
  {
    write_file("/log/RESTRICTED", ctime()+" Attempted "+name+" from "
              +lower_case(query_ip_name())+"\n");
    write("\nHmmm...\n"
   "Seems that your site has restricted access. To get a character, email\n"
   GAMESMGR " and request the name and password for your\n"
   "character. You will be notified about your character approval\n"
   "or denial.\n\n");
    return 1;
  }
  return 0;
}

//---------------------------------------------------------------------------
int ValidName (string name)

// Check that a name is valid. Allow only lowercase letters.

{
  int i, length;

  length = sizeof(name);
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
protected void
PrintWelcome ()

// Print the title screen and ask for the name.

{
  string * welcomes;
  int i;

  if (!interactive(this_object()))  // Connection lost on login
  {
    destruct(this_object());
    return;
  }

  welcomes = get_dir("/etc/WELCOME*");
  if (sizeof(welcomes))
  {
    i = random(sizeof(welcomes));
    cat("/etc/"+welcomes[i]);
  }
  if (!CheckShut())
  {
     destruct(this_object());
     return;
  }
  write(INITIAL);
  if (({int})this_player()->query_prompt_iacga())
    this_player()->print_ga();
  else
    this_player()->print_eor();
}

//---------------------------------------------------------------------------
protected void
PrintWarning ()

// Print the 'Warning' notice, written by Profezzorn.

{
  write(
"\n"
"WARNING: \n"
"*********\n"
"\n"
"The Surgeon General has determined that Mudding can cause Cancer,\n"
"Liver and Lung Disease, Miscarriages, Unwanted Pregnancies, Low Sperm\n"
"Counts, Brain Tissue Damage, Muscular Dystrophy, Forgetfulness,\n"
"Left-Leg Paralysis, Internal Hemorrhaging, Geekiness, Carbohydrates\n"
"Deficiency, Negligence, Slow Motioness, Bureaucracy, Deforestation,\n"
"Sexual Atrocities, Lack of Personality, Inflation, Heavy Machinery\n"
"Malfunction, Anaphora, Plagiarism, Lying-to-Congress Disease,\n"
"Gerrymandering, Incrimination, Schwa Deletion, Non-Confirmation\n"
"Syndrome, Defenestration, Illiteracy, Sarcasm, Broccoli Overcooking,\n"
"Frequent Defecation, Gossiping, Getting-Laid-on-the-First-Date\n"
"Syndrome, Mass Extinction, Ozone Depletion, Global Warming, Gas Leaks,\n"
"Hailstorms, Arms Race, Legs Race, Gender Confusion, Transexualism,\n"
"Misunderstandings, Gibberish, Boredom, Nausea, Vomiting, Hyperbation,\n"
"Hyperbole, Manuscript Loss, Anarchism, Liposuction, Unsuccessful\n"
"Barbecuing, Gargling, BONKing, Mavisms, Talking-in-Third-Person\n"
"Syndrome, Limosine Overcrowding, Elevator Stalling, Messianism, Low\n"
"Scoring on Standardized Tests, PBS Pledging, PTA Joining, PTL\n"
"Propaganda, S&L Bailouts, Deficit Spending, BL&T Indigestion, Armpit\n"
"Enlargement, Proletarianization, Antidisestablismenterianism, Panic,\n"
"Disillusionment, Asymmetrical Twins, Appointment Cancellation,\n"
"Foreplay Disruption, Stuttering, Dogmatism, Bath Tub Clogging,\n"
"Eviction, Infant Mortality, Cryptorchitis, Shistosomaisis, Nepotism,\n"
"Baldness, Bad Spelling, Maximum Verbosity, and General Failure in\n"
"Life.\n"
"\n"
"So be careful out there - Profezzorn\n"
"\n"
       );
}

//---------------------------------------------------------------------------
public mixed *
ParseInput (string input)

// Parse the input line <input> for the allowed login commands.
// Recognized options are set in the global vars:
//   '-open'    -> doecho
//   '-always' -> ignoreclosed
//   '-fresh'  -> enforce
//
// Return an array with the result:
//   ({ <cmd>, <arg1>, <arg2>, ... }).
// Defined commands are:
//   ({ "help", [<topic>] })
//   ({ "login", <name>, [<password>] })
//   ({ "create", <name>, [<password>] })
//   ({ "restore", <name>, [<password>] })
//   ({ "warning" })
//   ({ "quit" })
//   ({ }) for illegal inputs.

{
  string * words;
  int i;

  // Reset options
  enforce = 0;
  newplayer = 0;
  doecho = 0;
  ignoreclosed = 0;

  words = norm_id(input || "", 1);

  // Look for command options
  if (-1 != (i = member(words, "-open")))
  {
    doecho = 1;
    words[i..i] = ({});
  }
  if (-1 != (i = member(words, "-fresh")))
  {
    enforce = 1;
    words[i..i] = ({});
  }
  if (-1 != (i = member(words, "-always")))
  {
    ignoreclosed = 1;
    words[i..i] = ({});
  }

  // Parse for 'quit'
  if (!sizeof(words) || "quit" == words[0] || "exit" == words[0]
   || 4 == words[0][0] /* Ctrl-D */
     )
  {
    return ({ "quit" });
  }

  // Parse for 'warning'
  if (!sizeof(words) || "warning" == words[0] )
  {
    return ({ "warning" });
  }

  // Parse for 'help'
  if ("help" == words[0] || "'help" == words[0] || "'help'" == words[0]
   || "help'" == words[0]
     )
  {
    if (sizeof(words) > 0)
      return ({ "help" }) + words[1..];
    else
      return ({ "help" });
  }

  // Parse for 'create', 'restore' or 'login'
  if (-1 != member(({ "create", "restore", "login" }),  words[0])
   && (2 == sizeof(words) || 3 == sizeof(words))
     )
    return words;

  // Default: 'login'
  if (1 == sizeof(words) || 2 == sizeof(words))
  {
    return ({ "login" }) + words;
  }

  // Else: Nothing.
  return ({});
}

//---------------------------------------------------------------------------
public void
InitialLogin(string input)

// This function is called with the input for the initial login prompt.

{
  mixed *cmd;

  if (!interactive(this_object()) || retries-- <= 0 || sizeof(input) == 0)
  {
    destruct(this_object());
    return;
  }

  // Remove possible pending title callout
  while(-1 != remove_call_out(#'PrintWelcome /*'*/)) /* skip */ ;

  // Server support
  if ("GET " == input[0..3])
  {
    HtmlLogin(input);
    return;
  }
  if ("gopher:" == input[0..6] || '1' == input[0])
  {
    GopherLogin(input);
    return;
  }

  // Disallow logins at impending shutdown
  if (!CheckShut())
  {
    destruct(this_object());
    return;
  }

  // Parse and evaluate the command line.
  cmd = ParseInput(input);
  if (!sizeof(cmd) || sizeof(cmd[0]) < 2)
  {
    write("Sorry, I don't understand.\n" INITIAL);
    input_to(#'InitialLogin /*'*/, 0);
    return;
  }

  switch(cmd[0])
  {
  case "help":
    Help(cmd[1..]);
    write(INITIAL);
    input_to(#'InitialLogin /*'*/, 0);
    return;

  case "quit":
    destruct(this_object());
    return;

  case "warning":
    PrintWarning();
    write(INITIAL);
    input_to(#'InitialLogin /*'*/, 0);
    return;

  case "login":
    newplayer = 0;
    if (0 > file_size(SECURESAVEFILE(cmd[1])+".o"))
      newplayer = 1;
    break; // continue after the switch

  case "create":
    newplayer = 1;
    if (0 <= file_size(SECURESAVEFILE(cmd[1])+".o"))
    {
      write("A player with that name already exists.\n" INITIAL);
      input_to(#'InitialLogin /*'*/, 0);
      return;
    }
    break; // continue after the switch

  case "restore":
    newplayer = 0;
    if (0 > file_size(SECURESAVEFILE(cmd[1])+".o"))
    {
      write("There is no such player.\n" INITIAL);
      input_to(#'InitialLogin /*'*/, 0);
      return;
    }
    break; // continue after the switch

  default:
    LoginError("Unimplemented command '"+cmd[0]+"' in login.\n");
    return;
  }

  lname = cmd[1];
  lpassword = sizeof(cmd) > 2 ? cmd[2] : 0;

  // Do some extra checks for new players
  if (newplayer)
  {
    if (CheckClosed("NEWPLAYERS")
     || CheckClosed("PLAYERS")
     || CheckClosed("WIZARDS")
       )
    {
      destruct(this_object());
      return;
    }

    if (CheckRestricted(lname)
     || !ValidName(lname)
     || CheckBanish(lname)
       )
    {
      input_to(#'InitialLogin /*'*/, 0);
      return;
    }

    retries = 3;

    if (lpassword)
    {
      NewPlayerPwd(lpassword, 1);
      return;
    }

    write(NEWPASSWORD);
    input_to(#'NewPlayerPwd /*'*/, doecho ? 0 : 1, 0);
  }
  else // Old player
  {
    if (("guest" == lname && CheckRestricted(lname))
     || !ValidName(lname)
     || CheckBanish(lname)
       )
    {
      input_to(#'InitialLogin /*'*/, 0);
      return;
    }

    if ("guest" == lname)
    {
      if (CheckClosed("PLAYERS") || CheckClosed("WIZARDS"))
      {
        destruct(this_object());
        return;
      }
      lpassword = "";
      StartGuest();
      return;
    }

    if (!restore_object(SECURESAVEFILE(lname)))
    {
      LoginError("Error restoring savefile '"+SECURESAVEFILE(lname)+"'.\n");
      return;
    }

    if (!level && (CheckClosed("PLAYERS") || CheckClosed("WIZARDS")))
    {
      destruct(this_object());
      return;
    }

    if (level && CheckClosed("WIZARDS") && !ignoreclosed)
    {
      write("To log on nevertheless, specify option '-always'.\n" INITIAL);
      input_to(#'InitialLogin /*'*/, 0);
      return;
    }

    retries = 3;

    if (!password || !sizeof(password))
    {
      StartPlayer(0);
      return;
    }
    if (lpassword)
    {
      OldPlayerPwd(lpassword, 1);
      return;
    }

    write(PASSWORD);
    input_to(#'OldPlayerPwd /*'*/, doecho ? 0 : 1, 0);
  }
}

//---------------------------------------------------------------------------
public void ContinueLogin(string dummy) { StartPlayer(0); }

protected void
OldWizard (int removed)

// Inform an old wizard that he has been compressed/removed.

{
  write("\n          ++++++++++++++++++++++++++++++++++++++++\n\n"
        "Welcome back, "+capitalize(lname)+"!\n\n");
  if (removed)
    write("Due to your long absence your files were moved out of the mudlib\n"
          "into a save place.\n");
  else
    write("Due to your long absence your files were archived and compressed.\n");
  write("\nTo get them restored, contact one of the Librarians in the mud,\n"
        " or write a mail to  " GAMESMGR " .\n"
        "\nWe are glad to see you here again. Have fun!\n\n"
        "          ++++++++++++++++++++++++++++++++++++++++\n"
        "Press Return to continue login: ");
  input_to(#'ContinueLogin /*'*/);
}

//---------------------------------------------------------------------------
public void
OldPlayerPwd (string input, int direct)

// The function is called with the password of a player about to reconnect.
// If the function is called directly, not by input_to(), <direct> has
// to be true.

{
  if (!interactive(this_object()) || !retries--)
  {
     destruct(this_object());
     return;
  }

  if (!direct && !doecho)
    write("\n");

  lpassword = input;

  if (password && sizeof(password))
  {
    lpassword = input;
    if (!lpassword || !sizeof(password)
     || crypt(lpassword, password) != password)
    {
      if (retries <= 0)
      {
        write("Wrong password!\n");
        destruct(this_object());
	return;
      }
      write("Wrong password - try again...\n" PASSWORD);
      input_to("OldPlayerPwd", doecho ? 0 : 1);
      return;
    }
  }
  else
    write("You have NO PASSWORD!\nSet one with the 'password' command..\n\n");

  if (bRemoved)
    OldWizard(1);
  else if (0 <= file_size(WPATH+lname+"/"+lname+".tar.gz"))
    OldWizard(0);
  else
  {
    if (-1 == member(({"superj", "raistlin", "tune", "mateese"}), lname))
      write("Welcome back to " MUDNAME "!\n");
    else
      write("Oh no! Not you again! All of " MUDNAME " beware!\n");
  }

  StartPlayer(0);
}

//---------------------------------------------------------------------------
public void
NewPlayerPwd (string input, int direct)

// The function is called with the chosen password of a new player.
// If the function is called directly, not by input_to(), <direct> has
// to be true.

{
  int invalid, i;
  string str;

  if (!interactive(this_object()) || !retries--)
  {
     destruct(this_object());
     return;
  }

  if (!direct && !doecho)
    write("\n");

  lpassword = input || "";

  if (!sizeof(lpassword))
    VfyPlayerPwd(lpassword);
  else
  {
    invalid = (lower_case(lpassword) == lower_case(lname));
    str = lpassword;
    for (i = 1; i < sizeof(lpassword); i++)
      str[i-1] = lpassword[<i];
    invalid |= (lower_case(str) == lower_case(lname));
    invalid |= sizeof(lpassword) < 6;

    if (invalid)
    {
      write("That password is insecure.\nChoose a different password: ");
      input_to(#'NewPlayerPwd /*'*/, doecho ? 0 : 1);
    }
    else
    {
      write(VFYPASSWORD);
      input_to(#'VfyPlayerPwd /*'*/, doecho ? 0 : 1);
    }
  }
}

//---------------------------------------------------------------------------
public void
VfyPlayerPwd (string input)

// The function is called with the password verification of a new player.

{
  string rc;

  if (!interactive(this_object()))
  {
     destruct(this_object());
     return;
  }

  if (!doecho)
    write("\n");

  if (lpassword != input)
  {
    write("Entered passwords differed!\n" NEWPASSWORD);
    input_to(#'NewPlayerPwd /*'*/, doecho ? 0 : 1);
    return;
  }
  rc = CreateCharacter(lname, lpassword);
  if ("Ok." != rc)
  {
    LoginError("CreateCharacter() returned '"+rc+"'.\n");
    return;
  }
  StartPlayer(0);
}

//---------------------------------------------------------------------------
public void
StartPlayer(int guestno)

// Start the player <lname>. If it is a guest, <guestno> is its number
// (and <lname> already contains it, too).

{
  object obj, obj2;
  object blueprint;
  mixed tmp;
  string obname;
  int i;

  seteuid(lname);

  // Are we already playing?
  // We can't use find_player() since it relies on the property
  // P_NAME which can be compromised.
  obj = 0;
  for (i = sizeof(users()); i--; )
    if (getuid(users()[i]) == lname)
    {
      obj = users()[i];
    }
  if (!obj)
    obj = find_object("/"+capitalize(lname));

  if (obj)
  {
    if (guestno)
    {
      if (interactive(obj))
        tell_object(obj,
   "Sorry, your time is up.\n"
   "We hope you liked your stay and see you again next time.\n\n"
                   );
      destruct(obj);
    }
    else if (enforce)
    {
      write("Destructing old body...\n");
      MASTER->remove_player(obj);
    }
    else
    {
      write("Reincarnating old body...\n");
      if (interactive(obj))
      {
        tell_object(obj, "You are overtaken by yourself, or something...\n");
        remove_interactive(obj);
      }
      exec(obj, this_object());
      obj->restart_player(lname);
      if (call_resolved(&tmp, obj, "QueryPlayerStarted") && !tmp)
        obj->start_player(lname, 0);
      transfer_gmcp(obj);
      transfer_mxp(obj);
      destruct(this_object());
      return;
    }
  }

  // Allow for a shell
  if (shell && shell != "")
    obname = shell;
  else
    obname = STDPLAYER;

  // Connect to (new) player object
  if (level > 0)
    write(object_name(this_object())+": Connect to "+obname+"...\n");

  // Load the blueprint
  blueprint = find_object(obname);
  if (blueprint && environment(blueprint))
    destruct(blueprint);
  if (!find_object(obname))
    call_other(obname, "???");

  // Clone a player object
  if (catch(obj = clone_object(obname)) || !obj)
  {
    LoginError("Error on loading "+obname+".\n");
    return;
  }

  MASTER->expunge_userinfo(lname);
  if (obj2 = find_object(lname))
    destruct(obj2);
  rename_object(obj, capitalize(lname));
/*
  if (level > LVL_PLAYER)
    set_is_wizard(obj, 1);
*/
  exec(obj, this_object());
  if (guestno)
    obj->start_player(lname, 1);
  else
  {
    // TODO: Voting. Either here or by the start_player()
    obj->start_player(lname, 0);
  }
  transfer_gmcp(obj);
  transfer_mxp(obj);
  destruct(this_object());
}

//---------------------------------------------------------------------------
public void
StartGuest()

// Start a guest player by determining an identifying number for him.
// Allow only MAX_GUESTS online at one time. If an additional guest
// is requested, re-use the oldest one.
// Modify <lname> to hold the guestnumber and call StartPlayer().

{
  object *guests, oldguest;
  int    *flag;
  int     oldtime, i, guestno;
  string  name;

  guests = users();
  flag = allocate(MAX_GUESTS);

  // Detect the oldest guest, while removing netdead guests.
  oldtime = time();
  for (i = sizeof(guests); i--; )
  {
    if (interactive(guests[i]))
    {
      name = to_string(guests[i]);
      if ("/Guest" != name[0..5])
      {
        guests[i..i] = ({});
        continue;
      }
      sscanf(name, "/Guest%d", guestno);
      flag[guestno-1] = 1;
      if (program_time(guests[i]) < oldtime)
      {
        oldguest = guests[i];
        oldtime = program_time(oldguest);
      }
    }
    else
    {
      MASTER->remove_player(guests[i]);
      guests[i..i] = ({});
    }
  }

  if (sizeof(guests) >= MAX_GUESTS) // Say goodbye to the oldest one
  {
    tell_object(oldguest
               , "\nSorry, your time is up.\n"
         "We hope you liked your stay here and are going to see you again.\n\n"
               );
    MASTER->remove_player(oldguest);
    if (oldguest)
      destruct(oldguest);
  }
  else // Find a free guest
  {
    for (guestno = 1; guestno <= MAX_GUESTS; guestno++)
      if (!flag[guestno-1])
        break;
  }
  lname = "guest"+guestno;
  StartPlayer(guestno);
}

//---------------------------------------------------------------------------

/* for debugging purpose */

string QueryShort() { return (name || "<logon>")+" the titleless"; }
string QueryName() { return name || "<logon>"; }
string QueryRealName() { return name || "<logon>"; }

int query_prevent_shadow() { return 1; }


//---------------------------------------------------------------------------
protected void
GopherLogin (string input)
{
  object ob /*, blueprint*/ ;

  seteuid("gopher");

  /* clone a player object */
  ob = clone_object("/secure/net/gopher");
  if (!ob) {
    write("3Error while cloning gopher object\n");
    destruct(this_object());
    return;
  }
  MASTER->expunge_userinfo("gopher");
  exec(ob, this_object());
  if (input[0] == '1') ob->start_player(input[9..]);
  else ob->start_player(input[7..]);
  destruct(this_object());
  return;
}

//---------------------------------------------------------------------------
protected void
HtmlLogin (string input)
{
  object ob /*, blueprint*/ ;
  /*string trash;*/

  seteuid("gopher");
  if (input) sscanf(input, "GET %s", input);
  if (!input || input == "") input = "/";
  /* clone a player object */
  ob = clone_object("/secure/net/www");
  if (!ob) {
    write("Error while cloning www object\n");
    destruct(this_object());
    return;
  }
  MASTER->expunge_userinfo("gopher");
  exec(ob, this_object());
  ob->start_player(input);
  destruct(this_object());
  return;
}

//---------------------------------------------------------------------------
protected void
Help (string * args)

// Print the help information for <args>.

{
  if (!sizeof(args)
   || -1 != member(({"command", "commands", "help", "topic", "topics"}), args[0])
   || sizeof(args) > 1
     )
  {
    retries++;
    write(
"\n"
"At this login prompt you may give one of several commands to enter\n"
"the game. The available commands are:\n"
"\n"
"    login   <name> [<password>] [<options>]\n"
"    create  <name> [<password>] [<option>]\n"
"    restore <name> [<password>] [<options>]\n"
"    warning\n"
"    help    <topic>\n"
"    quit\n"
"\n"
"To get help about one of the commands, enter 'help <command>',\n"
"e.g. 'help login'. The help for the options is available as 'help options'.\n"
"\n"
         );
    return;
  }

  if ( -1 != member(({"option", "options"}), args[0]) )
  {
    retries++;
    write(
"\n"
"The commands 'login', 'create' and 'restore' understand some options which\n"
"modify the login behaviour. The options and their effects are:\n"
"\n"
"    -open : The password inputs are echoed to you.\n"
"       Usually, the password inputs are not echoed for security reasons, but\n"
"       some broken clients or firewalls get confused by this.\n"
"\n"
"    -fresh : You get a fresh player object.\n"
"       Usually, the game tries to find and reactivate the body (player object)\n"
"       you used in a prior session. Sometimes, e.g. if wizards fixed a bug\n"
"       in the player object, you want to be sure you get a fresh body, and the\n"
"       former body (if any) and all its possessions! is removed.\n"
"\n"
"    -always : Login even though the game is closed.\n"
"       Wizards only!\n"
"       Log into the game even though it is closed. As this is not done\n"
"       without reason, you might get into serious trouble by logging in!\n"
"\n"
"The options may be given anywhere in the login command.\n"
"Example: 'login -open wessex -always'\n"
"\n"
         );
    return;
  }

  if ("quit" == args[0] || "exit" == args[0])
  {
    retries++;
    write(
"\n"
"Command: quit\n"
"         exit\n"
"\n"
"Abort the login process and close the connection.\n"
"\n"
         );
    return;
  }

  if ("login" == args[0] )
  {
    retries++;
    write(
"\n"
"Command: login <name> [<password>] [<options>]\n"
"         <name> [<password>] [<options>]\n"
"\n"
"Log into the game with the charactername <name>.\n"
"If you just want to check out " MUDNAME ", login as 'guest'.\n"
"\n"
"The command can be used to login an already existing character, or\n"
"to create a new one.\n"
"If the <password> is not given with the command, the game will prompt\n"
"for it as next input (and turn off echoing to hide it from terminal\n"
"spoofers).\n"
"\n"
"To login an existing character, the given <password> must match the one\n"
"set for the character, else the login will be denied.\n"
"\n"
"When creating a new character, the given <password> will be set as 'the'\n"
"password for the character. It is later used on relogins to verify that\n"
"only the rightful owner of the character is using it.\n"
"For safety reasons, the game will prompt you to re-enter the password.\n"
"\n"
         );
    return;
  }

  if ("restore" == args[0] )
  {
    retries++;
    write(
"\n"
"Command: restore <name> [<password>] [<options>]\n"
"\n"
"Log into the game with the charactername <name>.\n"
"If you just want to check out " MUDNAME ", login as 'guest'.\n"
"\n"
"The command is used to login an already existing character. It will\n"
"deny a login if the character doesn not exist.\n"
"For the login to succeed, the given <password> must match the one\n"
"set for the character.\n"
"If the <password> is not given with the command, the game will prompt\n"
"for it as next input (and turn off echoing to hide it from terminal\n"
"spoofers).\n"
"\n"
"The command is mainly intended for robot logins.\n"
"\n"
         );
    return;
  }

  if ("create" == args[0] )
  {
    retries++;
    write(
"\n"
"Command: create <name> [<password>] [<options>]\n"
"\n"
"Create a new character <name> and log it in to the game.\n"
"If a character with this name already exists, the login will be denied.\n"
"\n"
"To verify that just the 'true' owner uses a character, each character\n"
"is protected by a password. If this <password> is not given with the\n"
"command, the game will prompt for it as next input (and turn off echoing\n"
"to hide it from terminal spoofers).\n"
"For safety reasons, the game will prompt you to re-enter the password.\n"
"\n"
         );
    return;
  }

  if ("warning" == args[0] )
  {
    retries++;
    write(
"\n"
"Command: warning\n"
"\n"
"Prints the 'official' warning notice. Read it before you log in!\n"
"\n"
         );
    return;
  }

  write("Sorry, no help available for that\n\n");
}

private void send_gmcp(string package, string message, varargs mixed* data)
{
#if __EFUN_DEFINED__(json_serialize)
    string pkg = lower_case(package);
    if (pkg != "core") // && !member(packages, pkg)
        return;

    efun::binary_message( ({ IAC, SB, TELOPT_GMCP }), 1 );
    string msg = package + "." + message;
    if (sizeof(data))
        msg += " " + json_serialize(data[0]);
#if __VERSION__ > "3.5.2"
    efun::binary_message(to_bytes(msg, "UTF-8"), 1);
#else
    efun::binary_message(msg, 1);
#endif
    // log_gmcp("Sending: " + msg);
    efun::binary_message( ({ IAC, SE }), 1 );
#endif
}

nosave string* gmcp_messages = ({});
static void receive_gmcp(string data)
{
#if __EFUN_DEFINED__(json_parse)
    string* words = explode(data, " ");
/*
    mixed args;

    if (sizeof(words) > 1)
        args = json_parse(implode(words[1..], " "));
*/
    switch(lower_case(words[0]))
    {
        // Modul Core
        case "core.ping":
            send_gmcp("Core", "Ping");
            return; // no buffering.
        case "char.login": // TODO implement gmcp login
            // name, password, token, wizname
            return; // no buffering
        default:
            break;
    }
#endif
    gmcp_messages += ({ data });
}

private void transfer_gmcp(object player)
{
    player->transfer_gmcp(gmcp_messages);
}

private void transfer_mxp(object player)
{
    player->init_mxp();
}

/***************************************************************************/
