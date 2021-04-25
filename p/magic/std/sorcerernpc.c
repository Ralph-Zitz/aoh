// This is a standard spellmonster which provides easy to use Sorcerer
// spells. One need only 2-3 commands to have a full NPC Sorcerer
// 
// [s] 11jul96 creation
//
// [s] Softbyte
//
//
//  SetQuestionLog(logfile))    : Logs not answered questions into a logfile
//                                if logfile=0 no logging is performed
// InitLowSpells()              : Mages of three different magic strength
// InitMediumSpells()           : are created.
// InitHighSpells()             : 
// InitLowResistance()          : Mages with three different magic
// InitMediumResistance()       : resistances are created
// InitHighResistance()         :

#include <moving.h>
#include <properties.h>
#include <secure/wizlevels.h>
#include <living.h>
#include <combat.h>
#include <magic.h>
#include <spellmonster.h>

inherit "/obj/monster/spellmonster";

/* Sets the file where the not answered questions are logged */
/* if this file is 0, nothing is logged                      */
private static string qlog=0;
string QueryQuestionLog() {return qlog;}
int SetQuestionLog(string str) {qlog=str;return 1;}


/* This propertie determines wheter we are nice and return all items */
/* when someone gives them to us                                     */
private static int r_item=0;
int DisableReturnItem() {r_item=0;return 1;}
int EnableReturnItem() {r_item=1;return 1;}
int QueryReturnItem()  {return r_item;}


/* Enable three levels of Resistance (Low,Medium and High)    */
varargs int InitHighResistance(mixed arg)
{
 SetAttribute(P_MAGIC_DEFENCE,70); /* The correct one */
 SetAttribute(P_MAGIC_DEFENCE,70);
 AddResistance(ST_FIRE,95);
 AddResistance(ST_COLD,95);
 AddResistance(ST_WATER,90);
 AddResistance(ST_GAS,90);
 AddResistance(ST_ZAP,100);
 AddResistance(ST_ELECTRO,100);
 AddResistance(ST_ACID,100);
 AddResistance(ST_POISON,95);
 AddResistance(ST_PSYCHO,95);
 AddResistance(ST_KNOW,90);
 AddResistance(ST_INFLUENCE,100);
 AddResistance(ST_CHANGE,100);

}
varargs int InitLowResistance(mixed arg) {return InitHighResistance(arg);}
varargs int InitMediumResistance(mixed arg) {return InitHighResistance(arg);}

