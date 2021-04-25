/*
** /d/atlantis/common/room/homarid/temple.c
** The temple of the homarids. The rules, disclaimer can be read here
** and the pit can be entered where small homarids start.
** Author: Thragor@Nightfall, 25jun95
** Update: Thragor@Nightfall, 29jun95
*/

#include "path.h"
#include <config.h>
#include <properties.h>
#include <rooms.h>
#include <magic.h>

#define SF(x) #'x //'
#define STR "/lib/string"
#define ME this_object()
#define TP this_player()
#define MAX_TXT_LENGTH 5000
#define COL_BLACK "black"
#define COL_WHITE "white"

#define D(x,y) ("/players/thragor/lib/debug"->debug("thragor",x,y))
inherit UNDERWATER;

private object item_oyster()
{
  object ob;
  string colour;
  
  ob = clone_object("/obj/chest");
  colour = random(2)?COL_WHITE:COL_BLACK;
  ob->SetProperties(
    ([
      P_SHORT:"a "+colour+" oyster",
      P_LONG:"This is a big "+colour+" oyster. It may be used to carry\n"
             "items around. It is said that some oysters contain beautiful\n"
             "pearls, which are sometimes quite valuable.\n",
      P_IDS:({"oyster","mussle"}),
      P_ADS:({colour,"big"}),
      P_ITEMS_REFRESH:
       ({
         ({"/obj/equipment",REFRESH_NONE,
           ([P_SHORT:"a "+colour+" pearl",
             P_LONG:"This is a beautiful "+colour+" pearl. There seems to be\n"
                    "a mystic aura around it.\n",
             P_INFO:"The pearl glows in a soft "+colour+" light.\n",
             P_MAGIC:"The pearl is a gift of Bor'kesh, God of the Homarids.\n",
 	     P_LIGHT:([COL_WHITE:20,COL_BLACK:-20])[colour],
             P_IDS:({"pearl"}),
             P_ADS:({colour,"glowing","beautiful","mystic"}),
             P_VALUE:50,
             P_SMELL:"It smells salty.\n"
           ]),0
         })
       }),
      P_LOCKSTATE:LOCK_CLOSED,
      P_WEIGHT:1000,
      P_WEIGHT_CHG:300,
      P_SMELL:"It smells salty.\n",
      P_VALUE:10
    ]));
  return ob;
}

private int get_num(string what)
{
  string h1,h2;
  int nr,i;
  string *parts;
  
  if ((i = sizeof(parts = explode(what," ")))<2)
    nr = 1;
  else
    {
      while(i--)
        if (atoi(parts[i])!="<not a number>")
	  break;
      nr = atoi(parts[i])!="<not a number>"?atoi(parts[i]):0;
    }
  if (!nr)
    {
      string *nrs;
      i = sizeof(nrs = ({"first","second","third","fourth","fifth","sixth"}));
      while(i--)
        if (strstr(what,nrs[i])!=-1)
	  break;
      nr = i+1;
    }
  if (nr>6) nr = 4;
  return nr;
}

private string detail_frieze(string what)
{
  string txt;
  int nr;
  nr = get_num(what);
  switch(nr)
    {
     case 1:
      txt = "Toplist of Homarids.\n";
      break;
     case 2:
      txt = "DISCLAIMER";
      break;
     case 3:
      txt = "RULES";
      break;
     case 4:
      txt = "Basic Help";
      break;
     case 5:
      txt = "Homarid Top-Explorers";
      break;
     case 6:
      txt = "List of Guilds of the Homarids\n";
      break;
    }
  return "On the frieze is written: "+txt+".\n";
}

private string detail_pediment(string what)
{
  string txt;
  int nr;
  nr = get_num(what);
  switch(nr)
    {
     case 1:
     case 3:
     case 5:
      txt = "of a dragon";
      break;
     case 2:
     case 4:
     case 6:
      txt = "of a dwarf and a pickaxe next to it";
      break;
    }
  return "On the pediment you see the head "+txt+".\n";
}

