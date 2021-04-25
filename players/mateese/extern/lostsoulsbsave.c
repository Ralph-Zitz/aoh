Article 13788 of rec.games.mud.admin:
Path: tubsibr!newsserver.rrzn.uni-hannover.de!news.rwth-aachen.de!news.rhrz.uni-bonn.de!RRZ.Uni-Koeln.DE!news.dfn.de!scsing.switch.ch!swidir.switch.ch!in2p3.fr!oleane!tank.news.pipex.net!pipex!usenet.eel.ufl.edu!gatech!newsfeed.internetmci.com!cdc2.cdc.net!uunet!in2.uu.net!puzzle.palace.net!chaos
From: chaos@crystal.palace.net (Matthew R. Sheahan)
Newsgroups: rec.games.mud.admin,rec.games.mud.lp
Subject: Re: Lost Souls notes
Date: 30 Jan 1996 11:25:18 GMT
Organization: Crystal Palace Networking
Lines: 290
Distribution: inet
Message-ID: <4ekv6u$16g@puzzle.palace.net>
NNTP-Posting-Host: crystal.palace.net
X-Newsreader: TIN [version 1.2 PL2]
Xref: tubsibr rec.games.mud.admin:13788 rec.games.mud.lp:16392

garth@pisces.systems.sa.gov.au wrote:
>   // prepare for changes in our mapping
>   void alert_attributes() {
>     mapping m_tmp;
>     m_tmp = copy(m_attributes);
>     m_attributes=m_tmp; // would direct assignment be safe?

m_attributes = copy(m_attributes) is fine.  we also need to call a special
function to change the mapping pointer in the module the attribute set
belongs to because of our architecture.

> [Note to self: restore_object() may want to call register_attributes().]

definitely.

> So far, so good.

> ... if you have absolute confidence in your trusty wizard team.

> [snip -- people messing with things]

> I've had a short think, and can't figure out a _nice_ way to prevent this.
> Forcing objects to use set_attribute() and get_attribute() functions to
> modify their own state seems a little CPU-expensive, but the expense might
> be trivial compared to the savings in memory consumption (and, therefore,
> page-faulting).  

> What is your approach at Lost Souls?

set_value() and query_value(); full encapsulation.  we don't let anyone
mess with the raw mappings; they're declared private.

> At Vertigo, we're using virtualisation techniques to hold all the room
> information for a particular directory in a single file.  Rooms are cloned
> and configured on the fly when required.  This saves an awful lot of
> needless coding; wizards wander around issuing commands to configure their
> room, and need code only the more complex behaviour of their rooms.  

nod.  we only do this with our map code so far.  we might want to look
into it.

for the general edification, i'm including the /basic/save.c module from
Lost Souls, which contains most of the handling for this.  some of the
things in here are silly, such as the use of the term "dataset", which
generally means modem, and a function dataset_modified() to notify the
module that a mapping is _about_ to be modified, but hey, it's evolved
code, what do you want from us.  the pseudo-type 'internal' is just a
macro for 'private static', and the query_info("Data_Registry") bit is
a kludgy way of letting modules inherited off the main inheritance tree
register their stuff.  comments welcome.

								chiaroscuro

/* Object saving mechanics by Chaos, Wed Aug 31 07:09:44 CDT 1994
 * Internal lfun closure handling added, Chaos Thu Oct 27 14:45:38 MST 1994
 * Centralized data distribution added, Chaos Sat Dec 23 02:51:16 EST 1995
 *
 * Under optimal conditions, saves all information about an object, including
 * all call_out()s active, object pointers for func references, and internal
 * lfun closures.
 *
 * To use, any inheriting object classes which may contain object pointers in
 * mapping data structures should call add_dataset(&mapping, "name") for those
 * structures (making sure the mapping has been initialized).  Optimally,
 * this should be placed in a data_check() function so as to ensure
 * that the references are set up at the proper times.  Example:
 *
 * void data_check() {
 * ::data_check();
 *         add_dataset(local, "local");
 *         add_dataset(secondary, "secondary");
 * }
 *
 * BUGS:
 *   Cannot save object pointers to cloned objects other than this_object()
 *     and this_player()
 *   Cannot save foreign lfun, efun, simul_efun, or lambda closures
 *
 * EXPANSION:
 *   This module, due to its privileged position of tracking all the mapping
 *   data structures items use throughout the lib, has been expanded to
 *   perform distribution of these structures through instances of a cloned
 *   object class, using a daemon for centralization.  What happens is this:
 *
 *   When an object first calls add_dataset() to register its mappings, the
 *   save module starts a call_out to examine them two seconds later, after
 *   it's had plenty of time to finish initializing itself.  If it finds that
 *   the object is a base class rather than a clone, it simply registers the
 *   object's datasets with the tracking daemon to be used as the baseline
 *   for future clones.  If it is a clone, it queries the daemon to see which,
 *   if any, of the object's mappings are functionally identical to the base
 *   for the class.  Those that are are replaced with the baseline mapping,
 *   allowing one data structure to fulfill the needs of an entire clone
 *   class and avoiding much wasted memory.
 *
 *   In order for this to function properly, however, the participating
 *   modules must notify the save module when they are about to modify one
 *   of their mappings, so that the mapping can be copied to a local version;
 *   otherwise, a change to one object in a clone class would change all of
 *   them, since they're all using the same distributed mapping.  This
 *   notification is done by calling dataset_modified("name") before a 
 *   potential change.
 *
 *   The module must also define a function set_name_data(mapping map) which
 *   will reassign the mapping corresponding to "name" to the argument given.
 *   This function must not call dataset_modified("name").  For instance, the
 *   object in the example above would need to define set_local_data(map) and
 *   set_secondary_data(map).
 */

// Directives

#pragma strict_types

// Includes

#include <daemons.h>

// Variables

internal status replacement_checked;
internal mapping datasets;
private mapping call_outs;

// Prototypes

mixed query_info(string key);

// Functions

static void data_check() {
string *funcs;
	if(!datasets)
	  datasets = m_allocate(5, 2);
	if(funcs = query_info("Data_Registry"))
	  filter(funcs, lambda(({ 'x }), ({#'call_other, this_object(), 'x})));
}

static void dataset_modified(string which) {
mapping new;
	if(replacement_checked && datasets[which, 1]) {
	  new = copy(datasets[which, 0]);
	  call_other(this_object(), "set_" + which + "_data", new);
	  datasets[which, 0] = new;
	  datasets[which, 1] = 0;
	}
}

static void check_replacement() {
string file, *ind, *sub;
mapping rep, empty;
int i, j, dr;
	replacement_checked = 1;
	file = file_name();
	if(!is_clone(file)) {
	  if(environment() || file[0..4] == "/std/")
	    return;
	  empty = (mapping) Mappings_Daemon->query_empty_mapping();
	  for(i = 0, j = sizeof(ind = m_indices(datasets)); i < j; i++) {
	    if(!sizeof(datasets[ind[i], 0])) {
	      call_other(this_object(), "set_" + ind[i] + "_data", empty);
	      datasets[ind[i], 0] = empty;
	      dr++;
	    }
	    datasets[ind[i], 1] = 1;
	  }
	  dr && Mappings_Daemon->empty_datasets_replaced(dr);
	  Mappings_Daemon->register_datasets(file, datasets);
	} else {
	  sscanf(file, "%s#%s", file);
	  rep = (mapping) Mappings_Daemon->query_replace_mappings(file, datasets);
	  for(i = 0, j = sizeof(ind = m_indices(rep)); i < j; i++) {
	    call_other(this_object(), "set_" + ind[i] + "_data", rep[ind[i]]);
	    datasets[ind[i], 0] = rep[ind[i]];
	    datasets[ind[i], 1] = 1;
	  }
	}
}

static void add_dataset(mapping new, string key) {
	if(!datasets)
	  datasets = ([ key : new; 0 ]);
	else if(!member(datasets, key))
	  datasets[key] = new;
	if(!replacement_checked && find_call_out("check_replacement") == -1)
	  call_out("check_replacement", is_clone() ? 3 : 1);
}

private mixed entities_to_codes(mixed field, mapping map) {
string code;
	if(map)
	  field = map[field];
	if(pointerp(field))
	  field = map(field, #'entities_to_codes);
	else if(mappingp(field))
	  field = map_indices(field, #'entities_to_codes, field);
	else if(closurep(field) && !get_type_info(field, 1))
	  field = "<SAVE:lfun:" + to_string(field) + ">";
	else if(objectp(field))
	  if(field == this_object())
	    field = "<SAVE:this_object>";
	  else if(field == this_player())
	    field = "<SAVE:this_player>";
	  else if(!is_clone(field))
	    field = "<SAVE:" + file_name(field) + ">";
	return field;
}

private mixed codes_to_entities(mixed field, mapping map) {
string code;
	if(map)
	  field = map[field];
	if(pointerp(field))
	  field = map(field, #'codes_to_entities);
	else if(mappingp(field))
	  field = map_indices(field, #'codes_to_entities, field);
	else if(stringp(field) && sscanf(field, "<SAVE:%s>", field))
	  if(sscanf(field, "lfun:%s", field))
	    field = symbol_function(field, this_object());
	  else if(field == "this_object")
	    field = this_object();
	  else if(field == "this_player")
	    field = this_player();
	  else catch(field = (object) field->load());
	return field;
}

private varargs mixed *processed_array(mixed *list, status out) {
	return map(list, out ? #'codes_to_entities : #'entities_to_codes);
}

private varargs mapping processed_mapping(mapping map, status out) {
	return map_indices(map, out ? #'codes_to_entities : #'entities_to_codes, map);
}

private void assign_mapping(mapping to, mapping from) {
	walk_mapping(from, lambda(({ 'x, 'y, 'z }),
	  ({#'=,
	    ({#'[,
	      'z,
	      'x,
	    }),
	    'y,
	  })), &to);
}

void perform_save(string file) {
mapping *temp, *stor;
mixed *call, *list;
string name;
int i, j;
	stor = m_values(datasets);
	for(i = 0, temp = allocate(j = sizeof(stor)); i < j; i++) {
	  temp[i] = copy(stor[i]);
	  assign_mapping(stor[i], processed_mapping(stor[i]));
	}
	call_outs = ([]);
	call = call_out_info();
	for(i = 0, j = sizeof(call); i < j; i++)
	  if(call[i][0] == this_object()) {
	    name = call[i][1];
	    list = ({ ({ call[i][2], processed_array(call[i][3..]) }) });
	    if(!call_outs[name])
	      call_outs[name] = list;
	    else call_outs[name] += list;
	  }
	save_object(file);
	for(i = 0, j = sizeof(stor); i < j; i++)
	  assign_mapping(stor[i], temp[i]);
	call_outs = 0;
}

void perform_restore(string file) {
int i, j, k, l;
mapping *stor;
string *names;
mixed *data;
	restore_object(file);
	datasets = 0;
	data_check();
	stor = m_values(datasets);
	for(i = 0, j = sizeof(stor); i < j; i++)
	  assign_mapping(stor[i], processed_mapping(stor[i], 1));
	if(call_outs)
	  for(i = 0, j = sizeof(names = m_indices(call_outs)); i < j; i++) {
	    for(k = 0, l = sizeof(data = call_outs[names[i]]); k < l; k++)
	      apply(#'call_out, names[i], data[k][0], processed_array(data[k][1], 1));
	  }
	call_outs = 0;
}


