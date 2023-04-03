//------------------------------------------------------------------------------
// Everybody needs a soul, or they will wind up being called Wayne or Garth
// Written by Profezzorn@nannymud
// Rewritten by Nostradamus for Dragonfire
// I guess I'm setting a new record in cpu-usage....
//
// future enhancements:
// think <person> <feeling>
// You think <person> <feeling>
// (Profezzorn thinks you smile.)
//
// Thanks goes to Xantrax (& Mav) @ Ultraworld for the TMI-2 mudlib support.
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Normally you shouldn't make objects that contains 'feelings' because they
// should be handled by /obj/soul. If you however feel that your feelings
// doesn't belong in the standard soul it is now possible to patch them into
// the standard soul with the use of the following functions in the soul.
//
//=============================================================================
// NAME
//	void add_verb(mapping m)
//
// DESCRIPTION
//	This function adds verbs to the soul. They will stay there until
//	removed or until soul is destructed. (ie. when player log off)
//	The argument m is a mapping of the same format as the one further
//	down in this program as the format is quite complex I will not
//	go into details here but advice you to learn from the numerous
//	examples in this program.
//
//	There is one special thing I'd like to mention though, if a verb
//	is defined as a string or an object, reduce_verb will be called
//	in that object instead of this object. (See reduce_verb) This
//	makes it possible to go outside the normal SIMP/DEFA etc., or
//	even disable certain verbs.
//
//=============================================================================
// NAME
//	void remove_verb(string *v)
//
// DESCRIPTION
//	Remove_verbs removes verbs previously added to the soul, note that
//	the standard verbs present from the beginning in the soul cannot
//	be changed nor removed.
//
//=============================================================================
// NAME
//	void add_adverb(string *v)
//
// DESCRIPTION
//	With this function you can add adverbs to the soul. It's as
//	simple as that. In all other resepects it works just as add_verbs.
//
//=============================================================================
// NAME
//	void remove_adverb(string *v)
//
// DESCRIPTION
//	With this function you can remove adverbs previously added.
//	Note that you can not remove adverbs that are there by default.
//
//=============================================================================
// NAME
//      string query_last_action()
//
// DESCRIPTION
//      This function is only valid during the call of a catch_tell()
//      from this object. It returns the feeling used last
//	with adverbs and persons expanded to their full names.
//
//=============================================================================
// NAME
//      mixed *query_brokendown_data()
//
// DESCRIPTION
//      This call is also only valid during calls of catch_tell() from this
//      object. It returns an easy-to-parse version of the command written
//      on the following format:
//
//      ({ meta-verb ,
//           ({ verb ,
//              ({ persons }),
//              ({ adverbs }),
//              message,
//              ({ where })
//           }),
//           ......
//      })
//
//=============================================================================
// NAME
//      mixed *query_feeling_for(object *who)
//
// DESCRIPTION
//      Works exactly like query_brokendown_data(), but returns
//      entries used towards the object who only.
//
//=============================================================================
//

#include <soul.h>
#include <msgclass.h>
#include <config.h>
#include <daemons.h>

public string globber_one_player(mapping ve);
public varargs string QueryName (mixed true_name); // /std/living/description

private nosave  object  remotePerson;
private nosave  string  morestring;
private nosave  string  last_action;
private nosave  string  parsed_part;
private nosave  string  SOUL;
private nosave  string  verb_string;
private nosave  string  adverb_string;
private nosave  string  unparsed_part;
private nosave  string  uncertain_part;
private nosave  string  total_list;
private nosave  string  channel_msg;
private nosave  mapping brokendown_on_person;
private nosave  mapping verbs;
private nosave  mapping adverbs;
private nosave  mapping how;
private nosave  mapping bodydata;
private nosave  mapping messages;
private nosave  mixed   *       brokendown_data;
private nosave  int     remoteFeel;
private         mapping xverbs;
private         mapping xadverbs;
private         string  xverb_string;
private         string  xadverb_string;

private string * my_explode( string a, string b )
{
    return explode( a, b );
}

private string fast_linebreak( string text, string pre, int width )
{
    return sprintf( "%s%-=*s", pre, width, text );
}

/* Standard sprintf has an internal buffer of 10000 bytes. To work around
 * this bug we call fast_linebreak a few times..... This also helps if you
 * don't have sprintf and are linebreaking something that has more words
 * than can be fitted into your arrays.
 */

private string big_linebreak( string s, string pre, int width )
{
    int e;
    string done = "", a, b;

    while ( sizeof( s ) )
    {
        if ( sizeof( s ) < 5000 )
        {
            e = 5000;
        }
        else
        {
            e = 5000;
            while ( e >= 0 && s[ e ] != ' ' )
                e--;
            if ( e == -1 )
                return done + "   " + s + "\n";
        }
        done += fast_linebreak( s[0..e-1], pre, width );
        pre   = SPACES[1..sizeof( pre )];
        s     = s[e+1..sizeof(s)-1];
        for( e = sizeof( done ) - 1; e >= 0 && done[ e ] != '\n'; e-- );
        a = 0;
        b = "";
        sscanf( s, "%s %s", a, b );
        while ( a && sizeof( a ) + sizeof( done ) - e <= width + 2 )
        {
            done += " " + a;
            s     = b;
            a     = 0;
            b     = "";
            sscanf( s, "%s %s", a, b );
        }
        done += "\n";
    }
    return done;
}

private string share_string( string s )
{
    return m_indices( ([ s : 0 ] ) )[ 0 ];
}

private varargs string implode_nicely( string * dum, string del )
{
    int s;

    if( !del )
        del = "and";

    switch( s = sizeof( dum ) )
    {
    case 0:
        return "";
    case 1:
        return dum[ 0 ];
    case 2:
        return dum[ 0 ] + " " + del + " " + dum[ 1 ];
    default:
        return implode( dum[0..s-2], ", ") + " " + del + " " + dum[s-1];
    }
}

private void mymore( string str )
{
    morestring += str;
}

public varargs void more_flush(string str)
{
    MORE->smore(morestring);
}

#if 0
public varargs void more_flush( string str )
{
    int rows = MORE_ROWS();
    string a;

    if ( rows < 2 )
        rows = 24;
    rows -= 2;

    if ( str == "q" )
    {
        morestring = "";
        return;
    }

    for( int e = 0; e < rows; e++ )
    {
        if ( sscanf( morestring, "%s\n%s", a, morestring ) )
        {
            write( a + "\n" );
        }
        else
        {
            write( morestring );
            e = 4711;
            morestring = "";
        }
    }

    if( sizeof( morestring ) )
    {
        write( "[Press return for more or 'q' to end] ");
        input_to( "more_flush" );
    }
}
#endif

protected mixed * query_brokendown_data( void )
{
    return brokendown_data;
}

protected string query_last_action( void )
{
    return last_action;
}

protected mixed * query_feeling_for( object o )
{
    /* this way might be slower, but it can take a lot of action */
    if( !brokendown_on_person )
    {
        mixed * tmp;
        brokendown_on_person = ([ ]);

        for( int e = 1; e < sizeof( brokendown_data ); e++ )
        {
            tmp = brokendown_data[ e ][ 1 ];

            for( int d = 0; d < sizeof( tmp ); d++ )
            {
                if( brokendown_on_person[ tmp[ d ] ] )
                    brokendown_on_person[ tmp[ d ] ] += ({ brokendown_data[ e ] });
                else
                    brokendown_on_person[ tmp[ d ] ] =
                    ({ brokendown_data[ 0 ], brokendown_data[ e ] });
            }
        }
    }
    return brokendown_on_person[ o ];
}

private void reset_last_action( void )
{
    last_action = brokendown_data = brokendown_on_person = 0;
}

private void set_last_action( string s )
{
    last_action = s + " ";
    brokendown_data = ({ s });
}