private string detail_slab(string what)
{
  string txt;
  int nr;
  nr = get_num(what);
  switch(nr)
    {
     case 1:
      txt =
       "The slab looks like a kind of temple. On the pediment you see\n"
       "the head of a dragon.\n"
       "On the frieze is written: Toplist of Homarids.\n"
       "The following text is guarded by one small ionic column on\n"
       "each side.\n\n"
       "--- Sorry, list not finished yet ---\n";
      break;
     case 2:
      txt =
       "The slab looks like a kind of temple. On the pediment you see\n"
       "the head of a dwarf and a pickaxe next to it.\n"
       "On the frieze is written: DISCLAIMER\n"
       "The following text is guarded by one small ionic column on\n"
       "each side.\n\n"+
       read_file("/etc/DISCLAIMER");
      break;
     case 3:
      txt =
       "The slab looks like a kind of temple. On the pediment you see\n"
       "the head of a dragon.\n"
       "On the frieze is written: RULES\n"
       "The following text is guarded by one small ionic column on\n"
       "each side.\n\n"+read_file("/etc/RULES");
       break;
     case 4:
      txt =
       "The slab looks like a kind of temple. On the pediment you see\n"
       "the head of a dwarf and a pickaxe next to it.\n"
       "On the frieze is written: Basic Help\n"
       "The following test is guarded by one small ionic column on\n"
       "each side.\n\n"
       "Welcome to Nightfall!\n"
       "You just entered a MUD, a multi-user-dungeon. You control a\n"
       "character in this mud and tell him what to do. To do this you\n"
       "need to know the commands. Some commands you already know like\n"
       "for example read/look/examine, because otherwise you wouldn't\n"
       "be able to read this text.\n"
       "The basic commands are described by typing: help<CR>\n"
       "If you first logged in, you should type: help start<CR>\n"
       "The most important commands for you might be:\n"
       "  who ................. get a list of all users\n"
       "  who wiz ............. get a list of all wizards\n"
       "                        Wizards create this MUD, so if you have\n"
       "                        problems contact them (see below)\n"
       "  tell <name> <msg> ... tell a player in this MUD a certain message\n"
       "                        Example: tell thragor Please Help!\n"
       "  say <msg> ........... tell a message to all players around you in\n"
       "                        the same room\n"
       "                        Example: say I need help\n"
       "\n"
       "Now you should be able to play this MUD. For some rules you should\n"
       "read slab 2 and slab 3.\n";
      break;
     case 5:
      txt =
       "The slab looks like a kind of temple. On the pediment you see\n"
       "the head of a dragon.\n"
       "On the frieze is written: Homarid Top-Explorers\n"
       "The following text is guarded by one small ionic column on\n"
       "each side.\n\n"
       "--- Sorry, list not finished yet ---\n";
      if (TP) TP->SetExplored();
      break;
     case 6:
      txt =
       "The slab looks like a kind of temple. On the pediment you see\n"
       "the head of a dwarf and a pickaxe next to it.\n"
       "On the frieze is written: List of Guilds of the Homarids\n"
       "The following text is guarded by one small ionic column on\n"
       "each side.\n\n"
       "--- Sorry, list not finished yet ---\n";
      break;
    }
  if (TP)
    STR->smore(txt,TP->QueryPageSize());
  else
    return txt;
  return "";
}

