// This is a standard spellmonster which provides easy to use Druids
// spells. One need only 2-3 commands to have a full NPC Druid
// 
// [s] 11jul96 creation
//
// [s] Softbyte
//
//
//  SetQuestionLog(logfile))    : Logs not answered questions into a logfile
//                                if logfile=0 no logging is performed
// InitLowSpells()              : Druids of three different magic strength
// InitMediumSpells()           : are created.
// InitHighSpells()             : 
// InitLowResistance()          : Druids with three different magic
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
int EnalbeReturnItem() {r_item=1;return 1;}
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
varargs int InitMediumSpells(mixed arg)
{
  SetCastChance(20);
  AddSpell("lightning",
	   ([
	     S_CHANCE:30, // chance of 30% that this spell is cast
	     S_COMBAT_DELAY:2, // wait 2 hbs after spell got cast
	     S_SIMULT:0, // don't do a physical attack when this spell gets cast
	     S_VICTIMS:1, // only one victim
	     S_DELAY:5, // only cast this spell every 10 rounds
	     S_DAMTYPE:DT_FIRE, // what kind of damage
	     S_MINDAM:30, // minimum amount of damage
	     S_MAXDAM:120, // maximum amount of damage
	     S_SP:75, // how much spellpoints this spell costs
	     S_GLOBAL_ENVMSG:({"@@MyName|1@@ invokes a lightning spell",""}),
	     S_ENVMSG:([
			MSG_NORMAL:
			({"@@MyName|1@@ turns towards @@VicName@@ and points "
			  "at @@VicObjective@@.\n"
			  "Thunder rolls and clouds begin to gather. Wind grows stronger and\n"
			  "stronger. Suddenly a lightning hits @@VicName@@.\n",
			  "Thunder rolls and clouds begin to gather. Wind grows stronger and\n"
			  "stronger. Suddenly a lightning hits someone.\n"
		        }),
			MSG_DEAD:
			({"@@MyName|1@@ laughs at @@VicName@@.\n",
			  "Someone laughs.\n"
			})
		      ]),
	     S_VICMSG:([
			MSG_NORMAL:
			({"@@MyName|1@@ turns towards you and points at you.\n"
			  "Thunder rolls and clouds begin to gather. Winds grow stronger and\n"
			  "stronger. Suddenly a lightning hits you.\n",
			  "Thunder rolls and clouds begin to gather. Winds grow stronger and\n"
			  "stronger. Suddenly you are hit by a lightning.\n"
		        }),
			MSG_DEAD:
			({"@@MyName|1@@ laughs at you.\n",
			  "Someone laughs.\n"
			})
		     ])
	   ])
	  );

  AddSpell("ground explosion",
	   ([
	     S_CHANCE:20, // chance of 20% that this spell is cast
	     S_COMBAT_DELAY:2, // wait 2 hbs after spell got cast
	     S_SIMULT:0, // don't do a physical attack when this spell gets cast
	     S_VICTIMS:10, // 10 victims
	     S_DELAY:5, // only cast this spell every 10 rounds
	     S_DAMTYPE:DT_FIRE, // what kind of damage
	     S_MINDAM:15, // minimum amount of damage
	     S_MAXDAM:60, // maximum amount of damage
	     S_SP:50, // how much spellpoints this spell costs
	     S_GLOBAL_ENVMSG:({"@@MyName|1@@ raises @@MyObjective@@ arms and then suddenly points towards the ground\n"
                             "A huge explosion shudders the ground.\n",""}),
	     S_ENVMSG:([
			MSG_NORMAL:
			({"@@VicName@@ is hit by stones and debris,\n",
			  "Someone is hit by stones and debris.\n"
		        }),
			MSG_DEAD:
			({"@@VicName@@'s body is covered by debris.\n",
			  "Someone's body is covered by debris.\n"
			})
		      ]),
	     S_VICMSG:([
			MSG_NORMAL:
			({"You are hit by stones and debris.\n",
			  "You are hit by stones and debris.\n"
		        }),
			MSG_DEAD:
			({"Your body is covered by debris.\n",
			  "Your body is covered by debris.\n"
			})
		     ])
	   ])
	  );

return 1;
}
varargs int InitLowSpells(mixed arg) {return InitMediumSpells(arg);}
varargs int InitHighSpells(mixed arg) {return InitMediumSpells(arg);}

/* Log not answer questions for later processing */
string MyShrugMsg(string what)
{
string tmp,file;
  if (file=QueryQuestionLog() && this_player() && what)
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

