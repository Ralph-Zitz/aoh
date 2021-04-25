/* proptest.c -- Mateese, 07.Jan.98
 *
 * Small object to evaluate the computational costs
 * of several methods of property access.
 *
 * To start a test, call 'test(<no of iterations>)'.
 * Timing results are given as <success> (<failure>) timings,
 * each timing showing <no of ticks>:<millisecs> figures.
 */

/*------------------------------------------------------------*/

/* Helper macro to extract the time in [ms] from the rusage
 * data.
 */

#define MILLISEC (rusage()[0])

/* Our 'properties' mapping */
private mapping mProps;

/* The string variable for the result text */
private static string sMsg;

/* Number of iterations */
private static int iIter;

/*------------------------------------------------------------*/
/* The set of property access functions and friends */

// Access method 2a: Query the mapping
public mixed Query(string name) { return mProps[name]; }

// Access Method 2b: Indirect mapping access
public mixed QueryIValue() { return Query("Value"); }

// Access Method 1a: Direct value access
public mixed QueryValue() { return sMsg; }

// Access Method 1b: Direct value access, stored in a mapping
public mixed QueryMValue() { return mProps["Value"]; }

// Filterfunction
public mixed FilterValue(mixed data) { return data; }

// Access method 3a: Query the mapping, then filter
public mixed QueryFilter(string name) {
  mixed data, rc;
  data = mProps[name];
  call_resolved(&rc, this_object(), "Filter"+name, &rc);
  return rc;
}

// Access method 3b: Query the mapping, then filter w/o filterfunction
public mixed QueryDontFilter(string name) {
  mixed data, rc;
  data = mProps[name];
  call_resolved(&rc, this_object(), "DontFilter"+name, &rc);
  return data;
}

// Access method 4: Try the direct method first, then the mapping
public mixed QueryBoth(string name) {
  mixed rc;
  if (call_resolved(&rc, this_object(), "Query"+name))
    return rc;
  return mProps[name];
}

/*------------------------------------------------------------*/
/* Setup the vars */

public void create() {
  mProps = ([ "Value": "test", "MapValue" : "test2" ]);
}

/*------------------------------------------------------------*/
/* The test and its stages */

private void stage1();
private void stage2();
private void stage3();
private void stage4();

public void test ( int iter )
{
  if (iter < 0) {
    write("Illegal number of iterations.\n");
    return;
  }
  iIter = iter;
  sMsg = "";
  stage1();
}

private void done()
{
  write("\nProperties tested over "+iIter+" iterations:\n"
        +sMsg);
}