public void create()
{
  (::create());
  SetSunBright(MAX_SUNBRIGHT/4);
  SetIntShort("The Temple Of Bor'kesh");
  SetIntLong(
    "You are in the temple of Bor'kesh, God of the Homarids.\n"
    "The place, surrounded by six ionic columns, appears in\n"
    "a dim light which comes from above.\n"
    "In the middle of the place is a ring of marble, a frame\n"
    "for the breeding-pit of the homarids.\n"
    "To the south is a big statue of a homarid. It's said that\n"
    "praying there will dispose Bor'kesh and perhaps he'll help\n"
    "you, for example if you died and want to regain your\n"
    "normal state.\n"
    "You suddenly think that you should 'READ SLAB 4'.\n");
//  AddDetail(({"light","dim light"}),SF(fsky));
  AddDetail("%light,dim light%","Test.\n");
  AddDetail(({"breeding-pit","%breeding% pit"}),
    "The walls of the breeding-pit are covered with seaweed.\n"
    "Only some small spots are left to see, that they are made\n"
    "of marble as the whole temple is.\n"
    "For Magic the Gathering player: Ok, ok. Homarids should\n"
    "start in a spawning bed, but the breeding-pit fits much\n"
    "more into my idea.\n");
  AddDetail(({"temple","temple of bor'kesh"}),
    "The temple is open to all directions. You may swim east, west,\n"
    "north, whereever you like.\n");
  AddDetail(({"bor'kesh","god","god of homarids","statue","statue south"}),
    "There's a statue south of you. It's a giant homarid. Perhaps\n"
    "this shows Bor'kesh.\n");
  AddDetail("place",
    "This place seems to be protected somehow.\n");
  AddDetail("%ionic,six% columns",
    "They are placed in a circle around this place.\n");
  AddDetail("%first,second,third,fourth,fifth,sixth,ionic% "
            "column %1,2,3,4,5,6%",
    "The column is about eight feet high and made of pure marble.\n"
    "The capital is build of two volutes. In the middle of the shaft\n"
    "is a big slab looking like a temple itself.\n"
    "The moulding is decorated with scenes of the homarid-life and the\n"
    "plinth is firmly fixed to the marble ground.\n");
  AddDetail("%first,second,third,fourth,fifth,sixth% slab "
            "%1,2,3,4,5,6,at,on,first,second,third,fourth,fifth,sixth,"
            "ionic,column%",
    SF(detail_slab));
  AddReadMsg("%first,second,third,fourth,fifth,sixth% slab "
             "%1,2,3,4,5,6,at,on,first,second,third,fourth,fifth,sixth,"
             "ionic,column%",
    SF(detail_slab));
  AddDetail("frieze %of,first,second,third,fourth,fifth,sixth,slab,"
            "1,2,3,4,5,6,at,on,ionic,column%",
    SF(detail_frieze));
  AddReadMsg("frieze %of,first,second,third,fourth,fifth,sixth,slab,"
             "1,2,3,4,5,6,at,on,ionic,column%",
    SF(detail_frieze));
  AddDetail("pediment %of,first,second,third,fourth,fifth,sixth,slab,"
            "1,2,3,4,5,6,at,on,ionic,column%",
    SF(detail_pediment));
  
  AddMagicProtection(ST_FIRE,
    "Bor'kesh doesn't allow the use of this dangerous enemy.\n");
  AddMagicProtection(ST_ZAP,
    "Bor'kesh doesn't allow this evil kind of magic.\n");
  AddMagicProtection(ST_DAMAGE,
    "Bor'kesh doesn't allow this evil kind of magic.\n");
  AddMagicProtection(ST_SUMMON,
    "Only Bor'kesh can summon down here.\n");
  AddMagicProtection(ST_INFLUENCE,
    "Bor'kesh leads the minds and noone else.\n");
  AddMagicProtection(ST_PSYCHO,
    "Bor'kesh doesn't allow this evil kind of magic.\n");
  AddMagicProtection(ST_DARK,
    "Bor'kesh doesn't allow this evil kind of magic.\n");
  AddItem(SF(item_oyster),REFRESH_REMOVE);
}

private string *parse_list(string *str)
{
  int i;
  string *res;
  res = ({});
  i = sizeof(str);
  while(i--)
    {
      string *new;
      int j;
      j = sizeof(new = explode(str[i],","));
      while(j--)
	res+=explode(new[j]," ");
    }
  return res;
}

