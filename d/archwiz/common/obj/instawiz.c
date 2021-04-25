/*  Edited for  One Step Beyond - 09/10/97 - Joan  */
inherit "std/thing";
#include <properties.h>
#include <secure/wizlevels.h>
#include <config.h>

#define TP this_player()

static int count;

create() {
  seteuid(getuid());
  (::create());
  SetShort("InstaWiz TM");
/* SetLong replaced by level-dependent QueryLong() */
  SetLong(
"Instawiz, a tool for instantly attaining wizardhood in One Step Beyond.\n"
"Made by Mr. Cheat in '94. Just say 'instawiz' to activate it.\n");
  SetIds(({"instawiz"}));
  SetProp(P_NOBUY,1);
  SetValue(0);
}


init()
{
  (::init());
  add_action("wiz","wiz");
  add_action("wiz","instawiz");
  count = 0;
}


wiz(str) {
  log_file("april",ctime(time())+" | "+this_player()->QueryName()+"\n");
  write("Nothing happens.... hmmm.. whats wrong?\n");
  switch(count) {
case 0: write("Maybe you did something wrong.. perhaps try it in the Temple...\n"); break;
	case 1: write("Still not working? Did someone watch you?\nMaybe you should find a quiet place and try again!\n"); break;
	default:
	  write("Oh.. I know.. its April, 1st!\n"); break;
  }
  count++;
  return 1;
}
  
