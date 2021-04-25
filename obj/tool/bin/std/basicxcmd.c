/*
** The basic xtool command
*/

#include "../bincmd.h"
#include <moving.h>

#define LPC_HEADER "#pragma verbose_errors\n#include <config.h>\n#include <properties.h>\n#include <moving.h>\n#include <rooms.h>\n#include <living.h>\n#include <attributes.h>\nget(str) {\n  return previous_object()->XFindObj(str);\n}\ngetfile(str) {\n  return previous_object()->XFindFile(str);\n}\n"

#ifndef MASTER
#define MASTER __MASTER_OBJECT__
#endif

inherit BASICCMD;

/*
** Provide Author and version for xtool
*/
string QueryVersion() {return "V1.3 OSB";}
string QueryAuthor() {return "Hyp 1992, Mica 1998";}
string QueryShort() {return "Xtool compatibility function.";}

/*
** Xtool-Commands always require the EUID
*/

public int QueryEUIDRequired() { return 1; }

/*
** some global vars
*/

int     morelines=20;
int     moreflag=0;
int     moreoffset=1;
int     term;
string  morefile;
int     moreautodes = 0;
mapping xvariables;

/*
** some helpfuns based on efuns
*/

string *old_explode(string str, string del) {
  int s, t;
  string *strs;

  if(del == "")
    return ({str});
  strs=efun::explode(str, del);
  t=sizeof(strs)-1;
  while(s<=t && strs[s++] == "");s--;
  while(t>=0 && strs[t--] == "");t++;
  if(s<=t)
    return strs[s..t];
  return 0;
}

string *long_get_dir(string pat, int all) {
  int i, s;
  string str, dir, *file, *tmp;

  if(!pat) return ({});
  pat=MASTER->make_path_absolute( pat );
  if(tmp=explode(pat, "/")) dir="/"+implode(tmp[0..sizeof(tmp)-2], "/")+"/";
  else dir="/";
  s=sizeof(tmp=get_dir(pat));
  file=({});
  for(i=0;i<s;i++) {
    str=dir+tmp[i];
    if(all||file_size(str)>=0) file+=({str});
  }
  return file;
}

/*
** other xtool functions
*/

object VarToObj(string str) {
  if(!(str&&str[0]=='$')) return 0;
  switch(str) {
  case "$m":
  case "$me":
    return cloner;
  case "$h":
  case "$here":
    return environment( cloner );
  default:
    if ( mappingp( xvariables ) )
      return xvariables[str[1..]];
  }
}

private varargs object _o_env( string str, object obj, int e, int silent ) {
  while( e-- ) {
    if( ! ( obj = environment( obj ) ) ) {
      if ( ! silent ) {
	tell_object( cloner, "Specified object has no environment [" );
	while( e-- ) tell_object( cloner, "^" );
	tell_object( cloner, str+"].\n");
      }
      return 0;
    }
  }
  return obj;
}

varargs object FindObj( string str, object env, int nopath, int silent ) {
  object obj, *inv;
  string tmp;
  int e;
  mixed num;

  if(!(str&&env)) return 0;

  /* replace backslashed . and count envs */
  str="/lib/string"->string_replace(str, "", ".");
  while(str[e++]=='^');
  str=str[(--e)..];

  /* replace backslashed ^ as we handled environments above */
  str="/lib/string"->string_replace(str, "", "^");

  if ( obj = VarToObj( str) )
    return _o_env( str, obj, e, silent ); /* str is variable */

  /* replace backslashed $ as we handled variables above */
  str="/lib/string"->string_replace(str, "", "$");

  if ( ( intp( num = atoi( str ) ) ) &&
       ( inv = all_inventory( env ) ) ) {
    if( ( num > 0 ) &&
	( num <= sizeof( inv ) ) ) {
      return _o_env( str, inv[ num - 1 ], e, silent ); /* str is object num in inv. */
    } else {
      if ( ! silent )
	tell_object( cloner,
		     "Specified object number out of range [1-"+sizeof(inv)+"].\n");
      return 0;
    }
  }

  if ( obj = present( str, env ) )
    return _o_env( str, obj, e, silent );

  if ( obj = find_player( lower_case( str ) ) )
    return _o_env( str, obj, e, silent );

  if ( obj = find_living( str ) )
    return _o_env( str, obj, e, silent );

  if ( ! nopath ) {
    if ( obj = find_object( (string) MASTER->make_path_absolute( str ) ) )
      return _o_env( str, obj, e, silent ); /* str is a filename */

    tmp = cloner->exp_fname( str ); /* expand filealiases */
    if ( obj = find_object( (string) MASTER->make_path_absolute( tmp ) ) )
      return _o_env( tmp, obj, e, silent ); /* str is a filename with falias */
  }

  if ( ! silent )
    tell_object( cloner, "Specified object does not exist ["+str+"].\n");
  return 0;
}

