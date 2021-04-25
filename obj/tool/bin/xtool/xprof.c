#include "../bincmd.h"

inherit BASICXCMD;

#define XPROF_MACRO BASE+"xtool/xprof.h"

QueryAbrev() { return "xprof"; }

int main( string str ) {
  string *funcs, inh, tmp;
  mixed *data, *d;
  mapping xpr;
  object obj;
  int i, rn;

  notify_fail( "xprof <<-c>|<-C> <file>>|<object>\n");
  if ( (!str)||(str=="?") ) return 0;

  if(str[0..2]=="-c "||str[0..2]=="-C ") {
    rn=(str[1]=='C');
    if(str=XFindFile(str[3..])) {
      inh=str=str[0..<3];
      if(obj=find_object(inh)) Destruct(obj);
      if(catch(call_other(inh,"???"))) return 1;
      obj=find_object(inh);
      if(rn) {
	inh+=".xprof.c";
	rm(inh);
	str+=".c";
	rename(str, inh);
      } else {
	str="/players/"+getuid(cloner)+"/xprof.c";
	rm(str);
      }
      tmp="inherit \""+inh+"\";\n#include \""+XPROF_MACRO+"\"\n";
      funcs=m_indices(mkmapping(functionlist(obj, 0x08000001)))-({"__INIT"});
      tmp+="F("+implode(funcs,",(A0,A1,A2,A3,A4,A5,A6,A7,A8,A9))\nF(")+
	",(A0,A1,A2,A3,A4,A5,A6,A7,A8,A9))\n";
      write_file(str, tmp);
      tell_object( cloner, "Done.\n" );
    }
  } else if(obj=XFindObj(str)) {
    if(xpr=(mapping)obj->__query_xprof_data__()) {
      funcs=m_indices(xpr);
      data=m_values(xpr);
      rm("/players/"+getuid(cloner)+"/.tmp" );
      str="Evaluation cost per function:\nNAME                     "+
	" CALLS |  TOT.EXCL.  TOT.INCL. | REL.EXCL. REL.INCL.\n";
      for(i=sizeof(funcs); i--;) {
	if(d=data[i]) {
	  str += sprintf( "-25.25'.'s %6.6'.'s | %10.10'.'s %10.10'.'s | %9.9'.'s %9.9'.'s\n", ""+funcs[i]+" ", " "+d[0], " "+d[1], " "+d[3], " "+d[1]/d[0], " "+d[3]/d[0] );
	}
      }
      write_file("/players/"+getuid(cloner)+"/.tmp",str);
      str="\nElapsed time per function:\nNAME                     "+
	" CALLS |  TOT.EXCL.  TOT.INCL. | REL.EXCL. REL.INCL.\n";
      funcs=m_indices(xpr);
      for(i=sizeof(funcs); i--;) {
	if(d=data[i]) {
	  str += sprintf( "-25.25'.'s %6.6'.'s | %10.10'.'s %10.10'.'s | %9.9'.'s %9.9'.'s\n", ""+funcs[i]+" ", " "+d[0], " "+(d[2]+5)/10+"ms", " "+(d[4]+5)/10+"ms", " "+d[2]/d[0]+"us", " "+d[4]/d[0]+"us" );
	}
      }
      write_file("/players/"+getuid(cloner)+"/.tmp",str);
      XMoreFile("/players/"+getuid(cloner)+"/.tmp", 0);
    } 
    else 
      tell_object( cloner, "No profiling information available.\n" );
  }
  return 1;
}




