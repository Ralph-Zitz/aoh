/*
 * rewritten from Orb's Excellent Marker for 3.0 --- Organimedes
 *
 * 1.0: - rewritten for 3.0
 *      - long() improved
 *      - help improved, now without additional file (because I don't know
 *        the directory structure of the future exactly)
 *      - silent status saved
 *      - silent status only for AWs
 *      - i_call() is able to give two arguments
 * 1.1: - i_call() can now print out arrays
 *      - i_call argument handling changed
 *      - properties are now queried by ourselves
 *      - i_clone() removed [I didn't see the sense of it!]
 * 1.2: - printf() changed (a little :-)
 *      - i_move() calls now move_object() if there is no obj->move()
 *      - i_dump() calls now lfuns if it doesn't find properties
 *      - marker can now be given, dropped or put into a chest
 * 1.3: - Mateese: Finishing touches; better suitable for native mode
 *        though some changes still will come (e.g. uid handling)
 *      - Mateese: i_clone() re-added (with help so use stands out better)
 *      - more() is now handled by filelib, since the Nightfile will
 *        go away some day. The place of the filelib might change.
 * 1.4: - Some of Mateese's comments removed (funny, ain't it?)
 *	- wmore() removed (wasn't necessary any more)
 *	- i_show() improved
 *	- i_call() improved
 *	- thing::SetLong now works with process_string()
 * 1.5:	- autoloading!
 *	- i_mark() now parses its commands quite intelligent (see 'i_help
 *	  mark')
 *	- i_mark() can now handle abbreviated pathnames
 *	- i_call() now also parses its arguments (see 'i_help call')
 *	- i_destruct() now first calls remove()
 *	- i_buf() without arguments now just calls i_show()
 *	- Remark: i_clone() probably won't work :-(
 * 1.6: - new autoload mechanism
 *	- filelib stuff removed
 *	- /lib/string introduced
 *	- loads all properties from /sys/properties.h
 * 1.7: - debug stuff added
 *	- some bugs in i_call removed (hopefully)
 *	- i_dump now also shows properties not defined in /sys/properties.h
 *	- i_clone now tries to move object as well
 *	- mark() slightly changed
 *	- adjusted to /sys/wizlevels.h
 * 1.8: - mark() slightly changed
 *      - i_call adapted to "new" (to be true rather old) explode() behaviour
 *      - i_trace fixed
 *      - i_inherit added
 * 1.9  - PROPERTY_HEADER instead of hardcoded filename (Pepel, 10-01-95)
 */
#include <moving.h>
#include <secure/wizlevels.h>
#include <properties.h>
#include <msgclass.h>
#ifndef PROPERTY_HEADER
#define PROPERTY_HEADER "/sys/properties.h"
#endif

inherit "/std/thing";
inherit "/lib/string";

#define MAX 5
#define LOGFILE "organimedes.bugs"
#define VERSION "1.8 (07.08.94)"
#define USER    this_player()
#define ME      this_object()

object *mark;
mixed  *prop_list;	/* alist of all properties in PROPERTY_HEADER */
int    level, silent;
status traceflag;

/* declared functions */
varargs mixed *_get_props(string blueprintname);
mixed         _show_slot(int nr);

void create() {
    string bpname, foo;

    thing::create();
    AddId("marker");
    AddAdjective("excellent");
    AddAdjective("improved");
    Set(P_WEIGHT,0);        /* autoloading items must not have any weight */
    Set(P_VALUE,10);
    SetNoGive(1);
    SetNoDrop(1);
    Set("IsTool", 1 ); /* to suppress the outdated raise_errors */
    SetShort("Orb's Excellent Marker Object");
    /* P_LONG defined as internal property */
    mark = allocate(MAX);
    silent    = 0;
    level     = 0;
    traceflag = 0;
    prop_list = ({ ({}), ({}) });
    if (sscanf(object_name(ME),"%s#%s",bpname,foo) != 2)
	prop_list = _get_props(); /* get properties from PROPERTY_HEADER */	
    else
	prop_list = _get_props(bpname);	/* get properties from blueprint */
}

/**** Internal functions ***/

