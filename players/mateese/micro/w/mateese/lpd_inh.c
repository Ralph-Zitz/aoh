#define TP this_player()
#define THIS this_object()

object cobj;
mapping vars;

add_actions() {
  add_action("execlpc", "lpc");
  add_action("execlpc", "eval");
}

execlpc(str) {
  string fname, decl, code, text;
  int i;
  mixed rc, val;
  if (!TP)
    return 0;
  if (!str) {
    if (!cobj) {
      write ("There is no previous action to redo.\n");
      return 1;
    }
    rc = cobj->magic();
  } else {
    fname = "/w/mateese/.lpd.c";
    if (file_size(fname) >= 0 && !rm(fname)) {
      write ("Can't remove previous file '"+fname+"'\n");
      return 1;
    }
    if (cobj) { destruct(cobj); cobj = 0; }
    if (cobj = find_object(fname)) { destruct(cobj); cobj = 0; }
    if (sscanf (str, "%s@%s", decl, code))
      code = "magic() {"+code+"}";
    else
      decl = "", code = "magic() { return ("+str+"); }";
    text = "#include \"/w/mateese/lpd.h\"\n" + decl + "\n" + code;
    write_file(fname, text);
    if (catch(call_other(fname, "???"))) {
      write ("Error loading '"+fname+"'.\n");
      return 1;
    }
    if (!(cobj = find_object(fname))) {
      write ("Can't find '"+fname+"' though loaded.\n");
      return 1;
    }
    cobj->set_demon(THIS);
    rc = cobj->magic();
  }
  write(sprintf("Result: %O\n", rc));
  return 1;
}

set_var(name,value) {
  if (vars)
    vars[name] = value;
  else
    vars = ([ name:value ]);
  return value;
}

get_var(name) {
  return vars && vars[name];
}

rm_var(name) {
  if (vars)
    m_delete(vars, name);
}
