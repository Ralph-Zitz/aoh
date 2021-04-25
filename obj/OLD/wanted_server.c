/* This object stores all people who are wanted cause of "misbehaviour" in
 * wasteland */

#define WANTED "/open/wanted/wanted"

mixed *wanteds;
static int *money;

no_wizard(mixed entry)
{
  return "/secure/master"->query_user_level(entry) == 0;
}

create()
{
  seteuid(getuid());
  restore_object(WANTED);
  wanteds = filter_mapping(wanteds,#'no_wizard);
  save_object(WANTED);
  money = ({10,20,50,100,200,500,1000,2000,5000,10000});
}

/* Here we register the new evil one. If he was already wanted, we increase the
 * money at the wanted poster */

wanted(mixed evil)
{
  int i;

#if 1
  tell_object(find_player("sique")||this_object(),"Wantedserver:\n\
Previous_object(): " + object_name(previous_object()) +
"\nInteractive: " + (this_interactive()?getuid(this_interactive()):"0") + "\n");
#endif
  if(objectp(evil)) {
    if(!interactive(evil)) return;
    if(getuid(evil) == "guest") return;
    save_wanted(getuid(evil));
  }
  if(stringp(evil) &&
     file_size("/secure/save/" + evil[0..0] + "/" + evil + ".o") >= 0)
    save_wanted(evil);
  if(pointerp(evil))
    for(i = sizeof(evil); i--; )
      wanted(evil[i]);
}

save_wanted(string evil)
{
  mixed already_wanted;

  if(!wanteds) wanteds = ([evil:({time(),money[0]})]);
  else
    if(!(already_wanted = wanteds[evil])) wanteds[evil] = ({time(),money[0]});
    else
      wanteds[evil] = ({time(),get_new_money(already_wanted[1])});
  save_object(WANTED);
}

/* We erase the evil one from the wanted-list */

remove_wanted(string evil)
{
  wanteds = m_delete(wanteds, evil);
  save_object(WANTED);
}

/* If the evil one is already wanted, we return the time of the last
 * and the money of his wanted poster, either 0 */

is_wanted(mixed evil)
{
  if(objectp(evil)) return wanteds[getuid(evil)];
  return wanteds[evil];
}

/* We calculate the new money for the wanted poster */

get_new_money(int old_money)
{
  int i;

  for(i = 0; i < sizeof(money); ++i)
    if(old_money < money[i]) return money[i];
  return money[<1];
}

QueryWanted() { return wanteds; }
