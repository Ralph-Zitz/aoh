/*
** A door for the mazemaster
**
** Author: Thragor@Nightfall, 09may95
*/

#include <doors.h>

inherit "/obj/door";

private nosave string Pmaster;

public string SetMaster(string m) { return Pmaster = m; }
public string QueryMaster() { return Pmaster; }

public mixed QueryOtherRoom()
{
  mixed dest;
  
  if (!Pmaster||!QueryDoorRoom()) return 0;
  dest = ({mixed})Pmaster->GetExitDest(QueryDoorRoom(),QueryDirection());
  if (stringp(dest))
    return load_object(dest);
  if (!pointerp(dest)
      ||!sizeof(dest)==2
      ||!sizeof(dest[1])==3)
    return 0;
  return ({mixed})Pmaster->create_room(dest[0],dest[1]);
}
