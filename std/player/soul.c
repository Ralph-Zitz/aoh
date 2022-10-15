/*
 * 'learning' soul
 *
 * by Joern
 * Pepel converted it to new QueryFOO() conventions
 * Wurzel removed some typos and added proper frog+ghost handling
 * Pepel cleaned up shouting and added earmuffs
 * Taube corrected the command '.'
 * Organimedes deleted all communcications stuff.
 * Organimedes cleaned up a bit and added comments and help.
 * Organimedes added lots of predefined verbs and adverbs.
 * (greatly inspired by the exsoul of Ereborn the Master of Souls)
 * Pepel did some space saving
 * Joern added some ideas by players
 * Sique implemented "to =<channel>"
 * Mistral 'fixed' use of 'the' in theoretically. now its 'teo'...
 *    the use of 'the' article was wanted more often.
 * Mistral implemented "my =<channel>"
 */

/* in the phrases we use the following abbrevs:
 * & : some object
 * @ : a living (player or npc)
 * ^ : the target the verb is directed to
 * ^^: the genitiv of the target the verb is directed to
 * < : the possesiv pronoun for the acting player (his/her/its)
 * | : replacement for a comma
 * & : replacement for 'and'
 */


#include <config.h>
#include <properties.h>
#include <msgclass.h>
#include <daemons.h>
#include <secure/wizlevels.h>

/*#include "soultst.h"*/

#define PL this_player()
#define TO this_object()
#define PO previous_object()

/* the default verbs, shared between the blueprint and all clones.
 * never fiddle with these !
 */
private nosave mapping dverbs;
private nosave mapping dadverbs;

/* Save these variables: */
mapping	verbs;
mapping adverbs;
string presay;	/* is it needed ? */

/* Function declaration */
string QueryName();
string QueryPronoun();
string QueryPossessive();
string QueryObjective();
string QueryGhost();
string QueryFrog();
int SetFrog(int f);
string QueryRace();
varargs mixed *do_transform(string verb, string params, int no_presay, int player_only);
public void announce_title(int news);   // std/player

static int set_verb(string str);
static int del_verb(string str);
static int del_verb(string str);
static int fverbs(string str);
static int set_adverb(string str);
static int del_adverb(string str);
static int fadverbs(string str);
static int to_target(string arg);
static int my_target(string arg);
static int doit(string str);

/* The procedure is called from move_player_to_start4() from /std/player.c */
void config_soul() {
  if (!presay) presay = "";
  if (!mappingp(verbs)) verbs = ([]);
  if (!mappingp(adverbs)) adverbs = ([]);
  add_action("doit","",1);                /*'*/
  add_action("set_verb","set_verb");      /*'*/
  add_action("del_verb","del_verb");      /*'*/
  add_action("fverbs","verbs");           /*'*/
  add_action("set_adverb","set_adverb");  /*'*/
  add_action("del_adverb", "del_adverb"); /*'*/
  add_action("fadverbs","adverbs");       /*'*/
  add_action("to_target","to");           /*'*/
  add_action("my_target","my");           /*'*/
}

/* some internal functions to share the default verbs/adverbs between the
 * blueprint and its clones
 */

mapping qv() {
  if (explode(object_name(PO), "#")[0] != "/std/player" /*object_name(TO)*/)
    return 0;
  return dverbs;
}

mapping qa() {
  if (explode(object_name(PO), "#")[0] != "/std/player" /*object_name(TO)*/)
    return 0;
  return dadverbs;
}

