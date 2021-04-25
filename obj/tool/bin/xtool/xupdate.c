#include "../bincmd.h"

inherit BASICXCMD;

QueryAbrev() { return "xup"; }

int main(string str) {
  object obj;
  string file;

  if ( (!str) || ( str == "?" ) ) 
    return notify_fail( "xup(date) <filename>\n"), 0;

  if ( ! ( file = XFindFile( str ) ) ) 
    return 1;
  if ( file[<2..] == ".c" ) 
    file = file[0..<3];

  if ( ( obj = XFindObj( file ) ) &&
       ( obj = find_object( blueprint( obj ) ) ) ) {
    PrintShort( "Updating: ", obj );
    Destruct(obj);
  } 
  else 
    tell_object( cloner, "Object not found.\n" );

  return 1;
}