string QueryLong() {
    string s;
    int    i;

    s = "This is an improved version of Orb's excellent marker object.\n";
    if (!IS_IMMORTAL(USER))
        s = s + "Only wizards can use this item.\n";
    else {
        s = s + "Actual version: "+VERSION+" by Organimedes.\n" +
		"For help type i_help.\n" +
		"Current settings: ";
        if (silent) s = s + "silent.\n";
        else        s = s + "not silent.\n";
        s = s + "Buffer content:\n";
        for (i = 0; i < MAX; i++)
            s = s + (string)_show_slot(i);
    }
    return s;
}

/*
 * return the read prop_list
 */
mixed *query_proplist() { return prop_list; }

/*
 * When the blueprint is created (flag == 0):
 * Get all properties from PROPERTY_HEADER and put them into an array
 * which is returned.
 * When the object is cloned from an existing blueprint (flag == 1):
 * Get all properties from blueprint object.
 */
varargs mixed *_get_props(string blueprintname) {
    int    i, n;
    string prop_string;
    string *prop_array;
    string alias, property, dummy;
    mixed  *the_list;		/* alist for making the property array */
    object blueprint;

    the_list = ({ ({}), ({}) });

    if (blueprintname) {
	blueprint = find_object(blueprintname);
	if (blueprint)
	    return (mixed *)blueprint->query_proplist();
    /* if the cloned object does NOT find the blueprint
     * (e.g. if it is the blueprint itself :-), do it yourself
     */
    }

    prop_string = read_file(PROPERTY_HEADER);
    prop_array  = explode("dummy\n" + prop_string,"\n");  
    n = sizeof(prop_array);

    /* don't check for comments :-) */
    for (i = n-1; i >= 0; i--) {
	/* by using the dummy we don't need to use strip_blank()
	 * which saves a lot of time (hopefully)
	 * "dummy" should contain all the unnecessary blanks afterwards
	 */
      if (sscanf(prop_array[i],"#define %s %s\"%s\"",alias,dummy,property) == 3) {
        if ( alias != "P_PROPERTIES" )
	    the_list = insert_alist(property,alias,the_list);
      }
    }
    the_list = order_alist(the_list);	/* is this necessary? */
    return the_list;
}

/*
 * This output routine can also handle arrays. It uses lots of functions
 * from Mateese's /lib/string and therefore produces a nice, formatted
 * output.
 * The function needs a lot of execution time. It might happen that the
 * gamedriver terminates its execution before it is finished.
 */