void create() {
  if (clonep()) {
    dverbs = ({mapping})"/std/player"->qv();
    dadverbs = ({mapping})"/std/player"->qa();
    return;
  }
  if (object_name(TO) != "/std/player") return;
  /* all verbs from the exsoul (sometimes without the very special features)
   * except:
   * "fail <str>"	 You try to <str> but fail.
   * "nose <str>"	 You rub your nose tenderly on <str>'s.
   * The parser tries to catch main sentences which are separated
   * by "and" by assuming that an 'and' startes the next sentence.
   * (except when the word before ends with a ',' (comma)
   */

   //  First string: what the player by giving 'verb' alone
   //  Second string:  verb <player>
   //  Last arg:  0 or 1, if this verb can be aimed at another player
   //  '@' :  placeholder for another player
   //  '&' :  placeholder for player OR object
   //  '$' :
   //  '$$' : player's name is shown in possesive (Joan's)
   //  '^' : placeholder for the target (player?)

  dverbs =
    ([
      "ack"  :  ({ "loudly", "at", 1 }),
      "admire" : ({ "&","",1 }),
      "apologize" : ({ "to the audience","to",1 }),
      "applaud" : ({ "$ give a round of applause","to",1 }),
      "blink" : ({ "", "at", 1 }),
      "bang" : ({ "your head against the next wall","",0 }),
      "blush" : ({ "$$ cheeks are burning","",0 }),
      "bop" : ({ "@", "$ bop ^ on the head", 1 }),
      "bounce" : ({ "around","",0 }),
      "bow" : ({ "gracefully","to",1 }),
      "burp" : ({ "rudely","",0 }),
      "burst" : ({ "into tears","",0 }),
      "cackle" : ({ "$ throw your head back and cackle with glee!","",0 }),
      "chuckle" : ({ "politely","",0 }),
      "cheer" : ({ "enthusiastically","",0 }),
      "clap" : ({ "briefly","",0 }),
      "comfort" : ({ "@","",1 }),
      "complain" : ({ "bitterly about Wessex, the world, and life in general",
			"bitterly about",1 }),
      "congrat" : ({ "@","",1 }),
      "cough" : ({ "noisily","",0 }),
      "croak" : ({ "$ croak emphatically","",0 }),
      "cry" : ({ "$ burst into tears","",0 }),
      "cuddle" : ({ "&","",1 }),
      "curse" : ({ "violently","",1 }),
      "curtsey" : ({ "gracefully","to",1 }),
      "dance" : ({ "some fancy footsteps",
		     "$ sweep ^ across the dancefloor",1 }),
      "excuse" : ({ "yourself shamefully","yourself to",1 }),
//    "fail" : ({ "what?","",0 }),
      "fart" : ({ "$ let off a real rip-roarer","",0 }),
      "fear" : ({ "$ shiver with fear","$ look frightfully at",1 }),
      "flip" : ({ "head over heels","",0 }),
      "fondle" : ({ "@","",1 }),
      "french" : ({ "@",
        "$ give ^ a deep & passionate kiss... it seems to take forever...",
		      1 }),
      "frown" : ({ "","at",1 }),
      "gack" : ({ "", "at", 1 }),
      "gasp" : ({ "in astonishment!","",0 }),
      "giggle" : ({ "inanely","at",1 }),
      "glare" : ({ "at the floor","at",1 }),
      "greet" : ({ "@","", 1 }),
      "grin" : ({ "evilly","at",1 }),
      "groan" : ({ "loudly","",0 }),
      "grope" : ({ "@","",1 }),
      "growl" : ({ "","",0 }),
      "hair" : ({ "$ pulls out handfuls of hair", "$ pulls ^^ hair", 1 }),
      "hate" : ({ "@","",1 }),
      "hiccup" : ({ "","",0 }),
      "hmm" : ({ "doubtfully","",0 }),
      "howl" : ({ "loudly!", "at", 1 }),
      "hug" : ({ "@","",1 }),
//    "ignore" : ({ "$ try to ignore all the present living beings","",1 }),
      "jump" : ({ "for joy", "", 0 }),
      "kick" : ({ "a long-forgotten, invisible, dead turtle","",1 }),
      "kiss" : ({ "@","",1 }),
      "knee" : ({ "@ in the groin","",1 }),
      "hop" : ({ "around with a hoppy smile", "on top of",1}),
      "kneel" : ({"down","down before",1 }),
      "laugh" : ({ "$ fall down laughing","at",1 }),
      "lick" : ({ "@","",1 }),
//    "listen" : handled by the /std/player/viewcmd, /std/npc/view
//    "look" : handled by the /std/player/viewcmd, /std/npc/view
      "love" : ({ "@","$ whisper softly to",1 }),
      "mercy" : ({ "$ kneel down and beg for mercy",
		     "$ kneel down before ^ and beg for mercy",1 }),
      "moan" : ({ "$ start moaning","",0 }),
      "mumble" : ({ "something","",0 }),
      "mutter" : ({"darkly", "at",1 }),
      "nein" : ({ "$ shake your head","",0 }),
      "nibble" : ({ "@","on ^^ ear",1 }),
      "no" : ({ "$ shake your head in disagreement","",0 }),
      "nyet" : ({ "$ shake your head in disagreement","",0 }),
      "nod" : ({ "solemnly","at",1 }),
      "nose" : ({ "@","",1 }),
      "pale" : ({ "$$ face turns as pale as ashes","",0 }),
      "panic" : ({ "","",0 }),
      "pat" : ({ "@","^ on the head",1 }),
      "pet" : ({ "&","",1 }),
      "pinch" : ({ "@","",1 }),
      "point" : ({ "at the sky","at",1 }),
      "poke" : ({ "@","^ in the ribs",1 }),
      "ponder" : ({ "deeply", "", 0 }),
      "pout" : ({ "","",0 }),
      "praise" : ({ "&","",1 }),
      "protest" : ({ "vehemently","",0 }),
      "puke" : ({ "$ double over and puke","on",1 }),
      "purr" : ({ "contently","",0 }),
      "puzzle" : ({ "$ look puzzled","",0 }),
      "raise" : ({ "^^ hands","",1 }),
      "relax" : ({ "$ sit comfortably down and relax","",0 }),
      "roll" : ({ "< eyes with exasperation", "< eyes at", 1 }),
      "ruffle" : ({ "@","playfully the hair of",1 }),
      "scratch" : ({ "yourself, thinking","",0 }),
      "scream" : ({ "loudly!","",0 }),
      "shake" : ({ "head vehemently","hands with",1 }),
      "shame" : ({ "$ stumble some words of excuse","",0 }),
      "shiver" : ({ "from the cold","",0 }),
      "shrug" : ({ "helplessly","",0 }),
      "sigh" : ({ "deeply","",0 }),
      "sing" : ({ "O Sole Mio","",0 }),
      "slap" : ({ "@","",1 }),
      "smile" : ({ "happily","at",1 }),
      "smirk" : ({ "","",0 }),
      "snap" : ({ "your fingers","",0 }),
      "sneeze" : ({ "","",0 }),
      "snicker" : ({ "","",0 }),
      "sniff" : ({ "","",0 }),
      "snore" : ({ "loudly","",0 }),
      "snuggle" : ({ "@","up to",1 }),
      "sob" : ({ "","",0 }),
      "spit" : ({ "on the ground","on",1 }),
      "squeeze" : ({ "@","^ fondly",1 }),
      "stare" : ({ "into space","at",1 }),
      "strut" : ({ "proudly","",0 }),
      "sulk" : ({ "in the corner","",0 }),
      "sweat" : ({ "like a palatinian brook cow","",0 }),
//    "tap" ??
      "thank" : ({ "@","",1 }),
      "think" : ({ "deeply","of",1}),
      "throat": ({ "$ clear your throat noisily","",0 }),
      "tickle" : ({ "@","",1 }),
      "tongue" : ({ "$ stick your tongue out",
		      "$ stick your tongue out at",1 }),
//    "turn" ??
      "twiddle" : ({ "your thumbs","",0 }),
//    "type" ??
      "wail" : ({ "quietly","",0 }),
      "wait" : ({ "patiently...","",0 }),
      "wake" : ({ "up @","",1 }),
      "wave" : ({ "farewell","farewell at",1 }),
//    "whisper" is treated separately
      "whistle" : ({ "appreciatively","",0 }),
      "wiggle" : ({ "your bottom","",0 }),
      "wink" : ({ "suggestively","at",1 }),
      "wipe" : ({ "the sweat from your forehead","",0 }),
      "yawn" : ({ "","",0 }),
      "yelp" : ({ "sharply","at",1 })
      ]);

dadverbs = ([
  "abs":"absentmindedly", "aim":"aimlessly", "ama":"amazedly",
  "amu":"amusedly", "ang":"angrily", "anx":"anxiously",
  "app":"appreciatively", "ast":"astonishedly", "awf":"awfully",
  "bad":"badly", "bit":"bitterly", "bor":"boredly", "brg":"brightly",
  "bri":"briefly", "bro":"broadly", "bus":"busily", "car":"carefully",
  "cha":"charmingly", "chi":"childishly", "cnf":"confusedly",
  "cns":"consolingly", "cof":"confidently", "col":"coldly",
  "con":"confusedly", "con":"contemptuously", "cou":"courageously",
  "cow":"cowardly", "cra":"crazily", "ctd":"contentedly",
  "cun":"cunningly", "cur":"curiously", "cut":"cutely",
  "cyn":"cynically", "dan":"dangerously", "dee":"deeply",
  "dem":"demonically", "dep":"depressively", "des":"desperately",
  "dev":"devilishly", "dig":"disgustedly", "dir":"dirtily",
  "dis":"disappointedly", "dou":"doubtfully", "dre":"dreamily",
  "egi":"egoistically", "ego":"egocentrically", "end":"endearingly",
  "ent":"enthusiastically", "ero":"erotically", "evi":"evilly",
  "exh":"exhaustedly", "fan":"fanatically", "fat":"fatherly",
  "fea":"fearfully", "fie":"fiercefully", "fir":"firmly",
  "foo":"foolishly", "frg":"frightfully", "fri":"friendly",
  "fun":"funnily", "gle":"gleefully", "gra":"gracefully",
  "hap":"happily", "har":"harmonically", "hat":"hatefully",
  "hea":"heartbrokenly", "hel":"helpfully", "hep":"helplessly",
  "hev":"heavily", "hon":"honestly", "hop":"hopefully",
  "hun":"hungrily", "hys":"hysterically", "ign":"ignorantly",
  "imp":"impatiently", "ina":"inanely", "inn":"innocently",
  "ins":"insanely", "int":"interestedly", "inw":"inwardly",
  "iro":"ironically", "ist":"instantly", "jea":"jealously",
  "joy":"joyfully", "kin":"kindly", "kno":"knowingly", "laz":"lazily",
  "lon":"longingly", "lou":"loudly", "lov":"lovingly",
  "lus":"lustfully", "mad":"madly", "man":"maniacally",
  "mel":"melancholically", "mer":"merrily", "mis":"mischievously",
  "mot":"motherly", "mus":"musically", "nas":"nastily",
  "nat":"naturally", "ner":"nervously", "nic":"nicely", "noi":"noisily",
  "pai":"painfully", "par":"patronizingly", "pas":"passionately",
  "pat":"patiently", "per":"perfectly", "pes":"personally",
  "phy":"physically", "pla":"playfully", "pol":"politely",
  "prf":"professionally", "pro":"proudly", "pru":"profoundly",
  "que":"questioningly", "qui":"quickly", "ran":"randomly",
  "rea":"really", "reb":"rebelliously", "rud":"rudely", "sad":"sadly",
  "sai":"sadistically", "sar":"sarcastically", "sat":"satanically",
  "sco":"scornfully", "sea":"searchingly", "sed":"seductively",
  "sen":"sensually", "ser":"seriously", "sex":"sexily",
  "sha":"shamelessly", "shy":"shyly", "sic":"sickly", "sil":"silently",
  "ski":"skilfully", "sle":"sleepily", "sli":"slightly", "slo":"slowly",
  "smi":"smilingly", "sof":"softly", "sol":"solemnly",
  "str":"strangely", "stu":"stupidly", "sug":"suggestively",
  "swe":"sweetly", "tea":"tearfully", "ten":"tenderly",
  "ter":"terribly", "tha":"thankfully", "teo":"theoretically",
  "tho":"thoughtfully", "tig":"tightly", "tir":"tiredly",
  "tra":"tragically", "trs":"trustfully", "tru":"truly",
  "und":"understandingly", "une":"unexpectedly", "unh":"unhappily",
  "unk":"unknowingly", "vic":"viciously", "vig":"vigorously",
  "vir":"virtually", "war":"warmly", "wea":"wearily",
  "who":"wholeheartedly", "wic":"wickedly", "wil":"wildly",
  "wis":"wisely", "wit":"wistfully"
]);

}

