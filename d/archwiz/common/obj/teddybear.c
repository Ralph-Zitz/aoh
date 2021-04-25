/* A cute teddybear. Every Player should have one!                 22-Jan-92
**
** This teddybear is meant as an object to cuddle and it gives a certain
** feeling of security. This may be taken literally: if worn, the bear
** serves as an amulet (yes, I know it's against the rules, but hey:
** it's for poor weak newbies, and they have to find out that they have
** to wear the teddy. (Upon request of the AWoD the defense capability 
** was restricted to newbies only).
*/

#include <config.h>
#include <properties.h>
#include <combat.h>
#include <equipment.h>
#include <search.h>

#define VERSION "970213"

inherit "/std/armour";

/*-------------------------------------------------------------------------*/
create() {
  ::create();
  SetStandard(AT_AMULET, 1);
  Set(P_SHORT, "a cute teddybear");
  // The QueryLong() extends this description :-)
  Set(P_LONG, "Holding it gives you a warm feeling of security.\n");
  Set(P_NAME, "Wessex");
  AddId(({ "teddy", "teddybear" }));
  AddAdjective("cute");
  Set(P_VALUE, 200);
  Set(P_WEIGHT, 500);
  Set(P_INFO, "Mateese's teddybear, #"+VERSION);
  Set(P_HELP_MSG,
"A teddybear is a toy which is fun to play with, e.g. to touch, to bounce, "
"to pet etc. Every teddy has a name, which you of course can change.\n"
     );
}

int id (string arg) {
  string name;
  name = Query(P_NAME);
  if (stringp(name) && strlen(name) && norm_id(arg) == norm_id(name))
    return 1;
  return ::id(arg);
}

/*-------------------------------------------------------------------------*/
/* Some special property handling...
 */

string QueryLong() {
  string desc, name;
  name = Query(P_NAME);
  if (stringp(name) && strlen(name))
    desc = "A cute teddybear called '"+capitalize(name)+"'.\n";
  else
    desc = "A cute teddybear. ";
  return desc+::QueryLong();
}

string QueryHelpMsg() {
  string msg;
  msg = ::QueryHelpMsg();
  if (this_player()->Query(P_REAL_LEVEL) < 6)
    msg += "Furthermore, wearing a teddy gives you at least the feeling of security.\n";
  return msg;
}

int QueryAC() {
  object wearer;
  
  wearer = Query(P_WORN);
  if (wearer && wearer->Query(P_REAL_LEVEL) < 6)
    return ::QueryAC();
  return 0;
}

void NotifyWear(object by, object thing, int flags)
{
  if (thing == this_object() && !(EWF_SILENT & flags)
   && by->Query(P_REAL_LEVEL) < 6
     )
    tell_object(by, "You get a warm feeling of security.\n");
  ::NotifyWear(by, thing, flags);
}

void NotifyUnwear(object by, object thing, int flags)
{
  if (thing == this_object() && !(EWF_SILENT & flags)
   && by->Query(P_REAL_LEVEL) < 6
     )
    tell_object(by, "You feel less secure.\n");
  ::NotifyUnwear(by, thing, flags);
}

/*-------------------------------------------------------------------------*/
void FartOn (object enemy, object wearer)
{
  tell_object(enemy, Query(P_NAME)+" farts on you.\n");
  tell_room(environment(wearer)
           , Query(P_NAME)+" farts on "+enemy->Query(P_NAME)+".\n"
           , ({ enemy })
           );
}

int Defend (object enemy, int damage, int dtype) 

/* Just a nice message in combat...
 */

