// Room layout. Northwestern tower
// created:       Isorn    [22-Jul-2001]
// changed:       Isorn    [24-Jul-2001] Added Nigh/Day thanks to Sonic
//

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
   "A guard's shout can be heard from outside.\n",
   "Faintly from below, snores can he heard.\n",
   "The chapel bell rings in the distance.\n",
   });

  return random_select(tmp);
}

string CheckSlit()
{
   string desc;

   desc=
    "The arrow slits are narrow and tall letting in a bit of wind from "
    "outside. ";
   switch(({int})NIGHTDAY->QueryState())
   {
      case ND_NIGHT:
         desc+=
          "It is dark outside and you can see nothing at all.\n";
         break;
      case ND_PREDAWN..ND_SUNRISE:
      case ND_TWILIGHT..ND_EVENING:
         desc+=
          "There's little light outside, allowing you to discern the "
          "shape of the northern battlement and a dim view of the "
          "fields below.\n";
         break;
      case ND_EARLY_MORNING..ND_SUNSET:
         desc+=
          "It's day outside of the tower. Some light shines through "
          "the arrow slits and helps illuminate the tower. Peeking "
          "outside you take a glance of the fields that surround the "
          "fortress and the northern battlement.\n";
         break;
      default:
         desc+=
          "There is something outside of the tower, but "
          "you can't clearly define it.\n";
         break;
   }
   return desc;
}

string CheckFields()
{
   string desc;

   desc=
    "Peeking north ";
   switch(({int})NIGHTDAY->QueryState())
   {
      case ND_NIGHT:
         desc+=
          "into the dark of the night you don't seem to be able to "
          "discern anything.\n";
         break;
      case ND_PREDAWN..ND_SUNRISE:
         desc+=
          "you can see the first light of the day crawl across the "
          "fields.\n";
         break;
      case ND_TWILIGHT..ND_EVENING:
         desc+=
          "in the diminishing light of the day the shadows grow longer "
          "across the fields.\n";
         break;
      case ND_EARLY_MORNING..ND_SUNSET:
         desc+=
          "the fields stretch as far as the eye can see in the clear "
          "light of the day.\n";
         break;
      default:
         desc+=
          "There is something outside of the tower, but "
          "you can't clearly define it.\n";
         break;
   }
   return desc;
}

string CheckBatt()
{
   string desc;

   desc=
    "Looking east from the arrow slits ";
   switch(({int})NIGHTDAY->QueryState())
   {
      case ND_NIGHT:
         desc+=
          "you can barely make out the shapes of the northern "
          "battlements, in the darkness. Only the light of the "
          "guard's torches illuminate them.\n";
         break;
      case ND_PREDAWN..ND_SUNRISE:
         desc+=
          "you can see as the first rays of the rising sun illuminate "
          "the top of the battlements.\n";
         break;
      case ND_TWILIGHT..ND_EVENING:
         desc+=
          "the last light of the day shines over the battlements as the "
          "night approaches.\n";
         break;
      case ND_EARLY_MORNING..ND_SUNSET:
         desc+=
          "the north battlements can be seen, they are made from stone and "
          "offer a strong cover against arrows shoot from the ground below.\n";
         break;
      default:
         desc+=
          "There is something outside of the tower, but "
          "you can't clearly define it.\n";
         break;
   }
   return desc;
}

varargs void create()
{
   ::create();
   Set(P_INDOORS,1);
   Set(P_INT_SHORT,"Northeastern corner of northwestern tower");
   Set(P_INT_LONG,
    "There's little light in here and most of it comes from the torches "
    "hanging from the torch-holders set in the wall and the little light "
    "that seeps from the arrow slits that dot the walls. There's a barrel "
    "filled with arrows next to the wall. A bit to the south lie the "
    "entrance to the tower and a staircase.\n");

   Set(P_INT_WIZMSG,
    " South: Entrance to northwestern tower, level 2.\n"
    " West:  NW Corner of NW tower, level 2.\n");

   AddDetail( ({"floor","ground"}),
    "The floor is made of stone slabs and quite clean. The stones are "
    "chipped at some places.\n");

   AddDetail( ({"wall","walls"}),
    "Made of stone, they look strong and solid without cracks. They are "
    "darkened where the torch-holders hold the torches. A bit to the "
    "south there are doorways to the battlements while around you there "
    "are several arrow slits.\n");

   AddDetail( ({"ceiling"}),
    "Big wooden beams hold the stone ceiling.\n.");

   AddDetail( ({"torch-holders","holders"}),
    "Beautifully crafted from iron, they are set into the wall, holding "
    "the torches that give light to the tower.\n");

   AddDetail( ({"torch","torches"}),
    "The torches are the only light source here, their flickering light "
    "creates shadows in the room.\n");

   AddDetail( ({"shadow","shadows"}),
    "Shadows play around the room, created by the flickering light of the "
    "torches.\n");

   AddDetail( ({"slit","slits","arrow slit","arrow slits"}),#'CheckSlit /*'*/);

   AddDetail( ({"field","fields"}),#'CheckFields /*'*/);

   AddDetail( ({"battlement","battlements","north battlement",
   "northern battlement"}),#'CheckBatt /*'*/);

   AddDetail( ({"door","doors"}),
    "The doors looks quite sturdy, made of oak and bound with metal straps, "
    "an iron bar can be used to block the doors.\n");

   AddDetail( ({"stairs","staircase"}),
    "A narrow staircase opens a bit to the south, leading up and down.\n");

   AddDetail( ({"barrel"}),
    "It's a big barrel, filled with arrows for its use when the fortress "
    "is besieged.\n");

   SetIntSmell("It smells of smoke and sweat are quite strong here, mixed "
    "with the smell of plants the wind blows into the room.\n");

   Set(P_INT_MAP,"fortress");
   Set(P_REGION,REGION_CITY);
//   Set(P_COORDINATES, ({ ({ -50,20,0 }), C_SILVERE }) );


   Set(P_NOWAY_MSG,
    "You run into a wall, must be the lack of light.\n");

   AddExit("south",  SIL_FORTRESS("nwto24"));
   AddExit("west",   SIL_FORTRESS("nwto21"));

}