#define SETVERB(verb, adverb, add_str, yesno) \
  verbs += ([ verb : ({ adverb, add_str, "yes"==yesno?1:0 }) ])
#define QUERYMYVERB(v) verbs[v]
#define QUERYVERB(v) (verbs[v] || dverbs[v])
#define DELVERB(v) verbs = m_delete(verbs, (v))
#define SETADVERB(abbr,adv) adverbs += ([ (abbr):(adv) ])
#define QUERYMYADVERB(a) adverbs[a]
#define QUERYADVERB(a) (adverbs[a] || dadverbs[a])
#define DELADVERB(a) adverbs = m_delete(adverbs, (a))

string SetPresay(string s) {
#if 1
  if (   !this_player()
      || (    this_player() != this_object()
           && query_user_level(this_player())
           && !query_user_level(this_object())
         )
     )
    log_file("TITLES", ctime()+" Presay of "+getuid(this_object())+" set to '"
                      +s+"' by "+(this_player()?getuid(this_player()):"???")
                      +"\n");
#endif
  presay = s;
  announce_title(0);
  return presay;
}
string QueryPresay() { return presay || ""; }

/***************************************************************************
 * Some more internal functions
 */
protected string caps(object ob) {
  if (!ob) return "Nobody";
  return capitalize(({string})ob->QueryName() || ({string})ob->QueryShort()
		    || "Nobody");
}

