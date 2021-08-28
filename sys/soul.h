#ifndef _SOUL_H_
#define _SOUL_H_

#define NOSTY(x)        if(find_player("nostradamus")) \
                        tell_object(find_player("nostradamus"),x)
#define DEBUG 0
#define HAVE_SPRINTF
#define replace(X,Y,Z)  implode( my_explode( X, Y ), Z )
#define MOVE(X,Y)       move_object( X, Y )
#define PNAME(X)        lower_case( ({string})( X ) -> query_name() || "")
#define CAP_NAME(X)     ({string})( ( X ) -> query_name() )
#define ISLIVING(X)     living( X )
#define ISVISIBLE(X)    ( ({int})(X) -> query_invis() == 0 )
#define FORCE_SELF(X)   command( ( X ) )
#define POSSESSIVE(X)   ({string})(X) -> query_possessive()
#define OBJECTIVE(X)    ({string})(X) -> query_objective()
#define PRONOUN(X)      ({string})(X) -> query_pronoun()
#define MORE_ROWS()     (({int})this_player() -> query_screenheight() )
#define MORE_WIDTH()    (({int})this_player() -> query_screenwidth() )
#define SORT(X)         sort_array( ( X ), #'letterorder )
#define SUB_ARRAY(X,Y)  ( ( X ) - ( Y ) )
#define WRITE(X)        TELL_OBJECT( this_player(),( X ) )

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
