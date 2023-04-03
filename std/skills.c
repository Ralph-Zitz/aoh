//---------------------------------------------------------------------------
// std/skills.c
//
// The basic skill handling of the objects in osb, i.e. room, living, thing
// (c) Softbyte@OSB 10nov97
// [Softbyte 10nov97] Basic skill handling and functions
// [Softbyte 22dec97] vitem support
//---------------------------------------------------------------------------

/* **************** External Functions of SKILLSERVER ******************

  string VerbToSkill(string skill_verb);
  string CannotUseSkill(string skill,object TP,int diff,string id,string extra);
  int ApplySkill(object TP,string skill,int difficulty,mixed props);
  string QueryFailString(string skill,object TP);
  string *QueryRoomVerbs());

********************************************************************* */


#include <config.h>
#include <properties.h>
#include <skills.h>
#include <classes.h>

#define TO this_object()
#define TP this_player()
#define THIS this_object()

public mixed Query(string arg); // std/base
public varargs mixed Set(string pn, mixed arg, int sc); // std/base

//--- global variables ------------------------------------------------------
// ([SK_CLIMBING:sk_map, SK_SWIMMING:sk_map,...])
// sk_map: ([name|NULL:int difficulty, string *|closure ids, mixed props, mixed unused])
private mapping m_skills;

// Debugging only
mapping QuerySkillMap() {return m_skills;}

// These verbs will be added to every room/thing/living
private nosave string *m_verbs;
string *QueryM_Verbs() {return m_verbs;}

#define M_SK_DIFF   0      // Access the skill mapping
#define M_SK_IDS    1
#define M_SK_PROPS  2
#define M_SK_UNUSED 3

//--- skill functions -------------------------------------------------------
// Adds an Skill to the object ...e.g. "climb tree"
// ids: string * or closure
varargs int AddSkill(string skill,string name,int difficulty,mixed ids,mixed props) {
    mapping sk_map,old_map;
    // Something went wrong
    if (!m_skills || !skill)
        return 0;

    // Prepare the skill entry
    sk_map=([name:difficulty;ids;props;0]);

    // Is already an entry for that skill available?
    old_map=m_skills[skill];
    if (old_map) {
        // Overwrite the old entry
        if (member(old_map,name))
            old_map=m_delete(old_map,name);

        m_skills[skill]=old_map+sk_map;
    } else {
        m_skills+=([skill:sk_map]);
    }
    return 1;
}

// Delete a skill from the room
int RemoveSkill(string skill, string name) {
    mapping old_map;
    if (!m_skills || !skill)
        return 0;
    old_map=m_skills[skill];
    if (old_map)
        return 0;
    if (member(old_map,name)) {
        old_map=m_delete(old_map,name);
        m_skills[skill]=old_map;
    }
    else {
        return 0;
    }
    return 1;
}

// Checks whether one of the ids is contained in str
// (i.e. at the beginning of str)
string MatchIds(string *ids,string str) {
    int i;
    for (i=0;i<sizeof(ids);i++) {
        if (strstr(str,ids[i])==0)
            return ids[i];
    }
    return 0;
}

