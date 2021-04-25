 
insert(arg) {
   string file, str;
   if (!holding()) return 0;
   if (security()) return 1;
   if (!object_name) {write("No file name selected.\n"); return 1;}
   if (!arg) {
      write("Please give a line number to insert after.\n");
      return 1;
   }
   if (sscanf(arg,"file %s at %s",file,str) == 2) {
      if (str == "start") insert_where = 0;
      else                insert_where = get_line(str);
      if (insert_where == -1) return 1;
      insert_file(file,insert_where);
      return 1;
   }
   if (arg == "start") insert_where = 0;
   else                insert_where = get_line(arg);
   if (insert_where != -1) {
      cur_line = insert_where;
      insert_buf = ({ });
      write( align_num(cur_line+1) );
      input_to("insert_loop");
   }
   return 1;
}
 
static insert_loop(arg) {
   int temp_line;
 
   if (arg==".") {
      buf_insert(insert_where);
      insert_buf = ({ });
      return 1;
   }
   cur_line++;
   insert_buf = insert_buf + ({ arg });
   if (sizeof(insert_buf) == 1000) {
      buf_insert(insert_where);
      insert_where = cur_line;
      insert_buf = ({ });
   }
   write( align_num(cur_line+1) );
   input_to("insert_loop");
   return 1;
}
 