/*
 * gets the real name :-) of a player
 */
protected string myname() {
  if (!presay || "" == presay) return QueryName();
  else return (presay+QueryName());
}

/* add full stops and "\n" as desired, capitalize sentence */
protected string tune(string s) {
  int c;
  c = s[<1];
  if (c != '.' && c != '!' && c != '?' && c != ',') s += ".";
  if (c != '\n') s += "\n";
#if 0
  if (s[<1..<1] != "." && s[<1..<1] != "!" && s[<1..<1] != "?"
      && s[<1..<1] != ",")
    s += ".";
  if (s[<1..<1] != "\n") s += "\n";
#endif
  return capitalize(s);
}

/***************************************************************************
 * These are the functions called by doit()
 * We follow mostly Mateese's convention: call with parameter "?" gives help
 * (anyways, this is not always reasonable :-)
 */

/***************************************************************************
 * set/remove/query verbs
 */

static int fverbs(string str) {
  mixed *s, *t;
  int yesno;

  if ("?" == str)
    write("verbs        to see all defined verbs,\n"
	  "verbs <verb> to see definition for <verb>\n");
  else if (str) {
    if (s = QUERYVERB(str)) {
      yesno = s[2];
      write(str+": "+s[0]+","+s[1]+","+(yesno?"yes":"no")+"\n");
      s  = do_transform(str);
      if (!s[1] || s[1] == "") s[1] = "\n"; /* better formatting */
      if (yesno) t = do_transform(str, QueryName());
      write("You would see:    "+s[0]);
      if (yesno) write("                  "+t[0]);
      write("Others would see: "+s[1]);
      if (yesno) {
	write("                  "+t[1]);
	write("<player> would see: "+t[3]);
      }
    } else write("Verb "+str+" is not defined.\n");
  } else {
    string *v;

    v = sort_array(m_indices(dverbs), #'>);
    write("Pre-Defined verbs (may be overloaded):\n");
    printf("%-78#s\n", implode(v, "\n"));

    v = sort_array(m_indices(verbs),#'>); /* sort alphabetically */
    write("Self-Defined verbs (may be changed or deleted):\n");
    printf("%-78#s\n", implode(v, "\n"));
  }
  return 1;
}

static int set_verb(string str) {
    string verb, adverb, add_str, yesno;
    string *s;

    if (!str || sscanf(str,"%s,%s,%s,%s",verb,adverb,add_str,yesno)!=4) {
	write("set_verb <verb>,<adverb>,[additional_string],yes|no\n\
       The [additional_string] is optional.\n\
       For more documentation, type 'help soul'.\n");
	return 1;
    } else if (s = QUERYMYVERB(verb)) {
        write("You already defined "+verb+" as\n"+
	      s[0]+","+s[1]+","+(s[2]?"yes":"no")+".\n"
     "For redefining it, remove the old definition first (with del_verb).\n");
	return 1;
    }

    SETVERB(verb, adverb, add_str, yesno);
    fverbs(verb);
    return 1;
}

static int del_verb(string str) {
  if (str == "?") write("del_verb <verb> to delete definition for <verb>\n");
  else if (!QUERYMYVERB(str)) write("You did not define verb "+str+".\n");
  else {
    DELVERB(str);
    write("Removed verb "+str+".\n");
  }
  return 1;
}

/***************************************************************************
 * set/remove/query adverbs
 */
static int set_adverb(string str) {
  string adv, abbr, s;

  if (!str || ("?" == str) || (sscanf(str,"%s,%s",abbr,adv) != 2)) {
    write("set_adverb <abbrev>,<adverb>\n"
	  "Example: 'set_adverb evi,evilly'\n");
  } else if (s = QUERYMYADVERB(abbr)) {
    write("You already defined abbreviation "+abbr+" for adverb "+s+".\n"
    "If you want to redefine it, you have to delete the old adverb first.\n");
  } else {
    SETADVERB(abbr,adv);
    write("Ok.\n");
  }
  return 1;
}

private void walkmap(string key, string val, string *arr, int n) {
  arr[--n] = sprintf("%'.'-5s%'.'19s", key, val);
}

static int fadverbs(string str) {
  string s, *a;
  int    n;

  if ("?" == str) {
    write("adverbs        to show all adverbs and abbreviations.\n"
	  "adverbs <abbr> to show the adverb for abbreviation <abbr>.\n");
  } else if (str) {
    s = QUERYADVERB(str);
    if (!s) write("No adverb found for abbreviation "+str+".\n");
    else    write(str+" is the abbreviation for "+s+".\n");
  } else {
    /* The design of the following list has been taken from the exsoul */
    write("Predefined adverbs (may be redefined):\n"
"Abbrev____________Adverb  Abbrev____________Adverb  Abbrev____________Adverb\n");
    a = allocate(n = sizeof(dadverbs));
    walk_mapping(dadverbs, #'walkmap, a, &n);
    a = sort_array(a, #'> /*'*/);
    printf("%-78#s\n", implode(a, "\n"));
    a = allocate(n = sizeof(adverbs));
    walk_mapping(adverbs, #'walkmap /*'*/, a, &n);
    a = sort_array(a, #'> /*'*/);
    printf("\nSelf-defined adverbs (may be changed or deleted):\n%-78#s\n",
	   implode(a, "\n"));
  }
  return 1;
}

static int del_adverb(string str) {
  string v;
  int    i;

  if (!str || "?" == str) {
    write("del_adverb <abbr>   to delete the abbreviated adverb <abbr>.\n"
	  "del_adverb <adverb> to delete the abbreviation for <adverb>.\n");
    return 1;
  }
  v = QUERYMYADVERB(str);
  if (v) {
    DELADVERB(str);
    write("Abbreviation "+str+" deleted.\n");
  } else {
    i = member(m_values(adverbs), str); /* YUCK */
    if (i >= 0) m_delete(adverbs, m_indices(adverbs)[i]);
    else write("You did not define an abbreviation or adverb "+str+".\n");
  }
  return 1;
}

/***************************************************************************
 * emote functions
 */

/*
 * don't send free emotes to npc's
 */
static void xsay(string str) {
  object *u;
  int i;

  u = all_inventory(environment(PL));
  for (i = 0; i < sizeof(u); i++)
    if (query_ip_number(u[i]))
      msg_object( u[i], CMSG_EMOTE, str );
}

/*
 * Emote any string, e.g. "me smiles." --> "Organimedes smiles."
 */
static int emo(string str) { xsay(tune(myname()+" "+str)); return 1; }
static int emo_gen(string str) { xsay(tune(myname()+"'s "+str)); return 1; }
static int emo_echo(string str) { xsay(tune(myname()+str)); return 1; }

/*
 * Emote with grammatical tricks: "I smile" --> "Organimedes smiles."
 * Pepel did look at this one further --- Organimedes
 */
static int em(string str) {
  string a,b,em;
  if (!(em=str)) return 0;
  if (sscanf(str,"%s %s",a,b) == 2) {
    switch(a) {
      case "have": a="has"; break;
      case "haven't": a="hasn't"; break;
      case "wish": a = "wishes"; break;
      case "do": a="does"; break;
      case "go": a="goes"; break;
      case "don't": a="doesn't"; break;
      case "am": a="is"; break;
    }
    if (sizeof(a) && a[<1..<1]=="y") a=a[0..<2]+"ie";
    if (member(
		     ({ "am","will","has","would","can","should","won't",
			  "hasn't","wouldn't","can't","shouldn't","must",
			  "mustn't","does","goes","don't","wishes",
			  "doesn't","is"}),a ) == -1)
      em = a + "s " + b;
    else
      em = a + " " + b;
  }
  while (sscanf(em,"%s I %s",a,b)==2) em = a+" "+QueryPronoun()+" "+b;
  while (sscanf(em,"%s my %s",a,b)==2) em=a+" "+QueryPossessive()+" "+b;
  while (sscanf(em,"%s me %s",a,b)==2) em=a+" "+QueryObjective()+" "+b;
  while (sscanf(em,"%s am %s",a,b)==2) em=a+" is "+b;
  xsay(tune(myname()+" "+em));
  return 1;
}

/***************************************************************************
 * the "to" command
 */

static int to_target(string arg)
{
  string target, action, channel, verb, params;
  mixed *msg;

  if (!arg) {
    notify_fail("Usage: to =<channel> <action>\n");
    return 0;
  }
  if (sscanf(arg, "%s %s", target, action) != 2) {
    notify_fail("Usage: to =<channel> <action>\n");
    return 0;
  }
  if (target[0] != '=') {
    notify_fail("Usage: to =<channel> <action>\n");
    return 0;
  }
  if ((channel = target[1..]) == "")
    channel = IS_WIZARD(TO)?"general":"players";

  if (member(({string *})TO->Query(P_CHANNELS)||({}), channel) == -1) {
    notify_fail("You are not on that channel!\n");
    return 0;
  }
  if (sscanf(action,"%s %s",verb,params) != 2)
    verb = action;

  if (!(msg = do_transform(verb, params, 1, 1))) {
    notify_fail("Can't put that on channel.\n");
    return 0;
  }
  if (msg[1] == "") {
    notify_fail(msg[0]);
    return 0;
  }
  if (sscanf(msg[1],"%s %s\n",target,action) != 2) {
    notify_fail("Can't put that on channel.\n");
    return 0;
  }
  CHANNEL_D->SendEmote(channel, capitalize(({string})TO->Query(P_REALNAME)), action);
#if __EFUN__DEFINED__(send_discord)
  if (channel == "general")
      send_discord(capitalize(({string})TO->Query(P_REALNAME)) + " " + action)
#endif
  return 1;
}
/***************************************************************************
 * the "my" command
 */

static int my_target(string arg)
{
  string target, text, channel;

  if (!arg) {
    notify_fail("Usage: my =[<channel>] <text>\n");
    return 0;
  }
  if (sscanf(arg, "%s %s", target, text) != 2) {
    notify_fail("Usage: my =[<channel>] <text>\n");
    return 0;
  }
  if (target[0] != '=') {
    notify_fail("Usage: my =[<channel>] <text>\n");
    return 0;
  }
  if ((channel = target[1..]) == "")
    channel = IS_WIZARD(TO)?"general":"players";

  if (member(({string *})TO->Query(P_CHANNELS)||({}), channel) == -1) {
    notify_fail("You are not on that channel!\n");
    return 0;
  }
  CHANNEL_D->SendEmote(channel, add_gen(capitalize(({string})TO->Query(P_REALNAME))), text);
#if __EFUN__DEFINED__(send_discord)
  if (channel == "general")
      send_discord(add_gen(capitalize(({string})TO->Query(P_REALNAME))) + " " + text)
#endif
  return 1;
}

/***************************************************************************
 * catch all input
 */
static int doit(string str) {
  string v;
  mixed  *msg;

  v = query_verb();

  /* Frogs and Ghosts cannot emote :-) */
  if (-1 != member(({ "me", "emote", "I" }), v)
   || -1 != member(":;.", v[0])
     )
  {
    if (QueryGhost())
      return emo("wavers slightly.");
    if (QueryFrog())
      return emo("hops high into the air.");
  }

  /* no overloading of these ones! (Organimedes) */
  if (v == "me" || v == "emote") return emo(str);
  if (v == "I")	    return em(str);
  if (v[0] == ':') {
    if (v[1] == ':') {
      if (!str || str == "") return emo_echo(v[2..<1]);
      if (sizeof(v) == 2) return emo_echo(str);
      return emo_echo(v[2..<1]+" "+str);
    }
    else {
      if (!str || str == "") return emo(v[1..<1]);
      if (sizeof(v) == 1) return emo(str);
      return emo(v[1..<1]+" "+str);
    }
  }
  if (v[0] == ';') {
    if (!str || str == "") return emo_gen(v[1..<1]);
    if (sizeof(v) == 1) return emo_gen(str);
    return emo_gen(v[1..<1]+" "+str);
  }
  if (v[0] == '.') {
    if (!str || str == "") return em(v[1..<1]);
    if (sizeof(v) == 1) return em(str);
    return em(v[1..<1]+" "+str);
  }

  if (QueryGhost())
    msg = do_transform("wail", "");
  else if (QueryFrog())
    msg = do_transform("croak", "");
  else
    msg = do_transform(v,str);

  if (msg) {
    msg_write( CMSG_EMOTE, msg[0] );
    if (msg[2]) {
      msg_say( CMSG_EMOTE,msg[1],msg[2]);
      msg_object(msg[2], CMSG_EMOTE, msg[3] );
    } else
      msg_say( CMSG_EMOTE, msg[1]);
    return 1;
  }
  return 0;
}

/* The following two functions make a reasonable string from the command
 * line (if soul commands used) using the alists 'verbs' and 'adverbs'.
 * They both return an array of four variables if the verb is found:
 *	var[0] contains the echo you get
 *	var[1] contains the emote others get
 *	var[2] contains the target (if any)
 *	var[3] contains the emote the target gets
 * (This is awful programming style, so if sb could improve it :-)
 */

/* This function transforms a string slightly prepared by the function
 * do_transform() and returns it either directly or to the function
 * do_transform().
 */
protected varargs void transform2(string str, mixed *val, int no_presay) {
  string *arr;
  string s, foo;
  int    i, n;
  status flag;

  flag = 0;
  arr  = explode(str," ");
  n    = sizeof(arr);
  for (i = 0; i < n; i++) {
    if (("$" == arr[i]) || ("YOU" == arr[i])) {
      val[0] = "you";
      val[1] = no_presay?QueryName():myname();
      val[3] = val[1];
      flag   = 1;
    } else if (("$$" == arr[i]) || ("YOUR" == arr[i])) {
      val[0] = "your";
      val[1] = no_presay?QueryName():myname();
      if ("s" == val[1][<1..<1])  val[1] += "'";
      else			  val[1] += "'s";
      val[3] = val[1];
    } else if ("your" == arr[i]) {
      val[0] += "your";
      val[1] += QueryPossessive();
      val[3] += QueryPossessive();
    } else if (sscanf(arr[i],"yourself%s",foo) == 1) {
      val[0] += ("yourself"+foo);
      val[1] += (QueryObjective()+"self"+foo);
      val[3] += (QueryObjective()+"self"+foo);
    } else if (1 == flag) {		/* this is a verb!!! */
      s = arr[i];
      /* conjugation */
      if ("have" == s)		s = "has";
      else if ("are" == s)	s = "is";
      else if ('s' == s[<1])	s += "es";		/* he kisses */
      else if ("sh" == s[<2..<1]) s += "es";		/* he wishes */
      else if ('o' == s[<1])	s += "es";		/* he does */
      else if ('y' == s[<1])	s = s[0..<2]+"ies";	/* he tries */
      else			s += "s";		/* he smiles */
      val[0] += arr[i];
      val[1] += s;
      val[3] += s;
      flag = 0;
    } else if (val[2] && ("^^" == arr[i])) {
      if (!(s = ({string})val[2]->QueryName()))
	s = ({string})val[2]->QueryShort();
      val[0] += s + "'s";
      val[1] += s + "'s";
      val[3] += "your";
    } else if ("&" == arr[i]) {
      val[0] += "and";
      val[1] += "and";
      val[3] += "and";
    } else if ("|" == arr[i]) {
      val[0] += ",";
      val[1] += ",";
      val[3] += ",";
    } else if (val[2] && ("^" == arr[i])) {
      if (!(s = ({string})val[2]->QueryName()))
	s = ({string})val[2]->QueryShort();
      val[0] += s;
      val[1] += s;
      val[3] += "you";
    } else {
      val[0] += arr[i];
      val[1] += arr[i];
      val[3] += arr[i];
      if (arr[i] == "and")	/* assume this connects two main sentences */
	flag = 1;
    }
    /* add blanks */
    if (i < n-1) {
      val[0] += " ";
      val[1] += " ";
      val[3] += " ";
    }
  }

  /* do some more fine tuning... */
  val[0] = tune(val[0]);
  val[1] = tune(val[1]);
  val[3] = tune(val[3]);
}

/* This function determines if the function is meant for other objects or
 * not, and prepares the string for transform2().
 */
varargs mixed *do_transform(string verb,
  string params,
  int    no_presay,
  int    player_only) {
  string adverb, add_str;
  int yesno;
  object target;
  mixed  *retval;
  mixed *data;
  string *s, s2;		/* workstrings */
  status dollar;		/* flag for $  */
  int    i, n;

  /* If verb is not in list, exit */
  if (!(data = QUERYVERB(verb))) return 0;
  adverb  = data[0];
  add_str = data[1];
  yesno   = data[2];

  /* init retval */
  retval    = allocate(4);
  retval[0] = "";	/* return string for player */
  retval[1] = "";	/* return string for environment */
  retval[3] = "";	/* return string for target */

  if (!params || "" == params) {
    if (!adverb || "" == adverb) transform2("YOU "+verb,retval,no_presay);
    else if ("&" == adverb)
      retval[0] = "You only can "+verb+" an object.\n";
    else if ("@" == adverb)
      retval[0] = "You only can "+verb+" a living.\n";
    else if ('$' == adverb[0]) transform2(adverb,retval,no_presay);
    else transform2("YOU "+verb+" "+adverb,retval,no_presay);
    return retval;
  }

  /* There are parameters: look for abbreviated adverbs and objects */
  s = explode(params," ");
  n = sizeof(s);
  dollar = 0;
  for (i = 0; i < n; i++) {
    /* replace abbreviated adverbs */
    if (s2 = QUERYADVERB(s[i])) s[i] = s2;
    /* look for an object */
    if (!target) {
      if (target =
	  player_only?find_player(s[i]):present(s[i],environment(PL))) {
	if (add_str && "" != add_str) {
	  if (sscanf(add_str,"%~s^%~s") != 2)
	    s[i] = add_str+" ^";
	  else
	    s[i] = add_str;
	  if (add_str[0] == '$')
	    dollar = 1;
	} else
	  s[i] = "^";
	retval[2] = target;
      }
    }
  }

  params = implode(s," ");

  /* yes, parameters contain objects */
  if (target) {
    if (!yesno) {
      /* Objects are not allowed! */
      retval[0] = "You cannot "+verb+" other objects.\n";
      return retval;
    } else {
      if (dollar) transform2(params,retval,no_presay);
      else transform2("YOU "+verb+" "+params,retval,no_presay);
    }

    /* Special Case : The Kiss Stuff */
    if ("kiss" == verb) {
      if (QueryFrog()) {
	retval[0] = retval[0] + "You turn into a " + QueryRace()+" again.\n";
	retval[1] = retval[1] + ({string})retval[2]->QueryName()+
	  " turns into a frog! "+ QueryName()+
	    " turns into a "+QueryRace()+".\n";
	retval[3] = retval[3] + "You turn into a frog!\n";
	SetFrog(0);
	retval[2]->SetFrog(1);
      } else if (({int})retval[2]->QueryFrog() > 0) {
	retval[0] = retval[0] + "You turn into a frog!\n";
	retval[1] = retval[1] + QueryName()+ " turns into a frog! "+
	  ({string})retval[2]->QueryName()+" turns into a "+
	  ({string})retval[2]->QueryRace()+".\n";
	retval[3] = retval[3] + "You turn into a " +
	  ({string})retval[2]->QueryRace()+" again.\n";
	SetFrog(1);
	retval[2]->SetFrog(0);
      }
    }
    return retval;
  }

  /* no, parameters don't contain objects */
  if ("&" == adverb) {
    retval[0] = "You can only "+verb+" an object.\n";
    return retval;
  } else if ("@" == adverb) {
    retval[0] = "You can only "+verb+" a living.\n";
    return retval;
  }

  transform2("YOU "+verb+" "+params,retval,no_presay);
  return retval;
}
