/* A lagmeter
 * measuring the lag!
 * (C) Loco 1993/94
 */

inherit "std/thing";
//inherit "obj/lib/string";
#include <properties.h>
#include <secure/wizlevels.h>
#include <config.h>

#define TP this_player()

static int count,counth,timeset,countlen,
  lagcountmode;
static object user;

create() {
  seteuid(getuid());
  (::create());
  SetShort("a lagmeter");
/* SetLong replaced by level-dependent QueryLong() */
  SetIds(({"lagmeter","multimeter","meter"}));
  lagcountmode=0;
  Set(P_AUTOOBJECT,1);
  Set(P_NOBUY,1);
  SetValue(500);
  Set(P_NODROP,1);
}


init()
{
  (::init());
  add_action("gdlagindex","gdlag");
}


QueryNoBuy() {
  return living(environment());
}


QueryLong() {
  return "This is a lagmeter. Use the following command:\n\
gdlag [len]    measures the GameDriver LAG using two different methods.\n\
               Optional arg is measurement duration (default 10).\n\
               A result of around 2.0 means little or no lag.\n";
}


gdlagindex(str) {
  user=this_player();
  if (!str || !sscanf(str,"%d",countlen)) countlen=10;
  write("measuring GameDriver Lag ("+countlen+" turns)\n");
  count=-countlen;
  counth=-1;
  call_out("lagcount",0);
  lagcountmode=1;
  set_heart_beat(1);
  return 1;
}


lagcount() {
  if (count<0) {
    timeset=time();
    count=-count+1;
  }
  --count;
  if (!count) {
    tell_object(user,"\nAverage call_out time  : "+divide(time()-timeset,countlen)+" sec.\n");
  }
  else call_out("lagcount",0);
}


heart_beat() {
  if (lagcountmode) {
    ++counth;
    if (!counth) timeset=time();
    if (counth==countlen) {
      tell_object(user,"\nAverage heart_beat time: "+divide(time()-timeset,countlen)+" sec.\n");
      lagcountmode=0;
    }
  }
  if (!(lagcountmode)) set_heart_beat(0);
}

string divide(int i1, int i2)
{ 
  int i;
  string s;
  if (i2==0) return "not calculable";
  i=i1*10/i2;
  if (i<10) return "0."+i;
  s=(string)(i);
  return s[0..<2]+"."+s[<1..<1];
}
