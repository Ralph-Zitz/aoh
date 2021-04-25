/*
** MasterMind
** Idea: Smeagol@Nightfall
** Author: Thragor@Nightfall, 05sep95
** Update: Deepthought@Nightfall, 09nov96 fixed correct & place evaluation
*/

#include <search.h> // SM_..
#include <config.h> // NOTIFY_...

#define SF(x) #'x //'
#define SYM(x) 'x //'
#define CL_BRK #'[ //'
#define TP this_player()
#define ME this_object()

#define GAME_SEARCHED 0
#define GAME_TRIES 1

#define TRY_ARRAY 0
#define TRY_CORRECT 1
#define TRY_PLACE 2

inherit "/obj/equipment";
inherit "/obj/lib/number";
inherit "/obj/lib/string";

private static string *Pcolours; // the different colours
private static mapping Pplayers;
// ([<player>:<searched>;([<try-nr>:<try>;<correct>;<wrong place>])
// where try is: ({<col1>,<col2>,<col3>,<col4>})
private static int Pwidth, // how many colours in one line
                   Ptries; // how many tries are allowed
private static status Pdouble, // if true: Pins of the same colour
                               //          may occur twice
                      Psilent; // if messages to the environment shall
                               // be printed or not

public status SetSilent(int i) { return Psilent = i; }
public status QuerySilent() { return Psilent; }

public string *SetColours(string *c) { return Pcolours = c; }
public string *QueryColours() { return (Pcolours||({}))+({}); }
private mixed alias(mixed try)
{
  if (stringp(try))
    return try[0..0];
  return map(try,lambda(({SYM(w)}),({SF([..]),SYM(w),0,0})));
}

private mixed unalias(mixed try)
{
  mapping alias_table;
  alias_table = mkmapping(alias(QueryColours()),QueryColours());
  alias_table+= mkmapping(QueryColours(),QueryColours());
  if (stringp(try))
    return alias_table[try];
  return map(try,lambda(({SYM(t),SYM(a)}),
                              ({CL_BRK,SYM(a),SYM(t)})),alias_table);
}

public string *AddColour(string c)
{
  int pos;
  if (!c||!strlen(c)) return Pcolours;
  if (!Pcolours)
    return Pcolours = ({c});
  if ((pos = member(alias(Pcolours),c[0..0]))!=-1)
    raise_error("New colour "+c+" collides with colour "+Pcolours[pos]+
                " because of same first letter.\n");
  Pcolours = Pcolours-({c})+({c});
  return Pcolours+({});
}

public int SetWidth(int i) { return Pwidth = i; }
public int QueryWidth() { return Pwidth; }

public int SetTries(int i) { return Ptries = i; }
public int QueryTries() { return Ptries; }

public status SetDouble(status s) { return Pdouble = s; }
public status QueryDouble() { return Pdouble; }

public mapping SetPlayers(mapping p) { return Pplayers = p; }
public mapping QueryPlayers() { return Pplayers||([]); }

