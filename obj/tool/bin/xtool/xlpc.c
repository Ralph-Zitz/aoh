#include "../bincmd.h"

inherit BASICXCMD;

QueryAbrev() { return "xlp"; }

int main(string str) {
  object obj;
  string file, error;
  int *ru1, *ru2, time;
  mixed res;

  if ( ( !str ) || ( str == "?" ) ) 
    return notify_fail( "xlpc <expression>\n"), 0;

  file = WriteLpcFile( ".tmp.lpc.c", 
             "eval( me, here ) {\n"+
	     str+";\n"+
	     "}\n" );

  if ( ! file ) {
    tell_object( cloner, "Could not create lpcfile.\n" );
    return 1;
  }

  if ( error = catch( obj = clone_object( file ) ) ) 
    tell_object( cloner, "Error: "+error[1..] );
  else {
    ru1=rusage();
    error = catch( res = (mixed) obj->eval( cloner, environment(cloner)));
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
  if ( ! error )
    rm(file);

  if ( obj )
    Destruct( obj );
  
  return 1;
}