varargs object XFindObj( string str, int silent ) {
  object obj, env;
  string *strs;
  int i, s;

  if(!str) return 0;
  env = environment( cloner );

  /* replace all backslashed specials by control characters */
  str="/lib/string"->string_replace(str, "\\.", "");
  str="/lib/string"->string_replace(str, "\\^", "");
  str="/lib/string"->string_replace(str, "\\$", "");
  str="/lib/string"->string_replace(str, "\\\\", "\\");

  s = sizeof(strs=explode(str, ".")-({""}));
  while(s--) {
    /* FindObj(..,i) hacks around the 'blueprint seems to be in inventory */
    /* problem. FindObj(..,0) searches in path, != 0 does not */
    if(obj=FindObj(strs[i], env, i, silent ))
      env=obj;
    else 
      break;
    i++;
  }
  return obj;
}

void Destruct(object obj) {
  if(!obj) return;
  catch(obj->remove());
  if(obj) destruct(obj);
}

string short_path(string file) {
  int s;
  string tmp;

  if ( ! file) return 0;

  if( file[0..8] == "/players/" ) {
    s = strlen( getuid( cloner ) ) + 8;
    return "~"+( file[9..s] == getuid( cloner ) ?
		 file[(s+1)..] : file[9..] );
  }
  else if ( file[0..2] == "/d/" ) 
    return "+"+file[3..];
  else if ( file[0..2] == "/p/" ) 
    return "&"+file[3..];
  else 
    return file;
}

string ObjFile(object obj) {
  return "["+ ( obj ? short_path( object_name(obj) ) : "<destructed>" ) +"]";
}

public varargs string PrintShort( string pre, object obj, int rmode ) {
  string str, str2;

  if(!obj) return 0;
  if(!((str=(string)obj->Short())||
       (str=(string)obj->QueryIntShort()))) {
    str = "- no short description -";
  }
  if(str=="")
    str = "- no short description -";

  str = sprintf( "%-34.34'.'s", str+" " ) + " ";
  if(interactive(obj)) str+="i";
  else if(living(obj)) str+="l";
  else str+=".";
  str+=(shadow(obj, 0) ? "s" : ".");
  
  str2 = pre + capitalize( str ) + " "+ ObjFile(obj );

  if ( rmode )
    return str2;
  else {
    tell_object( cloner, str2+"\n" );
    return 0;
  }
}

