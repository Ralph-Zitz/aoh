      
static buf_init() {
   is_changed = 0;
   cur_line = 0;
   buf = ({ ({ 0 }) });
}
                    
static buf_set(num,str) { 
   buf[num / 1000][num % 1000] = str;
   is_changed = 1;
}
static buf_read(num) {
   return buf[num / 1000][num % 1000];
}
 
static buf_high() { return (sizeof(buf            )-1); }
static buf_low()  { return (sizeof(buf[buf_high()])-1); }
 
static last_line() {return ( (1000*buf_high()) + buf_low() ); }
 
static buf_insert(location) {
   int loc_high,loc_low,size;
   string part1,part2;
   loc_high = location / 1000;
   loc_low  = location % 1000;
   size = sizeof(buf[loc_high]); 
   part1 = buf[loc_high][0..loc_low];
   part2 = buf[loc_high][loc_low+1..(size-1)];
   buf = buf[0..(loc_high-1)] + ({ part1 }) +
         ({ insert_buf }) + ({ part2 }) +
         buf[(loc_high+1)..(sizeof(buf)-1)];
   buf_fix();
}
 
static buf_fix() {
   int index,size_this,size_next,need;
   
   index = 0;
   while ( index < (sizeof(buf)-1) ) {
      size_this = sizeof(buf[index]);
      size_next = sizeof(buf[index+1]);
      if (size_this<1000) {
         need = 1000-size_this;
         if (need < size_next) {
            buf[index]   = buf[index] + buf[index+1][0..(need-1)];
            buf[index+1] = buf[index+1][need..(size_next-1)];
            index++;
         }
         else {
            buf[index] = buf[index] + buf[index+1];
            buf = buf[0..index] + buf[index+2..(sizeof(buf)-1)];
         }
      }
      else 
         index++;
   }
   is_changed = 1;
}
 
static buf_delete(start,end) {
   int start_hi,start_lo,end_hi,end_lo;
   string part1,part2;
 
   start_hi = start / 1000; start_lo = start % 1000;
   end_hi   = end / 1000;   end_lo   = end % 1000;
   part1 = buf[start_hi][0..(start_lo-1)];
   part2 = buf[end_hi][end_lo+1..sizeof(buf[start_hi])+1];
   buf = buf[0..(start_hi-1)] + ({ part1 }) + ({ part2 }) +
         buf[(end_hi+1)..(sizeof(buf)-1)];
   buf_fix();
}
 
static buf_copy(start,end,location) {
   int loc_hi, loc_lo ,st_hi, st_lo, end_hi, end_lo, size;
   string part1, part2, sub_part1, sub_part2, move_buf;
  
   loc_hi = location / 1000;  loc_lo = location % 1000;
   st_hi  = start / 1000;     st_lo  = start % 1000;
   end_hi = end / 1000;       end_lo = end % 1000; 
 
   size = sizeof(buf[loc_hi]);
   part1     = buf[0..(loc_hi-1)];
   part2     = buf[(loc_hi+1)..(sizeof(buf)-1)];
   sub_part1 = ({ buf[loc_hi][0..loc_lo]            });
   sub_part2 = ({ buf[loc_hi][(loc_lo+1)..(size-1)] });
   if (st_hi == end_hi)
      move_buf = ({ buf[st_hi][st_lo..end_lo] });
   else
      move_buf =
         ({ buf[st_hi][st_lo..(sizeof(buf[st_hi])-1)] }) +
         buf[(st_hi+1)..(end_hi-1)] +
         ({ buf[end_hi][0..end_lo] }) ;
   buf = part1 + sub_part1 + move_buf + sub_part2 + part2;
   buf_fix();
}
 
static buf_move(start,end,location) {
   int rem_start, rem_end;
 
   buf_copy( start, end, location );
   if (location < start) {
      rem_start = start + (end-start+1);
      rem_end   = end   + (end-start+1);
      buf_delete( rem_start, rem_end ); 
   }
   else
      buf_delete( start, end );
}
 
