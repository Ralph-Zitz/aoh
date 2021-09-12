/*
** /obj/mazemaster.c
**
** A mazemaster which may me used to inherit and build easily own mazes.
**
** 10apr95 [t] Creation
**
** [t] Thragor@Nightfall
*/

#define TP this_player()
#define ME this_object()
#define STR "/lib/string"
#define LSF(x) symbol_function(x,ME)
#define MAZE(x) "/obj/"+x

#include <mazemaster.h> // P_MASTER, X, Y, Z, ...
#include <config.h>
#include <moving.h> // M_...
#include <rooms.h> // REFRESH_...
#include <properties.h> // P_...
#include <secure/wizlevels.h> // IS_IMMORTAL

private nosave mapping rooms;
private nosave status getexitcall; // communication between GetExitDest and
                                   // MovePlayer
private nosave status compile_error;

protected int *build_coor(int *coor)
// Return the co-ordinates in the correct form, i. e. an array
// of size 3 with the co-ordinates ({x,y,z})
{
  if (!pointerp(coor))
    return 0;
  switch(sizeof(coor))
    {
     case 0:
      coor = ({0,0,0});
      break;
     case 1:
      coor += ({0,0});
      break;
     case 2:
      coor += ({0});
      break;
     case 3:
      break;
     default:
      return 0;
    }
  return coor;
}

public mapping SetRooms(mapping m) 
// Set all rooms of the maze
{
  return rooms = m;
}

public mapping QueryRooms()
{
  return rooms;
}

public mixed QueryRoomProps(string label,int *coor)
// Return the properties of a maze.
{
  if (!(coor = build_coor(coor))
      ||!rooms
      ||!member(rooms,label)
      ||!member(rooms[label],coor[Z])
      ||!member(rooms[label][coor[Z]],coor[Y])
      ||!member(rooms[label][coor[Z]][coor[Y]],coor[X]) )
    return 0;
  return rooms[label][coor[Z]][coor[Y]][coor[X],ROOM_PROPERTIES];
}

public varargs mapping AddRoom(string label,int *coor,mixed roomfile,
                               mixed properties)
// Add a new room to the maze.
{
  if (!coor = build_coor(coor)) return 0;
  if (!rooms)
    rooms = m_allocate(0,1);
  if (!rooms[label]) // here will the label be handled later on
    rooms[label] = m_allocate(0,1);
  if (!member(rooms[label],coor[Z]))
    rooms[label][coor[Z]] = m_allocate(0,1);
  if (!member(rooms[label][coor[Z]],coor[Y]))
    rooms[label][coor[Z]][coor[Y]] = m_allocate(0,3);
  rooms[label][coor[Z]][coor[Y]][coor[X],ROOM_FILE] = roomfile||"/std/room";
  if (!pointerp(properties))
    properties = ({properties})-({0});
  if (rooms[label][coor[Z]][coor[Y]][coor[X],ROOM_PROPERTIES])
    rooms[label][coor[Z]][coor[Y]][coor[X],ROOM_PROPERTIES] += properties;
  else
    rooms[label][coor[Z]][coor[Y]][coor[X],ROOM_PROPERTIES] = properties;
  return rooms;
}

public object create_room(string label,int *coor)
// Clone a new room and set the properties.
{
  mixed properties,file;
  object ob;
  int i;
  string err;

  seteuid(getuid());  
  if (!(coor = build_coor(coor))
      ||!rooms
      ||!member(rooms,label)
      ||!member(rooms[label],coor[Z])
      ||!member(rooms[label][coor[Z]],coor[Y])
      ||!member(rooms[label][coor[Z]][coor[Y]],coor[X]) )
    return 0;
  if (!ob = rooms[label][coor[Z]][coor[Y]][coor[X],ROOM_OBJECT])
    {
      file = rooms[label][coor[Z]][coor[Y]][coor[X],ROOM_FILE];
      if (closurep(file))
	{
	  file = funcall(file,label,coor);
	  if (objectp(file))
            ob = file;
	}
      if (!ob)
        if (err = catch(ob = clone_object(file)))
          MASTER->runtime_error("create_room: Room (label: "+label+
                                ", X: "+coor[X]+
                                " Y: "+coor[Y]+
                                " Z: "+coor[Z]+") error while loading: "+err,
                                blueprint(ME),object_name(ME),-1);
      rooms[label][coor[Z]][coor[Y]][coor[X],ROOM_OBJECT] = ob;
      ob->Set(P_LABEL,label); // tell the room to which label it belongs (soft)
      ob->Set(P_COOR,coor); // tell the room where it is located (soft)
      properties = rooms[label][coor[Z]][coor[Y]][coor[X],ROOM_PROPERTIES];
      for (i=0;i<sizeof(properties);i++)
        ob->SetProperties(properties[i]);
    }
  return ob;
}

