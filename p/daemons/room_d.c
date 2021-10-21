#include <macros.h>
#include <msgclass.h>
#include <secure/config.h>
#define DB_PATH DAEMONSAVEPATH "room_ids.sqlite3"

private nosave int db_open;

#ifdef __SQLITE__
private mixed init_table();
private int insert_record(string fname);
#endif

void create() {
  seteuid(getuid());
#ifdef __SQLITE__
  db_open = sl_open(DB_PATH);
  init_table();
#else
  raise_error("Room Daemon requires __SQLITE__\n");
#endif
}

public int remove() {
#ifdef __SQLITE__
  if (db_open)
    sl_close();
#endif
  destruct(TO);
  return 1;
}

#ifdef __SQLITE__
private mixed init_table() {
  string create_t_statement =
    "CREATE TABLE IF NOT EXISTS room_ids ("
    "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "  filename TEXT UNIQUE NOT NULL"
    ");";
  string create_i_statement =
    "CREATE UNIQUE INDEX IF NOT EXISTS idx_room_ids_filename "
    "ON room_ids(filename);";
 return sl_exec(create_t_statement) + sl_exec(create_i_statement);
}

private int insert_record(string fname) {
  string insert_statement =
    "INSERT INTO room_ids (filename) "
    "VALUES (?1);";
  sl_exec(insert_statement, fname); 
  return sl_insert_id();
}

private int insert_id_record(int id, string fname) {
  string insert_statement =
    "INSERT INTO room_ids (id, filename) "
    "VALUES (?1, ?2);";
  sl_exec(insert_statement, id, fname); 
  return sl_insert_id();
}

private mixed *read_record(int|string ident) {
  string query;

  if (stringp(ident) && sizeof(ident) > 0) {
    query =
      "SELECT * FROM room_ids "
      "WHERE filename = ?1;";

  } else {
    query =
      "SELECT * FROM room_ids "
      "WHERE id = ?1;";
  }
  return sl_exec(query, ident);
}

public int InsertRoom(string|object name) {
  if (!objectp(name) && !stringp(name))
    raise_error(sprintf("Illegal parameter 'name' in call to InsertRoom(): %O\n", name));
  name = load_name(name);
  return insert_record(name);
}

public int InsertRoomID(int id, string|object name) {
  if (!id || id < 1)
    raise_error(sprintf("Illegal parameter 'id' in call to InsertRoomID(): %O\n", id));
  if (!objectp(name) && !stringp(name))
    raise_error(sprintf("Illegal parameter 'name' in call to InsertRoomID(): %O\n", name));
  name = load_name(name);
  return insert_id_record(id, name);
}

public int QueryRoomID(string|object|int name) {
  mixed * res;
  if (objectp(name))
    name = load_name(name);
  res = read_record(name);
  if (res && sizeof(res))
    return res[0][0];
  return 0;
}

public mixed * QueryRoomRecord(int|string|object ident) {
  if (objectp(ident))
    ident = load_name(ident);
  return read_record(ident);
}

#endif