/*------------------------------------------------------------*/
private void stage1() {
  int eval0, eval1, eval2;
  int time0, time1, time2;
  int i;

  call_out(#'stage2, 0);

  write("Stage1...\n");

  /* Test Method 1a */
  eval0 = get_eval_cost();
  time0 = MILLISEC;
  for (i = iIter; i--; ) {
    this_object()->QueryValue();
  }
  eval1 = get_eval_cost();
  time1 = MILLISEC;
  for (i = iIter; i--; ) {
    this_object()->QueryNoValue();
  }
  eval2 = get_eval_cost();
  time2 = MILLISEC;
  sMsg += sprintf("1a: %30s: %6d:%6dms (%6d:%6dms)\n"
                 , "Direct Call"
                 , eval0-eval1, time1-time0
                 , eval1-eval2, time2-time1
                 );


  /* Test Method 1b */
  eval0 = get_eval_cost();
  time0 = MILLISEC;
  for (i = iIter; i--; ) {
    this_object()->QueryMValue();
  }
  eval1 = get_eval_cost();
  time1 = MILLISEC;
  for (i = iIter; i--; ) {
    this_object()->QueryMNoValue();
  }
  eval2 = get_eval_cost();
  time2 = MILLISEC;
  sMsg += sprintf("1b: %30s: %6d:%6dms (%6d:%6dms)\n"
                 , "Direct Call/Mapping"
                 , eval0-eval1, time1-time0
                 , eval1-eval2, time2-time1
                 );

  /* Test Method 2a */
  eval0 = get_eval_cost();
  time0 = MILLISEC;
  for (i = iIter; i--; ) {
    this_object()->Query("Value");
  }
  eval1 = get_eval_cost();
  time1 = MILLISEC;
  for (i = iIter; i--; ) {
    this_object()->Query("NoValue");
  }
  eval2 = get_eval_cost();
  time2 = MILLISEC;
  sMsg += sprintf("2a: %30s: %6d:%6dms (%6d:%6dms)\n"
                 , "Direct Mapping"
                 , eval0-eval1, time1-time0
                 , eval1-eval2, time2-time1
                 );

  /* Test Method 2b */
  eval0 = get_eval_cost();
  time0 = MILLISEC;
  for (i = iIter; i--; ) {
    this_object()->QueryIValue();
  }
  eval1 = get_eval_cost();
  time1 = MILLISEC;
  for (i = iIter; i--; ) {
    this_object()->QueryINoValue();
  }
  eval2 = get_eval_cost();
  time2 = MILLISEC;
  sMsg += sprintf("2b: %30s: %6d:%6dms (%6d:%6dms)\n"
                 , "Indirect Mapping"
                 , eval0-eval1, time1-time0
                 , eval1-eval2, time2-time1
                 );

  write("Stage1 done.\n");
}

/*------------------------------------------------------------*/
private void stage2() {
  int eval0, eval1, eval2;
  int time0, time1, time2;
  int i;

  call_out(#'stage3, 0);

  write("Stage2...\n");

  /* Test Method 3a */
  eval0 = get_eval_cost();
  time0 = MILLISEC;
  for (i = iIter; i--; ) {
    this_object()->QueryFilter("Value");
  }
  eval1 = get_eval_cost();
  time1 = MILLISEC;
  for (i = iIter; i--; ) {
    this_object()->QueryFilter("NoValue");
  }
  eval2 = get_eval_cost();
  time2 = MILLISEC;
  sMsg += sprintf("3a: %30s: %6d:%6dms (%6d:%6dms)\n"
                 , "Filter"
                 , eval0-eval1, time1-time0
                 , eval1-eval2, time2-time1
                 );

  write("Stage2 done\n");
}

/*------------------------------------------------------------*/
private void stage3() {
  int eval0, eval1, eval2;
  int time0, time1, time2;
  int i;

  call_out(#'stage4, 0);

  write("Stage3...\n");

  /* Test Method 3b */
  eval0 = get_eval_cost();
  time0 = MILLISEC;
  for (i = iIter; i--; ) {
    this_object()->QueryDontFilter("Value");
  }
  eval1 = get_eval_cost();
  time1 = MILLISEC;
  for (i = iIter; i--; ) {
    this_object()->QueryDontFilter("NoValue");
  }
  eval2 = get_eval_cost();
  time2 = MILLISEC;
  sMsg += sprintf("3b: %30s: %6d:%6dms (%6d:%6dms)\n"
                 , "Filter (unresolved)"
                 , eval0-eval1, time1-time0
                 , eval1-eval2, time2-time1
                 );

  write("Stage3 done\n");
}

/*------------------------------------------------------------*/
private void stage4() {
  int eval0, eval1, eval2;
  int time0, time1, time2;
  int i;

  call_out(#'done, 0);

  write("Stage4...\n");

  /* Test Method 4 */

  eval0 = get_eval_cost();
  time0 = MILLISEC;
  for (i = iIter; i--; ) {
    this_object()->QueryBoth("Value");
  }
  eval1 = get_eval_cost();
  time1 = MILLISEC;
  for (i = iIter; i--; ) {
    this_object()->QueryBoth("NoValue");
  }
  eval2 = get_eval_cost();
  time2 = MILLISEC;
  sMsg += sprintf("4a: %30s: %6d:%6dms (%6d:%6dms)\n"
                 , "OSB mixed (hard)"
                 , eval0-eval1, time1-time0
                 , eval1-eval2, time2-time1
                 );

  eval0 = get_eval_cost();
  time0 = MILLISEC;
  for (i = iIter; i--; ) {
    this_object()->QueryBoth("MapValue");
  }
  eval1 = get_eval_cost();
  time1 = MILLISEC;
  for (i = iIter; i--; ) {
    this_object()->QueryBoth("NoMapValue");
  }
  eval2 = get_eval_cost();
  time2 = MILLISEC;
  sMsg += sprintf("4b: %30s: %6d:%6dms (%6d:%6dms)\n"
                 , "OSB mixed (soft)"
                 , eval0-eval1, time1-time0
                 , eval1-eval2, time2-time1
                 );

  write("Stage4 done\n");
}

/*============================================================*/