public string *get_line()
{
  string *cols,*res;
  int i,size;
  cols = QueryColours();
  i = QueryWidth();
  res = ({});
  if (i)
    while(i--)
      {
        if (!size = sizeof(cols))
	  {
	    MASTER->runtime_error(
              "get_line(): Too few colours given compared to width set\n",
              blueprint(),object_name(),-1);
	    break;
	  }
	res+=({cols[random(size)]});
	if (!QueryDouble())
          cols-=res[<1..<1];
      }
  res = alias(res);
  return res;
}
	
  
public mapping AddTry(mixed pl,string *try)
{
  mapping tries,res;
  string *correct,*place;
  string *rest, *searched;
  int i;
  int pos;
  
  if (!pl) return 0;
  if (objectp(pl))
    pl = getuid(pl);
  try = alias(try);
  if (!Pplayers)
    Pplayers = m_allocate(0,2);
  if (!member(Pplayers,pl))
    {
      Pplayers[pl,GAME_SEARCHED] = get_line();
      Pplayers[pl,GAME_TRIES] = m_allocate(0,3);
    }
  tries = Pplayers[pl,GAME_TRIES];
  correct = ({});
  place = ({});
  rest = ({});
  searched = ({});
  i = sizeof(try);
  while(i--) {
    if (try[i]==Pplayers[pl,GAME_SEARCHED][i])
      correct += ({try[i]});
    else {
      rest += ({try[i]});
      searched += ({Pplayers[pl,GAME_SEARCHED][i]});
    }
  }
  i = sizeof(rest);
  while (i--) {
    if (-1 != (pos = member(searched, rest[i])))
    {
      place += ({rest[i]});
      searched = searched[0..(pos-1)]+searched[(pos+1)..];
    }
  }

  //correct = get_correct(try,Pplayers[pl,GAME_SEARCHED]);
  //place = get_place(try-correct,Pplayers[pl,GAME_SEARCHED]);
  tries[i = sizeof(tries)+1,TRY_ARRAY] = try;
  tries[i,TRY_CORRECT] = sizeof(correct);
  tries[i,TRY_PLACE] = sizeof(place);
  Pplayers[pl,GAME_TRIES] = tries;
  res = m_allocate(0,3);
  res[i,TRY_ARRAY] = try;
  res[i,TRY_CORRECT] = correct;
  res[i,TRY_PLACE] = place;
  return res;
}

private int check_double(string *try)
{
  int o;
  string *cols;
  cols = QueryColours();
  while(sizeof(try))
    {
      o = sizeof(try);
      if (o-1>sizeof(try-try[0..0]))
	return notify_fail("Colour "+unalias(try[0])+" appears twice!\n",
			   NOTIFY_NOT_VALID),1;
    }
  return 0;
}

private string *valid_move(string str)
{
  string *try,*cols,*h;
  try = map(explode(str,","),SF(norm_id));
  cols = alias(QueryColours());
  if (sizeof(h = try-QueryColours()-cols))
    return notify_fail("Colour "+h[0]+" not valid.\n",
		       NOTIFY_NOT_VALID),0;
  try = alias(try);
  if (sizeof(try)<QueryWidth())
    return notify_fail("You have too few colours given. "+
                       to_string(QueryWidth())+" are searched.\n",
		       NOTIFY_NOT_VALID),0;
  if (sizeof(try)>QueryWidth())
    return notify_fail("You have too many colours given. "+
                       to_string(QueryWidth())+" are searched.\n",
		       NOTIFY_NOT_VALID),0;
  if (!QueryDouble()&&check_double(try)) return 0;
  return try;
}

private void envmsg(string str)
{
  if (Psilent) return;
  say(str,TP);
}

private int check_move(string str)
{
  string pl,*try;
  mapping result;
  int correct, place,nr;
  
  if (!TP) return 0;
  if (!try = valid_move(str))
    return 0;
  result = AddTry(TP,try);
  nr = m_indices(result)[0];
  if (sizeof(result[nr,TRY_CORRECT])==QueryWidth())
    {
      write("Congrats you win!\n");
      TP->AddXP(100);
      envmsg(capitalize(TP->QueryName())+" wins MagicMasterMind.\n");
      m_delete(Pplayers,getuid(TP));
      return 1;
    }
  place = sizeof((result[nr,TRY_PLACE]));
  correct = sizeof(result[nr,TRY_CORRECT]);
  write("You set "+implode(try,", ")+" and the result is\n  "+
        itoa(correct)+" pin"+(correct==1?" is":"s are")+
        " at the right place and\n  "+
        itoa(place)+" pin"+(place==1?" is":"s are")+
        " of the correct colour.\n");
  if (m_indices(result)[0]>=QueryTries())
    {
      write("Sorry, your moves are exceeded.\n"
            "The searched order was: "+
            implode(unalias(Pplayers[getuid(TP),GAME_SEARCHED]),", ")+".\n");
      envmsg(capitalize(TP->QueryName())+" lost MagicMasterMind.\n");
      m_delete(Pplayers,getuid(TP));
    }
  return 1;
}