private mixed parse_detail(string str,mapping details)
{
  string *idx,*parts,*ads,id,*nr,h,*compare;
  int i,j,pos;
  
  if (!i = sizeof(idx = regexp(m_indices(details),"%.*%")))
    return 0;
  while(i--)
    {
      id = 0;
      parts = explode(idx[i],"%")-({""});
      if (  sscanf(idx[i],"%s%% %s %%%s",h,id,h)!=3
          &&sscanf(idx[i],"%s%% %s",h,id)!=2
          &&sscanf(idx[i],"%s %%%s",id,h)!=2)
        pos = sizeof(parts);
      else
        pos = member(parts," "+id+" ");
      ads = pos>0?parse_list(parts[0..pos-1]):({});
      nr = pos<sizeof(parts)?parse_list(parts[pos+1..]):({});
      compare = explode(str," ");
      for(j=0;j<sizeof(compare);j++)
        if (member(ads,compare[j])==-1)
	  break;
      compare = compare[j..];
      for(j=sizeof(compare)-1;j>=0;j--)
  	if (member(nr,compare[j])==-1)
	  break;
      if (!id&&!sizeof(compare))
        return details[idx[i]];
      if (id==implode(compare[0..j]," "))
        return details[idx[i]];
    }
}

private string read_long_file(string file)
{
  int s,r;
  string res;
  s = file_size(file);
  if (s<=0)
    return "";
  res = "";
  while(s-r>=MAX_TXT_LENGTH)
    {
      res+=read_bytes(file,r,MAX_TXT_LENGTH);
      r+=MAX_TXT_LENGTH;
    }
  res+=read_bytes(file,r,s-r);
  return res||"";
}

private void smart_log(string str)
{
  string creat,*names,msg,content;
  int logged;
  names = explode(file_name(), "/");
  msg = file_name()+": "+str;
  if (creat = MASTER->creator_file(ME))
    if (!sizeof(regexp(explode(read_long_file("/log/rep/"+creat+".details"),
                               "\n"),msg)))
      {
        log_file("rep/"+creat+".details",msg+"\n");
	logged = 1;
      }
  if (names[0] == "d" && sizeof(names)>1)
    if (!sizeof(regexp(explode(read_long_file("/log/rep/"+names[1]+".details"),
                               "\n"),msg)))
    {
      log_file("rep/"+names[1],msg+"\n");
      logged = 1;
    }
  if (logged)
    log_file("rep/DETAILS",msg+"\n");
}

private void scan_missing(string str)
// Scans if a detail is missing
// If str has just one character or is empty nothing will be logged.
{
  if (!str||strlen(str)<2) return;
  str = lower_case(str);
  if (sizeof(
       regexp(
         map(
           explode(read_file(blueprint()+".c"),"\n"),
           SF(lower_case)),
         str)))
    smart_log(str);
}

public mixed GetDetail(string str)
// Parse details with adjectives, etc.
// The detail may look like this:
// %fine,blue,high% column %1,2%
// and will be identified with:
// column, high column, fine high blue column 1, ...
{
  mapping map,Pdetails;
  mixed desc,rc;
  Pdetails = QueryDetails();
  DoExplore("look:"+str) || DoExplore(str);
  switch (str)
    {
     case "doors":
      if (sizeof(map = (mapping)ME->QueryDoors()))
        return (string) ME->Doors();
      break;
     case "sky":
     case "heaven":
      if (!(Pdetails && Pdetails[str]))
        return fsky(str);
     default:
      if (Pdetails&&!desc = Pdetails[str])
        desc = parse_detail(str,Pdetails);
      rc = funcall(desc,str);
      if (!rc)
        scan_missing(str);
      return rc;
    }
}

public string GetReadMsg(string str)
{
  mapping Preadmsgs;
  string desc;
  Preadmsgs = QueryReadMsgs();
  DoExplore("read:"+str) || DoExplore(str);
  if (Preadmsgs&&!desc = Preadmsgs[str])
    desc = parse_detail(str,Preadmsgs);
  return funcall(desc, str);
}