string _show_props(object ob) {
    int    i, n;
    string s;
    mixed  retval;
    string *buf, retarray;
    string *all_props;	/* contains all properties */
    mapping int_props;  /* contains internal properties */

    /* get all properties not defined via QueryFunc/SetFunc */
    int_props = (mixed)ob->QueryProperties();
    if (mappingp(int_props))
	all_props = m_indices(int_props);
    else
	all_props = ({ });
    n = sizeof(prop_list[0]);
    for (i = 0; i < n; i++)
	if (ob->ExistsProp(prop_list[0][i]) && (member(all_props, prop_list[0][i]) == -1))
	    all_props = all_props + ({ prop_list[0][i] });

    /* now all_props contains all properties defined in the object */
    all_props = sort_array( all_props, #'> /*'*/ );
    n = sizeof(all_props);
    retarray = "";		/* empty array */
    for (i = 0; i < n; i++) {
      retarray += sprintf( "%-30'.'s %s\n", 
                           (assoc(all_props[i],prop_list)||
                            "\""+all_props[i]+"\"")+" ",
                           mixed_to_string(ob->Query(all_props[i])) );
    }
    return retarray;
}

/* show a slot of the marker */
mixed _show_slot(int slot) {
    string s;

    if ((slot < 0) || (slot > MAX-1))
        return 0;
    else {
        s = slot+": ";
        if (mark[slot])
            s += object_name(mark[slot]);
        else
            s += "empty";
        if (slot == level)
            s += "\t(current)";
        s += "\n";
        return s;
    }
}

/**** Autoloading functions *****/

/*
 * The autoloading stuff is realized as built-in property as shown by
 * Mateese in XAUTOLOAD2.C. The value of the autoload property cannot
 * be influenced directly from outside (that is, there is no function
 * SetAutoObject() corresponding to Set(P_AUTOOBJECT,whatever)
 */

/*
 * Does some checks if object is allowed to be autoloading, and if,
 * returns argument for autoload (similar to the old query_auto_load()).
 * 
 */
QueryAutoObject() {
    /* Check if we are allowed to be autoloading */
    if (USER != environment(ME) || !query_user_level(USER)) return 0;
    return(VERSION+":"+silent+":");
}

/*
 * Just to give some messages on startup
 */
SetAutoObject(string str) {
    string version;

    if (sscanf(str,"%s:%d:", version, silent) == 2) {
	write("Orbs Marker... ");
	if (silent) write("silent ");
	if (version != VERSION) write("UPDATED!");
	write("\n");
	Set(P_WEIGHT,0);	/* autoloading items must not have any weight */
				/* Is this still true? */
    }
    else {
	write("Wrong argument format to SetAutoObject()! Autoload of marker failed.\n");
	silent = 0;
    }
}

/***** External functions *******/

void init() {
    ::init();
    if (!IS_IMMORTAL(USER)) return;

    add_action("i_clone",   "i_clone",   0);
    add_action("i_more",    "i_more",    0);
    add_action("i_dump",    "i_dump",    0);
    add_action("i_mark",    "i_mark",    0);
    add_action("i_destruct","i_destruct",0);
    add_action("i_call",    "i_call",    0);
    add_action("i_buf",     "i_buf",     0);
    add_action("i_show",    "i_show",    0);
    add_action("i_help",    "i_help",    0);
    add_action("i_move",    "i_move",    0);
    add_action("i_list",    "i_list",    0);
    add_action("i_silent",  "i_silent",  0);
    add_action("i_debug",   "i_debug",	 0);
    add_action("i_trace",   "i_trace",	 0);
    add_action("i_inherit", "i_inherit", 0);
}
    
/********* Marker functions ***************/

/*
 * mark object stuff. This function does all of the marking stuff.
 */
object mark(string str, object tmark) {
    int i;
    object ob,tmp;

    if (!str) return tmark;

    if (sscanf(str,"#%d",i)) {
	if (i > MAX) return tmark;
	return mark[i];
    }

    if (str == "up") {
        if (!tmark) return tmark;
        if (!environment(tmark)) return tmark;
        return environment(tmark);
    }

    if (str == "here") return environment(USER);

    if (str == "me") return USER;

    if (sscanf(str, "%d", i) == 1) {
        if (!tmark) return tmark;
        ob = first_inventory(tmark);
        while(ob) {
            if (--i == 0)
                return ob;
            ob = next_inventory(ob);
        }
        return tmark;
    }

    if (tmark) tmp = present(str,tmark);
    if (!tmp)  tmp = present(str,USER);
    if (!tmp)  tmp = present(str,environment(USER));
    if (!tmp)  tmp = find_living(str);
    if (!tmp)  tmp = find_object((string)USER->get_path(str));
    if (!tmp) return tmark;

    return tmp;
}

/*
 * i_mark to work ourselves with mark()
 */
int i_mark(string str) {
    string *list;
    string *arg;
    int    i, n, n2, arg_i, found;

    if ((!str) || (str == "?")) {
        write("I_MARK [str] to mark object in current buffer.\n");
        return 1;
    }

    /* parse argument list */
    list = explode(str," ");
    n    = sizeof(list);
    arg  = allocate(n);
    arg_i = 0;
    for (i = 0; i < n; i++) {
	if (found) {
	    if (list[i][<1] == '"') {
	 	arg[arg_i] = arg[arg_i] + " " + list[i][0..<2];
	 	found = 0;
	 	arg_i++;
            }
	    else
		arg[arg_i] = arg[arg_i] + " " + list[i];
	}
	else if (list[i][0] == '\"') {
	    found = 1;
	    arg[arg_i] = list[i][1..];
	}
	else {
	    arg[arg_i] = list[i];
	    arg_i++;
	}
    }

    /* mark parsed list */
    for (i=0; i<arg_i; i++)
        mark[level] = mark(arg[i],mark[level]);
    write(_show_slot(level));

    return 1;
}

/*
 * don't panic, this function will help you
 */
int i_help(string str) {
    if (!str) {
        write("Commands available:\n"+
        "  i_buf                     - show current buffer number\n"+
        "  i_buf [num]               - switch to buffer num\n"+
        "  i_show                    - show all buffers\n"+
        "  i_mark [object]           - marks specified objects\n"+
        "  i_list [object]           - show inventory of object\n"+
        "  i_dump [object]           - show properties of object\n"+
        "  i_inherit [object]        - show inheritance tree of object\n" +
        "  i_clone [object]          - clone yourself an instance of object\n"+
        "  i_destruct [object]       - destruct object in buffer\n"+
        "  i_debug [object]          - show some gamedriver information\n"+
        "  i_trace [object]          - trace calls in/of object\n"+
        "  i_move [ob1] [ob2]        - move ob1 to destination ob2\n"+
        "  i_call [function] [arg]   - call function with args\n"+
        "  i_more [object]           - see code for object\n"+
        "  i_silent                  - toggle some messages given by the marker\n"+
        "  i_help                    - this help\n"+
        "  i_help mark               - how to mark an object\n"+
        "  i_help call               - some more help about i_call\n"+
        "If you are still unsecure how to use a command, use '?' as argument.\n");
        return 1;
    }
    else if (str == "mark") {
        write("[object] can be of type #<num>, <num> or <name>. This works as follows:\n"+
          "  #<num>  -- buffer number\n"+
          "  <num>   -- object number [num] in inventory of last marked object\n"+
          "  <name>  -- searches for an object in the following sequence:\n"+
          "               find_living()\n"+
          "               find_object()\n"+
          "               in inventory of currently marked object\n"+
          "               in your inventory\n"+
          "               in your environment\n"+
          "             If you want to identify an object by adjective and id,\n"+
          "             quote it, e.g. 'i_mark \"green dragon\"' would only mark\n"+
          "             an object that identifies as 'green dragon'.\n"+
          "             Special strings for marking are:\n"+
          "             'up'  : marks environment of currently marked object\n"+
          "             'here': marks your environment\n"+
          "             'me'  : marks you\n"+
          "Marks can be stacked. The command 'i_mark me 1' would first mark you and\n"+
          "then mark the first item in your inventory. 'i_mark /room/xxx barrel 3'\n"+
          "would mark /room/xxx , using it as a base to mark barrel, which uses that\n"+
          "as a base to mark the item 3.\n");
        return 1;
    }
    else if (str == "call") {
	write("Since there is checking of types when using i_call, it is possible\n"+
	  "to determine which type of variable should be given to the called function:\n"+
	  "     \"var\"    --- var will be given as string.\n"+
	  "     var      --- var will be given as integer (if var cannot be changed\n"+
	  "                  into an integer, it will be given as string).\n"+
	  "     @var     --- var will be given as object.\n"+
	  "It is currently not possible to give array variables.\n"+
	  "The maximum of variables you can give is five.\n");
	  return 1;
    }
    return 0;
}

/*
 * have a look at the code of an object (very convenient)
 */
int i_more(string str) {
    string *list;

    if (str == "?") {
        write("I_MORE [object] to see code of object,\n");
        write("I_MORE          to see code of last object marked.\n");
        return 1;
    }
    if (str)
        i_mark(str);
    list = explode(object_name(mark[level]),"#");
    return (int)USER->more("/"+list[0]+".c");
}

/*
 * dumps all properties of an object
 */
int i_dump(string str) {
    string buf;
    object what;

    if (str == "?") {
        write("I_DUMP [obj] to see all properties of object,\n");
        write("I_DUMP       to see all properties of last object marked.\n");
        return 1;
    }

    if (str) i_mark(str);

    what = mark[level];

    if (!what) {
        write("i_dump: nothing marked.\n");
        return 1;
    }

#ifndef COMPAT_FLAG
    write("UId : "+getuid(what)+"\n");
    write("eUId: "+geteuid(what)+"\n");
#endif

    /* show all properties in a nice, formatted output */
    if (what->ExistsProp(P_IDS)) {
	buf = _show_props(what);
	msg_write( CMSG_GENERIC|MMSG_MORE, buf );
    }
    else {
	log_file(LOGFILE,ctime(time())+": "+object_name(what)+" has no properties!\n");
	write("This object has no properties! Showing code instead...\n");
	i_more(str);
    }
    return 1;
}

/*
 * show current buffer usage
 */
int i_show(string str) {
    int i;

    if (str == "?") {
        write("I_SHOW to show current buffer usage.\n");
        return 1;
    }
    else if (str && str != "") return 0;

    for (i=0; i < MAX; i++)
        write(_show_slot(i));
    return 1;
}

/*
 * set actual buffer
 */
int i_buf(string str) {
    int new_level;

    if (str == "?") {
        write("I_BUF [nr] to set a new buffer between 0 and "+MAX+",\n");
        write("I_BUF      to query actual buffer.\n");
        return 1;
    }

    if (!str)
	return i_show("");

    new_level = 0;
    sscanf(str, "%d", new_level);

    if ((new_level >= 0) && (new_level < MAX)) {
        level = new_level;
        write(_show_slot(level));
    }
    else
        write("i_buf: illegal argument.\n");
    return 1;
}

/*
 * clone (marked) object (by Mateese)
 */
int i_clone(string str) {
    object ob;
    string *foo;

    if (str == "?") {
        write("I_CLONE [str] to clone object [str],\n");
        write("I_CLONE       to clone last object marked.\n");
        write("Using this command, you can clone yourself an instance of an interesting\n");
        write("object for further examination. The new object is marked and moved into\n");
	write("your inventory, or, if that fails, in your environment.\n");
        return 1;
    }

    if (str) i_mark(str);

    if (!mark[level]) {
        write("i_clone: nothing marked.\n");
        return 1;
    }

    foo = explode(object_name(mark[level])+"#","#");
    ob = clone_object (foo[0]);
    if (ob) {
        mark[level] = ob;
        write("Ok. New object: "+object_name(ob)+"\n");
        if ((int)ob->move(USER) != ME_OK) {
	    if ((int)ob->move(environment(USER)) != ME_OK)
		write("i_clone: couldn't move object.\n");
	}
    }
    else
        write("i_clone: cloning failed.\n");
    return 1;
}

/*
 * destruct marked object
 */
int i_destruct(string str) {
    object ob;
    string s;

    if (str == "?") {
        write("I_DESTRUCT [str] to destruct object [str],\n");
        write("I_DESTRUCT       to destruct last object marked.\n");
        write("                 The environment of the destructed object is marked.\n");
        return 1;
    }

    if (str) i_mark(str);

    if (!mark[level]) {
        write("i_destruct: nothing marked.\n");
        return 1;
    }

    ob = environment(mark[level]);
    s  = object_name(mark[level]);
    mark[level]->remove();
    if (find_object(s)) {
	write("i_destruct: object refused to be remove()d ... destructing.\n");
	destruct(mark[level]);
    }
    if (ob)
        write("Ok. New: "+object_name(ob)+"\n");
    else
        write("Ok. New: empty\n");
    mark[level] = ob;
    return 1;
}

/*
 * call lfun in marked object
 */
int i_call(string str) {
    mixed  retval;
    mixed  *arg;
    string func, args;
    int    i, n, n2, ibuf, arg_i;
    status is_string;
    object obuf;

    if ((!str) || (str == "?")) {
        write("I_CALL [func] [args] to call func with args in last object marked.\n");
        write("                     You can call a function with a maximum of 5 args.\n");
        write("                     To specify the type of argument, see 'i_help call'.\n");
        return 1;
    }

    if (!mark[level]) {
        write("i_call: nothing marked.\n");
        return 1;
    }

    if (sscanf(str, "%s %s", func, args) != 2)
        func = str;

    if (!args) {
	write("i_call: \n");
        write(mixed_to_string(call_other(mark[level],func))+"\n");
	if (!silent)
	    say(USER->QueryName() +
	    " patched the internals of " + mark[level]->QueryShort() + ".\n");
	return 1;
    }

    /* We have args... we need to parse them */
    arg = explode(args," ");
    n   = sizeof(arg);
    arg_i     = 0;
    is_string = 0;

    /* parse command line
     * probably not the most elegant way to do it :-) */
    for (i = 0; i < n; i++) {
	/* We are NOT in the middle of a string */
	if (!is_string) {
	    if (arg[i][0..0] == "\"") {
		n2 = strlen(arg[i]) - 1;
		if (arg[i][n2..n2] == "\"") {
		    /* quoted string */
		    if (n2 > 1)
			arg[arg_i] = arg[i][1..n2-1];
		    else
			arg[arg_i] = "";
		    arg_i++;
		}
		else {
		    /* begin of quoted string */
		    arg[arg_i] = arg[i][1..n2];
		    is_string = 1;
		}
	    }
	    else if (arg[i][0] == '@') {
		/* object */
		obuf = find_object(arg[i][1..]);
		if (obuf) arg[arg_i] = obuf;
		arg_i++;
	    }
	    else if (sscanf(arg[i],"%d",ibuf) == 1) {
		/* integer */
		arg[arg_i] = ibuf;
		arg_i++;
	    }
	    else {
		/* unquoted string */
		arg[arg_i] = (string)arg[i];
		arg_i++;
	    }
	} /* if (!is_string) */

	/* We ARE in the middle of a quoted string */
	else {
	    if (arg[i][<1] == '"') {
		if (arg[i][<2] == '\\')
		    /* \" does not mark end of string */
		    arg[arg_i] = arg[arg_i] + " " + arg[i];
		else {
		    /* end of string */
	 	    arg[arg_i] = arg[arg_i] + " " + arg[i][0..<2];
	 	    is_string = 0;
	 	    arg_i++;
	 	}
	    }
	    /* middle of string */
	    else
		arg[arg_i] = arg[arg_i] + " " + arg[i];
	} /* else */
    } /* for */
		        

    if (arg_i == 1) {
	write("i_call: "+mixed_to_string(arg[0])+"\n");
	write(mixed_to_string(call_other(mark[level],func,arg[0]))+"\n");
    }
    else if (arg_i == 2) {
	write("i_call: "+mixed_to_string(arg[0])+" "
			+mixed_to_string(arg[1])+"\n");
	write(mixed_to_string(call_other(mark[level],func,arg[0],arg[1]))+"\n");
    }
    else if (arg_i == 3) {
	write("i_call: "+mixed_to_string(arg[0])+" "
			+mixed_to_string(arg[1])+" "
			+mixed_to_string(arg[2])+"\n");
	write(mixed_to_string(call_other(mark[level],func,arg[0],arg[1],arg[2]))+"\n");
    }
    else if (arg_i == 4) {
	write("i_call: "+mixed_to_string(arg[0])+" "
			+mixed_to_string(arg[1])+" "
			+mixed_to_string(arg[2])+" "
			+mixed_to_string(arg[3])+"\n");
	write(mixed_to_string(call_other(mark[level],func,arg[0],arg[1],arg[2],arg[3]))+"\n");
    }
    else if (arg_i == 5) {
	write("i_call: "+mixed_to_string(arg[0])+" "
			+mixed_to_string(arg[1])+" "
			+mixed_to_string(arg[2])+" "
			+mixed_to_string(arg[3])+" "
			+mixed_to_string(arg[4])+"\n");
	write(mixed_to_string(call_other(mark[level],func,arg[0],arg[1],arg[2],arg[3],arg[4]))+"\n");
    }
    else write("i_call: too many parameters... aborted.\n");

    if (!silent)
        say(USER->QueryName() +
        " patched the internals of " + mark[level]->QueryShort() + ".\n");
    return 1;
}

/*
 * move object around
 */
int i_move(string str) {
    object item, dest;
    string x,y;
    int    retval;

    if ((!str) || (str == "?")) {
        write("I_MOVE [str1] [str2] to move str1 into inventory of str2,\n");
        write("I_MOVE [str]         to move last marked object into inventory of str.\n");
        return 1;
    }

    if (sscanf(str,"%s %s",x,y) == 2) {
        item = mark(x,mark[level]);
        dest = mark(y,ME);
    }
    else {
        item = mark[level];
        dest = mark(str,mark[level]);
    }

    if (!item) {
	write("i_move: nothing marked.\n");
	return 1;
    }
    if (!dest) {
	write("i_move: could not mark destination.\n");
	return 1;
    }

    if (!function_exists("move", item))
        write("i_move: failed: object does not possess lfun move().\n");
    else {
   retval = (int)item->move(dest,M_NOCHECK);
	if (present(item,dest)) {
	    if (retval == 1)
		write("i_move: object moved.\n");
	    else
		write("i_move: object moved, got: "+retval+"\n");
	}
	else {
	    if (retval == 0)
		write("i_move: failed.\n");
	    else
		write("i_move: failed; got: "+retval+"\n");
	}
    }
    return 1;
}

/*
 * List inventory of an object
 */
void inventory(object object_list) {
    object ob;
    string s;
    int i;

    ob = first_inventory(object_list);
    while(ob) {
        i++;
        write(i + ": \t");
        write(ob); write("\n");
        if (!silent) {
            s = (string)ob->Query(P_SHORT);
            if (!s)				/* for items which don't  */
                s = (string)ob->short();	/* define P_SHORT */
            write("\t" + s + "\n");
        }
        ob = next_inventory(ob);
    }
}

int i_list(string str) {
    object what;

   if (str == "?") {
        write("I_LIST [object] to get inventory of object,\n");
        write("I_LIST          to get inventory of last marked object.\n");
        return 1;
    }

    if (str) i_mark(str);
    if (!mark[level])
        write("i_list: nothing marked.\n");
    else
        inventory(mark[level]);
    return 1;
}

/*
 * toggle silent mode
 */
int i_silent(string str) {
    if (str == "?") {
        write("I_SILENT to toggle silent mode.\n");
        return 1;
    }
    else if (str) return 0;

    silent = !silent;
    write("i_silent: ");
    if (!silent)
        write("not ");
    write("silent.\n");
    return 1;
}

/*
 * debug function
 */
int i_debug(string str) {
    if (str == "?")
	write("I_DEBUG [object] to show gamedriver debug information about object\n");
    else  {
        if (str)
            i_mark(str);
        else
	    write(_show_slot(level));
	if (!mark[level])
	    write("i_debug: nothing marked.\n");
	else {
	    debug_info(0,mark[level]);
	    debug_info(1,mark[level]);
	}
    }
    return 1;
}

/*
 * trace function
 */
int i_trace(string str) {
    string flagname, obname, prefix;
    int    flag;

    if (str == "?") {
	write("I_TRACE [flag] [object] to trace object.\n");
	write("I_TRACE                 to stop tracing.\n");
	write("Flags allowed are:\n");
	write("    lfun   --- trace all function calls to lfuns.\n");
	write("    call   --- trace all calls to call_other.\n");
	write("    return --- trace all function returns.\n");
	write("    func   --- print arguments at function calls and return values.\n");
	write("Only one flag is allowed at a time.\n");
	return 1;
    }

    if (!str) {
	trace(0);
	write("i_trace: terminated.\n");
	traceflag = 0;
	return 1;
    }

    if (traceflag) {
	write("i_trace: change traced object.\n");
	trace(0);
	traceflag = 0;
    }

    if (sscanf(str,"%s %s",flagname,obname) != 2) {
	write("i_trace: wrong number of parameters.\n");
	return 1;
    }

    i_mark(obname);
    if (!mark[level]) {
	write("i_trace: Nothing marked.\n");
	return 1;
    }

    if	    (flagname == "lfun")	flag = 1;
    else if (flagname == "call")	flag = 2;
    else if (flagname == "return")	flag = 4;
    else if (flagname == "func")	flag = 8;
    else {
	write("i_trace: unknown flag.\n");
	return 1;
    }

    prefix = object_name(mark[level]);
    prefix = prefix[1..strlen(prefix)-1];	/* cut off leading "/" */
    traceprefix(prefix);
    trace(flag);
    traceflag = 1;
    write("i_trace: Ok.\n");
    return 1;
}

/*
 * i_inherit: show inheritance tree
 */
int i_inherit(string str) {
    string *s;

    if (str == "?") {
        write("I_INHERIT [obj] shows the names of all files inherited by object\n"+
              "                [obj]. If [obj] is omitted, the last object marked\n"+
              "                is used instead.\n");
        return 1;
    }

    if (str)
        i_mark(str);
    else
        write(_show_slot(level));

    s = inherit_list(mark[level]);
    msg_write( CMSG_GENERIC|MMSG_MORE, s );
    return 1;
}
