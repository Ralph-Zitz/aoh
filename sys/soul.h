#ifndef _SOUL_H_
#define _SOUL_H_

#define NOSTY(x)        if(find_player("nostradamus")) \
                        tell_object(find_player("nostradamus"),x)
#define DEBUG 0
#define TP              this_player()
#define HAVE_SPRINTF
#define replace(X,Y,Z)  implode(my_explode(X,Y),Z)
#define MOVE(X,Y)       move_object(X, Y)
#define PNAME(X)        lower_case(({string})(X) -> QueryName() || "")
#define CAP_NAME(X)     ({string})((X) -> QueryName())
#define ISLIVING(X)     living( X )
#define ISVISIBLE(X)    (({int})(X) -> QueryInvis() == 0)
#define FORCE_SELF(X)   command( ( X ) )
#define POSSESSIVE(X)   ({string})(X) -> QueryPossessive()
#define OBJECTIVE(X)    ({string})(X) -> QueryObjective()
#define PRONOUN(X)      ({string})(X) -> QueryPronoun()
#define MORE_ROWS()     (({int})TP -> QueryPageSize())
#define MORE_WIDTH()    (({int})TP -> QueryPageWidth())
#define SORT(X)         sort_array(X, #'letterorder /*'*/)
#define SUB_ARRAY(X,Y)  ((X) - (Y))
#define WRITE(X)        TELL_OBJECT(TP, X)

#define SIMP 0
#define DEFA 1
#define DEUX 2
#define PERS 3
#define QUAD 4
#define PREV 5
#define SHRT 6
#define PHYS 7
#define FULL 8
#define SPACES "                                                            "

#endif /* _SOUL_H_ */