private varargs void show_board(status solution)
{
  string pl,board;
  mapping tries;
  int *tidx,i,w;
  
  if (!TP) return;
  pl = getuid(TP);
  if (!Pplayers||!member(Pplayers,pl))
    {
      if (solution)
        write("The solution is: "+implode(get_line(),", ")+".\n");
      else
        write("The board is empty, waiting for you to give your first "
              "move.\n"
              "The following colours can be used:\n"+
              implode(map(QueryColours(),
                                lambda(({SYM(s)}),({SF(capitalize),SYM(s)}))),
                      ", ")+".\n");
      return;
    }
  tries = Pplayers[pl,GAME_TRIES];
  i = sizeof(tidx = sort_array(m_indices(tries),SF(<)));
  board = "\n+-"+mkstr(4*QueryWidth(),"-")+"--"+mkstr(2*9+23,"-")+"-+\n"
          "| "+cadjust("MagicMasterMind (tm)",4*QueryWidth()+2+(2*9+23))+" |\n"
          "+-"+mkstr(4*QueryWidth(),"-")+"+-"+mkstr(2*9+23,"-")+"-+\n"
          "| "+mkstr(4*QueryWidth()," ")+"| "+mkstr(2*9+23," ")+" |\n";
  
  while(i--)
    {
      board+="| ";
      for(w=0;w<QueryWidth();w++)
        board+="("+tries[tidx[i],TRY_ARRAY][w]+") ";
      board+="| ";
      board+=ladjust(itoa(tries[tidx[i],TRY_CORRECT])+" black and "+
                     itoa(tries[tidx[i],TRY_PLACE])+" white pins",2*9+23)
             +" |\n";
    }
  board+="| "+mkstr(4*QueryWidth()," ")+"| "+mkstr(2*9+23," ")+" |\n"
         "+-"+mkstr(4*QueryWidth(),"-")+"+-"+mkstr(2*9+23,"-")+"-+\n"
         "| "+ladjust("The following colours can be used:",
                     4*QueryWidth()+2+(2*9+23))+" |\n";
  board+=implode(
           map(
             explode(
               wrap(
                 implode(
                   map(QueryColours(),
                             lambda(({SYM(s)}),({SF(capitalize),SYM(s)}))),
                   ", "),
                  4*QueryWidth()+2+(2*9+23)),
                "\n"),
             lambda(({SYM(s)}),
                    ({SF(+),"| ",({SF(+),({SF(ladjust),SYM(s),
                                             4*QueryWidth()+2+2*9+23}),
                                         " |\n"})}))),
                 "\n");
  // The line above simply puts "| " and " |\n" around each line of the
  // wrapped list of colours
  i = QueryTries()-max(tidx);
  board+="+-"+mkstr(4*QueryWidth(),"-")+"--"+mkstr(2*9+23,"-")+"-+\n\n"
         "You "+(solution?"had":"have")+" "+itoa(i)+" "+
         (i==1?"try":"tries")+" left.\n\n"+
         "black pin = correct colour at right place\n"
         "white pin = correct colour at wrong place\n";
  if (solution)
    board+="\nThe solution is: "+implode(unalias(Pplayers[pl,GAME_SEARCHED]),
                                         ", ")+".\n";
  smore(board,TP->QueryPageSize());
}

public int cmd_show(string str)
// 'show board'
// Take a look at the board.
{
  if (!str||!TP||search(TP,str,SM_IGNLIGHT|SM_OBJECT|SEARCH_INV_ENV)!=ME)
    return notify_fail("Show what? The board?\n",NOTIFY_NOT_OBJ),0;
  envmsg(capitalize(TP->QueryName())+" looks at "+QueryShort()+".\n");
  show_board();
  return 1;
}

