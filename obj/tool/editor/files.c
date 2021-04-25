 
edit(arg) {
   if (!holding()) return 0;
   if (security()) return 1;
   if (is_changed) {
      write("Current buffer is not saved!  Either SAVE or DISCARD.\n");
      return 1;
   }
   object_name = "/secure/master"->
     valid_write(arg,getuid(this_player()),"ed_start");
   if (!object_name) {
      write("You don't have write access to this file.\n");
      return 1;
   }
   object_name = "/" + object_name;
   buf_init();
   if ( insert_file(object_name,0) )
      write("Now editing: " + object_name + "\n" );
   is_changed = 0;
   return 1;
}
 
static insert_file(file, where) {
   int read_size, read_line, line_count;
   string str;
   if (file_size(file) < 0 ) {
      write("File is not readable.\n");
      return 1;
   }
   read_size = 500;  read_line = 1;  line_count = 0;
   while( read_size == 500 ) {
      str = read_file(file,read_line,500);
      if (str) {
        insert_buf = explode( str , "\n" );
         insert_buf = insert_buf[0..<2];
         buf_insert( where );
      }
      where += 500;  read_line += 500;
      read_size = sizeof(insert_buf);
      write("Read "+read_size+" more lines.\n");
      line_count = line_count + read_size;
   }
   write( line_count + " lines read.\n");
   return 1;
}   
          
save(arg) {
   int lines;
   string save_buf;
 
   if (!holding()) return 0;
   if (security()) return 1;
   if (!object_name) {write("No filename selected.\n"); return 1;}
   if (file_size(object_name) >= 0)
      rm(object_name);
   lines = last_line();
   while (last_line() >= 500) {
      save_buf = buf[0][1..500];
      buf_delete( 1,500 );
      save_buf = implode( save_buf , "\n" ) + "\n" ;
      write_file( object_name , save_buf );
   }
   if (last_line()) {
      save_buf = buf[0][1..last_line()];
      buf_delete( 1,last_line() );
      save_buf = implode( save_buf , "\n" ) + "\n" ;
      write_file( object_name, save_buf );
   }
   write("File saved: " + object_name + " (" + lines + " lines)\n" );
   object_name = 0;
   buf_init();     /* just to clear up memory */
   return 1;
}
 
