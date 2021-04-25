/* This is a chair for to sit down on it and leave it again */

inherit "/std/thing";

static string design_short,design_long;

/*----------------------------------------------------------------------------*/
create() {
  ::create();
  SetNoGet("Better you don't take this thing with you. What do you want with\n"+
           "it out in the world?\n");
  SetWeight(2500);
}
/*----------------------------------------------------------------------------*/
void SetDesignShort(string str) {
  design_short=str;
}
/*----------------------------------------------------------------------------*/
void SetDesignLong(string str) {
  design_long=str;
}
/*----------------------------------------------------------------------------*/
string QueryDesignShort() {
  return design_short;
}
/*----------------------------------------------------------------------------*/
string QueryDesignLong() {
  return design_long;
}
/*----------------------------------------------------------------------------*/
string QueryShort() {
  if (query_verb()=="inform" || query_verb()=="read") return QueryDesignShort();
  return ::QueryShort();
}
/*----------------------------------------------------------------------------*/
string QueryLong() {
  if (query_verb()=="inform") return QueryDesignLong();
  return ::QueryLong();
}
/*----------------------------------------------------------------------------*/