private mapping get_verbs( void )
{
    if (verbs)
        return verbs;
    return
    ([
      "flex":     ({DEUX,0," flex \nYOUR muscles \nHOW"," flexes \nYOUR muscles \nHOW"}),
      "fuck":     ({DEUX,0," fuck \nWHO \nHOW"," fucks \nWHO \nHOW"}),
      "orgasmon": ({DEUX,0," have a sloppy orgasm all over \nWHO",
        " has a sloppy orgasm all over \nWHO"}),
      "worm" : ({DEUX,0," do the worm \nHOW next to \nWHO",
        " does the worm \nHOW next to \nWHO"}),
      "orgasmin": ({DEUX,0," have a sloppy orgasm in \nWHO \nHOW",
        " has a sloppy orgasm in \nWHO \nHOW"}),
      "fingerfuck":({DEUX,0," finger \nWHO \nHOW"," fingers \nWHO \nHOW"}),
//      "jump":     ({DEUX,0," jump into \nPOSS arms", " jumps into \nPOSS arms"}),
      "curl":     ({SIMP,0," curl$ comfortably into \nPOSS arms"}),
      "oh":       ({DEUX,({"entirely"})," look \nHOW blank and go 'Oh'", " looks \nHOW "
        "blank and goes 'Oh'"}),

      "rage":     ({SIMP,0," rage$ around the room, slamming everyone against the wall! Ouch!"}),
      "snort":    ({SIMP,0," snort$ \nHOW \nAT"," at"}),
      "pant":     ({SIMP,({"heavily"})," pant$ \nHOW \nAT"," at"}),
      "hmm":      ({SIMP,0," hmm$ \nHOW \nAT"," at"}),
      "ack":      ({SIMP,0," ack$ \nHOW \nAT"," at"}),
      "guffaw":   ({SIMP,0," guffaw$ \nHOW \nAT"," at"}),
      "eraise":   ({SIMP,0," \nHOW raise$ an eyebrow \nAT"," at"}),
      "snap":     ({SIMP,0," snap$ \nYOUR fingers \nAT"," at"}),
      "tip":      ({SIMP,0," tip$ \nYOUR hat \nHOW \nAT", " to"}),
      "pee":      ({SIMP,0," \nHOW pee$ all over \nWHO \nWHERE"}),
      "lust":     ({DEFA,0,"", " for"}),
      "burp":     ({DEFA,({"rudely"}),""," at"}),
      "wink":     ({DEFA,({"suggestively"}),""," at"}),
      "smile":    ({DEFA,({"happily"}),""," at"}),
      "cricket":({DEUX,0," listen to the sound of crickets chirping near \nWHO", " listens to the sound of crickets chirping near \nWHO" }),
      "buzz":({DEUX,0," \nHOW buzz around \nWHO", " \nHOW buzzes around \nWHO"}),
      "robot":({DEUX,0," \nHOW suspect \nWHO of robotting", " \nHOW suspects \nWHO of robotting"}),
      "yawn":     ({DEFA,0,""," at"}),
      "swoon":    ({DEFA,({"romantically"}),""," at"}),
      "sneer":    ({DEFA,({"disdainfully"}),""," at"}),
      "beam":     ({DEFA,0,""," at"}),
      "point":    ({DEFA,0,""," at"}),
      "moo":    ({DEFA,0,""," at"}),
      "whee":   ({DEUX,0," \nHOW go 'WHEE!'"," \nHOW goes 'WHEE!'"}),
      "wtf":    ({DEUX,0," \nHOW go 'WTF!'"," \nHOW goes 'WTF!'"}),
      "grin":     ({DEFA,({"evilly"}),""," at"}),
      "laugh":    ({DEFA,0,""," at"}),
      "nog":      ({DEFA, 0, "", " towards"}),
      "nod":      ({DEFA,({"solemnly"}),""," at"}),
      "rofl":     ({SIMP,0," roll$ on the floor laughing \nHOW \nAT", " at" }),
      "wave":     ({DEFA,0,""," at"}),
      "cackle":   ({DEFA,({"gleefully"}),""," at"}),
      "chuckle":  ({DEFA,0,""," at"}),
      "bow":      ({DEFA,0,""," to"}),
      "surrender":({DEFA,0,""," to"}),
      "capitulate":({DEFA,({"unconditionally"}),""," to"}),
      "glare":    ({DEFA,({"stonily"}),""," at"}),
      "giggle":   ({DEFA,({"merrily"}),""," at"}),
      "groan":    ({DEFA,0,""," at"}),
      "grunt":    ({DEFA,0,""," at"}),
      "growl":    ({DEFA,0,""," at"}),
      "breathe":  ({DEFA,({"heavily"}),""," at"}),
      "argh":     ({DEFA,0,""," at"}),
      "scowl":    ({DEFA,({"darkly"}),""," at"}),
      "snarl":    ({DEFA,0,""," at"}),
      "recoil":   ({DEFA,({"with fear"}),""," from"}),
      "moan":     ({DEFA,0,""," at"}),
      "howl":     ({DEFA,({"in pain"}),""," at"}),
      "puke":     ({DEFA,0,""," on"}),
      "drool":    ({DEFA,0,""," on"}),
      "sneeze":   ({DEFA,({"loudly"}),""," at"}),
      "spit":     ({DEFA,0,""," on"}),
      "stare":    ({DEFA,0,""," at"}),
      "whistle":  ({DEFA,({"appreciatively"}),""," at"}),
      "applaud":  ({DEFA,0,"",""}),
      "leer":     ({DEFA,0,""," at"}),
      "agree":    ({DEFA,0,""," with"}),
      "believe":  ({PERS,0," believe$ in \nMYself \nHOW"," believe$ \nWHO \nHOW"}),
      "understand":({PERS,({"now"})," understand$ \nHOW"," understand$ \nWHO \nHOW"}),
      "disagree": ({DEFA,0,""," with"}),
      "fart":     ({DEFA,0,""," at"}),
      "dance":    ({DEFA,0,""," with"}),
      "flirt":    ({DEFA,0,""," with"}),
      "meow":     ({DEFA,0,"", " at"}),
      "bark":     ({DEFA,0,"", " at"}),
      "ogle":     ({PREV,0,""}),
      "pet":      ({SIMP,0," pet$ \nWHO \nHOW \nWHERE"}),
      "barf":     ({DEFA,0,"", " on"}),
      "purr":     ({DEFA,0,""," at"}),
      "curtsey":  ({DEFA,0,""," before"}),
      "puzzle":   ({SIMP,0," look$ \nHOW puzzled \nAT"," at"}),
      "grovel":   ({DEFA,0,""," before"}),
/*      "listen":   ({DEFA,0,""," to"}), */
      "tongue":   ({SIMP,0," stick$ \nYOUR tongue out \nHOW \nAT"," at"}),
      "apologize":({DEFA,0,""," to"}),
      "complain": ({DEFA,0,""," about"}),
      "rotate":   ({PERS,0 , " rotate$ \nHOW"," rotate$ \nWHO \nHOW"}),
      "excuse":   ({PERS,0," \nHOW excuse$ \nMYself"," \nHOW excuse$ \nMYself to \nWHO"}),
      "beg":      ({PERS,0," beg$ \nHOW"," beg$ \nWHO for mercy \nHOW"}),
      "fear":     ({PERS,0," shiver$ \nHOW with fear"," fear$ \nWHO \nHOW"}),
      "headshake":({SIMP,0," shake$ \nYOUR head \nAT \nHOW"," at"}),
      "shake":    ({SIMP,({"like a bowlful of jello"})," shake$ \nAT \nHOW",""}),
      "grimace":  ({SIMP,0," \nHOW make$ an awful face \nAT"," at"}),
      "stomp":    ({PERS,0," stomp$ \nYOUR foot \nHOW"," stomp$ on \nPOSS foot \nHOW"}),
      "snigger":  ({DEFA,({"jeeringly"}),""," at"}),
      "watch":    ({QUAD,({"carefully"})," watch the surroundings \nHOW",
        " watches the surroundings \nHOW",
        " watch \nWHO \nHOW",
        " watches \nWHO \nHOW",}),
      "scratch":  ({QUAD,({0,0,"on the head"}),
        " scratch \nMYself \nHOW \nWHERE",
        " scratches \nMYself \nHOW \nWHERE",
        " scratch \nWHO \nHOW \nWHERE",
        " scratches \nWHO \nHOW \nWHERE",
      }),
      "tap":      ({PERS,({"impatiently",0,"on the shoulder"})," tap$ \nYOUR foot \nHOW"," tap$ \nWHO \nWHERE"}),
      "wobble":  ({SIMP,0," wobble$ \nAT \nHOW",""}),
      "yodel":   ({SIMP,0," yodel$ a merry tune \nHOW",""}),

      /* Message-based verbs */
      "curse": ({PERS,0," curse$ \nWHAT \nHOW"," curse$ \nWHO \nHOW"}),
      "swear":  ({SIMP,0," swear$ \nWHAT \nAT \nHOW"," before"}),
      "criticize": ({PERS,0," criticize$ \nWHAT \nHOW"," criticize$ \nWHO \nHOW"}),
      // "lie":    ({PERS,0," lie$ \nMSG \nHOW"," lie$ to \nWHO \nHOW"}),
      "mutter": ({PERS,0," mutter$ \nMSG \nHOW"," mutter$ to \nWHO \nHOW"}),
      "say":   ({SIMP,({0,"'nothing"})," \nHOW say$ \nMSG \nAT"," to"}),
      "babble":({SIMP,({"incoherently","'something"})," babble$ \nMSG \nHOW \nAT"," to"}),
      "chant":  ({SIMP,({0,"Hare Krishna Krishna Hare Hare"})," \nHOW chant$: \nWHAT",""}),
      "sing":  ({SIMP,0," sing$ \nWHAT \nHOW \nAT"," to"}),
      // "go":    ({DEUX,({0,"ah"})," go \nMSG \nHOW"," goes \nMSG \nHOW"}),
      "hiss":  ({QUAD,0,
        " hiss \nMSG \nHOW"," hisses \nMSG \nHOW",
        " hiss \nMSG to \nWHO \nHOW"," hisses \nMSG to \nWHO \nHOW",
      }),
      "exclaim":  ({SIMP,0," \nHOW exclaim$ \nAT: \nWHAT!",""}),
      "quote":  ({SIMP,0," \nHOW quote$ \nAT \nMSG"," to"}),
      // "ask":   ({SIMP,0," \nHOW ask$ \nAT: \nWHAT?",""}),
      "mumble":({SIMP,0," mumble$ \nMSG \nHOW \nAT"," to"}),
      "murmur":({SIMP,0," murmur$ \nMSG \nHOW \nAT"," to"}),
      "scream":({SIMP,({"loudly"})," scream$ \nMSG \nHOW \nAT"," at"}),
      "yell":({SIMP,({"in a high pitched voice"})," yell$ \nMSG \nHOW \nAT"," at"}),
      "utter":({SIMP,({})," \nHOW utter$ \nMSG \nAT"," to"}),

      /* Verbs that require a person */
      "hide":        ({SIMP,0," hide$ \nHOW behind \nWHO"}),
      "ffinger":     ({SIMP,0," give$ \nWHO the finger"}),
      "high5":       ({SIMP,0," \nHOW slam$ a thunderous high-five with \nWHO" }),
      "mercy":       ({SIMP,0," beg$ \nWHO for mercy"}),
      "gripe":       ({PREV,0," to"}),
      "peer":        ({PREV,0," at"}),
      "remember":    ({SIMP,0," remember$ \nAT \nHOW",""}),
      "surprise":    ({PREV,0,""}),
      "pounce":      ({PHYS,({"playfully"}),""}),
      "bite":        ({PERS,0," \nHOW bite$ \nYOUR lip"," bite$ \nWHO \nHOW \nWHERE"}),
      "lick":        ({SIMP,0," lick$ \nWHO \nHOW \nWHERE"}),
      "caper":       ({PERS,({"merrily"})," caper$ \nHOW about"," caper$ around \nWHO \nHOW"}),
      "beep":        ({PERS, ({"triumphantly",0,"on the nose"}),
        " \nHOW beep$ \nMYself \nWHERE"," \nHOW beep$ \nWHO \nWHERE"}),
      "blink":       ({PERS,0," blink$ \nHOW"," blink$ \nHOW at \nWHO"}),
      "bonk":        ({PHYS,({0,0,"on the head"}),""}),
      "bop":         ({PHYS,({0,0,"on the head"}),""}),
      "stroke":      ({PHYS,({0,0,"on the cheek"}),""}),
      "hold":        ({PHYS,({0,0,"in \nYOUR arms"}),""}),
      "embrace":     ({PHYS,({0,0,"in \nYOUR arms"}),""}),
      "handshake":   ({SIMP,0," shake$ hands with \nWHO",""}),
      "tickle":      ({PREV,0,""}),
      "worship":     ({PREV,0,""}),
      "admire":      ({PREV,0,""}),
      "mock":        ({PREV,0,""}),
      "tease":       ({PREV,0,""}),
      "taunt":       ({PREV,0,""}),
      "strangle":    ({PREV,0,""}),
      "hate":        ({PREV,0,""}),
      "fondle":      ({PREV,0,""}),
      /* "dandle":      ({PREV,0,""}),   In what dictionary can I find this ? */
      "squeeze":     ({PREV,({"fondly"}),""}),
      "comfort":     ({PREV,0,""}),
      "nudge":       ({PHYS,({"suggestively"}),""}),
      "slap":        ({PHYS,({0,0,"in the face"}),""}),
      "hit":         ({PHYS,({0,0,"in the face"}),""}),
      "kick":        ({PHYS,({"hard"}),""}),
      "tackle":      ({SIMP,0," tackle$ \nWHO \nHOW",""}),
      "spank":       ({PHYS,({0,0,"on the butt"}),""}),
      "pat":         ({PHYS,({0,0,"on the head"}),""}),
      "punch":       ({DEUX,({0,0,"in the eye"})," punch \nWHO \nHOW \nWHERE"," punches \nWHO \nHOW \nWHERE"}),
      "hug":         ({PREV,0,""}),
      /* "snog":        ({PREV,0,""}), Not in any dictionary as far as I know */
      "want":        ({PREV,0,""}),
      "pinch":       ({DEUX,0," pinch \nWHO \nHOW \nWHERE"," pinches \nWHO \nHOW \nWHERE"}),
      "kiss":        ({DEUX,0," kiss \nWHO \nHOW \nWHERE"," kisses \nWHO \nHOW \nWHERE"}),
      "caress":      ({DEUX,({0,0,"on the cheek"})," caress \nWHO \nHOW \nWHERE"," caresses \nWHO \nHOW \nWHERE"}),
      "smooch":      ({DEUX,0," smooch \nWHO \nHOW"," smooches \nWHO \nHOW"}),
      "envy":        ({DEUX,0," envy \nWHO \nHOW"," envies \nWHO \nHOW"}),
      "touch":       ({DEUX,0," touch \nWHO \nHOW \nWHERE"," touches \nWHO \nHOW \nWHERE"}),
      "knee":        ({PHYS,({0,0,"where it hurts"}),""}),
      "love":        ({PREV,0,""}),
      "adore":       ({PREV,0,""}),
      "grope":       ({PREV,0,""}),
      "poke":        ({PHYS,({0,0,"in the ribs"}),""}),
      "snuggle":     ({PREV,0,""}),
      // Kneel disabled - because it's a bodyposition - Nosty
      // "kneel":       ({SIMP,0," \nHOW fall$ on \nYOUR knees \nAT"," in front of"}),
      "trust":       ({PREV,0,""}),
      "like":        ({PREV,0,""}),
      "greet":       ({PREV,0,""}),
      "welcome":     ({PREV,0,""}),
      "thank":       ({PREV,0,""}),
      "cuddle":      ({PREV,0,""}),
      "salute":      ({PREV,0,""}),
      "french":      ({SIMP,0," give$ \nWHO a REAL kiss, it seems to last forever"}),
      "nibble":      ({SIMP,0," nibble$ \nHOW on \nPOSS ear"}),
      "ruffle":      ({SIMP,0," ruffle$ \nPOSS hair \nHOW"}),
      "ignore":      ({PREV,0,""}),
      "forgive":     ({PREV,0,""}),
      "congratulate":({PREV,0,""}),
      "ayt":         ({SIMP,0," wave$ \nYOUR hand in front of \nPOSS face, \nIS \nSUBJ \nHOW there?"}),

      /* Verbs that don't need, nor use persons */
      "woo":    ({SIMP,({"loudly"})," woo$ \nHOW"}),
      "roll":   ({SIMP,({"to the ceiling"})," roll$ \nYOUR eyes \nHOW"}),
      "boggle": ({SIMP,0," boggle$ \nHOW at the concept"}),
      "cheer":  ({SHRT,({"enthusiastically"}),""}),
      "twiddle":({SIMP,0," twiddle$ \nYOUR thumbs \nHOW"}),
      "wiggle": ({SIMP,0," wiggle$ \nYOUR bottom \nAT \nHOW"," at"}),
      "wrinkle":({SIMP,0," wrinkle$ \nYOUR nose \nAT \nHOW"," at"}),
      "thumb":  ({SIMP,0," \nHOW suck$ \nYOUR thumb"}),
      "heh":    ({DEUX,0," smile \nHOW and go, Heh heh!", " smiles and \nHOW goes, Heh heh!" }),
      "flip":   ({SIMP,0," flip$ \nHOW head over heels"}),
      "cry":    ({DEUX,0," cry \nHOW"," cries \nHOW"}),
      "ah":     ({DEUX,0," go 'ah' \nHOW"," goes 'ah' \nHOW"}),
      "clear":  ({SIMP,0," clear$ \nYOUR throat \nHOW"}),
      "sob":    ({SHRT,0,""}),
      "lag":    ({SHRT,({"helplessly"}),""}),
      "whine":  ({SHRT,0,""}),
      "cringe": ({SIMP,({"in terror"})," cringe$ \nHOW"}),
      "sweat":  ({SHRT,0,""}),
      "gurgle": ({SHRT,0,""}),
      "grumble":({SHRT,0,""}),
      "panic":  ({SHRT,0,""}),
      "pace":   ({SIMP,({"impatiently"})," start$ pacing \nHOW"}),
      "pale":   ({SIMP,0," turn$ white as ashes \nHOW"}),
      "die":    ({DEUX,0," fall \nHOW down and play dead"," falls \nHOW to the ground, dead"}),
      "sleep":  ({DEUX,({"soundly"})," fall asleep \nHOW"," falls asleep \nHOW"}),
      "stumble":({SHRT,0,""}),
      "bounce": ({SHRT,0,""}),
      "sulk":   ({SHRT,({"in the corner"}),""}),
      "strut":  ({SHRT,({"proudly"}),""}),
      "sniff":  ({SHRT,0,""}),
      "snivel": ({SHRT,({"pathetically"}),""}),
      "snore":  ({SHRT,0,""}),
      "clue":   ({SIMP,0," need$ a clue \nHOW"}),
      "stupid": ({SIMP,0," look$ \nHOW stupid"}),
      "bored":  ({SIMP,0," look$ \nHOW bored"}),
      "snicker":({SHRT,0,""}),
      "smirk":  ({SHRT,0,""}),
      "squint": ({SHRT,0 ,""}),
      "huff":   ({SHRT,0 ,""}),
      "puff":   ({SHRT,0 ,""}),
      "fume":   ({SHRT,0 ,""}),
      "steam":  ({SHRT,0 ,""}),
      "choke":  ({SHRT,0 ,""}),
      "faint":  ({SHRT,0,""}),
      "shrug":  ({SHRT,0,""}),
      "pout":   ({SHRT,0,""}),
      "hiccup": ({SHRT,0,""}),
      "frown":  ({SHRT,0,""}),
      "gasp":   ({SHRT,({"in astonishment"}),""}),
      "think":  ({SHRT,({"carefully"}),""}),
      "ponder": ({SHRT,({"over some problem"}),""}),
      "wonder": ({DEFA,0,""," at"}),
      "clap":   ({SHRT,0,""}),
      "sigh":   ({SHRT,0,""}),
      "cough":  ({SHRT,({"noisily"}),""}),
      "shiver": ({SHRT,({"from the cold"}),""}),
      "tremble":({SHRT,0,""}),
      "twitch": ({DEUX,0," twitch \nHOW"," twitches \nHOW"}),
      "bitch":  ({DEUX,0," bitch \nHOW"," bitches \nHOW"}),
      "blush":  ({DEUX,0," blush \nHOW"," blushes \nHOW"}),
      "stretch":({DEUX,0," stretch \nHOW"," stretches \nHOW"}),
      "relax":  ({DEUX,0," relax \nHOW"," relaxes \nHOW"}),
      "duck":   ({PERS,0," duck$ \nHOW out of the way"," duck$ \nHOW out of \nPOSS way"}),

    ]);
}

