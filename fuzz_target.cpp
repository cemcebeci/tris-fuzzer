#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <ostream>
#include <stdint.h>
#include <string>
#include <sys/types.h>
#include <vector>

#define RLC_GET_FUNCTION_DECLS
#define RLC_GET_TYPE_DECLS

extern "C" {
    #include "include/tris.h"
}

/* -------------------------------------------------------------------------- */
/*                          What we put in the fuzzer                         */
/* -------------------------------------------------------------------------- */

/*
    This enum is populated with the names of all actions exported by RLC.
*/
enum RLC_Action {
    #define RLC_VISIT_ACTION(action_name, ...) \
        action_name,
    #include "include/tris.h"
    RLC_ACTION_UNKNOWN
};

/*
    This enum is popuplated with the names of all subactions of all actions exported by RLC.
    TODO: There might be name clashes, we might want to prepend subaction names with the aciton name.
*/
enum RLC_Subaction {
    #define select_name_visitor(subaction_name, resume_index, ...) subaction_name
    #define RLC_VISIT_ACTION(action_name, entity_type, start_function, isdone_function, visit_subactions) \
        visit_subactions(select_name_visitor, ),
    #include "include/tris.h"
};

/* --------------------- Generate subaction executors ------------------------ */
/*
    This section exapands to an "executor" function for each subaction of each action exported by RLC.
    These functions receive a reference to an action entity of their parent action.
    They pick arguments for the corresponding subaction, then call the subaction's wrapper function.
*/
#define declare_args_visitor(name, type) type name;
#define assign_args_visitor(name, type) name = (rand() % (int)(pow(2,sizeof(type)))); 
// #define print_assigned_args_visitor(name, size) std::cout << #name << ": " << name << "\n";
#define select_arg_references_visitor(name, size) &name
#define select_arg_names_visitor(name, size) name 
#define COMMA ,
#define EXECUTOR_NAME(subaction_name) subaction_name ## _execute
#define generate_executor_visitor(subaction_name, subaction_resume_index, action_entity_type, visit_args, invoker, tester)\
    void EXECUTOR_NAME(subaction_name)(action_entity_type &entity){\
        visit_args(declare_args_visitor, )\
        uint8_t args_are_valid = false;\
        int discarded_arg_count = -1;\
        do {\
        discarded_arg_count ++; \
        visit_args(assign_args_visitor, )\
        tester(&args_are_valid , &entity, visit_args(select_arg_references_visitor, COMMA));\
        } while( !args_are_valid);\
        std::cout << "Number of discarded argument sets: " << discarded_arg_count << "\n";\
        std::cout << "Invoking subaction " #subaction_name "(" << visit_args(select_arg_names_visitor, << ", " <<) << ")\n";\
        invoker(&entity, visit_args(select_arg_references_visitor, COMMA));\
    }

#define RLC_VISIT_ACTION(action_name, entity_type, start_function, isdone_function, visit_subactions) \
    visit_subactions(generate_executor_visitor, )
#include "include/tris.h"
/* -------------------------------------------------------------------------- */

/* ------------------- Generate list_available_subactions ------------------- */
/*
    This section expands to a LIST_AVAILABLE_SUBACTIONS(&entity_type) function for each action exported by RLC.
*/
#define check_index_and_emplace_visitor(name, subaction_resume_index, ...)\
        if(subaction_resume_index == entity.resume_index){\
            result.emplace_back(name);\
        }
#define LIST_AVAILABLE_SUBACTIONS list_available_subactions
#define RLC_VISIT_ACTION(action_name, entity_type, start_function, isdone_function, visit_subactions) \
    std::vector<RLC_Subaction> LIST_AVAILABLE_SUBACTIONS(entity_type &entity) {\
        std::vector<RLC_Subaction> result;\
        visit_subactions(check_index_and_emplace_visitor, )\
        \
        if(result.empty()) {\
            std::cout << "No available subactions for resume_index: " << entity.resume_index << "!\n" << "for action " #action_name;\
        }\
        return result;\
    }
