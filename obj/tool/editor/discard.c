 
discard(arg) {
   if (!holding()) return 0;
   if (security()) return 1;
   if (!object_name) {write("Nothing to discard!\n"); return 1;}
   buf_init();
   object_name = 0;
   write("All changes have been discarded.\n");
   return 1;
}
 
/* -----------------------------------------------------------------
static discard_check(arg) {
   arg = lower_case(arg);
   if (arg=="y") {
      buf_init();
      object_name = 0;
      write("All changes have been discarded.\n");
   }
   else if (arg=="n") 
      write("Not discarded.\n");
   else {
      write("Please answer 'y' or 'n'.\n");
      write("Discard all changes (y/n) ? ");
      input_to("discard_check");
   }
}
------------------------------------------------------------------ */
