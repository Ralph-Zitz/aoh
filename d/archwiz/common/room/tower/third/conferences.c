/* the conference hall
 * (c) 24.11.1992 by Taube
 */

inherit "/std/room";

#include "path.h"
#include <moving.h>
#include "/players/taube/lib/noobvious.c"

#define LINEWIDTH 75

string format(string str);


create()
{
  (::create());
  SetIntShort("The conference hall");
  SetIntLong(format
	     ("This is Taube's conference hall.\n"+
	      "Although it is not the biggest one you have ever seen, "+
	      "it is quite comfortable and useful.\n"));
  SetIndoors(1);
}


init()
{
  (::init());
  add_action("getaction", "", 1);   /* *grin* */
  input_to("getinput");             /* *grin evilly* */
}


getaction(string str) { return getcmds(query_verb()+" "+str); }
getinput(string str)
{
  mixed ret;
  if(ret=getcmds(str)) input_to("getinput");
  return ret;
}


getcmds(string str)
{
  switch(str) {
  case "": return 1;
  case "help": help(); break;
  case "l": case "look": look(); break;
  case "leave": case "out": case "south": leave(); return 0;
  default:
    write(format(capitalize(this_player()->QueryRealName()))+": "+str+"\n");
  }
  return 1;
}


help()
{
  write(format("\nHow to behave in this room:\n\n"+
	       "This room logs everything that is said in it. If you want "+
	       "to say something, just type it in - there's a kind of "+
	       "'auto-converse-mode' active. In fact, there are only a few "+
	       "commands possible:\n"+
	       "- 'help' shows this text\n"+
	       "- 'l' or 'look' shows you a list of the people in this room\n"+
	       "- 'leave', 'out' or 'south' lead you out of this room.\n\n"+
	       "If you type lines with more than "+LINEWIDTH+" characters, "+
	       "they are automatically wrapped.\n\n"));
}


look()
{
  string str;
  object ob;
  str="People currently in this room:\n";
  for(ob=first_inventory(); ob; ob=next_inventory(ob))
    if(interactive(ob)) str+=capitalize(ob->QueryRealName())+", ";
  str=str[0..strlen(str)-3]+".\n";
  write(format(str));
}


leave()
{
  this_player()->move(TOWER("third/center"), M_GO,
		      ({"the conference",
			  "north",
			  "the conference"}));
}


string format(string str)
{
  string out, *lines, *line;
  int    n, index, length, slen;
  lines=explode("$\n"+str+"\n$", "\n");
  for(n=1; n<sizeof(lines)-1; n++) {
    line=explode("$ "+lines[n], " ");
    out="";
    length=0;
    for(index=1; index<sizeof(line); index++) {
      slen=strlen(line[index]);
      if(!length) {
	out+=line[index]; length=slen;
      } else {
	if(length+slen>=LINEWIDTH-1) {
	  out+="\n  "+line[index]; length=slen+2;
	} else {
	  out+=" "+line[index]; length+=slen+1;
	}
      }
    }
    lines[n]=out;
  }
  return implode(lines, "\n")+"\n";
}
