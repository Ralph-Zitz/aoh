/*   Another simple thing  (Mateese, 05-Jul-1994)
**
** A simple thing, but this time the descriptions are dynamic.
** This is done by /std/thing via process_string()ing the descriptions.
*/

#include <properties.h>     /* needed for setting value and light */

inherit "/std/thing";

/*----------------------------------------------------------------------------
** Initialize us 
*/

create() {

  ::create();

   /* Both long and short description are dynamic.
    * The long one consists of a static and a dynamic part, so the static
    * is set here.
    */
  Set(P_LONG,
 "This is just another simple thing (/d/archwiz/common/lpc/exm/xthing2.c).\n"
+"It has a weight of 100 gr, a value of 10 coins, and emits no light.\n"
         );

   /* We have a weight of 100 grams.
   ** Note that the old obj/treasure (implicitely) counted in kilograms.
   */
 Set(P_WEIGHT, 100);

    /* We also have a value of 10 coins.
    ** It could be set by 'SetValue(10)' but we do it directly just for fun.
    */
  Set(P_VALUE, 10);

    /* Now ensure that this key responds to the id "frob" as well
    ** as to the id "xthing".
    */
  AddId("frob");
  AddId("xthing");

    /* Emit no light. 
    ** Though this is the default, we show the method here.
    */
  Set(P_LIGHT, 0);
}

/*----------------------------------------------------------------------------
** Return a short description with a random color.
** Doing this with the Short()/QueryShort() combination would have been
** possible as well.
*/

QueryShort() {
  string rc;
  switch(random(4)) {
  case 0: rc = "red";    break;
  case 1: rc = "blue";   break;
  case 2: rc = "violet"; break;
  case 3: rc = "weird";  break;
  default: rc = "black";
  }
  return "a "+rc+" frob";

}

/*----------------------------------------------------------------------------
** Return the long description, extended by the current time.
*/

Long() {
  return ::Long()+"The actual time is "+ctime(time())[11..18]+".\n";
}

/*--------------------------------------------------------------------------*/