#include "include/tris.h"
/* -------------------------------------------------------------------------- */

// This function is action-agnostic since it operates on a vector of enum entries. 
RLC_Subaction pick_subaction(std::vector<RLC_Subaction> options) {
    return options.at(rand() % options.size());
}

/* ----------------------- Generate action simulators ----------------------- */
/*
    This section expands to a simulate_ACTION_NAME function for each exported action.
    These functions run a full simulation of the action, currently using random numbers but ultimately using fuzz input.
*/
#define SIMULATE(action_name) simulate_ ## action_name
#define call_executor_case_visitor(subaction_name, ...)\
                case subaction_name:\
                    EXECUTOR_NAME(subaction_name)(entity); break;

#define RLC_VISIT_ACTION(action_name, entity_type, start_function, isdone_function, visit_subactions)\
    void SIMULATE(action_name) () {\
        entity_type entity;\
        start_function(&entity);\
        \
        uint8_t isDone = false;\
            while(!isDone) {\
            std::cout << "-------------------\n";\
            auto available_subactions = LIST_AVAILABLE_SUBACTIONS(entity);\
            std::cout << "Available subactions: \n";\
            for(auto subaction : available_subactions) {\
                std::cout << subaction << "\n";\
            }\
            std::cout << "\n";\
            \
            RLC_Subaction subaction = pick_subaction(available_subactions);\
            std::cout << "Picked subaction: " << subaction << "\n\n";\
            \
            switch (subaction) {\
                visit_subactions(call_executor_case_visitor, )\
            }\
            isdone_function(&isDone, &entity);\
            std::cout << "-------------------\n";\
        }\
    }
#include "include/tris.h"
/* -------------------------------------------------------------------------- */

int main() {
    #ifndef RLC_ACTION
    std::cout << "Define the macro RLC_ACTION before compiling " __FILE__ "!\n";
    #else   
    #define SIMULATE_PRIMITIVE(name) SIMULATE(name)
    SIMULATE_PRIMITIVE(RLC_ACTION)();
    #endif
}


extern "C" int LLVMFuzzerTestOneInput(const char *Data, size_t Size) {
    return 0;
}



/* -------------------------------------------------------------------------- */
/*                 What we expect to be in the exported header                */
/* -------------------------------------------------------------------------- */

// #if defined(RLC_VISIT_ACTION)
//     #define RLC_VISIT_ARGS_play_mark_(arg_visitor, separator)\
//       arg_visitor(x, int64_t) separator\
//       arg_visitor(y, int64_t)
//     #define RLC_VISIT_SUBACTIONS_play(subaction_visitor, separator)\
//       subaction_visitor(mark, 1, playEntity, RLC_VISIT_ARGS_play_mark_, markplayEntity_int64_t_int64_t_, can_mark_playEntity_)
//     RLC_VISIT_ACTION(play, playEntity, playplayEntity_, is_donebool_playEntity_, RLC_VISIT_SUBACTIONS_play)
//   #undef RLC_VISIT_ACTION
// #endif

// #ifdef RLC_GET_FUNCTION_DECLS
// void can_mark_playEntity_(uint8_t * __result, playEntity * arg0, int64_t *x, int64_t *y);

// // we expect this method to be implemented by RLC, but this is practical for the PoC.
// extern "C" void can_mark_playEntity_(uint8_t * __result, playEntity * arg0, int64_t *x, int64_t *y) {
  
//   if(!(*x <3 && *x >= 0 && *y <3 && *y >= 0)) {
//     *__result = false;
//     return;
//   }
//   int64_t board_get_result;
//   getint64_t_Board_int64_t_int64_t_(&board_get_result, &(arg0->board), x, y);
  
//   *__result = board_get_result == 0;
// }
// #endif
// /* -------------------------------------------------------------------------- */

