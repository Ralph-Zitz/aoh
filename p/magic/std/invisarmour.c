// A Standard Armour or Ring providing invisibility 
//
// [s] Creation 22.07.96
//
//
// [s] Softbyte, AWofMagic
//


#include <properties.h>
#include <combat.h>

inherit "/std/armour";


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

private static int _spcost;
int QuerySPCost() {return _spcost;}
int SetSPCost(int i) {_spcost=i;return 1;}

private static int _epcost;
int QueryEPCost() {return _epcost;}
int SetEPCost(int i) {_epcost=i;return 1;}

private static int _sptime;
int QuerySPTime() {return _sptime;}
int SetSPTime(int i) {_sptime=i;return 1;}

private static int _eptime;
int QueryEPTime() {return _eptime;}
int SetEPTime(int i) {_eptime=i;return 1;}

private static int _warntime;
int QueryWarnTime() {return _warntime;}
int SetWarnTime(int i) {_warntime=i;return 1;}

private static int _removetime;
int QueryRemoveTime() {return _removetime;}
int SetRemoveTime(int i) {_removetime=i;return 1;}

private static string *_vismsg;
int SetVisibleMsg(string *str) { _vismsg=str; return 1; }
string *QueryVisibleMsg() {return _vismsg;}

private static string *_invismsg;
int SetInvisibleMsg(string *str) { _invismsg=str; return 1; }
string *QueryInvisibleMsg() {return _invismsg;}

private static string *_spmsg;
int SetSPMsg(string *str) { _spmsg=str; return 1; }
string *QuerySPMsg() {return _spmsg;}

private static string *_epmsg;
int SetEPMsg(string *str) { _epmsg=str; return 1; }
string *QueryEPMsg() {return _epmsg;}

private static string *_warnmsg;
int SetWarnMsg(string *str) { _warnmsg=str; return 1; }
string *QueryWarnMsg() {return _warnmsg;}

private static string *_removemsg;
int SetRemoveMsg(string *str) { _removemsg=str; return 1; }
string *QueryRemoveMsg() {return _removemsg;}

int do_vis();

void create() {
  ::create();
  AddId("ring");
  AddId("invisibility");
  AddAdjective("of");
  AddAdjective("ring");
  AddAdjective("invisibility");
  SetShort("Ring of Invisibility");
  SetLong ("Wear the ring to activate its powers.\n");
  SetValue (10000);
  SetWeight (500);
  SetAC(0);
  SetArmourType(AT_RING);
  Set(P_MAGIC,1);
  owner=0;

  SetSPCost(0);
  SetSPTime(0);
  SetSPMsg(({"You loose some mental energy.\n",""}));

  SetEPCost(0);
  SetEPTime(0);
  SetEPMsg(({"Your endurance decreases.\n",""}));

  SetRemoveTime(60);
  SetWarnTime(30);
  SetWarnMsg( ({"You begin to become partly visible.\n",""}) );
  SetRemoveMsg( ({"Your @@QueryShort@@ dissolves.\n",""}) );


  SetVisibleMsg( ({"You turn visible again.\n",
   "@@OwnerName|1@@ appears from nowhere.\n"}) );
  SetInvisibleMsg( ({"You turn invisible.\n",
   "@@OwnerName|1@@ vanishes.\n"}) );

  "/obj/lib/register"->RegisterCreator(this_object(),"INVISARMOUR");

}

/* analogous to ReduceSP but does not reside in TP */
int ReduceEP(int amount, object who)
{
  if (!who) return 0;
  if (who->QueryAttr("EP")<amount) return 0;
  who->SetAttr("EP",who->QueryAttr("EP")-amount);
  return amount;
}

int reduce_ep()
{
  if (!owner) return 0;
  if(QueryEPCost() && !ReduceEP(QueryEPCost(),owner)) 
  {
    Unwear(0);
    return 0;
  }
  tell_object(owner,process_string(QueryEPMsg()[0] || ""));
  tell_room(environment(owner),process_string(QueryEPMsg()[1] || ""),({owner}));
  call_out("reduce_ep",QueryEPTime());
  return 1;
}
int reduce_sp()
{
  if (!owner) return 0;
  if(QuerySPCost() && !owner->ReduceSP(QuerySPCost())) 
  {
    Unwear(0);
    return 0;
  }
  tell_object(owner,process_string(QuerySPMsg()[0] || ""));
  tell_room(environment(owner),process_string(QuerySPMsg()[1] || ""),({owner}));
  call_out("reduce_sp",QuerySPTime());
  return 1;
}
warn()
{
  if (!owner) return 0;
  tell_object(owner,process_string(QueryWarnMsg()[0]||""));
  tell_room(environment(owner),process_string(QueryWarnMsg()[1]||""),({owner}));
  SetWarnTime(0);
}
remove_it()
{
  if (!owner) return 0;
  tell_object(owner,process_string(QueryRemoveMsg()[0]||""));
  tell_room(environment(owner),process_string(QueryRemoveMsg()[1]||""),({owner}));
  Unwear(EWF_SILENT);
  call_out("remove",1); 
}

do_vis()
{
int t;
  if (owner && owner->Query(P_INVIS) )
  {
    owner->Set(P_INVIS,0);
    tell_object(owner,process_string(QueryVisibleMsg()[0] || ""));
    tell_room(environment(owner),
        process_string(QueryVisibleMsg()[1] || ""),({owner}));
  }
  remove_call_out("reduce_sp");
  remove_call_out("reduce_ep");
  if ( (t=find_call_out("warn"))>=0 && QueryWarnTime()>0 )
    SetWarnTime(t);
  remove_call_out("warn");
  if ( (t=find_call_out("remove_it"))>=0 && QueryRemoveTime()>0 )
    SetRemoveTime(t);
  remove_call_out("remove_it");
  owner=0;
}

int ChkWear(object living,object me,int flags)
{
  if (living->Query(P_INVIS)) {
    tell_object(living,"You are already invisible.\n");
    return EQ_FORBID;
  }
  if (QuerySPCost() && !living->ReduceSP(QuerySPCost())) {
    tell_object(living,"You don't have the spellpoints to become invisible.\n");
    return EQ_FORBID;
  }
  if (QueryEPCost() && !ReduceEP(QueryEPCost(),living)) {
    tell_object(living,"You don't have the endurance points to become invisible.\n");
    return EQ_FORBID;
  }
   return EQ_OK; 
}
void NotifyWear(object living,object me,int flags)
{
  if (!living || !me) return;
  owner=living;
  tell_object(living,process_string(QueryInvisibleMsg()[0]||""));
  tell_room(environment(owner),
     process_string(QueryInvisibleMsg()[1] || ""),({living}));
  living->Set(P_INVIS,1);
  if (QuerySPTime()>0) call_out("reduce_sp",QuerySPTime());
  if (QueryEPTime()>0) call_out("reduce_ep",QueryEPTime());
  if (QueryWarnTime()>0) call_out("warn",QueryWarnTime());
  if (QueryRemoveTime()>0) call_out("remove_it",QueryRemoveTime());
}
void NotifyUnwear(object living,object me,int flags)
{
  do_vis();
}
remove()
{
  do_vis();
  return ::remove();
}
