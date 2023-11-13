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

enum RLC_Action {
    #define RLC_VISIT_ACTION(action_name, ...) \
        action_name,
    #include "include/tris.h"
    RLC_ACTION_UNKNOWN
};

enum RLC_Subaction {
    #define select_name_visitor(subaction_name, resume_index, ...) subaction_name
    #define RLC_VISIT_ACTION(action_name, visit_subactions) \
        visit_subactions(select_name_visitor, ),
    #include "include/tris.h"
};

/* --------------------- Generate subaction executors ------------------------ */
#define declare_args_visitor(name, type) type name;
#define assign_args_visitor(name, type) name = (rand() % (int)(pow(2,sizeof(type)))); 
#define print_assigned_args_visitor(name, size) std::cout << #name << ": " << name << "\n";
#define select_arg_references_visitor(name, size) &name
#define select_arg_conditions_visitor(name, size) name >= 0 && name < 3
#define select_arg_names_visitor(name, size) name 
#define COMMA ,
#define EXECUTOR_NAME(subaction_name) subaction_name ## _execute
#define generate_executor_visitor(subaction_name, subaction_resume_index, visit_args, invoker)\
    void EXECUTOR_NAME(subaction_name)(playEntity &p){\
        visit_args(declare_args_visitor, )\
        do {\
        visit_args(assign_args_visitor, )\
        visit_args(print_assigned_args_visitor, )\
        std::cout << "\n";\
        } while( !(visit_args(select_arg_conditions_visitor, &&)));\
        invoker(&p, visit_args(select_arg_references_visitor, COMMA));\
        std::cout << "Successfully invoked subaction " #subaction_name "(" << visit_args(select_arg_names_visitor, << ", " <<) << ")\n";\
    }

#define RLC_VISIT_ACTION(action_name, visit_subactions) \
    visit_subactions(generate_executor_visitor, )
#include "include/tris.h"
/* -------------------------------------------------------------------------- */

std::vector<RLC_Subaction> list_available_subactions(playEntity &p) {
    std::vector<RLC_Subaction> result;
    #define check_index_and_emplace_visitor(name, subaction_resume_index, ...)\
        if(subaction_resume_index == p.resume_index){\
            result.emplace_back(name);\
        }
    
    #define RLC_VISIT_ACTION(action_name, visit_subactions) \
        visit_subactions(check_index_and_emplace_visitor, )
    
    #include "include/tris.h"

    return result;
}

RLC_Subaction pick_subaction(std::vector<RLC_Subaction> options) {
    return options.at(rand() % options.size());
}

int main() {
    playEntity p;
    initplayEntity_(&p);

    auto available_subactions = list_available_subactions(p);
    #ifndef NO_DEBUG
        std::cout << "Available subactions: \n";
        for(auto subaction : available_subactions) {
            std::cout << subaction << "\n"; //TODO would be nice if this printed the name.
        }
        std::cout << "\n";
    #endif

    RLC_Subaction subaction = pick_subaction(available_subactions);
    #ifndef NO_DEBUG
        std::cout << "Picked subaction: " << subaction << "\n\n";
    #endif

    switch (subaction) {
        #define test_and_call_executor_visitor(subaction_name, ...)\
            case subaction_name:\
                EXECUTOR_NAME(subaction_name)(p); break;
        #define RLC_VISIT_ACTION(action_name, visit_subactions)\
            visit_subactions(test_and_call_executor_visitor, )
        #include "include/tris.h"
    }

    return 0;
}


extern "C" int LLVMFuzzerTestOneInput(const char *Data, size_t Size) {
    // playEntity play;
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
//       subaction_visitor(mark, 0, RLC_VISIT_ARGS_play_mark_, markplayEntity_int64_t_int64_t_)
//     RLC_VISIT_ACTION(play, RLC_VISIT_SUBACTIONS_play)
//   #undef RLC_VISIT_ACTION
// #endif