public varargs mixed MovePlayer(object pl,string label, int *coor,
                                int method, mixed extra)
{
  if (getexitcall)
    return ({label,coor});
  if (!pl) return 0;
  if (!coor = build_coor(coor))
    raise_error("MovePlayer: Wrong room-coordinates: "+
	        ({string})STR->mixed_to_string(coor)+"\n");
//    MASTER->runtime_error("MovePlayer: Wrong room-coordinates: "+
//                          STR->mixed_to_string(coor)+"\n",
//                          blueprint(ME),object_name(ME),-1);
  
  if (!rooms
      ||!member(rooms,label)
      ||!member(rooms[label],coor[Z])
      ||!member(rooms[label][coor[Z]],coor[Y])
      ||!member(rooms[label][coor[Z]][coor[Y]],coor[X]) )
     raise_error("MovePlayer: Room (label: "+label+
                 ", X: "+coor[X]+
                 " Y: "+coor[Y]+
                 " Z: "+coor[Z]+") not existant\n");
  if (IS_IMMORTAL(pl)&&interactive(pl))
    printf("Maze: Label: %s  X: %d Y: %d Z: %d\n",label,coor[X],coor[Y],
           coor[Z]);
  return ({int})pl->move(create_room(label,coor),method,
    extra?extra:(method==M_GO?query_verb():0));
}

public void scan_error(string file,int line)
{
  string *li,*fit;
  int i;
  file = blueprint(ME)[1..]+".c";

  li = last_instructions(100);
  fit = regexp(li,"^.* "+file+" line [0..9]*");
  if (sizeof(fit))
    for(i=member(li,fit[0]);i<sizeof(li);i++)
      if (sscanf(li[i],"%~s cstring0 %~sline %d",line))
        break;
  if (!line) line = -1;
}

