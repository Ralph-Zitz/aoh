
change(arg) {
   int which;
   if (!holding()) return 0;
   if (security()) return 1;
   if (!object_name) {write("No file name selected.\n"); return 1;}
   if (!arg) {
      write("Please give a line number to change.\n");
      return 1;
   }
   which = get_line( arg );
   if ( which < 1 ) return 1;
   cur_line = which;
   write("OLD\n");
   write( align_num(cur_line) + buf_read(cur_line) + "\n");
   write("NEW\n");
   write( align_num(cur_line) );
   input_to("change_line");
   return 1;
}
 
static change_line(arg) {
   if (arg==".") {
      write("Line NOT changed.\n");
      return 1;
   }
   write("Line changed.\n");
   buf_set(cur_line,arg);
}
 
