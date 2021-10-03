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

public int main( string arg ) {
  string locs = implode(AIM_LOCATIONS[..<2], ", ") + ", and " + AIM_LOCATIONS[<1] + ".\n";
  if (!arg || arg == "?") {
    msg_write(CMSG_GENERIC, "You are currently aiming at: " + ({string})TP->QueryAim()+ ".\n"
                            "Valid locations are: " + locs);
    return 1;
  }
  if (!VALID_AIM(lower_case(arg))) {
    notify_fail("You cannot aim at '"+arg+"'\nPlease choose from: " + locs, NOTIFY_ILL_ARG);
    return 0;
  }
  TP->SetAim(lower_case(arg));
  msg_write(CMSG_GENERIC, "Now aiming at: " + arg + "\n");
  return 1;
}
