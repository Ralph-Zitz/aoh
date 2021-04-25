/*
** Culture Center in Nightfall City - Musicroom
** Author: Thragor@Nightfall, 04apr97
** Update:
*/

#include "path.h"
#include <properties.h>
#include <rooms.h>

#define SONG_SINGER 0
#define SONG_WALL   1
#define SONG_SIZE   2
#define SONG_EMPTY  m_allocate(0,SONG_SIZE)

#define ME this_object()
#define SF(x) #'x //'
#define TP this_player()

inherit "/std/room";

private static mapping titles;
private static string played_title;

public string QueryIntNoise()
{
  return sprintf("You listen to '%s' from %s.\n",
		 played_title,titles[played_title,SONG_SINGER]);
}

public int cmd_dance(string str)
{
  if (str||!TP) return 0;
  printf("You dance to '%s' from %s.\n",
	 played_title,titles[played_title,SONG_SINGER]);
  if (!TP->QueryInvis())
    show(capitalize(TP->QueryName())+
	 sprintf(" dances to '%s' from %s.\n",
		 played_title,titles[played_title,SONG_SINGER]));
  return 1;
}

public void AddTitle(string title,string singer,string wall_side)
{
  if (!titles) titles = SONG_EMPTY;
  titles[title,SONG_SINGER] = singer;
  titles[title,SONG_WALL] = wall_side;
}

private void new_title()
{
  int i;
  string *idx,old;
  
  if (!titles) return;
  idx = m_indices(titles)-({played_title});
  old = played_title;
  if (!sizeof(idx)) return;
  i = random(sizeof(idx));
  played_title = idx[i];
  if (old)
    if (titles[old,SONG_WALL]==titles[played_title,SONG_WALL])
      show_room(ME,sprintf("The laser beam jumps to another CD on the "
			   "%sern wall.\n",titles[old,SONG_WALL]));
    else
      show_room(ME,sprintf("The laser beam jumps from the %sern wall "
			   "to the %sern wall.\n",
			   titles[old,SONG_WALL],
			   titles[played_title,SONG_WALL]));
  else
    show_room(ME,sprintf("A laser beam appears and jumps to the %sern wall.\n",
			 titles[played_title,SONG_WALL]));
  tell_room(ME,QueryIntNoise());
}

private void play_title()
{
  if (find_call_out(SF(play_title))!=-1) return;
  if (!sizeof(filter(all_inventory(),SF(interactive))))
    return;
  new_title();
  call_out(SF(play_title),300+random(200));
}

public int remove()
{
  while(remove_call_out(SF(play_title))!=-1);
  return (::remove());
}

public void notify_enter(mixed from,int method,mixed extra)
{
  (::notify_enter(from,method,extra));
  if (   interactive(previous_object())
      && find_call_out(SF(play_title))==-1
     )
    call_out(SF(play_title),2);
}

public void create()
{
  (::create());
  SetIntShort("Musicroom - Culture Center");
  SetIntLong("The walls sparkle in all colours of the rainbow. Several CDs "
	     "serve as mural art. Four loudspeakers play music all around "
	     "the clock. But you don't see any DJ changing discs. But a "
	     "laser beam wanders from one of the discs at the wall to "
	     "another when the title changes.\n"
	     "A board is the only piece of furniture inside here, standing "
	     "besides the exit to the south. The rest of the room is kept "
	     "free for dancing.\n");
  Set(P_CANSEE,1);
  AddDetail("walls","They are full of CDs.\n");
  AddDetail(({"colours","cds","cd","discs","disc","rainbow","art",
	      "mural art"}),
	    "The CDs reflect the light in all colours of the rainbow.\n");
  AddDetail(({"loudspeakers","loudspeaker","four loudspeakers"}),
	    "They play music all around the clock. Just listen to it.\n");
  AddNoise("music",SF(QueryIntNoise));
  AddDetail(({"beam","laser beam"}),
	    "It seems to read the CDs.\n");
  AddTitle("The Miracle","Queen","west");
  AddTitle("We Will Rock You","Queen","north");
  AddTitle("Marmor, Stein und Eisen bricht","Drafi Deutscher","south");
  AddTitle("Yesterday","Beatles","east");
  AddTitle("Beat It!","Michael Jackson","north");
  AddTitle("Blechbuechsen Roll","Dolls United","south");
  AddTitle("Don't Cry For Me Argentine","Madonna","east");
  AddTitle("Lemon Tree","Peter, Paul & Mary","west");
  AddTitle("Puff The Magic Dragon","Peter, Paul & Mary","south");
  AddTitle("Magic Carpet Ride","Steppenwolf","north");
  AddTitle("Don't Pay The Ferryman","Chris de Burgh","west");
  AddTitle("Where Peaceful Waters Flow","Chris de Burgh","south");
  AddTitle("Tubular Bells","Mike Oldfield","east");
  AddTitle("Tubular Bells II","Mike Oldfield","west");
  AddTitle("Help!","Beatles","north");
  AddTitle("We Can't Dance","Genesis","east");
  AddTitle("Eine Insel mit zwei Bergen","Dolls United","west");
  AddTitle("What's Love Got To Do With It","Tina Turner","north");
  AddTitle("Ave Maria","Luciano Pavarotti","north");
  AddTitle("Tonight","Jose Carreras","west");

  AddExit("south","./culture");
  AddRoomCmd("dance",SF(cmd_dance));
  AddItem("/std/board",
	  REFRESH_HOME,
	  ([
	    P_SHORT:"The Music Board",
	    P_LONG:("Here you can leave notes about the latest devellopment "
		    "in the music business, CD reviews and star gossip.\nTry "
		    "\"help board\"!\n"),
	    "Groups":({"rec.music"})
	  ]) );
}
