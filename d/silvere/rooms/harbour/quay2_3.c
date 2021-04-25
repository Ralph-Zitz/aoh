// The way to the southern quay and the tower.
// created:          Sonic [26-May-1997]
// last changed:     Sonic [05-Jul-1997] added regions/coordinates
//                   Taurec [10-Oct-1997] improved roomdesc.
//                   Taurec [21-Jan-1998] squeeze fun added... look and enjoy:)


#include <silvere.h>
#include <properties.h>
#include <coordinates.h>
#include <nightday.h>
#include <regions.h>
#include <moving.h>
#include <macros.h>
#include <rooms.h>
inherit HARBOUR_BASEROOM;

int squeeze_through(string str);
mixed look_dunes();

varargs void create()
{
   ::create();
   Set(P_INDOORS,0);
   Set(P_INT_SHORT,"A street");
   Set(P_INT_LONG,
     "The road leading in from the east is turning to a smaller path here. "
     "It meanders westwards along the quay towards the southern defense "
     "tower. Rows of bushes and small froja trees line the southern border "
     "of the path. @@TimeMsg@@\n");

   Set(P_TIME_MSG,
    ([
     DAY_DEFAULT_MSG:
       "Just behind the bushes, beige sand dunes start waving smoothly up "
       "and down, joining the foothills someway to the southeast. "
       "The protection of the bay already lessens here a bit, a strong "
       "breeze blows directly into your face.",
     NIGHT_DEFAULT_MSG:
       "In the darkness you can't identify much of the country to the south. "
       "A strong wind blows directly into your face, seems as if you "
       "are about to leave the protection of the bay here."
    ]));

   Set(P_INT_WIZMSG,
     "east: City-Gates, Harbourmaster, Inn, quays\n"
     "West: The southern defense tower\n");

// **************** VITEMS *******************

   AddVItem(([
     P_IDS: ({"bush","bushes","row","rows"}),
     P_ADS: ({"thick-leaved","thorny"}),
     P_LONG:
    "Evergreen, thick-leaved thorny bushes, standing so close together "
    "that no one can pass them. Or perhaps... oh, your eyes - sharp like "
    "a knife - detect a small gap in the hedge. Maybe you can squeeze "
    "through the gap somehow...\n"
    ]));

   AddVItem(([
     P_IDS: ({"gap"}),
     P_ADS: ({"small","hidden","secret"}),
     P_LONG:
    "Footprints force their way though the gap in the bush-row. Might "
    "there be a pirates treasure hidden somewhere in the dunes? Or is "
    "there a camp of smugglers?\n"
    ]));

   AddVItem(([
     P_IDS: ({"tree","trees","froja"}),
     P_ADS: ({"mysterious","froja","small"}),
     P_LONG:
     "Yes! You are sure this must be the mysterious froja trees. It is said "
     "they have fruits of great healing power, but these fruits only grow "
     "during full-moon nights. Strange indeed.\n"
    ]));

   AddVItem(([
     P_IDS: ({"street","road","path"}),
     P_ADS: ({"small","sandy"}),
     P_LONG:
      "It's a small path. Here and there, some old cobblestones can be "
      "seen beneath a cover of sand.\n"
   ]));

   AddVItem(([
     P_IDS: ({"dune","dunes"}),
     P_ADS: ({"beige","sandy"}),
     P_LONG: #'look_dunes /*'*/
   ]));

//******************************************

   Set(P_INT_MAP,"harbour");
   Set(P_REGION, REGION_CITY|REGION_HARBOUR);
   Set(P_COORDINATES, ({ ({ -260,20,0 }), C_SILVERE }) );
   Set(P_NOWAY_MSG,"You run silly around and nearly fall in the water.\n");
   AddRoomCmd("squeeze","squeeze_through");
   AddExit("west", SIL_HARBOUR("stower_enter"));
   AddExit("east", SIL_HARBOUR("quay2_2"));
}

