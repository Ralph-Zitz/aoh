/*
* A notesheet
* Created by Thragor, 27oct94
*/

#define STR "/obj/lib/string"

#include "path.h"

inherit "/std/thing";

private static string msg;

create()
{
 if (!is_clone(this_object())) return;
 ::create();
 SetShort("a notesheet");
 SetLong("You may 'write a note' on it and read it later.\n");
 SetIds(({"note","notesheet","sheet","paper"}));
 SetAds(({"paper","a","note","sheet","of"}));
 SetValue(5);
 SetWeight(40);
}

init()
{
 ::init();
 add_action("write_msg","write");
}

int write_msg(string str)
{
 if (!str) return (int)notify_fail("Write what?\n")&&0;
 if (member(({"note","a note","notesheet"}),lower_case(str))==-1)
  return (int)notify_fail("Write what?\n")&&0;
 if (msg) return (int)notify_fail("There's already something writton on it.\n")&&0;
 write("Now write your note:\n");
 STR->input_text();
 return 1;
}

void input_done(string str)
{
 if (str) msg = str;
}

string QueryReadMsg()
{
 return (msg?msg:"The notesheet is empty.\n");
}

