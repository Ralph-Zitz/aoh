// Statistic object for gathering the object numbers
// of all loaded NPCs.

mixed npcs;

void create()
{
  npcs = ({ });
}

mixed display(object ob)
{
  string str;
  str = sprintf("%-20s %3d %O",({string})ob->QueryName(),({int})ob->QueryGoChance(),({mixed})ob->HeartDebug());
  return str;
}

mixed QueryStats() 
{
  npcs = npcs - ({ 0 });
  return map(npcs, "display", this_object());
}

void Add(object ob)
{
  npcs += ({ ob });
}
