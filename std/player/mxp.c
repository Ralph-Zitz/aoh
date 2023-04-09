#include <mxp.h>
#include <newmxp.h>
#include <macros.h>
#include <driver/telnet.h>

private nosave mapping mxp_version_info;
private nosave mapping mxp_support_info;
private int wants_mxp;

public mixed QueryMXP();
public void PublishMXPStats();
public void EndMXP();
public void StartMXP();
public int QueryWantsMXP();
public int SetWantsMXP(int i);
public void init_mxp();
public varargs void Receive(string s);

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

    Receive(
        VT_MXP_LINE_SECURE_MODE
        VT_MXP_OPEN(VT_MXP_VERSION)
        VT_MXP_OPEN(VT_MXP_SUPPORT));
    PublishMXPStats();
    Receive(
        VT_MXP_LINE_SECURE_MODE
        "<!ELEMENT rnum    '' FLAG=RoomNum>"
        "<!ELEMENT rname   '' FLAG=RoomName>"
        "<!ELEMENT rdesc   '' FLAG=RoomDesc>"
        "<!ELEMENT rexits  '' FLAG=RoomExit>"
        "<!ELEMENT rexpire '<expire exits>'>"
        "<!ELEMENT hexpire '<expire html>'>"
        "<!ELEMENT prompt  '' FLAG=Prompt>"
        "<!ELEMENT ex      '<send expire=\"exits\">'>"
        "<!ELEMENT link    '<a href=&url;>' ATT='url'>"
        "<stat hp max=maxhp caption=\"HP:\">"
        "<stat sp max=maxsp caption=\"SP:\">"
        VT_MXP_LOCK_LOCKED_MODE
    );
}

public void PublishMXPStats() {
    if (!interactive() || !QueryMXP())
        return;
    Receive(
        sprintf(
VT_MXP_TEMP_SECURE_MODE "<!ENTITY hp \"%d\" DESC=\"Hit points\" PUBLISH>"
VT_MXP_TEMP_SECURE_MODE "<!ENTITY maxhp \"%d\" DESC=\"Maximum hit points\" PUBLISH>"
VT_MXP_TEMP_SECURE_MODE "<!ENTITY sp \"%d\" DESC=\"Spell points\" PUBLISH>"
VT_MXP_TEMP_SECURE_MODE "<!ENTITY maxsp \"%d\" DESC=\"Maximum spell points\" PUBLISH>"
"%s",
({int})TO->QueryHP(),
({int})TO->QueryMaxHP(),
({int})TO->QuerySP(),
({int})TO->QueryMaxSP(),
VT_MXP_LOCK_LOCKED_MODE));
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

