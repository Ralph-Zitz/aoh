// Room layout. The road here leads around the palace
// created:       Sonic    [15-Jun-1997]
// changed:       Sonic    [01-Jul-1997]  added region/coordinates
//                Joan     [01-Nov-1997]  added some details and stuff

#include <silvere.h>
#include <properties.h>
#include <coordinates.h>
#include <nightday.h>
#include <regions.h>
#include <moving.h>
#include <rooms.h>
inherit SILBASEROOM;

string QueryIntNoise()
{
  string *tmp;

  tmp = ({
   "The noise of the city rudely intrudes upon the peace "
   "of the quiet avenue.\n",
   "The clickety clack coach of wheels intrude into palace solitude.\n",
   "You hear a horse clip clop along in the distance.\n",
   "A chapel bell rings in the distance.\n",
   "The wind whispers echos of the sad story of Xalandre's "
   "lost love...\n",
  });

  if( ({int})NIGHTDAY->IsDay() )
    tmp += ({
      "Jays gaily chatter and mourning doves coo while mocking "
      "birds scold you for coming to near.\n"
      });

  else
    tmp += ({
      "Cicadas and crickets chirp merry night symphonies all around "
      "you.\n",
      "An owl calls out from somewhere in the trees.\n"
    });

  return random_select(tmp);
}

varargs void create()
{
   ::create();
   Set(P_INDOORS,0);
   Set(P_INT_SHORT,"The palace avenue");
   Set(P_INT_LONG,
"Lime trees border west side of the wide palace avenue as "
"it gracefully continues to the north and south. To the north "
"the avenue elegantly intersects a road to the west. Delicate "
"tiny cassia cinnamon trees and blue gentleman's buttons border "
"the base of the west palace wing. @@TimeMsg@@\n");

   Set(P_INT_WIZMSG,
" North: Follow the road to the north around the palace.\n"
" South: Follow the road to the entrance of the palace.\n");

   AddDetail( ({"road","avenue","path"}),"Elegant lime trees border "
   "the smooth brownstone avenue gently continues north and "
   "south.\n");

   AddDetail( ({"stone","stones","brownstones","brownstone"}),
   "The smooth brownstones are carefully cut and laid with "
   "almost seamless precision.\n");

   AddDetail( ({"tree","lime trees","lime tree","trees"}),
   "A row of well tended lime trees border the avenue. Clusters "
   "of ripening limes crowd the branches of the mature trees.\n");

   AddDetail( ({"flower","flowers","gentleman's buttons",
   "gentleman's button", }),"Piles of gorgeous blue gentleman's "
   "buttons line the base of the brownstone palace wall.\n");

   AddDetail( ({"limes","lime"}),"The limes are not ripe yet.\n");

   AddDetail( ({"cinnamon","cinnamon trees","cassia cinnamon",
   "cassia cinnamon tree","cassia cinnamon trees","cinnamon trees",
   "cinnamons"}),"Beautiful tiny cinnamons line the base of the "
   "the brownstone palace wall. Their heady scent makes you "
   "think of home, your mother and cinnamon bread.\n");

   AddDetail( ({"gull","seagull","seagulls","gulls"}),"Big "
   "grey and white gulls glide lazily along searching for "
   "an easy meal.\n");

   AddDetail( ({"jays","mocking birds","jay","mocking bird",}),
   "Noisy jays and mocking birds chase each other through "
   "the trees.\n");

   SetIntSmell("You are delighted with the clean fresh scent "
   "of ripening limes and heady smell of cinnamon.\n");

   Set(P_TIME_MSG,
    ([
     DAY_DEFAULT_MSG:
       "Seagulls glide silently high over head. Mourning "
       "doves coo as they forage though the soft grass under "
       "the trees and through the flower beds.\n",
     NIGHT_DEFAULT_MSG:
       "Bright lights from the palace light the avenue "
       "and trees. The trees cast long dancing shadows upon "
       "the stone road.\n"]));

   Set(P_INT_MAP,"palace");
   Set(P_REGION,REGION_CITY);
   Set(P_COORDINATES, ({ ({ -50,20,0 }), C_SILVERE }) );


   Set(P_NOWAY_MSG,
"You walk into a tree like an idiot. Somehow you have the feeling that\
 everyone laughs at you.\n");

   AddExit("south",  SIL_CENTER("outside4"));
   AddExit("north",  SIL_CENTER("outside2"));
}
