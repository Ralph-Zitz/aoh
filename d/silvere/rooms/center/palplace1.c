// Room layout. The road here leads around the palace
// created:       Sonic    [15-Jun-1997]
// changed:       Sonic    [01-Jul-1997]  added region/coordinates
// changed:       Joan     [03-Sept-2001] Added stuff



#include <silvere.h>
#include <properties.h>
#include <coordinates.h>
#include <nightday.h>
#include <regions.h>
#include <moving.h>
#include <macros.h>
#include <rooms.h>

inherit SILBASEROOM;

string QueryIntNoise()
{
  string *tmp;

  tmp = ({
   "A distant shout from the city reaches your ears.\n",
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

   Set(P_INT_SHORT,"The palace square");
   Set(P_INT_LONG,
    "This is the western side of a small square which rests between the "
    "wings of the royal palace. Beds of exotic flowers curve around "
    "paths of gravel and crushed seashells below the windows of the "
    "palace. @@TimeMsg@@\n");


   Set(P_INT_WIZMSG,
    " North: A small place with the entrance into the palace.\n"
    " West:  To the western part, the harbour and some other places.\n"
    " East:  To the south and east part of the town.\n");

   AddVItem(
    ([ P_IDS : ({ "lantern", "light", "lanterns", "lights" }),
       P_ADS : ({ "cast", "iron", "alabaster" }),
       P_LONG :
        "Cast iron frames hold thin, rectangular sheets of alabaster "
        "as side panels, which glow with a soft white light when the "
        "lanterns are lit.\n"
    ]) );

   AddDetail( ({"palace","palace wall","front palace wall"}),
   "The front palace wall stretches into the distance. The "
   "palace is constructed of the same brown stone that paves "
   "the elegant avenue.\n");

   AddDetail( ({"gull","seagull","seagulls","gulls"}),"Big "
   "grey and white gulls glide lazily along searching for "
   "an easy meal.\n");

   AddDetail( ({"dove","mourning dove","mourning doves","doves"}),
   "Small dappled grey doves forage for food beneath the shrubs.\n");

   AddDetail( ({"rose","roses","shrubs"}),"Thick shrub "
   "roses line the base of the palace wall. Their profuse "
   "blooms of soft pink and creamy white roses tumble from "
   "mid wall to the emerald grass below in a breathtaking "
   "display of natural beauty.\n");

  AddDetail( ({"flower","flowers","saliums","salium", }),
   "A myriad of soft pink saliums border the "
   "shrub rose and palace wall.\n");


   SetIntSmell("The flowers fill the air with their delicate "
   "perfume.\n");

   Set(P_TIME_MSG,
    ([ NIGHT_DEFAULT_MSG:
       "Cast iron lanterns with alabaster sides glow softly, casting a soft "
       "glow onto the paths through the foliage. The fountain in the center "
       "of the square is also illuminated by the soft glow of alabaster "
       "lamps.",
       DAY_DEFAULT_MSG:
       "Cast iron lanterns with alabaster panels line the edges of the "
       "paths."
    ]) );



   Set(P_INT_MAP,"palace");
   Set(P_REGION,REGION_CITY);
   Set(P_COORDINATES, ({ ({ -10,30,0 }), C_SILVERE }) );

   AddExit("south", SIL_CENTER("outside9"));
   AddExit("north", SIL_CENTER("palplace4"));
   AddExit("east",  SIL_CENTER("palplace2"));
}
