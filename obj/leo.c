/* GrandPa Leo - bored by native mode */
/* the gamedriver license is needed for the collector quest */

#include <properties.h>
#include <config.h>
#include <attributes.h>
#include <secure/wizlevels.h>
#include <moving.h>

inherit "std/npc";

/* lies, leo and native mode : */
/*
 * This is the object wich tries to behave like Leo the archwizard.
 * The purpose is to converse with players and give them portable castles.
 * 
 * Only wizards GREATER level 20 can be a sponsors 	Tares 05/92 
 * 'castle' and 'castle2' are made 'static private'   	Tares 05/92
 * 'castle' and 'castle2' are made only 'static'   	Tares 05/92
 */

nosave int f;

void create() {
  /* Don't do anything with the blueprint. */
  if(::create()) return;

  SetName("Leo");
  SetShort("Leo the retired Archwizard");
  SetLong(
"He is a very old, skinny man with long silver hair and a long silver "
"beard. He wears a silk blue robe with strange patterns woven into it. "
"On his head he wears a high, pointed, dark blue hat, which has magical "
"stars painted on it. In his left hand, Leo holds a map of some "
"hyperdimensional area. In his right he holds a flower with two red "
"blooms. It seems very strange to you that each of the blooms has an "
"eye on it. The flower is tensely looking at Leo.\n");
  SetIds(({"leo","man","human","archwizard","wizard"}));
  SetAds(({"tired","retired","old","very old","skinny"}));
  SetRace("human");
  SetGender (1);			/* male */
  SetWeight(70000);		/* 70 kg */
  SetMaxWeight(200000);
  SetMaxFood(230);		/* Can eat a lot */
  SetFood(180);		/* is quite satiated */
  SetXP(10000);
  SetMaxHP(300);
  SetHP(300);
  SetMaxSP(300);
  SetSP(300);
  SetStr(10);
  SetDex(10);
  SetCon(20);
  SetInt(30);
  SetAlign(1000);
  SetHands( ({
	    ({ "right hand", 0, 15 /*WEAPON_CLASS_OF_HANDS*/ }),
	    /* maybe in some future...
	     * ({ "left hand", ob, ob->Query(P_WC) }),
	     */
	    ({ "left hand", 0, 15 /*WEAPON_CLASS_OF_HANDS*/ })
	    })
	  );
  InitAChats(20, ({
    "Leo casts a blazing fire ball.\n",
    "You notice a wrongness in the fabric of time and space.\n"
    }));
  InitChats(2, ({
    "Leo murmurs something strange.\n",
    "Leo says: I don't have any castles left.\n",
    "Leo smiles sadly at the flower.\n",
    "Leo thinks of the good old times.\n",
    "Leo checks his Gamedriver license.\n",
    "Leo asks you: If you want a Gamedriver driving license, just say it.\n",
    "The flower winks at Leo.\n",
    "Leo says: Nobody comes to me anymore to get a castle.\n"
    }));
  /* greetings from pepel
  catch(l = clone_object("/d/daydream/pepel/gdl"));
  if (l) {
    catch("/d/daydream/pepel/gd"->signit(l, this_object()));
    l->move(this_object(), 1);
  }
  */
  if (object_name(previous_object()) == "/d/archwiz/common/room/city/wiz_hall")
    call_out("go_pub", 1000+random(1600), 0);
  else f = 1;
}

void init() {
  add_action("give","give");
}

