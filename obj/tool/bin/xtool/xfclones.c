/*
** xfclones - find and print clones of a specific blueprint
*/

#include "../bincmd.h"

inherit BASICXCMD;

QueryAbrev() { return "xfclo"; }

int main( string str ) {
  int vmode, i;
  string * sx, file, fname;
  object * clns, obj2;

  notify_fail( "xfclo(nes) [-v] <file>\n"), 0;

  if ( (!str) || ( str == "?" ) )
    return 0;

  if ( ! sizeof ( sx = explode( str, " " ) - ({ "" }) ) )
    return 0;

  vmode = 0;
  if ( sizeof( sx ) > 1 ) {
    while( sx[0][0] == '-' ) {
      /* test for known params */
      switch( sx[0] ) {
      case "-v":
	vmode = 1;
	break;
      default:
	break;
      }
      /* remove params */
      sx = sx[1..];
    }
    if ( ! sizeof( sx ) )
      return 0;
  }

  if ( ! ( fname = XFindFile( sx[0] ) ) )
    return 1; /* error handling is done by XFindFile */

  file = "/players/"+getuid(cloner)+"/.tmp";
  if ( file_size( file ) > 0 )
    rm( file );

  if ( fname[<2..] == ".c" )
    fname = fname[0..<3]; /* remove trailing .c */

  clns = find_clones( fname );

  for ( i = 0; i < sizeof( clns ); i ++ ) {
    write_file(file, PrintShort("",clns[i],1)+"\n");
    if ( vmode ) { 
      if ( obj2 = environment(clns[i]) ) 
	write_file(file, PrintShort(" (E) ",obj2,1)+"\n");
      else
	write_file(file, " (E) <no environment>\n" );
    }
  }
  
  write_file(file,"\nTotal matching objects to '"+fname+"': "+sizeof( clns )+"\n");

  XMoreFile(file, 0);
  return 1;
}




