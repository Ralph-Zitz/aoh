/* a button that can be worn by anyone */
/* can be created by every adventurer */

inherit "/std/thing";

private static string text;

string QueryText() { return text; }

string SetText(string str) {
  if (!str) return text="I have no clue of anything.";
  text=str;
  SetReadMsg("You read: "+text+"\n");
  SetLong("You have a big button with something written on it.\n");
  return text;
}



string QueryExtralook() {
  object env;
  if ((!env=environment()) || (!query_once_interactive(env))) return "";
  return ((text || "")=="")?"":
    (capitalize(env->QueryPronoun())+
     " wears a BIG button with the inscription:\n"+text+"\n");
}


create() {
  ::create();
  SetShort("a BIG button");
  SetLong("An empty button.\n");
  SetWeight(50);
  SetValue(1);
  SetIds( ({"button"}) );
  SetAds( ({"a","big","BIG"}) );
  SetReadMsg("There is NOTHING written on this button!\n");
  SetNoise("A button does not have sound!\n");
  SetSmell("The button doesn't smell.\n");
}
