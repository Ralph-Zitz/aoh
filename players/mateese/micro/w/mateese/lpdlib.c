#define TP this_player()

static object demon;

set_demon(obj) { demon = obj; }

static object find_item (string item, object prev) {
  object ob;
  string str, foo, bar;
  int i, nr, mode;
  if (item == "env" && !prev) return 0;
  if (item == "env" ) return environment(prev);
  if (item == "here") return environment(TP);
  if (item == "me"  ) return TP;
  if (sscanf (item, "*%s", str) == 1) {
    if (ob = find_player(lower_case(str))) return ob;
    return 0;
  }
  if (sscanf (item, "@%s", str) == 1) {
    if (ob = find_living(str)) return ob;
    return 0;
  }
  if (sscanf (item, "$%s", str) == 1) {
    if (ob = find_object((string)TP->get_path(str))) return ob;
    return 0;
  }
  if (ob = find_player(lower_case(item))) return ob;
  if (ob = find_living(item)) return ob;
  if (sscanf (item, "%s %s", foo, bar) != 2 &&
      (ob = find_object((string)TP->get_path(item, TP))) ) return ob;
  if (!prev) return 0;
  mode = 0;
  if (sscanf (item, "\"%s", str) == 1) { mode = 1; str = "%s"+str+"%s"; }
  else if (sscanf (item, "!%s", str) == 1) { mode = 2; str = "%s"+str+"%s"; }
  else if (sscanf(item, "%d", nr)) mode = 3;
  if (mode) {
    ob = first_inventory(prev);
    i = 1;
    while (ob) {
      if (mode == 1) {
        if (sscanf (" "+ob->Short()+" ", str, foo, bar) == 2) return ob;
      }
      if (mode == 2) {
        if (sscanf (" "+file_name(ob)+" ", str, foo, bar) == 2) return ob;
      }
      if (mode == 3 && i == nr) return ob;
      ob = next_inventory(ob);
      i++;
    }
    return 0;
  }
  if (ob = present(item, prev)) return ob;
  return 0;
}


varargs object find (string items, object prev) {
  string *item;
  int i;
  object mark;

  items = ""+items;
  if (!prev) mark = environment(TP); else mark = prev;
  if (items == "" || !mark) return 0;
  item = explode(items,":");
  for (i = 0; i < sizeof(item); i++) {
    if (item[i] == "") continue;
    if (!(mark=find_item(item[i], mark))) return 0;
  }
  return mark;
}

/*************************************************************************/