int MoveObj(object obj1, object obj2, int silent) {
  int err;
  object oldenv;

  if(!(obj1&&obj2)) return 0;
  oldenv=environment(obj1);
  err=(int)obj1->move(obj2, M_SILENT, 0);
  if(!silent) {
    switch(err) {
    case ME_NO_MOVE:
      tell_object( cloner, "Object returned ME_NO_MOVE.\n");
      if(function_exists("move", obj1))
        tell_object( cloner, "Object has a lfun move() defined.\n");
      return 0;

    case ME_OK:
      tell_object( cloner, "Object returned ME_OK.\n");
      if(environment(obj1) == obj2) {
        PrintShort("Move: ", obj1);
        PrintShort("into: ", obj2);
        return 1;
      } else
        tell_object( cloner, "Object has not been moved.\n" );
      return 0;

    case ME_DESTRUCTED_SELF:
      tell_object( cloner, "Object returned ME_DESTRUCTED_SELF.\n");
      if(obj1)
        tell_object( cloner, "Object is still valid.\n" );
      return 0;

    case ME_NO_LEAVE:
      tell_object( cloner, "Object returned ME_NO_LEAVE.\n" );
      if(oldenv && oldenv != environment(obj1))
        tell_object( cloner, "Object has been moved.\n" );
      return 0;

    case ME_NO_ENTER:
      tell_object( cloner, "Object returned ME_NO_ENTER.\n" );
      if(oldenv && oldenv != environment(obj1))
        tell_object( cloner, "Object has been moved.\n");
      return 0;

    case ME_TOO_HEAVY:
      tell_object( cloner, "Object returned ME_TOO_HEAVY.\n" );
      if(oldenv && oldenv != environment(obj1))
        tell_object( cloner, "Object has been moved");
      return 0;

    default:
      tell_object( cloner, "Object returned unknown return value.\n" );
      return 0;
    }
  }

  return 1;
}

string XFile(string file) {
  object ob;
  string tmp, tmp2;

  if(file) {
    switch(file[0]) {
    case '@':
      if ( ob = XFindObj( file[1..] ) )
	return blueprint( ob ) + ".c";
      else 
	return 0;
    case '$':
      if ( ob = XFindObj( file ) )
	return blueprint( ob ) + ".c";
      else
	return 0;
    default:
      tmp = (string)MASTER->make_path_absolute( file );
      if ( ( file_size( tmp ) > 0 ) ||
	   ( file_size( tmp+".c" ) > 0 ) )
	return tmp; /* if file exits we do not expand faliases */
      return (string)MASTER->make_path_absolute( cloner->exp_fname( file ) );
    }
  }
  return 0;
}

string XFindFile(string file) {
  if ( file = XFile( file ) ) {
    if(file_size(file)>=0) return file;
    if( file[<2..]!=".c"&&file_size(file+".c")>0) return file+".c";
  }
  tell_object( cloner, "File not found or readable ["+short_path(file)+"].\n");
  return 0;
}

int CatFile() {
  int end;
  string l;

  end=moreoffset+morelines;
  while(moreoffset<end) {
    if(l=read_file(morefile, moreoffset, 1)) {
      moreoffset++;
      tell_object( cloner, l );
    } else return 0;
  }
  if(read_file(morefile, moreoffset+1, 1)) return 1;
  else return 0;
}

