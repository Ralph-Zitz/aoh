/*
** /obj/disease
** This object may be used to configure diseases, illnesses, curses, viruses,
** etc.
**
** Author: Thragor@Nightfall, 07jul95
** Update:
*/

inherit "/std/thing";

#include <properties.h>
#include <health.h>

#define ME this_object()

private static mixed Pcure_msg;

// QueryExtralook might give a nice view of the player suffering from this
// disease, e. g.:
// public string QueryExtralook()
// {
//   object pl;
//   if (  !(pl=environment())
//       ||!query_once_interactive(pl))
//     return "";
//   return capitalize(pl->QueryPronoun())+
//     " seems to suffer from something.\n";
// }

// The CureMsg will be queried and evaluated by the healer.

public mixed SetCureMsg(mixed msg)
// How <msg> has to look like, see 'man healer' and look at the
// Message-Command. To insert the name of the healer either use
// @@Name@@ for the normal name or @@Name|1@@ for the capitalized name.
{
  return Pcure_msg = msg;
}

public mixed QueryCureMsg() { return Pcure_msg; }

public int create()
{
  if (!is_clone(ME)) return 1;
  (::create());
  SetShort("a disease"); // a disease might be invisible, but then
                         // long, info, etc. can't be read. Only
                         // a general check with 'inform health'
                         // will reveal an info about this disease.
  SetLong("You seem to suffer from something.\n");
  SetInfo("You just have an uneasy feeling about this disease.\n");
  SetValue(1); // it's very easy to cure this disease
  SetIds(({"disease",D_DISEASE}));
  // The ids are elementary for the diseases, curses, etc.
  // The ids will be checked in the following order (by the healer):
  // 'curse' -> It's a curse we have to remove.
  // 'poison' -> It's a poison which shall be removed. Most of the time
  //             you may use P_POISON in the player instead of an extra
  //             object.
  // 'virus',
  // 'illness',
  // 'disease' -> The 'normal' disease. It can be cured by the healer.
  SetNoDrop("To get rid of a disease go to a healer.\n");
  SetNoGive("To get rid of a disease go to a healer.\n");
  SetNoGet("Did you ever TAKE a disease? Better don't do that.\n");
  Set(P_NOSELL,"Hm. You really shouldn't sell this at a shop ;-)\n");
  // Such a disease might be autoloading, but it's always nice if there's
  // at least one escape to get rid of a disease: quitting.
}
