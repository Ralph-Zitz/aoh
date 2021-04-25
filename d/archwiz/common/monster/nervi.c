/*
     Nervi ;-)   
                    changes and bugfix by Thragor
                    idea and buggy code by Distar
*/

#include <moving.h>
#include <living.h>

#define TO this_object()
#define NEWBIE 3

inherit "/std/npc";

private static object pl, ob;

int CheckNewbie(object player)
{
 return !(query_user_level()||player->QueryLevel()>NEWBIE);
}

create()
{
 if (::create()) return 1;
 SetName("Nervi");
 SetShort("Nervi");
 SetLong(
"Nervi is your biggest fan. He will follow you ! "
"If you don't like him anymore you can command him "
"to 'nerve' another player in the same room with you.\n"
 );
 SetRace("human");
 SetWeight(60000);
 SetAttribute("MagicDefence",20);
 SetLevel(1);
 SetHP(10);
 SetAlign(0);
 SetGender(GENDER_MALE);
 SetNoGet("Nervi vehemently refuses to be taken.\n");
 pl = 0;
}

InformDistar()
{
 object distar;
 if (distar=find_player("distar"))
  tell_object(distar,"Nervi now nervs: "+capitalize(getuid(pl))+".\n");
}

StartAdmire()
{
 if(interactive(pl)&&!CheckNewbie(pl))
 {
  remove_call_out("follow");
  call_out("follow",10);
  InformDistar();
 }
 else pl=0;
}

void init()
{
 ::init();
 add_action("do_nerv","nerve");
 if (pl) return;
 if (pl = this_player()) StartAdmire();
}

do_nerv(string str)
{
 object newPL;
 if(!str)
 {
  notify_fail("Whom do you want to nerve? \n");
  return;
 }
 if(!newPL = find_player(str))
 {
  notify_fail("This player isn't logged on.\n");
  return;
 }
 if(!present(newPL,environment(this_object())))
 {
  notify_fail("This player in not in the same room with Nervi.\n");
  return;
 }
 if (!query_once_interactive(newPL))
 {
  notify_fail("Nervi only admires players but not boring NPCs.\n");
  return;
 }
 if (!interactive(newPL))
 {
  notify_fail("Nervi tells you: This statue is boring ...\n");
  return;
 }
 if(CheckNewbie(newPL))
 {
  notify_fail("You shouldn't nerve a newbie!\n");
  return;
 }
 write("Nervi will nerve "+newPL->QueryName()+" now.\n");
 pl = newPL ;
 StartAdmire();
 return 1;
}

follow()
{
 string emsg,pmsg;
 object env;

 if (!pl) return;
 if(!interactive(pl))
 {
  pl=0;
  return;
 }
 if (CheckNewbie(pl))
 {
  pl=0;
  return;
 }
 if((env = environment(pl))!=environment(TO))
 {
  this_object()->move(env,M_GO);
 }
 switch(random(5))
 {
  case 1: emsg = "Nervi says: I love you, "+pl->QueryName()+"!!!\n";
          pmsg = "Nervi says: I love you, "+pl->QueryName()+"!!!\n";
          break;
  case 2: pmsg = "Nervi admires you.\n";
          emsg = "Nervi admires "+pl->QueryName()+".\n";
          break;
  default: emsg = ""; pmsg = "";
 }
 tell_room(env,emsg,({pl}));
 tell_object(pl,pmsg);
 call_out("follow",10);
}

void Kill(object ob)
{
 tell_room(environment(TO),"Nervi refuses to fight.\n");
}

Attack()
{
 tell_room(environment(TO),"Nervi stands around but doesn't want to fight.\n");
 StopAttack();
}

StartHunting(e)
{
 e->StopHunting(this_object());
 tell_room(environment(TO),
"Nervi refuses to hunt anyone. He just wants to admire.\n");
}

Defend(dam,dam_type,weapon)
{
 if (!pl)
 {
  if (!pl = this_player()) return;
  write("Nervi thinks you're fascinating.\n");
  StartAdmire();
 }
 tell_object(this_player()," but you miss Nervi.\n");
 tell_room(environment(TO)," but misses Nervi.\n",({this_player()}));
 this_player()->StopHunting(this_object());
 if (this_player()==pl)
 {
  tell_room(environment(TO),"Nervi cries: Don't kill me master!\n");
  return;
 }
 if (!TO) {return;}
 if (!pl) {return;}
 tell_room(environment(TO),"Nervi is too busy admiring "+pl->QueryName()+".\n");
 this_player()->StopHunting(this_object());
}

DoDamage(arg)
{
 if (!pl)
 {
  if (!pl = this_player()) return;
  write("Nervi thinks you're fascinating.\n");
  StartAdmire();
 }
 if (this_player()==pl)
 {
  tell_room(environment(TO),"Nervi cries: Don't kill me master!\n");
  return;
 }
if (!pl) return 0;
 tell_room(environment(TO),"Nervi is too busy admiring "+pl->QueryName()+".\n");
}

Die(){
 if (pl) {
 tell_room(environment(this_object()),
"Nervi says: I'm bored to fight ... I just want to admire "+pl->QueryName()+".\n"
"Nervi turns away and smiles at "+pl->QueryName()+": I LOVE YOU!!!\n");
 }
 else
  if (!pl = this_player())
  {
   tell_room(environment(this_object()),
"Nervi is bored. Whom should he admire?\n");
  }
  else
  {
   write("Nervi thinks you're fascinating.\n");
   StartAdmire();
  }
}
