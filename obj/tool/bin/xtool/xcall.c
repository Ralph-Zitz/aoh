#include "../bincmd.h"

inherit BASICXCMD;

int main( string str ) {
  object obj, callobj;
  string file, callstr, callfun, callexpr, error, errlog;
  int *ru1, *ru2, time;
  mixed res;

  if ( ( ! str ) ||
       ( str == "?" ) ||
       ( sscanf(str, "%s->%s(%s", callstr, callfun, callexpr)!=3 ) )
    return notify_fail( "xcall <object>-><function>(<arguments>)\n" ), 0;

  if ( ! ( callobj = XFindObj( callstr ) ) ) 
    return 1;
  else {
    file = WriteLpcFile( ".tmp.lpc.c", 
			 "eval(obj,me,here){\n"+
			 "return obj->"+callfun+"("+callexpr+";}\n" );

    if ( ! file ) {
      tell_object( cloner, "Could not create lpcfile.\n" );
      return 1;
    }

    errlog = "/players/"+getuid(cloner)+"/.err";
 
    if ( file_size( errlog ) > 0 ) rm ( errlog );

    if ( error = catch( obj = clone_object( file ) ) ) 
      tell_object( cloner, "Error: "+error[1..] );
    else {
      ru1=rusage();
      error = catch( res = (mixed) obj->eval( callobj, cloner, environment(cloner)));
      ru2=rusage();
      if ( error )
	tell_object( cloner, "Error: "+error[1..]);
      else {
        time=ru2[0]-ru1[0]+ru2[1]-ru1[1];
	tell_object( cloner, 
		     "Evaluation time: "+(time<0 ? 0 : time)+" ms.\n");
	tell_object( cloner,
		     "Result: "+"/lib/string"->mixed_to_string( res, 6 )+"\n" );

	if ( objectp( res ) )
	  AddXVariable( "result", res, 0 );
      }
    }
    rm(file);
  }
  
  if ( obj )
    Destruct( obj );

  return 1;
}