/* Enable three levels of spells (Low,Medium and High)        */
varargs int InitLowSpells(mixed arg)
{
  SetCastChance(20);
  AddSpell("arcfire",
	   ([
	     S_CHANCE:30,  // chance of 30% that this spell is cast
	     S_COMBAT_DELAY:2, // wait 2 hbs after spell got cast
	     S_SIMULT:0,  // don't do a physical attack when this spell gets cast
	     S_VICTIMS:1, // only one victim
	     S_DELAY:5,   // only cast this spell every 5 rounds
	     S_DAMTYPE:DT_FIRE, // what kind of damage
	     S_MINDAM:25, // minimum amount of damage
	     S_MAXDAM:45, // maximum amount of damage
	     S_SP:20,     // how much spellpoints this spell costs
	     S_GLOBAL_ENVMSG:({"@@MyName|1@@ casts an arcfire spell.",""}),
	     S_ENVMSG:([
			MSG_NORMAL:
			({
"@@MyName|1@@ sweeps the air with @@MyPossessive@@ hand. A blazing clot "
"of arcanic\n"
"fires starts crackling on @@MyPossessive@@ hand. @@MyPronoun|1@@ "
"hurls the burning substance\n"
"at @@VicName@@ engulfing @@VicObjective@@ in turbulent golden flames.\n",
"Someone sweeps the air with @@MyPossessive@@ hand. A blazing clot\n"
"of arcanic\n"
"fires starts crackling on @@MyPossessive@@ hand. @@MyPronoun|1@@ "
"hurls the burning substance\n"
"at @@VicName@@ engulfing @@VicObjective@@ in turbulent golden flames.\n"
		        }),
			MSG_DEAD:
			({"@@MyName|1@@ laughs at @@VicName@@.\n",
			  "Someone laughs at @@VicName@@.\n"
			})
		      ]),
	     S_VICMSG:([
			MSG_NORMAL:
			({
"@@MyName|1@@ sweeps the air with @@MyPossessive@@ hand. A blazing clot "
"of arcanic fires\n"
"starts crackling on @@MyPossessive@@ hand. @@MyPronoun|1@@ "
"hurls the burning substance\n"
"at you engulfing you in turbulent golden flames.\n",
"Someone sweeps the air with @@MyPossessive@@ hand. A blazing clot "
"of arcanic\n"
"fires starts crackling on @@MyPossessive@@ hand. @@MyPronoun|1@@ "
"hurls the burning substance\n"
"at you engulfing you in turbulent golden flames.\n"
		        }),
			MSG_DEAD:
			({"@@MyName|1@@ laughs at you.\n",
			  "Someone laughs at you.\n"
			})
		     ])
	   ])
	  );
  AddSpell("flame dart",
	   ([
	     S_CHANCE:40, 
	     S_COMBAT_DELAY:3,
	     S_SIMULT:0,  
	     S_VICTIMS:1,
	     S_DELAY:4, 
	     S_DAMTYPE:DT_FIRE,
	     S_MINDAM:8,
	     S_MAXDAM:16, 
	     S_SP:9,     
	     S_GLOBAL_ENVMSG:({"@@MyName|1@@ casts a flame dart spell.",""}),
	     S_ENVMSG:([
			MSG_NORMAL:
			({
"@@MyName|1@@ raises @@MyPossessive@@ arm towards @@VicName@@.\n"
"Instantly a red hot missle shoots of at @@VicName@@.\n",
"Suddenly you hear a load crackle and a cloud of fire emerges out of nothing.\n"
"Instantly a red hot missle shoots of at @@VicName@@.\n"
		        }),
			MSG_DEAD:
			({"@@MyName|1@@ laughs at @@VicName@@.\n",
			  "Someone laughs at @@VicName@@.\n"
			})
		      ]),
	     S_VICMSG:([
			MSG_NORMAL:
			({
"@@MyName|1@@ raises @@MyPossessive@@ arm towards you.\n"
"Instantly a red hot missle shoots of at you.\n",
"Suddenly you hear a load crackle and a cloud of fire emerges out of nothing.\n"
"Instantly a red hot missle shoots of at you.\n"
		        }),
			MSG_DEAD:
			({"@@MyName|1@@ laughs at you.\n",
			  "Someone laughs at you.\n"
			})
		     ])
	   ])
	  );
  AddSpell("shock shere",
	   ([
	     S_CHANCE:30, 
	     S_COMBAT_DELAY:2,
	     S_SIMULT:0,  
	     S_VICTIMS:1,
	     S_DELAY:6, 
	     S_DAMTYPE:DT_PSYCHO,
	     S_MINDAM:8,
	     S_MAXDAM:22, 
	     S_SP:16,     
	     S_GLOBAL_ENVMSG:({"@@MyName|1@@ casts a shock shere spell.",""}),
	     S_ENVMSG:([
			MSG_NORMAL:
			({
"@@MyName|1@@ closes @@MyPossessive@@ eyes and concentrates.\n"
"Suddenly a shock sphere hits @@VicName@@.\n",
"Suddenly a shock sphere hits @@VicName@@.\n"
		        }),
			MSG_DEAD:
			({"@@MyName|1@@ laughs at @@VicName@@.\n",
			  "Someone laughs at @@VicName@@.\n"
			})
		      ]),
	     S_VICMSG:([
			MSG_NORMAL:
			({
"@@MyName|1@@ closes @@MyPossessive@@ eyes and concentrates.\n"
"Suddenly a shock sphere hits you.\n",
"Suddenly a shock sphere hits you.\n"
		        }),
			MSG_DEAD:
			({"@@MyName|1@@ laughs at you.\n",
			  "Someone laughs at you.\n"
			})
		     ])
	   ])
	  );
return 1;
}
varargs int InitMediumSpells(mixed arg) {return InitLowSpells(arg);}
varargs int InitHighSpells(mixed arg) {return InitLowSpells(arg);}

/* Log not answer questions for later processing */
string MyShrugMsg(string what)
{
string tmp,file;
  if (file=QueryQuestionLog())
  {
    tmp=(this_player()->QueryRealName()||"Someone")+" asks "+QueryName()+": '"+
        what+"' on "+ctime()+".\n";
    write_file(file,tmp);
  }
  return ::QueryShrugMsg(what);
}
mixed QueryShrugMsg() {return #'MyShrugMsg;}

void returnitem(object whom,object what)
{
  if (!whom || !what ) return;
  if (environment(whom)!=environment() || !Give(what,whom) )
    Drop(what);
}

varargs void notify_enter (mixed oldenv, int method, mixed extra) 
{
object what;
  what = previous_object();
  if (QueryReturnItem()) call_out(#'returnitem,1,this_player()||this_interactive(),what);
  ::notify_enter (oldenv, method, extra);
}

