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

inherit "/std/base";

private static mapping objects;

public mapping SetObjects(mapping m) 
//([<file>:<obfile>;<properties>])
{
  return objects = m;
}

public mapping QueryObjects()
{
  return objects;
}

public varargs mapping AddObject(string file,string obfile,mixed properties)
{
  if (!objects)
    objects = allocate_mapping(0,2);
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
  if (!member(objects,file)) return 0;
  obfile = objects[file,OBJECT_FILE];
  properties = objects[file,OBJECT_PROPERTIES];
  return 1;
}

public object compile_object(string file)
{
  mixed properties,obfile;
  object ob;
  int i,j;
  string err;

  seteuid(getuid());  
  
  if (!objects
      ||!member(objects,file))
    return 0;
  obfile = objects[file,OBJECT_FILE];
  if (closurep(obfile))
    {
      obfile = funcall(obfile,file);
      if (objectp(obfile))
        ob = obfile;
    }
  if (!ob)
    ob = clone_object(obfile);
  properties = objects[file,OBJECT_PROPERTIES];
  for (i=0;i<sizeof(properties);i++)
    if (mappingp(properties[i]))
      ob->SetProperties(properties[i]);
    else
      for(j=sizeof(properties[i]);i--;)
	apply(SF(call_other),ob,properties[i][0],properties[1..]);
  ob->SetIsVirtual(1);
  return ob;
}