public varargs void AddExit(string label, // room-label
                            int *coor,    // room-coor
                            mixed dir,  // the command to type
                            mixed dest,   // function, file or array of int
                            mixed obj)    // the object where to call the func
{
  mixed props;
  string tdir,file;
  int i,line;

  if (!coor = build_coor(coor))
    {
      scan_error(&file,&line);
      MASTER->log_error(object_name(ME),
        (!compile_error?" -- "+ctime()+" --\n":"")+
        file+" line "+line+" :"
          "AddExit: Wrong room-coordinates\n");
      compile_error = 1;
      coor = ({0,0,0});
    }

  if (!rooms
      ||!member(rooms,label)
      ||!member(rooms[label],coor[Z])
      ||!member(rooms[label][coor[Z]],coor[Y])
      ||!member(rooms[label][coor[Z]][coor[Y]],coor[X]) )
    AddRoom(label,coor,"/std/room");

  if (stringp(dest) || objectp(dest) || pointerp(dest))
    {
      if (!obj && stringp(dest))
        dest = resolve_file(dest);
      if (stringp(obj))
        obj = resolve_file(obj);
      else
        if (objectp(obj) && obj == ME)
          {
            dest = symbol_function(dest, ME);
            obj = 0;
          }
        else
          if (pointerp(dest))
	    if (!dest = build_coor(dest))
	      {
                scan_error(&file,&line);
                MASTER->log_error(object_name(ME),
                  (!compile_error?" -- "+ctime()+" --\n":"")+
                  file+" line "+line+" :"
                  "AddExit: Wrong dest-coordinates\n");
                compile_error = 1;
		dest = "/std/void";
	      }
	    else
	      {
		dest = lambda(0,({LSF("MovePlayer"),({#'this_player}),label,
                              '({dest[X],dest[Y],dest[Z]}),
                       M_GO}));
                obj = 0;
	      }
    }
  else
    if (closurep(dest))
      obj = 0;

  if (!pointerp(dir))
    dir = ({ dir });

  props = rooms[label][coor[Z]][coor[Y]][coor[X],ROOM_PROPERTIES];
  if (sizeof(props))
    props = props[<1];
  else
    {
      props = ([]);
      rooms[label][coor[Z]][coor[Y]][coor[X],ROOM_PROPERTIES] = ({0});
    }
  if (!member(props,P_EXITS))
    props[P_EXITS]=m_allocate(0,5);

  for (i = sizeof(dir); i-- > 0; )
    {
      tdir = dir[i];
      props[P_EXITS][tdir,EXIT_DEST] = dest;
      props[P_EXITS][tdir,EXIT_OBJ] = obj;
    }
  rooms[label][coor[Z]][coor[Y]][coor[X],ROOM_PROPERTIES][<1]=props;
}

public mixed GetExitDest(object room,string dir)
// Needed for the doors to see where we may go to
{
  string label;
  int *coor,i;
  closure func;
  mixed props;
  mapping exits;

  if (!room) return 0;
  label = ({string})room->Query(P_LABEL);
  coor = ({int *})room->Query(P_COOR);
  i = sizeof(props =
             rooms[label][coor[Z]][coor[Y]][coor[X],ROOM_PROPERTIES]);
  exits = ([]);
  while(i--)
    exits+=props[i][P_EXITS]||([]);
  if (!closurep(func = exits[dir,EXIT_DEST]))
    return exits[dir,EXIT_DEST];
  getexitcall = 1;
  props = funcall(func);
  getexitcall = 0;
  return props;
}

public void HideExit(string label,int *coor,mixed *dirs,int new)
{
  mixed props;
  int i,line;
  string file;

  if (!coor = build_coor(coor))
    {
      scan_error(&file,&line);
      MASTER->log_error(object_name(ME),
        (!compile_error?" -- "+ctime()+" --\n":"")+
        file+" line "+line+" :"
        "HideExit: Wrong room-coordinates\n");
      compile_error = 1;
      coor = ({0,0,0});
    }

  if (!rooms
      ||!member(rooms,label)
      ||!member(rooms[label],coor[Z])
      ||!member(rooms[label][coor[Z]],coor[Y])
      ||!member(rooms[label][coor[Z]][coor[Y]],coor[X]) )
    AddRoom(label,coor,"/std/room");

  props = rooms[label][coor[Z]][coor[Y]][coor[X],ROOM_PROPERTIES];
  props = props[<1];
  if (!member(props,P_EXITS))
    props[P_EXITS]=m_allocate(0,5);

  if (stringp(dirs))
    dirs = ({dirs});
  for (i = sizeof(dirs); i-- > 0; )
    props[P_EXITS][dirs[i],EXIT_HIDDEN] = new;
  rooms[label][coor[Z]][coor[Y]][coor[X],ROOM_PROPERTIES][<1]=props;
}

public void AddDoor(string label,int *coor,
                    string dir, mixed dest,
                    mixed doorid, mixed doorlong,
                    mixed * keyid, int state
                   )
{
  string   doorobj,doorshort,file;
  mixed    keycode,props;
  int line;

  if (!coor = build_coor(coor))
    {
      scan_error(&file,&line);
      MASTER->log_error(object_name(ME),
        (!compile_error?" -- "+ctime()+" --\n":"")+
        file+" line "+line+" :"
        "AddDoor: Wrong room-coordinates\n");
      compile_error = 1;
      coor = ({0,0,0});
    }

  AddExit(label,coor,({ dir }), dest);
  props = rooms[label][coor[Z]][coor[Y]][coor[X],ROOM_PROPERTIES];
  props = props[<1];
  if (!member(props,P_EXITS))
    props[P_EXITS]=m_allocate(0,5);

  dest = props[P_EXITS][dir,EXIT_DEST];

  if (mappingp(doorid))
    {
      mapping dprops;
      mixed   ids;

      dprops = doorid;
      doorobj = to_string(doorlong || MAZE("mazedoor"));
      ids = ( (dprops[P_IDS] || ({ })) 
              + (({string *})doorobj->QueryIds() || ({}))
              + ({ dprops[P_DOORCODE] })
            ) - ({ 0, "" });
      if (sizeof(ids))
        dprops[P_IDS] = ids;
      if (!m_contains(&ids, dprops, P_OTHERROOM))
        dprops[P_OTHERROOM] = stringp(dest) ? dest : 0;
      dprops[P_DIRECTION] = dir;
      // The doorobject itself calls NewDoor().
      if (!member(props,P_ITEMS_REFRESH))
        props[P_ITEMS_REFRESH]=allocate(0);
      props[P_ITEMS_REFRESH]+=({({doorobj,REFRESH_REMOVE,
                                  ({dprops,([P_MASTER:object_name(ME)])}),0})});
      return;
    }

  // Compatibility for stupid doors.

  if (!keyid)
    {
      keycode = 0;
      keyid = 0;
    }
  else if (stringp(keyid))
    keyid = ({ keyid });
  else
    {
      keycode = keyid[0];
      keyid = keyid[1..];
    }
  doorshort = add_a(doorshort);
  props[P_ITEMS]+=({({MAZE("mazedoor"),REFRESH_REMOVE,
                      ([
		        P_SHORT:doorshort,
		        P_LONG:doorlong,
		        P_KEYIDS:keyid,
		        P_KEYCODE:keycode,
		        P_LOCKSTATE: state,
		        P_DIRECTION:dir,
		        P_DOORCODE: doorid,
		        P_OTHERROOM: stringp(dest) ? dest : 0,
                        P_ADS: explode(doorid, " ") - ({ "", "door" }) +
                               ({ dir }),
		        P_CHECKOBJ: ME,
                        P_MASTER:object_name(ME)
	             ])
                     ,0})});
}

public void clean_up() { }

public void remove()
{
  string *labels;
  int i;
  if (rooms)
    {
      i = sizeof(labels = m_indices(rooms));
      while(i--)
	{
	  int zc,*zs;
	  zc = sizeof(zs = m_indices(rooms[labels[i]]));
	  while(zc--)
	    {
	      int yc,*ys;
	      yc = sizeof(ys = m_indices(rooms[labels[i]][zs[zc]]));
	      while(yc--)
		{
		  int xc,*xs;
		  xc = sizeof(xs=m_indices(rooms[labels[i]][zs[zc]][ys[yc]]));
		  while(xc--)
		    if (rooms[labels[i]][zs[zc]][ys[yc]][xs[xc],ROOM_OBJECT])
		      rooms[labels[i]][zs[zc]][ys[yc]][xs[xc],ROOM_OBJECT]
                        ->remove();
		}
	    }
	}
    }
}

public void print_rooms()
{
  string *labels;
  int i;
  if (rooms)
  {
    i = sizeof(labels = m_indices(rooms));
    while(i--)
    {
      int zc,*zs;
      zc = sizeof(zs = m_indices(rooms[labels[i]]));
      while(zc--)
      {
        int yc,*ys;
        yc = sizeof(ys = m_indices(rooms[labels[i]][zs[zc]]));
        while(yc--)
        {
          int xc,*xs;
          xc = sizeof(xs = m_indices(rooms[labels[i]][zs[zc]][ys[yc]]));
          while(xc--)
          {
            write(labels[i]+" ("+xs[xc]+","+ys[yc]+","+zs[zc]+"): "+
              ({string})STR->mixed_to_string(
              rooms[labels[i]][zs[zc]][ys[yc]][xs[xc],
              ROOM_OBJECT])+"\n");
            write("Props: "+({string})STR->mixed_to_string(
              QueryRoomProps(labels[i],({xs[xc],ys[yc],zs[zc]})))+"\n");
		  }
		}
	  }
    }
  }
}
  
