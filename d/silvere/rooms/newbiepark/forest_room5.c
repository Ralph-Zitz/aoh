// A little crossing. To the north are some rats and rabbits, to the
// south you can find some cats
// created:       Sonic    [07-Jun-1997]
// last changed:  Sonic [05-Jul-1997] added region/coordinates

#include <silvere.h>
#include <properties.h>
#include <coordinates.h>
#include <regions.h>
#include <moving.h>
#include <macros.h>
#include <rooms.h>
inherit BASEROOM;

varargs mixed IntLong()
{
   string desc,wizmsg;    // w is temporary until P_WIZ_MSG is no longer needed
   mapping exits;
   mixed exitkey, exitdesc;

   desc=::Query(P_INT_LONG);
   if(-1 != member("\n",desc[<1])) desc[<1]=' ';
   exits=({mapping})TO->Query(P_EXITS);
   if(exits)
   {
      int a;

      exitkey= m_indices(exits);
      exitdesc=m_values(exits);
      for(a=0;a<sizeof(exitkey); a++)
      {
         if(exitkey[a]=="northeast")
         {
            load_object(exitdesc[a]);
            if(present("rat",find_object(exitdesc[a])))
               desc+="Squeaks and other noises are coming from the north. ";
         }
      }
      desc+="\n";
   }

/* this is a little temporary action, until P_WIZ_MSG is not needed anymore.
 * Or better, if it is not that urgent anymore
 */
   if((TP) && (query_user_level(TP)) && (!({int})TP->Query(P_NOWIZ))
           && (wizmsg=Query(P_INT_WIZMSG))) desc+="\n"+wizmsg;
   return desc;
}

varargs void create()
{
   ::create();
   Set(P_INDOORS,0);
   Set(P_INT_SHORT,"In a dimly lit forest.");
   Set(P_INT_WIZMSG,
"This is a crossing in a dimly lit forest. To the northeast is an\
 area with some rats and rabbits, and to the south you can find some\
 cats.\n\
 West:      The path leads here to the northern exit of the park.\n\
 Northeast: Slay some animals there.\n\
 Southeast: Follow the road.\n");

   Set(P_INT_LONG,
      "This is a crossing in the forest. A path leads in from the west "
      "and splits here to the north- and southeast. There are squeaks "
      "from the north, if there are rats present, otherwise you hear "
      "nothing. Maybe a little bank can be placed here, where you can "
      "sit down for a moment and relax.\n");

   /*
    * Add some details here. *grin*
    */

   Set(P_INT_MAP,"silvere2");
   Set(P_REGION, REGION_CITY|REGION_FOREST);
   Set(P_COORDINATES, ({ ({-140,80,0 }), C_SILVERE }) );

   Set(P_NOWAY_MSG,
"You bump into a tree like an idiot. If you do such things often, you\
 probably need a new face in a not too far future.\n");

   AddExit("west",      SIL_PARK("forest_room4"));
   AddExit("northeast", SIL_PARK("forest_north1"));
   AddExit("southeast", SIL_PARK("forest_room6"));
}
