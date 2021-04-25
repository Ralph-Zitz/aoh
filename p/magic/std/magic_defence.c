// A Standard Armour or Ring providing protection against some damage
//
// [s] Creation 26nov96
//
//
// [s] Softbyte, AWofMagic
//


#include <properties.h>
#include <combat.h>
#include <magic.h>

inherit "/std/armour";

// Dummy code to provide notify functions - overwrite these if you want 
// Will be called when the Warn time is exceeded 
void Notify_warn(object who)
{
  tell_object(who,"The "+QueryShort()+" is beginning to dissolve.\n");
}
// Will be called if the remove time is exceeded, i.e. the amulet is destructed
void Notify_remove(object who) 
{
  tell_object(who,"The "+QueryShort()+" is fading away.\n");
}
// Will be called if the amulet is activated
void Notify_activate(object who) 
{
  tell_object(who,"The magic protection is active.\n");
}
// Will be called if the amulet is deactivated
void Notify_deactivate(object who) 
{
  tell_object(who,"The magic protection is no longer active.\n");
}

// The "owner" of the amulet
private static object owner;
object QueryOwner() {return owner;}
int SetOwner(object ob) {owner=ob;return 1;}
varargs string OwnerName(int flag)
{
string s;
  if (owner) s=owner->QueryName();
  else s="someone";
  if (flag) return capitalize(s);
  return s;
}

// The amulet protects against WHAT (DT_XXX) with VALUE (0..100)
private mapping mydef;
int AddProtection(mixed what,int value)
{
  if (mydef[what]==0) mydef[what]=value;
  else mydef[what]+=value;
  return mydef[what];
}

mapping QueryProtections()
{
  return mydef;
}

// The amulet is activates if you wear it and deactivated if you unwear it
private static int activate_worn;
int SetActivateWorn() {activate_worn=1;}
int ResetActivateWorn() {activate_worn=0;}
int QueryActivateWorn() {return activate_worn;}

// After this time a warning message is displayed that the amulet is about to destruct
private static int _warntime;
int QueryWarnTime() {return _warntime;}
int SetWarnTime(int i) {_warntime=i;return 1;}

// After this time the amulet is removed
private static int _removetime;
int QueryRemoveTime() {return _removetime;}
int SetRemoveTime(int i) {_removetime=i;return 1;}

create() {
  ::create();
  SetShort("a small amulet");
  SetLong(
"This is a small magic amulet.\n");
  SetArmourType(AT_AMULET);
  SetAC(0);
  SetAds(({"small","magic"}));
  SetIds(({"amulet"}));
  SetInfo("It seems to be useless, but perhaps there's magic about it?\n");
  SetWeight(0);
  SetValue(0);
  Set(P_MAGIC,1);
  owner=0;
  mydef=([]);

  SetRemoveTime(600);
  SetWarnTime(500);
  SetActivateWorn();

  "/obj/lib/register"->RegisterCreator(this_object(),"DEFENCEAMULET");

}

warn()
{
  if (!owner) return 0;
  Notify_warn(owner);
  SetWarnTime(0);
}
remove_it()
{
  if (!owner) return 0;
  Notify_remove(owner);
  Unwear(EWF_SILENT);
  call_out("remove",1); 
}
int activate(object who)
{
mixed ind;
int i,size;
mixed *defs;
  if (!who) return 0;
  owner=who;
  defs=owner->QueryDefences()||({});
  size=sizeof(defs);
  ind=m_indices(mydef)||({});
  for (i=0;i<sizeof(ind);i++)
  {
    if (ind[i]<size) 
      owner->AddDefence(ind[i],mydef[ind[i]]+defs[ind[i]]);
    else 
      owner->AddDefence(ind[i],mydef[ind[i]]);
  }
  if (QueryWarnTime()>0) call_out("warn",QueryWarnTime());
  if (QueryRemoveTime()>0) call_out("remove_it",QueryRemoveTime());
  Notify_activate(owner);
  return 1;
}
int deactivate()
{
mixed ind;
mixed *defs;
int size,i,t;
  if (!owner) return 0;
  defs=owner->QueryDefences()||({});
  size=sizeof(defs);
  ind=m_indices(mydef)||({});
  for (i=0;i<sizeof(ind);i++)
  {
    if (i<size) 
      owner->AddDefence(ind[i],-mydef[ind[i]]+defs[ind[i]]);
    else 
      owner->AddDefence(ind[i],-mydef[ind[i]]);
  }
  if ( (t=find_call_out("warn"))>=0 && QueryWarnTime()>0 )
    SetWarnTime(t);
  remove_call_out("warn");
  if ( (t=find_call_out("remove_it"))>=0 && QueryRemoveTime()>0 )
    SetRemoveTime(t);
  remove_call_out("remove_it");

  Notify_deactivate(owner);
  owner=0;
  return 1;
}


void NotifyWear(object living,object me,int flags)
{
  if (!living || !me) return;
  if (activate_worn) activate(living);
}
void NotifyUnwear(object living,object me,int flags)
{
  if (activate_worn) deactivate();
}
remove()
{
  deactivate();
  return ::remove();
}
