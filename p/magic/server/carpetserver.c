/* carpetserver
*
*  Maintains a highscore lsit of the player who use the carpets most
*
*   31-Mar-97 [Softbyte]
*
*/

#include <secure/wizlevels.h>

#define VERSION 0.1


#define MAXSAVECNT 2
#define TOPLIST "/log/magic/CarpetToplist"

#define MAXLEVEL 5    // level 50/10

private static int savecnt=0;

mapping toplist;    // ([1:"softbyte";35,2:"ciaran":145,...])
mapping QueryToplist() {return toplist;}
int SetToplist(mapping i) {toplist=i;return 1;}

int Debug(string str)
{
object ob;
  ob=find_player("softbyte");
  if (!ob) return 0;
  tell_object(ob,blueprint(this_object())+"::"+str);
  return 1;
}

// Saves this object to a file
int SaveData()
{
  savecnt=0;
  save_object(TOPLIST);
  Debug("Saving "+TOPLIST+"\n");
  return(1);
}
// Calulated leve 0..50 to 0..4 
private int ConvertLevel(int level)
{
  level--;
  if (level<0) level=0;
  level=level/10;          // 0..49 -> 0..4 (5=MAXLVEL)
  if (level<0) level=0;
  if (level>=MAXLEVEL) level=MAXLEVEL-1;
  return level;
}

// Gives the name of the top flyer for level 
public string GetTopname(int level)
{
  level=ConvertLevel(level);
  if (!toplist[level]) return 0;
  return toplist[level,0];
}
// Gives the number of flights of the top flyer for level 
public int GetTopnumber(int level)
{
  level=ConvertLevel(level);
  if (!toplist[level]) return 0;
  return toplist[level,1];
}

public string Toplist()
{
int i;
string s,tmp,pre;
  pre="       ";
  s=pre+ "The TOPLIST of carpet flyers\n";
  s+=pre+ "----------------------------\n";
  for (i=MAXLEVEL-1;i>=0;i--)
  {
    if (toplist[i])
      tmp=sprintf(" %2d-%2d: %6d %s\n",1+i*10,1+i*10+9,toplist[i,1],toplist[i,0]);
    else 
      tmp=sprintf(" %2d-%2d: ------ -----------\n",1+i*10,1+i*10+9);
    s+=pre+tmp;
  }
  return s;
}

// Checks whether a player with 'number' flights and a certain level
// would be taken into the topscore list
public int TestHighscore(int number,int level)
{
  if (number>GetTopnumber(level)) return 1;
  return 0;
}
// recorrds the name 'flyer' with 'number' flights for 'level'
// if it is higher than existing entries
public int RecordFlight(string flyer,int number,int level)
{
int ret;
  level=ConvertLevel(level);
  if (!toplist[level])               // new entry
  {
    toplist+=([level:flyer;number]);
    ret=1;
  } 
  else if (toplist[level,1]<number)  // better entry
  {
    toplist[level,0]=flyer;
    toplist[level,1]=number;
    ret=2;
  }
  else ret=0;                        // not good enough

  savecnt++;
  if (savecnt>MAXSAVECNT) SaveData();
  return ret;
}

create()
{
  if (member(object_name(this_object()), '#') != -1) 
  {
    destruct (this_object());
    return;
  }
  seteuid(getuid());
  if (file_size(TOPLIST+".o")>=0) restore_object(TOPLIST);
  else
  {
    toplist=m_allocate(5,2);
  }
  Debug("Init carpet toplist server.\n");
  savecnt=0;
}

public varargs int remove(int arg)
{
  Debug("Remove Called with "+arg+"\n");
  SaveData();
  destruct(this_object());
  return 1;
}