//*************************************************
// At night, you don't see far... no dunes then :)
//*************************************************

mixed look_dunes()
{
  if (({int})NIGHTDAY->IsDay()) return
   "Large dunes with steep walls. You wonder how many billions of "
   "billions of sandcorns are needed to form such things.\n";
  else return 0;
}

//*****************************************
// function moves player to smugglers area
//*****************************************

int squeeze_through(string str)
{
   object somebody;
   string *exp,*sb,*who;
   int i;
   notify_fail("Squeeze? What do you want to squeeze... yourself "+
     "through the bushes perhaps?\n",NOTIFY_NOT_VALID);
   if(!str) return 0;
   who=({"me","myself"});
   who+=({({string})TP->QueryName()});
   str=norm_id(str);
   exp=explode(str," through ");
   if (sizeof(exp)!=2) {
     return 0;
   }
   exp[0]=strip_article(exp[0]);
   exp[1]=strip_article(exp[1]);
   if (member(({"gap","bush","bushes","bushrow"}),exp[1])==-1) {
     return 0;
   }
   somebody=TP;
   if (member(who,exp[0])==-1){  // squeezing somebody through, not oneself
     sb=explode(exp[0]," ");
     if (sizeof(sb)==0) return 0;  // No argument in object place
     for (i=0;i<sizeof(sb);i++){   // Check if arguments before through=living
       if (somebody=find_living(lower_case(sb[i]))) break;
     }
     if(!somebody) return 0; // no living obj found, returning.
     if(environment(somebody)!=TO) return 0;
   }
   if(somebody!=TP){
     write("With an unexpected movement and a mischievous grin on your "
      "face, you push "+CAP(NAME(somebody))+" into the thorny bushes. "
      "Cursing and struggling wildly with the thorns, "+HESHE(somebody)+
      " disappears through the gap.\n");
     tell_object(somebody,"With an unexpected movement and a mischievous "
      "grin on "+HISHER(TP)+" face, "+CAP(NAME(TP))+" pushes you into the "
      "thorny bushes. Ouch! Struggling wildly, you finally reach the other "
      "side of the bushrow.\n");
     show_room(TO,"With an unexpected movement and a mischievous grin on "+
       HISHER(TP)+" face, "+CAP(NAME(TP))+" pushes "+CAP(NAME(somebody))+
      " into the thorny bushes. Cursing and struggling wildly with the "
      "thorns, "+HESHE(somebody)+" disappears through the gap.\n",
      "Someone chuckles mischievously. ",({TP,somebody}));
   }
   else{
     write("You take a deep breath and squeeze yourself through these "
      "thorny bushes, carefully trying to avoid the sharp "
      "bloodlusty thorns. Ouch! Struggling wildly.. Ouch! Ouch! ... "
      "you finally... OUCH! ... get through. \"Phew!!!\" - You "
      "wipe the sweat from your forehead. Hey, you are inmidst "
      "the dunes now.\n");
   show_room(TO,
    "Cursing and struggling wildly with the thorns, "+CAP(NAME(somebody))+
    " disappears through the bushes.\n",
    "You hear someone cursing and the sound of breaking "
    "branches.\n",({somebody}));
   }
   show_room(SIL_HARBOUR("smugglers1.c"),"There's a sudden noise in "
    "the bushes to the north! Branches are cracking with a "
    "sound as if something huuuge breaks through the bushes! "
    "Suddenly, "+CAP(NAME(somebody))+" appears. Wiping the sweat from "
    +HISHER(somebody)+" forehead, "+HESHE(somebody)+" goes: \"Phew!!!\"\n",
    "There's a sudden noise in the bushes to your south! "
    "Branches are cracking with a sound as if something huuge "
    "breaks through the bushes! Finally you hear someone "
    "saying: \"Phew!!!\"\n");
   somebody->move(SIL_HARBOUR("smugglers1.c"),M_GO);
   return 1;
}