private string * low_get_adverbs( void )
{
    string * q, f = efun::read_file(SOUL_ADVERBS);

    if ( f )
        q = my_explode( f, "\n" ) - ({ "" });
    else
        q = ({ });
    return q;
}

private mapping get_adverbs( void )
{
    string * q;

    if ( adverbs )
        return adverbs;
    q = low_get_adverbs();
    return mkmapping( q, q );
}

private mapping get_how( void )
{
    if ( how && sizeof( how ) )
        return how;
    return
    ([
      "very"     : 1,
      "quite"    : 1,
      "barely"   : 1,
      "extremely": 1,
      "somewhat" : 1,
      "almost"   : 1,
    ]);
}

private mapping get_bodydata( void )
{
    if (bodydata)
        return bodydata;
    return
    ([
      "hand" : "on the hand",
      "forehead" : "on the forehead",
      "head" : "on the head",
      "face" : "in the face",
      "hurts" : "where it hurts",
      "eye" : "in the eye",
      "ear" : "on the ear",
      "stomach" : "in the stomach",
      "butt" : "on the butt",
      "behind" : "on the behind",
      "leg" : "on the leg",
      "foot" : "on the foot",
      "toe" : "on the right toe",
      "nose" : "on the nose",
      "neck" : "in the neck",
      "back" : "on the back",
      "arm" : "on the arm",
      "chest" : "on the chest",
      "cheek" : "on the cheek",
      "side" : "in the side",
      "everywhere" : "everywhere",
      "shoulder" : "on the shoulder"
    ]);
}

