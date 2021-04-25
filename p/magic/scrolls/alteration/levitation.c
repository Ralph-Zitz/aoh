/* Adapted and improved by Softbyte 1995 */
#include <magic.h>
#include <guild.h>
#include <properties.h>
#include <moving.h>
inherit "/obj/scroll";

static object *ows;
stopflying()
{
object mon,shield;
int i;
  if (!ows) return;
  for (i=0;i<sizeof(ows);i++)
  {
     mon=ows[i];
     if (!mon) continue;
     mon->Set(P_FLYING,0);
     shield=present("levitator",mon);
     if (shield)
     {
          tell_object(mon,"You stop levitating.\n");
          shield->remove();
     }
  }
  remove();
}

int MyCast(string str) 
{
object mon, shield;
int i;
  if (str)
  {
   if (str!=this_player()->QueryName())
      write("You cannot use an argument with this "+QuerySpellNoun()+".\n");
    return CAST_FAIL_NOMSG;
  }
  if (present("levitator",this_player()) )
  {
     write("You are still levitating.\n");
     return CAST_FAIL_NOMSG;
  }
  SetWeight(0);
  SetShort(0);
  ForbidCast();
  SetIds(({"levitation"}));
  move(this_player(),M_NOCHECK);
  SetNoDrop();
  seteuid(getuid(this_object()));
  ows=all_inventory(environment(this_player()));
  for (i=0;i<sizeof(ows);i++)
  {
     mon=ows[i];
     if (mon->QueryIsPlayer()) 
     {
        mon->Set(P_FLYING,1);
   	shield=clone_object(MAGIC_OBJ_DIR "levitator");
        if (ME_OK!=shield->move(mon,M_SILENT))
          tell_object(mon,"Seems you cannot levitate - strange!\n");
        else
          tell_object(mon,"You are now levitating some inches over the ground.\n");
        call_out("stopflying",600);
     }
  }
  return CAST_SUCCESS;
}

create() 
{
 ::create();
 AddSpell("levitation",ST_CHANGE,30,#'MyCast,"minor levitation",0);
 SetSpellDesc(
 "This spell allow you and your companions to levitate over an obstacle once.\n");
 SetSpellUsage("cast levitation\n");

 ForbidMemo("all");
 NoSell();
 NoMemorize();
 NoDestruct();
 ows=({});
}

