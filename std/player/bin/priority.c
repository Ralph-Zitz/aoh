/*
    Simply a command to set the aiming location

	October 2021

        V1.0: Nostradamus
*/

#include <properties.h>
#include <daemons.h>
#include <msgclass.h>
#include <skills.h>
#include <combat.h>
#include <macros.h>
#include <config.h>
#include <search.h>

public int main( string arg ) {
  object target;

  if (!arg || arg == "?") {
    string t = objectp(({object})TP->QueryTarget()) ? ({string})TP->QueryTargetShort() : "nobody";
    msg_write(CMSG_GENERIC, "You are current priority target is: " + t + ".\n");
    return 1;
  }
  target = search_env(lower_case(arg));
  if (!({int})TP->CanSee() || !target || !objectp(target) || !living(target))
    return notify_fail("You can't see that creature here!\n", NOTIFY_ILL_ARG);
  TP->SetTarget(target);
  msg_write(CMSG_GENERIC, "Your priority target is now: " + ({string})TP->QueryTargetShort() + "\n");
  return 1;
}