void catch_tell(string str) {
  object from;
  string next_out;
  if (!(from = this_player())
      || !interactive(this_player())) return;	/* Not from a real player. */
  if (sscanf(str, "%~sello%~s") == 2 ||
      sscanf(str, "%~s hi%~s") == 2 ||
      sscanf(str, "%~s Hi%~s") == 2){
    next_out = "Leo says: Welcome, " + ({string})from->QueryName() + ".\n";
    call_out("sayit", 2, next_out);
    return;
  }
  if (sscanf(str, "%~sgive%~scastle%~s") == 3 ||
      sscanf(str, "%~swant%~scastle%~s") == 3) {
    if (IS_WIZARD(from)) {
      next_out =
	"Leo searches in his pockets for a castle, but does not find any.\n";
      call_out("sayit", 2, next_out);
      return;
    }
    next_out = "Leo says: What ! Give a castle to you ?\n";
    call_out("sayit", 2, next_out);
    return;
  }
  /* greetings from pepel */
  if (this_player() != this_object() &&
      (sscanf(str, "%~ssay%~sgive%~slicense%~s") == 4 ||
      sscanf(str, "%~ssay%~swant%~slicense%~s") == 4)) {
    call_out("givit", 2, this_player());
    return;
  }
  log_file("LEO", str);
}

 /* added by Joern */
int give(string str) {
  object ob,from;
  string a;
  if (!str) return 0;
  if (!sscanf(str,"%s to le%~s",a)==2) return 0;
  if (!sscanf(str,"%s to Le%~s",a)==2) return 0;
  if (a!="orc slayer" && a!="slayer" && a!="blade" && a!="orcslayer" && a!="sword") return 0;
  from=this_player();
  ob=present("orc slayer",from);
  if (!ob) return 0;
  if (member(({string *})QUESTMASTER->QueryPlayerQuests(from), "orc_slayer") == -1) {
    /* I want them to seek the _real_ Leo - Pepel */
    if (f) {
      say("Leo blushes deeply.\n");
      write(
"Leo stutters: I can't take that sword from you. I must confess ... I\n"
"am only a double of Leo the Archwizard. Leo himself managed to\n"
"escape. I am sorry, you need to search him, in the now almost\n"
"forgotten capital of this world, below the Ruin City.\n");
      say("Leo whispers something to "+({string})from->QueryName()+"\n", from);
      say("Leo returns a sword to "+({string})from->QueryName()+".\n");
      log_file("LEO", ctime(time())+" "+({string})from->QueryRealName()
	       +" was denied by the double in "
	       +object_name(environment())+"\n");
      return 1;
    } else {
      write("Leo says: Well done. You have fullfilled this quest.\n");
      filter(users()-({ from }), #'tell_object /*'*/, ({string})from->QueryName()+" has solved the orc slayer quest.\n");
      write("You give the sword to Leo.\n");
      QUESTMASTER->SetPlayerQuest("orc_slayer", from);
      destruct(ob);
      log_file("LEO", ctime(time())+" "+({string})from->QueryRealName()
	       +" solved the orc slayer quest.\n");
      write("\n");
      return 1;
    }
  } else {
    write("You have solved this quest already!\n");
    destruct(ob);
    return 1;
  }
}

void sayit(string s) { if (s) tell_object(this_player(), s); }

void givit(string who) {
  object l;
  if (!who) return;
  catch(l = clone_object("/d/daydream/pepel/gdl"));
  if (l) {
    say("Leo fetches something from another dimension.\n");
    if (ME_OK == ({int})l->move(who, M_GIVE)) {
      say("Leo gives "+({string})l->QueryShort()+" to " +({string})who->QueryName()+".\n");
      if (f) l->Set("fake", object_name(environment()));
    } else {
      say("Leo fails to give "+({string})l->QueryShort()+" to " +({string})who->QueryName()+".\n");
      destruct(l);
    }
  } else /* not l */
    say("Leo tries to fetch something from another dimension, but fails.\n");
}

varargs void Die() {
  write(QueryShort()+" died.\n"
	"But to your horror you see that his magical flower revives him.\n");
  say(QueryShort()+" died.\n"
	"But you shudder as you see that his magical flower revives him.\n",
      ({ this_player() }));
  SetMaxHP(300);
  SetHP(300);
  SetMaxSP(300);
  SetSP(300);
}

/* when things get too boring, Leo goes for a beer */
void go_pub() {
}
