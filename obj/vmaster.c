/*
** /obj/vmaster.c
**
** A master to handle virtual rooms and objects (see /doc/concepts/virtual)
**
** 17apr95 [t] Creation
** 12mar97 [t] Now you may also use "AddItem" with the properties-array.
**
** [t] Thragor@Nightfall
*/

#define OBJECT_FILE 0
#define OBJECT_PROPERTIES 1

#define SF(x) #'x //'

#include <stdrooms.h>
#include <moving.h>

inherit "/std/base";

private nosave mapping objects;

public mapping SetObjects(mapping m) 
//([<file>:<obfile>;<properties>])
{
  return objects = m;
}

public mapping QueryObjects()
{
  return (objects||m_allocate(0,2))+m_allocate(0,2);
}

public varargs mapping AddObject(string file,string obfile,mixed properties)
{
  if (!objects)
    objects = m_allocate(0,2);
  if (file)
    {
      if (file[<2..]!=".c")
        file+=".c";
      file = resolve_file(file);
    }

  objects[file,OBJECT_FILE] = resolve_file(obfile||"/std/thing.c");
  if (!pointerp(properties))
    properties = ({properties})-({0});
  if (objects[file,OBJECT_PROPERTIES])
    objects[file,OBJECT_PROPERTIES] += properties;
  else
    objects[file,OBJECT_PROPERTIES] = properties;
  return objects;
}

public varargs status QueryObject(string file,string obfile,mixed properties)
{
  mapping obs;
  obs = QueryObjects();
  if (!member(obs,file)) return 0;
  obfile = obs[file,OBJECT_FILE];
  properties = obs[file,OBJECT_PROPERTIES];
  return 1;
}

public object compile_object(string file)
{
  mixed properties,obfile;
  object ob;
  int i;
  string err;

  seteuid(getuid());  
  
  if (file[<2..]!=".c")
    file+=".c";
  file = implode(explode(object_name(this_object()), "/")[0..<2], "/")+"/"+file;
  if (!QueryObject(file,&obfile,&properties))
    return 0;
  if (closurep(obfile))
    {
      obfile = funcall(obfile,file);
      if (objectp(obfile))
        ob = obfile;
    }
  if (!ob)
    ob = clone_object(obfile);
  for (i=0;i<sizeof(properties);i++)
    if (mappingp(properties[i]))
      ob->SetProperties(properties[i]);
    else
      apply(SF(call_other),ob,properties[i][0],properties[i][1..]);
  ob->SetIsVirtual(1);
  return ob;
}

private status clean_room(object ob)
{
  if (ob && query_once_interactive(ob))
    {
      if (interactive(ob))
	tell_object(ob,"Everything dissolves.\n");
      ob->move(VOID,M_GO);
      return 0;
    }
  return 1;
}

public varargs int remove()
{
  string *files;
  object ob;
  int i;
  files = m_indices(QueryObjects());
  for(i=sizeof(files);i--;)
    if (   (ob = find_object(files[i]))
        && !is_clone(ob))
      {
	filter(deep_inventory(ob),SF(clean_room));
	ob->remove();
      }
  return (::remove());
}

public varargs int clean_up(int arg)
{
  string *files;
  object ob;
  int i,res;
  files = m_indices(QueryObjects());
  res = 0;
  for(i=sizeof(files);i--;)
    if (   (ob = find_object(files[i]))
        && !is_clone(ob))
      res = res || ob->clean_up(arg);
  if (res)
    return 1;
  (::remove());
  return 0;
}