public int cmd_set(string str)
// Make one move, e. g. 'set v, o, b, y'
{
  if (!TP) return 0;
  if (member(({"brief","verbose"}),lower_case(str||""))!=-1)
    {
      if (lower_case(str)=="brief")
        if (QuerySilent())
          return notify_fail("Board already set to brief mode.\n",
			     NOTIFY_NOT_VALID),0;
        else
	  {
	    SetSilent(1);
	    write("Board set to brief mode.\n");
	  }
      else
        if (!QuerySilent())
          return notify_fail("Board already set to verbose mode.\n",
			     NOTIFY_NOT_VALID),0;
        else
	  {
	    SetSilent(0);
	    write("Board set to verbose mode.\n");
	  }
      return 1;
    }
  envmsg(capitalize(TP->QueryName())+" places some pins at "+
         QueryShort()+".\n");
  return check_move(str);
}

public int cmd_retreat(string str)
// Give up and show the solution
{
  if (  !str
      ||!TP
      ||!sscanf(norm_id(str),"from %s",str)
      ||search(TP,str,SM_IGNLIGHT|SM_OBJECT|SEARCH_INV_ENV)!=ME)
    return notify_fail("Retreat from game?\n",NOTIFY_NOT_OBJ),0;
  envmsg(capitalize(TP->QueryName())+" retreats from the game.\n");
  show_board(1);
  m_delete(Pplayers,getuid(TP));
  return 1;
}

public int cmd_read(string str)
{
  string *cols;
  if (!TP||lower_case(str||"")!="instructions")
    return 0;
  cols = QueryColours();
  smore(
    "MagicMasterMind\n"
    "---------------\n\n"
    "You have pins of "+itoa(sizeof(cols))+" different colours:\n"+
    wrap(implode(cols,", "))+".\n"
    "The MagicMasterMind-board thinks of a combination of these\n"
    "colours taking "+itoa(QueryWidth())+" "+
    (QueryDouble()?"":"different ")+"pins from it.\n"
    "Your task is to guess in "+itoa(QueryTries())+" moves, what the\n"
    "MagicMasterMind-board has hidden in its inside.\n"
    "To take a look at the board, simply type 'show board'.\n"
    "To try a sequence of pins type e. g.: 'set v, y, b, r'\n"
    "  where v, y, ... are the first letters of the colours you mean.\n"
    "  You may also give the complete colour-names.\n"
    "To retreat from the game (i. e. to give up and see the solution)\n"
    "  type 'retreat from game'.\n"+
    (Psilent?
    "To switch the board to verbose mode (environment will see what you\n"
    "  are doing), type 'set verbose'.\n":
    "To switch the board to silent mode (environment will not see what you\n"
    "  are doing, type 'set brief'.\n"),TP->QueryPageSize());
  return 1;
}

public void init()
{
  (::init());
  add_action("cmd_show","show");
  add_action("cmd_set","set");
  add_action("cmd_retreat","retreat");
  add_action("cmd_read","read");
}

public void create()
{
  (::create());
  SetIds(({"board","game","mastermind","magicmastermind","mind",
           "magicmastermind-board"}));
  SetAds(({"magic","master","mind","mastermind","magicmastermind"}));
  SetWidth(4);
  SetTries(10);
  SetDouble(1);
  AddColour("yellow");
  AddColour("blue");
  AddColour("red");
  AddColour("orange");
  AddColour("green");
  AddColour("violett");
  AddColour("white");
  AddColour("turquoise");
  SetShort("a MagicMasterMind-board");
  SetLong(
    "It's a small wooden plate with many holes in it.\n"
    "The edges of the plate are decorated with gold and on a small\n"
    "golden plate you see the words: 'read instructions'.\n");
  SetWeight(500);
  SetValue(222);
  SetReadMsg("Read the instructions to get informations about it.\n");
}