void MoreFile(string str) {
  int i, off;
  string f, l, r;

  if(str) {
    if(term) tell_object( cloner,"M[1M");
    switch(str[0]) {
    case 'q':
    case 'x':
      moreflag=0;
      moreoffset=1;
      if(morefile=="/players/"+getuid(cloner)+"/.tmp") rm(morefile);
      if ( moreautodes ) {
	remove();
      }
      return;
      break;
    case 'P':
    case 'U':
      moreflag=0;
      moreoffset=moreoffset-morelines;
    case 'p':
    case 'u':
      moreoffset=moreoffset-2*morelines;
      break;
    case 'D':
    case 'N':
      moreoffset+=morelines;
    case 0:
    case 'd':
    case 'n':
      if(moreflag) {
        moreflag=0;
        moreoffset=1;
        if(morefile=="/players/"+getuid(cloner)+"/.tmp") rm(morefile);
	if ( moreautodes ) {
	  remove();
	}
        return;
      }
      break;
    case '/':
      i=moreoffset-morelines;
      r=str[1..];
      if(!regexp(({"dummy"}), r))
        tell_object( cloner, "Bad regular expression.\n");
      else while((l=read_file(morefile, i++, 1))&&!sizeof(regexp(({l}), r)));
      if(l) {
        tell_object(cloner,"*** Skipping ...\n");
        moreoffset=i-1;
      } else {
        tell_object(cloner,"*** Pattern not found\n");
        moreoffset-=morelines;
      }
      break;
    case '0'..'9':
      sscanf(str, "%d", i);
      moreoffset=i;
      break;
    }
  }
  /* [Mica] this is not needed, as basicxcmd is cloned to die and always
            initializes moreoffset to 1 and moreflag to 0
	    else {
	    moreoffset=1;
	    moreflag=0;
	    }
	    */

  if(moreoffset<1) moreoffset=1;
  if(CatFile()) {
    tell_object(cloner,
		"*** More: q,u,U,d,D,/<regexp>,<line> ["+(moreoffset-1)+"] *** ");
  } else {
    tell_object(cloner,
		"*** More: q,u,U,d,D,/<regexp>,<line> ["+(moreoffset-1)+"=EOF] *** ");
    moreflag=1;
  }
  SetDestruct(0);
  moreautodes = 1;
  input_to("MoreFile");
  return;
}

void XMoreFile(string file, int flag) {
  if(!file) return;
  term=(string)cloner->QueryTTY()!="dumb";
  moreautodes = 0;

  if(file_size(morefile=(string)MASTER->make_path_absolute(file))>0)
    MoreFile(0);
  else if (flag) 
    tell_object( cloner, "Cannot read file.\n");
}

public int create() {
  if ( ::create() ) return 1;
  if ( dispatcher )
    xvariables = dispatcher->QueryTempData( "xvariables" );
}

public varargs void AddXVariable( string key, mixed data, int nosave ) {
  if ( mappingp( xvariables ) )
    xvariables[key] = data;
  else
    xvariables = ([ key : data ]);

  if ( ( dispatcher ) && ( ! nosave ) )
    dispatcher->SetTempData( "xvariables", xvariables );
}

public varargs void RemoveXVariable( string key, int nosave ) {
  if ( ! mappingp( xvariables ) )
    return;

  m_delete( xvariables, key );

  if ( ! sizeof( xvariables ) )
    xvariables = 0;

  if ( ( dispatcher ) && ( ! nosave ) )
    dispatcher->SetTempData( "xvariables", xvariables );
}

void CheckXVariables( int show, int nosave ) {
  int i, s;
  string * keys;

  if ( ! mappingp( xvariables ) ) return;
  
  keys = m_indices( xvariables );
  if ( ! sizeof( keys ) ) return;
 
  for( i = 0; i < sizeof( keys ); i++ ) {
    if ( xvariables[keys[i]] ) continue;
    if ( show ) 
      tell_object( cloner, 
		   "*** Variable $"+keys[i]+" has been destructed.\n");
    m_delete( xvariables, keys[i] );
  }

  if ( ( dispatcher ) && ( ! nosave ) )
    dispatcher->SetTempData( "xvariables", xvariables );
}

/*
** create lpc-file with 'name' at wizards directory, first write '.lpchdr.h' or
** LPCHEADER to file and then data. Return full name on success
*/

string WriteLpcFile( string name, string data ) {
  string file, nx;
  int rc;
  object obj;

  file = "/players/"+getuid(cloner)+"/"+name;

  if ( file_size( file ) > 0 ) {
    if ( ! rm( file ) )
      return 0;
  }

  if ( obj = find_object( file ) )
    Destruct( obj );

  nx = "/players/"+getuid(cloner)+"/.lpchdr.h";
  if ( file_size( nx ) > 0 ) 
    rc = write_file( file, read_file( nx ) );
  else
    rc = write_file( file, LPC_HEADER );

  if ( ! rc )
    return 0;

  if ( ! write_file( file, data ) )
    return 0;

  return file;
}