{
  object wearer;
  wearer = Query(P_WORN);
  if ((Query(P_NAME)||"") == "Wessex" 
   && wearer && wearer != enemy
   && damage
   && random(10) > 7
   && this_player()
     )
  {
    call_out(#'FartOn, 0, enemy, wearer);
  }
  return ::Defend(enemy, damage, dtype);
}

/*-------------------------------------------------------------------------*/
init() {
  ::init();
  add_action ("fname",    "name");
  add_action ("finspect", "inspect");
  add_action ("fpet",     "touch");
  add_action ("fpet",     "pet");
  add_action ("fpet",     "stroke");
  add_action ("fcuddle",  "ruffle");
  add_action ("fcuddle",  "cuddle");
  add_action ("fcuddle",  "hug");
  add_action ("fpress",   "squeeze");
  add_action ("fpress",   "press");
  add_action ("fpress",   "push");
  add_action ("fplay",    "play");
  add_action ("ffeed",    "feed");
  add_action ("fadmire",  "admire");
  add_action ("fbounce",  "bounce");
  add_action ("fflip",    "flip");
  add_action ("ffondle",  "fondle");
  add_action ("fkiss",    "kiss");
  add_action ("flick",    "lick");
  add_action ("flove",    "love");
  add_action ("fnose",    "nose");
  add_action ("fpraise",  "praise");
  add_action ("fbite",    "bite");
  add_action ("fbite",    "vbite");
}

/*-------------------------------------------------------------------------*/
/* This makes the description like "his teddy" or "<name>, her teddy" */

static string makedesc() {
  string s, name;
  s = this_player()->QueryPossessive()+" teddy";
  name = Query(P_NAME);
  if (stringp(name) && strlen(name)) 
    s = capitalize(name) + ", " + s;
  return s;
}

static int isitme(string arg, string emsg)
{
  if (this_object() == search_inv_env(arg))
    return 1;
  notify_fail(emsg, NOTIFY_NOT_OBJ);
  return 0;
}

/*-------------------------------------------------------------------------*/
int fname (string arg) 

/* Rename the teddy. */

{
  string *args;
  string name;

  if (!arg) return 0;
  args = norm_id(arg, 1);
  name = args[<1];
  if (this_object() != search_inv_env(implode(args[0..<2], " ")))
  {
    if (this_object() != search_inv_env(implode(args, " ")))
    {
      notify_fail("What do you want to give a name?\n", NOTIFY_NOT_OBJ);
      return 0;
    }
    name = "";
  }

  Set(P_NAME, capitalize(name));
  if (strlen(name))
    write("The teddy is now called '"+capitalize(name)+"'\n");
  else
    write("The teddy has no name anymore.\n");
  return 1;
}

/*-------------------------------------------------------------------------*/
int finspect (string arg) {
  if (!arg || this_object() != search_inv_env(arg))
  {
    notify_fail("What do you want to inspect?\n", NOTIFY_NOT_OBJ);
    return 0;
  }
  write ("A small sign reads:\n"+
         "  Cute Teddybear -- #"+VERSION+"\n"+
         "  Biontic Magics & Darkness\n"
        );
  return 1;
}

/*-------------------------------------------------------------------------*/
fadmire (str) {
  if (!isitme(str, "Admire what?\n")) return 0;
  show (this_player()->Query(P_NAME)+" "+query_verb()+"s "+makedesc()+".\n");
  write ("What a nice teddy you have!\n");
  return 1;
}

fbounce (str) {
  if (!isitme(str, "Bouncing what?.\n")) return 0;
  show (this_player()->Query(P_NAME)+" "+query_verb()+"s with "+makedesc()+".\n");
  write ("You bounce your teddy around the room.\n");
  return 1;
}

fflip (str) {
  if (!isitme(str, "Flip what?\n")) return 0;
  show (this_player()->Query(P_NAME)+" "+query_verb()+"s "+makedesc()+".\n");
  write ("You flip your teddy upside down.\n");
  return 1;
}

ffondle (str) {
  if (!isitme(str, "Fondle who?n")) return 0;
  show (this_player()->Query(P_NAME)+" tries to "+query_verb()+" "+makedesc()+".\n");
  write ("Funny idea.\n");
  return 1;
}

fkiss (str) {
  if (!isitme(str, "Kiss who?\n")) return 0;
  show (this_player()->Query(P_NAME)+" "+query_verb()+"es "+makedesc()+".\n");
  write ("You give your teddy a soft kiss.\n");
  return 1;
}

flick (str) {
  if (!isitme(str, "Lick who?\n")) return 0;
  show (this_player()->Query(P_NAME)+" "+query_verb()+"s "+makedesc()+".\n");
  write ("Yeech! You get a mouthful of teddy fluff\n");
  return 1;
}

flove (str) {
  if (!isitme(str, "Love who?\n")) return 0;
  show (this_player()->Query(P_NAME)+" really"+query_verb()+"s "+makedesc()+".\n");
  write ("If nobody else would love you, this teddy does.\n");
  return 1;
}

fnose (str) {
  if (!isitme(str, "Nose who?\n")) return 0;
  show (this_player()->Query(P_NAME)+" "+query_verb()+"s "+makedesc()+".\n");
  write ("You rub your nose at you teddy. Wonderful!\n");
  return 1;
}

fpraise (str) {
  if (!isitme(str, "Praise who?\n")) return 0;
  show (this_player()->Query(P_NAME)+" "+query_verb()+"s "+makedesc()+".\n");
  write ("Your teddy is the best!\n");
  return 1;
}

fpet (str) {
  if (!isitme(str, "Touching the pure air isn't quite helpful.\n")) return 0;
  show (this_player()->Query(P_NAME)+" "+query_verb()+"s "+makedesc()+".\n");
  write ("You feel the warm teddy skin under your fingers.\n");
  return 1;
}

fcuddle (str) {
  if (!isitme(str, "It needs two for that.\n")) return 0;
  show (this_player()->Query(P_NAME)+" "+query_verb()+"s "+makedesc()+".\n");
  write ("Aaah, you feel more secure now.\n");
  return 1;
}

fpress (str) {
  if (!isitme(str, "What?\n")) return 0;
  show (this_player()->Query(P_NAME)+" "+query_verb()+"es "+makedesc()+".\nBAAAAH!\n");
  write ("BAAAAH!\n");
  return 1;
}

fplay (str) {
  string s;

  if (str && !isitme(str, "You can't play with that!\n")) return 0;
  show (this_player()->Query(P_NAME)+" has fun playing with "+makedesc()+".\n");
  s = Query(P_NAME);
  if (stringp(s) && strlen(s))
    s = capitalize(s);
  else
    s = "your teddy";
  write ("You play a while with "+s+". Wow, what fun!\n");
  return 1;
}

ffeed (str) {
  if (!isitme(str, "Who do you want to feed?\n")) return 0;
  show (this_player()->Query(P_NAME)+" ridiciously tries to feed "+this_player()->QueryPossessive()+
       " teddy.\n");
  write ("This is just a normal teddybear! You can't feed it!\n");
  return 1;
}

fbite (str) {
  if (!isitme(str, "Who do you want to bite?\n")) return 0;
  if ("vampire" == this_player()->Query(P_GUILD))
  {
    show(({ this_player()->Query(P_NAME)+" bites into "
	    +this_player()->QueryPossessive()+" teddy.\n"
	    "Its painful squeak pierces your ears.\n"
          , "A sudden, painful squeak pierces your ears.\n"
          })
	);
    write("Your teddy squeaks painfully as you sink your fangs deep into its neck.\n");
  }
  else
  {
    show (this_player()->Query(P_NAME)+" bites into "
	  +this_player()->QueryPossessive()+" teddy, then spits out little "
	   "pieces of fluff.\n");
    write("You bite your teddy and get the mouth full of fluff. Yech!\n");
  }
  return 1;
}