// Executes the skill command..e.g. the swimming
int cmd_skills(string str) {
    int i,null_p,found_p,diff;
    string fun,skill_verb,result,name,id_str,extra_str;
    mapping sk_map;
    string *idx;
    mixed skill,ids,erg;

    skill_verb=query_verb();
    //write("cmd_skills:: skill="+skill_verb+"\n");

    if (!TP || !skill_verb)
        return 0;

    if (str)
        str=norm_id(str);

    skill=({mixed})call_other(SKILLSERVER,"VerbToSkill",skill_verb);
    if (!skill)
        return notify_fail("BUG: This skill is not yet available.\n",
                NOTIFY_NOT_VALID);

    sk_map=m_skills[skill];

    if (!sk_map) {
        result=({string})call_other(SKILLSERVER,"QueryFailString",skill,TP);
        if (result)
            return notify_fail(result,NOTIFY_NOT_OBJ);
        else
            return notify_fail("Seems you can not do that now.\n",
                    NOTIFY_NOT_OBJ);
    }

    idx=m_indices(sk_map);

    // Loop over all objects belonging to the skill
    null_p=-1;   // NULL entry
    found_p=-1;  // Found match
    for (i=0;i<sizeof(idx) && found_p<0;i++) {
        // Check whether the ids match the string
        ids=sk_map[idx[i],M_SK_IDS];   // The ids or the closure

        // Check for the NULL entry  later
        if (!ids ) {
            null_p=i;
            continue;
        }

        // printf("i=%d idx[i]=%s ids=%O\n",i,idx[i],ids);

        if (ids && closurep(ids)) {
            if (extra_str=funcall(ids,str)) {
                found_p=i;
                id_str=0;
            }
        }
        else if (!str) {  // TODO: what to do with no argument?
        }
        // The search for an vitem needs to return the given vitem
        // e.g. ids=vitem#4 or the strings shall match if a single ids string
        // is given, i.e. str=bush ids=bush
        // The argument "2" returns the vitem's mapping index, i.e. vitem#4
        else if (ids && stringp(ids) &&
                ( ({string})TO->GetVItem(str,2)==ids || strstr(str,ids)==0 ) ){
            found_p=i;
            id_str=ids;
            // Extract the rest of the string
            if (sizeof(str)>sizeof(id_str)+1)  // +1=" "
                extra_str=str[sizeof(id_str)+1..<1];
            else
                extra_str=0;
        }
        else if (ids && pointerp(ids) && (id_str=MatchIds(ids,str)) ) {
            found_p=i;
            // Extract the rest of the string
            if (sizeof(str)>sizeof(id_str)+1)  // +1=" "
                extra_str=str[sizeof(id_str)+1..<1];
            else
                extra_str=0;
        }
        else {
            if (!ids) {
                found_p=i;
                id_str=0;
                extra_str=str;
            }
        }
    } // end for loop

    //  write("skills:: found_p="+found_p+" null_p="+null_p+"\n");
    //  printf("         str=[%s] id_str=[%s] extra_str=[%s]\n",
    //    str,id_str||"NULL",extra_str||"NULL");


    // Only the NULL entry is found
    if (found_p<0 && null_p>=0)
        found_p=null_p;

    if (found_p>=0) {
        diff=sk_map[idx[found_p],M_SK_DIFF];
        // Does the skillmaster forbid the use of the skill?
        result=({string})call_other(SKILLSERVER,"CannotUseSkill",skill,TP,diff,
                id_str,extra_str);
        if (result)
            return notify_fail(result,NOTIFY_NOT_VALID);

        // The name of the Skillfunction or the generic one
        name=lower_case(idx[found_p]||"");         // river
        fun=capitalize(skill)+capitalize(name);  // SwimRiver or Swim

        // CannotSwim returns a string when TP may not use the skill here
        result=({string})call_other(TO,"Cannot"+fun,TP,diff,id_str,extra_str,
                sk_map[idx[found_p],M_SK_PROPS]);
        if (result)
            return notify_fail(result,NOTIFY_NOT_VALID);
        // Now the Cannotcheck in the skillserver itself
        // We call the RelayCall function with the argument skill
        // to forward the call to the skil modules itself
        // Looks complicated but is not. It results in the function
        // "CannotXXX" be called be caled in the module/skill
        result=({string})call_other(SKILLSERVER,"RelayCall",skill,
                "Cannot"+fun,TP,diff,id_str,extra_str,
                sk_map[idx[found_p],M_SK_PROPS]);
        if (result)
            return notify_fail(result,NOTIFY_NOT_VALID);

        // Ask the skillmaster whether the skill succeeds here
        // i<0...-1000: Skill fails more (-1000) or less (-1) severly
        // i>0..1000: Skill succeeds more (1000) or less (1) good
        i=({int})call_other(SKILLSERVER,"ApplySkill",TP,skill,diff,
                sk_map[idx[found_p],M_SK_PROPS]);
        if (i<=0) {
            // call our room first
            erg=0;
            if (!(erg=({mixed})call_other(TO,"OnFail"+fun,TP,i,id_str,extra_str,
                            sk_map[idx[found_p],M_SK_PROPS]))) {
                // Try to call the skillmaster
                if (!(erg=({mixed})call_other(SKILLSERVER,"RelayCall",skill,
                                "OnFail"+capitalize(skill),TP,i,id_str,
                                extra_str, sk_map[idx[found_p],M_SK_PROPS])))
                    return notify_fail("You "+skill_verb+" "+(str?str+" ":"")+
                            "but nothing happens...\n",NOTIFY_NOT_OBJ);
            }
            // if we get a string returned we notify_fail it !
            if (erg && stringp(erg)) return notify_fail(erg,
                    NOTIFY_NOT_VALID);
        }
        else {
            // call our room first
            erg=0;
            if (!(erg=({mixed})call_other(TO,"OnSucceed"+fun,TP,i,id_str,extra_str,
                            sk_map[idx[found_p], M_SK_PROPS])) ) {
                // Try to call the skillmaster
                if (!(erg=({mixed})call_other(SKILLSERVER,"RelayCall",skill,
                                "OnSucceed"+capitalize(skill), TP,i,id_str,extra_str,
                                sk_map[idx[found_p], M_SK_PROPS])))
                    return notify_fail("You "+skill_verb+" "+(str?str+" ":"")+
                            "but nothing happens..\n",NOTIFY_NOT_OBJ);
            }
            // if we get a string returned we notify_fail it !
            if (erg && stringp(erg))
                return notify_fail(erg, NOTIFY_NOT_VALID);
        }
    } // end if found
    else {
        // ask the skillmaster for the correct fail string
        result=({string})call_other(SKILLSERVER,"QueryFailString",skill,TP);
        return notify_fail(result,NOTIFY_NOT_VALID);
    }
    return 1;
}
//--- standard functions ----------------------------------------------------
public void create() {
    string *cids;
    string *new_verbs;
    int i;

    m_skills=([]);
    seteuid(getuid());

    // Find out what we are and add the correct actions
    // TODO: Maybe this idendification has to be improved later on
    cids=({string *})TO->QueryClassIds()||({});
    //printf("Classids=%O\n",cids);
    m_verbs=({});
    // Loop over all class_ids to add all skills of the object
    for (i=0;i<sizeof(cids);i++) {
        switch(cids[i]) {
            case C_ROOM:
                new_verbs=({string *})call_other(SKILLSERVER,"QueryRoomVerbs")||({});
                break;
            case C_THING:
                new_verbs=({string *})call_other(SKILLSERVER,"QueryThingVerbs")||({});
                break;
            case C_LIVING:
                new_verbs=({string *})call_other(SKILLSERVER,"QueryLivingVerbs")||({});
                break;
            default:
                // do nothing
        }
        // I guess I have to think how to insert an array into another
        // without double entries...
        foreach(string v : new_verbs)
            if (member(m_verbs, v) == -1)
                m_verbs += ({v});
        //    sect=intersect_alist(m_verbs,new_verbs);
        //    m_verbs=m_verbs+new_verbs-sect+sect;
    }
}

public void init() {
    int i;

    // if (!is_clone(this_object())) return;

    // Add the skill actions
    for (i=0;i<sizeof(m_verbs);i++) {
        add_action("cmd_skills",m_verbs[i]);
        // write("Action:"+object_name(this_object())+": "+m_verbs[i]+" added\n");
    }
}