private void TELL_OBJECT( object o, string s )
{
    if ( messages[ o ] )
        messages[ o ] += s;
    else
        messages[ o ] = s;
}

private void my_tell_room( object o, string s, object * a )
{
    if ( pointerp( a ) )
        a = SUB_ARRAY( all_inventory( o ), a );
    else
        a = all_inventory( o );
    map( a, #'TELL_OBJECT /*'*/, s );
}

private varargs void intsay( string s, object o )
{
    my_tell_room( environment(TP), s, ({ TP, o }) );
}

private string linebreak( string s )
{
    return big_linebreak( s, "   ", 75 );
}

private void flush( void )
{
    object * a = m_indices( messages );
    string * b = m_values( messages ), msg;
    int e      = sizeof( a );

    while ( e-- )
    {
        if ( a [ e ] )
        {
            msg = b[ e ];
            if( interactive( a[ e ] ) )
                msg = fast_linebreak( msg, "", 75 );
        }
        msg_object(a[e], CMSG_EMOTE, msg);
    }
    messages = ([ ]);
    reset_last_action();
}
/*
public void init( void )
{
   add_action( "do_feel", "", 1 );
   add_action( "do_emote", "emote" );
   add_action( "do_help", "help" );
   add_action( "fail", "fail" );
   add_action( "again", "again" );
   add_action( "dont", "don't" );
   add_action( "dont", "dont" );
   add_action( "feeling", "feeling" );
   add_action( "suddenly", "suddenly" );
}
*/
public void add_soul_cmds( void )
{
    add_action( "do_feel", "", 1 );
    add_action( "do_emote", "emote" );
    add_action( "do_help", "help" );
    add_action( "fail", "fail" );
    add_action( "again", "again" );
    add_action( "dont", "don't" );
    add_action( "dont", "dont" );
    add_action( "feeling", "feeling" );
    add_action( "suddenly", "suddenly" );
}

private int letterorder( string a, string b )
{
    return a > b;
}

private string get_verb_string( void )
{
    if ( verb_string )
        return verb_string;
    return verb_string =
    sprintf( "%-*#s\n", MORE_WIDTH(),
      implode( SORT( m_indices( verbs ) ), "\n" )
    );
}

private string get_adverb_string( void )
{
    if ( adverb_string )
        return adverb_string;
    return adverb_string =
    sprintf( "%-*#s\n", MORE_WIDTH(),
      implode( SORT( m_indices( adverbs ) ), "\n" )
    );
}

private string get_xverb_string( void )
{
    if ( xverb_string )
        return xverb_string;
    return xverb_string =
    sprintf( "%-*#s\n", MORE_WIDTH(),
      implode( SORT( m_indices( xverbs ) ), "\n" )
    );
}

private string get_xadverb_string( void )
{
    if ( xadverb_string )
        return xadverb_string;
    return xadverb_string =
    sprintf( "%-*#s\n", MORE_WIDTH(),
      implode( SORT( m_indices( xadverbs ) ), "\n" )
    );
}

public void create()
{
    SOUL           = "/std/npc/soul";
    verbs          = get_verbs();
    adverbs        = get_adverbs();
    how            = get_how();
    bodydata       = get_bodydata();
    messages       = ([ ]);
    xverbs         = ([ ]);
    xadverbs       = ([ ]);
    morestring     = "";
    xverb_string   = "";
    xadverb_string = "";
    channel_msg    = "";
}

public int do_emote( string s )
{
    if( !s || s == "" || !sizeof( s ) )
        return notify_fail("Emote what?\n", NOTIFY_ILL_ARG);
    msg_write(CMSG_EMOTE, "You emote: " +
      ({string})TP->QueryName() + " " + s + "\n"
    );
    msg_room(environment(TP),
      CMSG_EMOTE,
      ({string})TP->QueryName() + " " + s + "\n",
      ({ TP })
    );
    return 1;
}

public int do_help( string s )
{
    string res;

    if ( !s )
        return notify_fail("Help <topic>\n", NOTIFY_ILL_ARG);
    morestring = "";
    switch ( s )
    {
    case "feelings":
        mymore( "General commands available:\n" );
        mymore( get_verb_string() );
        if ( sizeof( xverbs ) )
        {
            mymore( "Extra commands available:\n" );
            mymore( get_xverb_string() );
        }
        mymore( "grades:\n" + linebreak( implode_nicely( m_indices( how ) ) ) );
        mymore( "All of these commands can be combinated with 'and' to make it\n");
        mymore( "possible to do several things in several ways to several "
          "people.\n"
        );
        mymore( "All feelings can also be prepended with: suddenly, fail, "
          "again or dont\n"
        );
        mymore( "Persons and adverbs can be shortened to shorted unique "
          "prefix.\n"
        );
        mymore( "See also: help adverbs and help feeling list\n");
        more_flush();
        return 1;
    case "adverbs":
        mymore( "Adverbs that can be used together with feeling-commands:\n" );
        mymore( get_adverb_string() );
        more_flush();
        if ( sizeof( xadverbs ) )
        {
            mymore( "Extra adverbs available:\n" );
            mymore( get_xadverb_string() );
            more_flush();
        }
        return 1;
    case "feeling list":
        if ( !( res = ({string})SOUL -> query_total_list() ) )
        {
            res = globber_one_player( verbs );
            TO -> set_total_list( res );
            if ( sizeof( xverbs ) )
                res += "<TRUNCATED> (try again)\n";
            else
                res += globber_one_player( xverbs );
        }
        res = "Verb          Short description\n" + res;
        mymore( res );
        more_flush();
        return 1;
    case "soul version":
        msg_write(CMSG_EMOTE, "Soul version 1.2, written by hubbe@lysator.liu.se.\n");
        return 1;
    }
    return 0;
}

private string get_name( object o )
{
    return PNAME( o );
}

/* This function should return 1 if the object is a player or monster
 * that is not invisible.
 */
private int isplay( object o )
{
    return ISLIVING( o ) && ISVISIBLE( o );
}

private object * get_persons( void )
{
    return filter( all_inventory( environment( TO ) ),
      #'isplay /*'*/
    );
}

private object * channel_listeners( string channel )
{
    return (({object *})CHANNEL_D -> ChannelListeners( channel ) || ({ }));
}

private mixed prefix( string * dum, string pr, string errm )
{
    string * q;
    int len = sizeof( pr );

    while ( len-- )
    {
        if ( ( pr[len] < 'a' || pr[len] > 'z') && pr[len] != ' ' )
            return 0;
    }
    q = regexp( dum,"^" + pr );
    if ( !pointerp( q ) || !sizeof( q ) )
        return 0; /* Brom 960415 */
    if( sizeof( q ) == 1 )
        return q[ 0 ];
    notify_fail( sprintf( "%s\n%-#79s\n", errm, implode( q, "\n" ) ) );
    return -1;
}

private string WHO( object o, object who )
{
    if ( who == o )
    {
        if ( o == TP )
            return "yourself";
        else
            return "you";
    }
    else
    {
        if ( o == TP )
            return OBJECTIVE( o ) + "self";
        else
            return CAP_NAME( o );
    }
}

private string POSS( object o, object who )
{
    if ( who == o )
    {
        if ( o == TP )
            return "your own";
        else
            return "your";
    }
    else
    {
        if ( o == TP )
            return POSSESSIVE( o ) + " own";
        else
        {
            string s = CAP_NAME( o );

            if ( s[<1] == 's' )
                return s + "'";
            else
                return s + "'s";
        }
    }
}

private string gloerp( string q, object * t, mixed who, int prev )
{
    string * s  = my_explode( q, "\n" );
    string mess = s[ 0 ];
    string b;

    for ( int e = 1; e < sizeof( s ); e++ )
    {
        if ( !prev && sscanf( s[ e ], "WHO%s", b ) )
        {
            mess += implode_nicely( map( t, #'WHO /*'*/, who ) ) + b;
        }
        else if ( !prev && sscanf( s[ e ], "POSS%s", b ) )
        {
            mess += implode_nicely( map( t, #'POSS /*'*/, who ) ) + b;
        }
        else if ( sscanf( s[ e ], "YOUR%s", b ) )
        {
            if( TP == who )
                mess += "your" + b;
            else
                mess += POSSESSIVE( TP ) + b;
        }
        else if ( sscanf( s[ e ], "YOU%s", b ) )
        {
            if( TP == who )
                mess += "you" + b;
            else
                mess += OBJECTIVE( TP ) + b;
        }
        else if ( sscanf( s[ e ], "MY%s", b ) )
        {
            if ( TP == who )
                mess += "your" + b;
            else
                mess += OBJECTIVE( TP ) + b;
        }
        else if ( sscanf( s[ e ], "PRON%s", b ) )
        {
            if ( TP == who )
                mess += "you" + b;
            else
                mess += PRONOUN( TP ) + b;
        }
        else if ( sscanf( s[ e ], "THEIR%s", b )
          ||
          ( prev && sscanf( s[ e ], "POSS%s", b ) ) )
        {
            if( sizeof( t ) > 1 )
            {
                if( member( t, who ) != -1 )
                    mess += "your" + b;
                else
                    mess += "their" + b;
            }
            else
            {
                if( t[ 0 ] == who )
                    mess += "your" + b;
                else
                    mess += POSSESSIVE( t[ 0 ] ) + b;
            }
        }
        else if ( sscanf( s[ e ], "OBJ%s", b )
          ||
          ( prev && sscanf( s[ e ], "WHO%s", b ) ) )
        {
            if ( sizeof( t ) > 1 )
            {
                if ( member( t, who ) !=-1 )
                    mess += "all of you" + b;
                else
                    mess += "them" + b;
            }
            else
            {
                if ( t[ 0 ] == who )
                {
                    if ( who == TP )
                        mess += "yourself" + b;
                    else
                        mess += "you" + b;
                }
                else
                {
                    if( t[ 0 ] == TP )
                        mess += OBJECTIVE( TP ) + "self" + b;
                    else
                        mess += OBJECTIVE( t[ 0 ] ) + b;
                }
            }
        }
        else if ( sscanf( s[ e ], "SUBJ%s", b ) )
        {
            if ( sizeof( t ) > 1 )
            {
                if ( member( t, who ) != -1 )
                    mess += "you" + b;
                else
                    mess += "they" + b;
            }
            else
            {
                if ( t[ 0 ] == who )
                    mess += "you" + b;
                else
                    mess += PRONOUN( t[ 0 ] ) + b;
            }
        }
        else if ( sscanf( s[ e ], "IS%s", b ) )
        {
            if ( member( t, who ) != -1 )
                mess += "are" + b;
            else if ( sizeof( t ) <= 1 )
                mess += "is" + b;
            else
                mess += "are" + b;
        }
        else
            mess += "\n" + s[ e ];
    }
    return mess;
}

private void make_channel_msg( string msg )
{
    channel_msg += msg;
}

private string query_channel_msg( void )
{
    return channel_msg;
}

/* Output parsed feeling to */
private void feel( mixed * d, int flag )
{
    int prev;
    mixed * q = ({ });
    for ( int e = 0; e < sizeof( d ); e++ )
    {
        prev = !sizeof( SUB_ARRAY( q, d[ e ][ 0 ] ) )
        &&
        !sizeof( SUB_ARRAY( d[ e ][ 0 ], q ) )
        &&
        -1 == member( q, TP );
        q = d[ e ][ 0 ];
        make_channel_msg( gloerp( d[ e ][ 3 + flag * 3 ], q, 0, prev ) );

        if(!remoteFeel)
            my_tell_room( environment( TP ),
              gloerp( d[ e ][ 3 + flag * 3 ], q , 0, prev ),
              q + ({ TP })
            );

        for ( int w = 0; w < sizeof( q ); w++ )
        {
            if( TP != q[ w ] && w == member( q, q[ w ] ) )
                TELL_OBJECT( q[ w ], gloerp( d[ e ][ 2 + flag * 3 ], q, q[ w ], prev )
                );
        }
        WRITE( gloerp( d[ e ][ 1 + flag * 3 ], q, TP, prev ) );
        switch( sizeof( d ) - e )
        {
        default:
            if(!remoteFeel)
                my_tell_room( environment( TP ), ",", ({ }) );
            make_channel_msg( "," );
            break;
        case 2:
            if(!remoteFeel)
                my_tell_room( environment( TP ), " and", ({ }) );
            make_channel_msg( " and" );
            break;
        case 1:
        case 0:
        }
    }
}

/* This function takes a verb and the arguments given by
 * the user and converts it to an internal representation suitable for passing
  * to feel()
 */
private mixed * reduce_verb( string verb,
  mixed verbdata,
  object * who,
  string * adverb,
  string mess,
  string * body )
{
    mixed * q;
    string how, a, b, * aa;
    string where, msg;
    int e;

    if ( objectp( verbdata ) || stringp( verbdata ) )
        return ({mixed *})verbdata -> reduce_verb( verb, who, adverb, mess, body );
    if ( pointerp( q = verbdata[ 1 ] ) )
    {
        if ( !sizeof( adverb ) && sizeof( q ) > 0 && q[ 0 ] )
            adverb = ({ q[ 0 ] });
        if ( ( !mess || mess == "" ) && sizeof( q ) > 1 && q[ 1 ] )
        {
            mess = q[ 1 ];
            if ( mess[ 0 ] == '\'' )
                mess = msg = " " + mess[1..];
        }
        if ( !sizeof( body ) && sizeof( q ) > 2 && q[ 2 ] )
            body = ({ q[ 2 ] });
    }
    if ( !mess || mess == "" )
    {
        mess = "";
        if ( !msg )
            msg = "";
    }
    else
    {
        if ( !msg )
            msg = " '" + mess + "'";
        mess = " " + mess;
    }
    where = "";
    if ( sizeof( body ) )
        where = " " + implode_nicely( body );

    how = implode_nicely( SUB_ARRAY( adverb, ({ "" }) ) );
    switch ( verbdata[ 0 ] )
    {
    case DEFA:
        if ( !a )
            a = " " + verb + "$ \nHOW \nAT";
    case PREV:
        if ( !a )
            a = " " + verb + "$" + verbdata[ 2 ] + " \nWHO \nHOW";
    case PHYS:
        if ( !a )
            a = " " + verb + "$" + verbdata[ 2 ] + " \nWHO \nHOW \nWHERE";
    case SHRT:
        if ( !a )
            a = " " + verb + "$" + verbdata[ 2 ] + " \nHOW";
    case PERS:
        if( !a )
            if( sizeof( who ) )
                a = verbdata[ 3 ];
            else
                a = verbdata[ 2 ];
    case SIMP:
        if ( !a )
            a = verbdata[ 2 ];
        if ( sizeof( who ) && sizeof( verbdata ) > 3 )
            a = replace( a," \nAT", verbdata[ 3 ] + " \nWHO" );
        else
            a = replace( a, " \nAT", "" );
        if ( !sizeof( who ) && ( sscanf( a, "%~s\nWHO" )
            ||
            sscanf( a, "%~s\nPOSS" )
            ||
            sscanf( a, "%~s\nTHEIR" )
            ||
            sscanf( a, "%~s\nOBJ" ) )
        )
            return notify_fail( "Need person for verb " + verb + ".\n" ), 0;
        if( how == "" )
            a = replace( a, " \nHOW", "" );
        else
            a = replace( a, " \nHOW", " " + how );

        a = replace( a, " \nWHERE", where );
        a = replace( a, " \nWHAT", mess );
        a = replace( a, " \nMSG", msg );
        b = a;
        a = replace( a, "\\$", "" );
        b = replace( b, "\\$", "s" );
        return ({ ({ who, a, b, b, a, a, a }) });
    case DEUX:
        a = verbdata[ 2 ];
        b = verbdata[ 3 ];
        if ( !sizeof( who ) && ( sscanf( a, "%~s\nWHO" )
            ||
            sscanf( a, "%~s\nPOSS" )
            ||
            sscanf( a, "%~s\nTHEIR" )
            ||
            sscanf( a, "%~s\nOBJ" ) )
        )
            return notify_fail( "Need person for verb " +verb + ".\n" ), 0;
        a = replace( a, " \nWHERE", where );
        b = replace( b, " \nWHERE", where );
        a = replace( a, " \nWHAT", mess );
        a = replace( a, " \nMSG", msg );
        b = replace( b, " \nWHAT", mess );
        b = replace( b, " \nMSG", msg );
        if ( how == "" )
        {
            a=replace(a," \nHOW","");
            b=replace(b," \nHOW","");
        }
        else
        {
            a=replace(a," \nHOW"," "+how);
            b=replace(b," \nHOW"," "+how);
        }
        return ({ ({ who, a, b, b, a, a, a }) });
    case QUAD:
        if ( !sizeof( who ) )
        {
            a = verbdata[ 2 ];
            b = verbdata[ 3 ];
        }
        else
        {
            a = verbdata[ 4 ];
            b = verbdata[ 5 ];
        }
        a = replace( a, " \nWHERE", where );
        b = replace( b, " \nWHERE", where );
        a = replace( a, " \nWHAT", mess );
        a = replace( a, " \nMSG", msg );
        b = replace( b, " \nWHAT", mess );
        b = replace( b, " \nMSG", msg );
        if ( how == "" )
        {
            a = replace( a, " \nHOW", "" );
            b = replace( b, " \nHOW", "" );
        }
        else
        {
            a = replace( a, " \nHOW", " " + how );
            b = replace( b, " \nHOW", " " + how );
        }
        return ({ ({ who, a, b, b, a, a, a }) });

    case FULL:
        if ( !sizeof( who ) )
            aa = verbdata[2..7];
        else
            aa = verbdata[8..13];
        for ( e = 0; e < sizeof( aa ); e++ )
        {
            aa[ e ] = replace( aa[ e ], " \nWHERE", where );
            aa[ e ] = replace( aa[ e ], " \nWHAT", mess );
            aa[ e ] = replace( aa[ e ], " \nMSG", msg );
        }
        if ( how == "" )
        {
            for ( e = 0; e < sizeof( aa ); e++ )
                aa[ e ] = replace( aa[ e ], " \nHOW", "" );
        }
        else
        {
            for ( e = 0; e < sizeof( aa ); e++ )
                aa[ e ] = replace( aa[ e ], " \nHOW", " " + how );
        }
        return ({ ({ who }) + aa });
    }
    return 0;
}

private varargs mixed * webster( string t,
  int offset,
  string channel,
  int remote )
{
    string verb, tmp, _how, mess = "";
    string * q = SUB_ARRAY( my_explode( t, " " ), ({ "" }) );
    string * adv = ({ }), * body = ({ }), * tmp2;
    int u, except;
    object * who = ({ }), * people, ob;
    mixed p, * verbdata, * Y = ({ });
    mapping persons;

    remote = 0;
    for( int e = 0; e < sizeof( q ); e++ )
    {
        t = q[ e ];
#if DEBUG
        write( "Webster: q[" + e + "]=\"" + t + "\"\n" );
#endif
        /* Handle message strings, like sing "dum didum di dum do-dum" */
        if ( t[ 0 ] == '"' )
        {
            mess = t[1..];
            for ( e++; mess[<1] != '"' && e < sizeof( q ); e++ )
                mess += " " + q[ e ];
            if ( mess[<1] == '"' )
            {
                mess = mess[0..<2];
                e--;
            }
            continue;
        }
        if ( t[<1] == ',' )
            t = q[ e ] = t[..<2];
        if ( how[ t ] )
        {
            _how = t;
            continue;
        }
        switch ( t )
        {
            /* Null words */
        case "and":
        case "&":
        case "":
        case "at":
        case "to":
        case "before":
        case "in":
        case "on":
        case "the":
        case "with":
            break;
        case "me":
        case "myself":
        case "I":
            if ( except )
                who = SUB_ARRAY( who, ({ TP }) );
            else
                who += ({ TP });
            break;
        case "them":
        case "him":
        case "her":
        case "it":
            if ( !sizeof( Y ) )
                return notify_fail( "Who?\n" ), 0;
            if ( t == "them" )
            {
                if ( sizeof( Y[sizeof( Y ) - 1][ 0 ] ) < 2 )
                    return notify_fail( "Who?\n" ), 0;
            }
            else
            {
                if( sizeof( Y[sizeof( Y ) - 1 ][ 0 ] ) != 1
                  ||
                  OBJECTIVE( Y[ sizeof( Y ) - 1 ][ 0 ][ 0 ] ) != t
                )
                    return notify_fail( "Who?\n" ), 0;
            }
            if ( except )
                who = SUB_ARRAY( who, Y[sizeof( Y ) -1 ][ 0 ] );
            else
                who += Y[sizeof( Y ) - 1 ][ 0 ];
            break;
        case "all":
        case "everybody":
        case "everyone":
            if ( except )
                who = ({ });
            else
            {
                if ( !people && !channel )
                    people = get_persons();
                else if( !people && channel )
                    people = channel_listeners( channel );
                who += SUB_ARRAY( people, ({ TP }) );
            }
            break;
        case "except":
        case "but":
            if ( !except && !sizeof( who ) )
                return notify_fail( "That '" + t + "' doesn't look grammatically "
                  "right there.\n" ), 0;
            except = !except;
            break;
        case "plainly":
            adv = ({ "" });
            break;
        default:
            if ( channel && !people )
                people = channel_listeners( channel );
            if ( ( persons && ( ob = persons[ t ] ) )
              ||
              ( people && member( people, t ) != -1 &&
                ( ob = people[ member( people, t ) ] )
              )
              ||
              ( ob = present( t, environment( TP ) ) )
              &&
              isplay( ob )
              ||
              //removed wizard only remotes - uuga
              //wizardp( this_object() ) && ob = find_player( t )
              ob = find_player( t )
            )
            {
                if ( except )
                    who = SUB_ARRAY( who, ({ ob }) );
                else
                    who += ({ ob });
                if ( !present( ob, environment( TO ) ) )
                {
                    remote = 1;
                    remotePerson = ob;
                }
                else {
                    remote = 0;
                    remotePerson = 0;
                }
                break;
            }
            if ( ( p = xverbs[ t ] ) || ( p = verbs[ t ] ) )
            {
                if ( verb )
                {
                    verbdata = reduce_verb( verb, verbdata, who, adv, mess, body );
                    brokendown_data += ({ ({ verb, who, adv, mess, body }) });
                    except = 0;
                    if ( !verbdata )
                        return 0; /* An error was found. */
                    Y += verbdata;		/* verbdata can be more than one verb */
                    mess = "";
                    adv  = ({ });
                    who  = ({ });
                    body = ({ });
                }
                verb     = q[ e ];
                verbdata = p;
                break;
            }
            if ( adverbs[ t ] || xadverbs[ t ] )
            {
                if ( _how )
                {
                    adv += ({_how + " " + t });
                    _how = 0;
                }
                else
                    adv += ({ t });
                break;
            }
            if (p = bodydata[ t ])
            {
                body += ({ p });
                break;
            }
            if ( !people )
                people = get_persons();
            if ( !persons )
                persons = mkmapping( map( people, #'get_name /*'*/), people );
            if ( p = prefix( m_indices( persons ), t, "Who do you mean?" ) )
            {
                if ( p == -1 )
                {
                    parsed_part = last_action + ( e
                      ? implode( q[0..e-1], " " )
                      : ""          );
                    uncertain_part = t;
                    unparsed_part = implode( q[e+1..sizeof( q )], " " );
                    last_action="";
                    return 0;
                }
                q[ e ] = get_name( persons[ p ] );
                if ( except )
                    who = SUB_ARRAY( who,({ persons[ p ] }) );
                else
                    who += ({ persons[ p ] });
                break;
            }
            u   = e;
            tmp = t;
            p = prefix( m_indices( adverbs ), tmp, "Which adverb was that?" );
            while ( p == -1 && u + 1 < sizeof( q ) )
            {
                u++;
                tmp += " " + q[ u ];
                p = prefix( m_indices( adverbs ), tmp, "Which adverb was that?" );
            }
            if ( !p )
            {
                u   = e;
                tmp = t;
                p = prefix( m_indices( xadverbs ), tmp, "Which adverb was that?" );
                while ( p == -1 && u + 1 < sizeof( q ) )
                {
                    u++;
                    tmp += " " + q[ u ];
                    p = prefix( m_indices( xadverbs ), tmp, "Which adverb "
                      "was that?" );
                }
            }
            if ( p )
            {
                if ( p == -1 )
                {
                    parsed_part = last_action + ( e
                      ? implode( q[0..e-1], " " )
                      : ""
                    );
                    uncertain_part = t;
                    unparsed_part  = implode( q[u+1..sizeof( q )], " " );
                    last_action    = "";
                    return 0;
                }
                tmp2 = explode( p, " " );
                for ( u = 0; tmp2 && u < sizeof( tmp2 ) && e < sizeof( q ); u++ )
                {
                    if ( tmp2[ u ] == q[ e ] )
                    {
                        e++;
                        continue;
                    }
                    if ( tmp2[ u ][0..sizeof( q[ e ] ) - 1] == q[ e ] )
                        e++;
                    break;
                }
                e--;
                if ( p== "plainly" )
                    adv = ({ "" });
                else if( _how )
                {
                    adv += ({ _how + " " + p });
                    _how = 0;
                }
                else
                    adv += ({ p });
                break;
            }
            switch ( offset + e )
            {
            case 1:  verb = "first";    break;
            case 2:  verb = "second";   break;
            case 3:  verb = "third";    break;
            case 4:  verb = "fourth";   break;
            case 5:  verb = "fifth";    break;
            case 6:  verb = "sixth";    break;
            case 7:  verb = "seventh";  break;
            case 8:  verb = "eigth";    break;
            case 9:  verb = "ninth";    break;
            case 10: verb = "tenth";    break;
            case 11: verb = "eleventh"; break;
            case 12: verb = "twelvth";  break;
            default:
                switch ( ( offset + e ) % 10 )
                {
                case  1: verb = ( offset + e ) + "st"; break;
                case  2: verb = ( offset + e ) + "nd"; break;
                case  3: verb = ( offset + e ) + "rd"; break;
                default: verb = ( offset + e ) + "th"; break;
                }
            }
            return notify_fail( "The " + verb + " word in that sentence doesn't "
              "make sense to me.\n" ), 0;
        }
    }
    if ( !verb )
        return notify_fail( "No verb?\n" ), 0;
    verbdata = reduce_verb( verb, verbdata, who, adv, mess, body );
    brokendown_data += ({ ({ verb, who, adv, mess, body }) });
    if ( !verbdata )
        return 0; /* An error was found. */
    last_action += implode( q, " " );
    Y += verbdata; /* verbdata can be more than one verb */
    return Y;
}

public int do_feel( string p )
{
    string v = query_verb();
    mixed * q;
    int e;

    remoteFeel = 0;
    // Emote
    if ( v && sizeof(v) > 0 && v[ 0 ] == ':' )
        return do_emote( ( v[ 1.. ] + (  p ? " " + p : "" ) ) );
    if ( uncertain_part )
    {
        if ( uncertain_part == v[0..sizeof( uncertain_part ) - 1]
          &&
          sizeof( uncertain_part ) < sizeof( v ) )
        {
            parsed_part += " " + ( p
              ? v + " " + p
              : v
            ) + " " + unparsed_part;
            uncertain_part = 0;
            while ( sscanf( parsed_part," %s", parsed_part ) );
            FORCE_SELF( parsed_part );
            return 1;
        }
        else
            uncertain_part = 0;
    }
    // Channel handling - Nosty
    if ( member(
        ( ({string *})TP ->
          QueryChannels() || ({ }) ), v ) != -1 )
    {
        string command = query_command();
        string * parts = explode( command, " " );
        if ( sizeof( parts ) > 1 )
        {
            switch ( parts[ 1 ][ 0 ] )
            {
            case '!':
                {
                    string feeling = parts[ 1 ][1..] +
                    ( ( sizeof( parts ) > 2 )
                      ? " " + implode( parts[2..], " " )
                      : ""
                    );
                    string msg;
                    // Handle the 'bofh' channel seperately
                    if ( parts[ 0 ] == "bofh" )
                    {
                        CHANNEL_D ->
                        SendChannel( parts[ 0 ],
                          TP,
                          implode( parts[1..], " " ),
                          0
                        );
                        return 1;
                    }
                    if ( !verbs[ parts[ 1 ][1..] ] && !xverbs[ parts[ 1 ][1..] ] )
                    {
                        mixed cl;
                        channel_msg = "";
                        set_last_action( parts[ 1 ][1..] );
                        q = webster( implode( parts[2..], " " ), 2 );
                        messages = ([ ]);
                        cl = symbol_function( parts[ 1 ][1..], TO );
                        if ( cl )
                            funcall( cl, implode( parts[2..], " " ), 1 );
                        msg = query_channel_msg();
                        if ( !( function_exists( parts[ 1 ][1..] ) ) )
                            return notify_fail( "Can't put that on the channel, "
                              "'" + parts[ 1 ][1..] + "' is not "
                              "a valid feeling.\n" ), 0;

                        if ( !( sizeof( msg ) ) )
                            return 0;
                        CHANNEL_D ->
                        SendEmote( parts[ 0 ],
                          "",
                          msg,
                          0
                        );
                        return 1;
                    }
                    channel_msg = "";
                    set_last_action( "" );
                    q = webster( feeling, 1, parts[ 0 ] );
                    messages = ([ ]);
                    feel( q, 0 );
                    msg = query_channel_msg();
                    if ( msg && !sizeof( msg ) )
                        return notify_fail( "Can't possibly put that on a "
                          "channel.\n" ), 0;
                    CHANNEL_D ->
                    SendEmote( parts[ 0 ],
                      ({string})TP->QueryRealName(),
                      msg[1..],
                      0
                    );
#if __EFUN_DEFINED__(send_discord)
                    if ( parts[0] == "general" )
                        send_discord(capitalize(({string})TP->QueryRealName()) + " " +
                        msg[1..]);
#endif
                    return 1;
                }
            case '@':
                parts[ 1 ] = parts[ 1 ][1..];
                CHANNEL_D ->
                SendEmote( parts[ 0 ],
                  ({string})TP->QueryRealName(),
                  implode( parts[1..], " " ),
                  0
                );
#if __EFUN_DEFINED__(send_discord)
                if ( parts[0] == "general" )
                    send_discord(capitalize(({string})TP->QueryRealName()) + " " +
                    implode(parts[1..], " "));
#endif
                return 1;
            default:
                CHANNEL_D ->
                SendTell( parts[ 0 ],
                  ({string})TP->QueryRealName(),
                  implode( parts[1..], " " ),
                  0
                );
#if __EFUN_DEFINED__(send_discord)
                if ( parts[0] == "general" )
                    send_discord(capitalize(({string})TP->QueryRealName()) + ": " +
                            implode( parts[1..], " "));
#endif
                return 1;
            }
        }
        else
            return notify_fail( "What do you want to put on channel '" + v +
              "' ?\n"
            );
    }
    if ( !verbs[ v ] && !xverbs[ v ] )
        return 0;
    if ( v == "say" )
        return 0;
    if ( p )
        v += " " + p;
    set_last_action( "" );
    if ( !( q = webster( v, 1, 0, &remoteFeel ) ) )
        return 0;
    if ( !q )
        return notify_fail( "Hmm, what?\n" ), 0;
    messages = ([ ]);
    WRITE( "You" );
    if ( remoteFeel )
    {
//        my_tell_room( environment( this_player() ), QueryName(), ({ TP }) );
        TELL_OBJECT( remotePerson, ({string})TP->QueryName() );
    } else
        intsay( CAP_NAME( TP ) );
    feel( q, 0 );
    if ( remoteFeel )
        TELL_OBJECT( remotePerson, " from afar.\n" );
    v = messages[ TP ];
    e = v[<1];
/*
    if (remoteFeel)
        WRITE(" from afar.\n");
*/
    if (e != '.' && e != '?' && e != '!' )
    {
        if ( remoteFeel )
            my_tell_room( environment( TP ), " from afar.\n", ({ }) );
        else
            my_tell_room( environment( TP ), ".\n", ({ }) );
    }
    else
    {
        if ( remoteFeel )
            my_tell_room( environment( TP ), " from afar\n", ({ }) );
        else
            my_tell_room( environment( TP ), "\n", ({ }) );
    }
    flush();
    return 1;
}

public varargs int suddenly( string p, int chan )
{
    string v;
    mixed * q;
    int e;
    if( !p )
        return notify_fail( "Suddenly what?\n" ), 0;
    set_last_action( "suddenly" );
    if ( !( q = webster( p, 2, 0, &remoteFeel ) ) )
        return 0;
    if( !q )
        return notify_fail( "Hmm, what?\n" ), 0;
    messages = ([ ]);

    WRITE( "Suddenly, you" );
    if ( chan )
        make_channel_msg( "Suddenly, " + CAP_NAME( TO ) );
    else
        intsay( "Suddenly, " + CAP_NAME( TP ) );
    if ( remoteFeel )
        TELL_OBJECT( remotePerson, "Suddenly, " + ({string})TP->QueryName() );
    feel( q, 0 );
    if ( remoteFeel )
        TELL_OBJECT( remotePerson, " from afar.\n" );
    v = messages[ TP ];
    e = v[<1];
    if( e != '.' && e != '?' && e != '!' )
    {
        if ( remoteFeel )
            my_tell_room( environment( TP ), " from afar.\n", ({ }) );
        else
            my_tell_room( environment( TP ), ".\n", ({ }) );
    }
    else
    {
        if ( remoteFeel )
            my_tell_room( environment( TP ), " from afar\n", ({ }) );
        else
            my_tell_room( environment( TP ), "\n", ({ }) );
    }
    if ( !( chan ) )
        flush();
    return 1;
}

public varargs int again( string p, int chan )
{
    mixed * q;

    if ( !p )
        return notify_fail( "Do what again?\n" ), 0;
    set_last_action( "again" );
    if ( !( q = webster( p, 2, 0, &remoteFeel ) ) )
        return 0;
    if ( !q )
        return notify_fail( "Hmm, what?\n" ), 0;
    messages =([ ]);

    WRITE( "You" );
    if ( remoteFeel )
        TELL_OBJECT( remotePerson, ({string})TP->QueryName() );
    if ( chan )
        make_channel_msg( CAP_NAME( TO ) );
    else
        intsay( CAP_NAME( TO ) );
    feel( q, 0 );
    if ( remoteFeel )
        TELL_OBJECT( remotePerson, " again from afar.\n" );
    my_tell_room( environment( TP ), " again.\n", ({ }) );
    if ( chan )
        make_channel_msg( " again." );
    if ( !( chan ) )
        flush();
    return 1;
}

public varargs int fail( string p, int chan )
{
    mixed * q;

    if(!p)
        return notify_fail( "Fail with what?\n" ), 0;
    set_last_action( "fail" );
    if( !( q = webster( p, 2, 0, &remoteFeel ) ) )
        return 0;
    if( !q )
        return notify_fail( "Hmm, what?\n" ), 0;
    messages = ([ ]);

    WRITE( "You try to" );
    if ( remoteFeel )
        TELL_OBJECT( remotePerson, ({string})TP->QueryName() + " tries to");
    if ( chan )
        make_channel_msg( CAP_NAME( TO ) + " tries to" );
    else
        intsay( CAP_NAME( TO ) + " tries to" );
    feel( q, 1 );
    WRITE( ", but fail miserably.\n" );
    if ( remoteFeel )
        TELL_OBJECT( remotePerson, " from afar, but fails miserably.\n" );
    if ( chan )
        make_channel_msg( ", but fails miserably." );
    else
        intsay( ", but fails miserably.\n" );
    if ( !( chan ) )
        flush();
    return 1;
}

public varargs int dont( string p, int chan )
{
    mixed * q;

    if ( !p )
        return notify_fail( "Don't do what?\n" ), 0;
    set_last_action( "dont" );
    if( !( q = webster( p, 2, 0, &remoteFeel ) ) )
        return 0;
    if ( !q )
        return notify_fail( "Hmm, what?\n" ), 0;
    messages = ([ ]);

    WRITE( "You try not to" );
    if ( remoteFeel )
        TELL_OBJECT( remotePerson, ({string})TP->QueryName() + " tries not to" );
    if ( chan )
        make_channel_msg( CAP_NAME( TO ) + " tries not to" );
    else
        intsay( CAP_NAME( TO ) + " tries not to" );
    feel( q, 1 );
    WRITE( ", but fail miserably.\n" );
    if ( remoteFeel )
        TELL_OBJECT( remotePerson, " from afar, but fails miserably.\n" );
    if ( chan )
        make_channel_msg( ", but fails miserably." );
    else
        intsay( ", but fails miserably.\n" );
    if ( !( chan ) )
        flush();
    return 1;
}

public int feeling( string p )
{
    string v;
    mixed * q;
    int e;

    if ( !p )
        return notify_fail( "What feeling?\n" ), 0;
    set_last_action( "feeling" );
    if ( !( q = webster( p, 2 ) ) )
        return 0;
    if ( !q )
        return notify_fail( "Hmm, what?\n" ), 0;
    messages = ([ ]);

    WRITE( "You" );
    intsay( CAP_NAME( TP ) );
    feel( q, 0 );
    v = messages[ TP ];
    e = v[<1];
    if ( e != '.' && e != '?' && e != '!' )
        my_tell_room( environment( TP ), ".\n", ({ }) );
    else
        my_tell_room( environment( TP ), "\n", ({ }) );
    flush();
    return 1;
}


/* Takes an array of verbs, only removes extra verbs */
public void remove_verb( string * v )
{
    for ( int e = 0; e < sizeof( v ); e++ )
        xverbs = m_delete( xverbs, v[ e ] );
    xverb_string = 0;
}

/* Takes an array of verbs that follow the same format as the one in the
 * beginning of this file. Here is a short explanation of the components:
 * ([ "verb":([verb_type,defaults,data...})
 * verb_type is one of the defines in the beginning, you have to look
 *   at the examples to what they actually do.
 * defaults is zero or an array containing:
 *        ({defaultadverb,defaultwhat,bodypart})
 * Data has to do with what type of verb it is, again: see the examples,
 *  there should be enogh of them.
 */

public void add_verb( mapping v )
{
    /*  log_file("Feelings",implode_nicely(m_indices(v))+"\n"); */
    remove_verb( m_indices( xverbs ) & m_indices( v ) );
    xverbs += v;
    xverb_string = 0;
}

public void remove_adverb( string * v )
{
    v = SUB_ARRAY( m_indices( xadverbs ), v );
    xadverbs = mkmapping( v, v );
    xadverb_string = 0;
}

/* Takes an array of adverbs */
public void add_adverb( string * v )
{
    /*  log_file("Adverbs",implode_nicely(v)+"\n"); */
    v = SUB_ARRAY( v, m_indices( xadverbs ) );
    xadverbs += mkmapping( v, v );
    xadverb_string = 0;
}

public mapping query_xadverbs( void )
{
    return xadverbs;
}

public mapping query_xverbs( void )
{
    return xverbs;
}

private string feel_to_this_player( mixed * d, int flag )
{
    int prev;
    mixed * q = ({ });
    string res = "";

    for ( int e = 0; e < sizeof( d ); e++ )
    {
        prev = !sizeof( SUB_ARRAY( q, d[ e ][ 0 ] ) )
        &&
        !sizeof( SUB_ARRAY( d[ e ][ 0 ], q ) )
        &&
        -1 == member( q, TP );
        q = d[ e ][ 0 ];
        res += gloerp( d[ e ][ 1 + flag * 3 ], q, TP, prev );
        switch ( sizeof( d ) - e )
        {
        default:
            res += ",";
            break;
        case 2:
            res += " and";
            break;
        case 1:
        case 0:
        }
    }
    return res;
}

public string query_total_list( void )
{
    return total_list;
}

public void set_total_list( string s )
{
    total_list = share_string( s );
}

public string globber_one_player( mapping ve )
{
    string * foo = m_indices( ve ), v, res = "";
    mixed * bar = m_values( ve ), * q;
    int e,t;

    for ( e = 0; e < sizeof( foo );e++ )
    {
        q = reduce_verb( foo[ e ], bar[ e ], ({ }), ({ }), "", ({ }) );
        if ( !q )
        {
            q = reduce_verb( foo[ e ], bar[ e ], ({ TP }),
              ({ }), "", ({ })
            );
        }
        if ( !q )
            continue; /* Maybe I should write 'disabled' or something */
        messages = ([ ]);
        v = foo[ e ] + "              "[sizeof( foo[ e ] )..12] + ":You";
        v += feel_to_this_player( q, 0 );
        t = v[<1];
        if( t != '.' && t != '?' && t != '!' )
            res += v + ".\n";
        else
            res += v + "\n";
    }
    return res;
}

