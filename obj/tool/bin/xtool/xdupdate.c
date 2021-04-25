#include "../bincmd.h"

inherit BASICXCMD;

QueryAbrev() { return "xdu"; }

int main( string str ) {
  int i, s;
  object obj;
  string file, *list;

  if ( (!str) || ( str == "?" ) )
    return notify_fail( "xdu(pdate) <filename>\n"), 0;

  if(!(file=XFindFile(str))) return 1;
  if(file[<2..]==".c") file=file[0..<3];
  if(obj=XFindObj(file)) {
    PrintShort("Deep updating: ", obj);
    list=inherit_list(obj);
    for(s=sizeof(list); i<s; i++) {
      if(obj=find_object(list[i])) Destruct(obj);
    }
  }
  return 1;
}




