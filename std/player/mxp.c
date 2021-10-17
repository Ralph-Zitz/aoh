#include <mxp.h>
#include <driver/telnet.h>

private nosave mapping mxp_version_info;
private nosave mapping mxp_support_info;
private int wants_mxp;

public mixed QueryMXP();
public void EndMXP();
public void StartMXP();
public int QueryWantsMXP();
public int SetWantsMXP(int i);
public void init_mxp();

public void end_mxp() {
  efun::write(MXPMODE(3));
  SetWantsMXP(0);
  EndMXP();
}

public void start_mxp() {
  StartMXP();
  init_mxp();
}

public void init_mxp() {
  mxp_version_info = ([]);
  mxp_support_info = ([ MXP_SUPPORT_ENABLED: ({}), MXP_SUPPORT_DISABLED: ({}) ]);

  if (QueryMXP())
    SetWantsMXP(1);
  else {
    SetWantsMXP(0);
    return;
  }

  // Tell the client that from now on we are sending MXP sequences
  //binary_message(({IAC, SB, TELOPT_MXP, IAC, SE}));

  // Switch to "permanent secure" mode, MXP tags now enabled
#if 0  
  efun::write(MXPMODE(6) +
    MXPTAG2("VERSION") +
    MXPTAG2("SUPPORT") +
    MXPTAG2("!-- Set up MXP elements --") +
    MXPTAG2("!ELEMENT RNum FLAG=\"RoomNum\" ATT=\"id\" EMPTY") +
    MXPTAG2("!ELEMENT RName FLAG=\"RoomName\"") +
    MXPTAG2("!ELEMENT RDesc FLAG=\"RoomDesc\"") +
    MXPTAG2("!ELEMENT RExits FLAG=\"RoomExit\"") +
    MXPTAG2("!ELEMENT Prompt FLAG=\"Prompt\"") +
    MXPTAG2("!ELEMENT red '<COLOR red><B>'") +
    MXPTAG2("!ELEMENT Ex \"<send>\""));
#endif
   // Switch to "permanent secure" mode, MXP tags now enabled
  efun::write(process_mxp(MXPMODE(6), QueryMXP()));
  efun::write(process_mxp(MXPTAG("VERSION"), QueryMXP()));
  efun::write(process_mxp(MXPTAG("SUPPORT"), QueryMXP()));
  efun::write(process_mxp(MXPTAG("!-- Set up MXP elements --"), QueryMXP()));
  efun::write(process_mxp(MXPTAG("!ELEMENT RNum FLAG=\"RoomNum\" ATT=\"id\" EMPTY"), QueryMXP()));
  efun::write(process_mxp(MXPTAG("!ELEMENT RName FLAG=\"RoomName\""), QueryMXP()));
  efun::write(process_mxp(MXPTAG("!ELEMENT RDesc FLAG=\"RoomDesc\""), QueryMXP()));
  efun::write(process_mxp(MXPTAG("!ELEMENT RExits FLAG=\"RoomExit\""), QueryMXP()));
  efun::write(process_mxp(MXPTAG("!ELEMENT Prompt FLAG=\"Prompt\""), QueryMXP()));
  efun::write(process_mxp(MXPTAG("!ELEMENT red '<COLOR red><B>'"), QueryMXP()));
  efun::write(process_mxp(MXPTAG("!ELEMENT Ex \"<send>\""), QueryMXP()));
}

public int QueryWantsMXP() {
  return wants_mxp;
}

public int SetWantsMXP(int i) {
  return i ? wants_mxp = 1 : wants_mxp = 0;
}

public void SetMXPVersion(mapping m = ([])) {
  if (!mxp_version_info)
    mxp_version_info = ([]);
  mxp_version_info = m;
}

public void SetEnabledMXPSupportInfo(string *s = ({})) {
  if (!mxp_support_info)
    mxp_support_info = ([]);
  mxp_support_info[MXP_SUPPORT_ENABLED] = s;
}

public void SetDisabledMXPSupportInfo(string *s = ({})) {
  if (!mxp_support_info)
    mxp_support_info = ([]);
  mxp_support_info[MXP_SUPPORT_DISABLED] = s;
}

public mapping QueryMXPVersion() {
  return mxp_version_info || ([]);
}

public mapping QueryMXPSupportInfo() {
  return mxp_support_info || ([ MXP_SUPPORT_ENABLED: ({}), MXP_SUPPORT_DISABLED: ({}) ]);
}

