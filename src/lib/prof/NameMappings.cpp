//******************************************************************************
//******************************************************************************
// NameMappings.cpp
//
// Purpose: 
//    To support renamings, this file supports one operation: normalize_name,
//    while maps input name into its output name. If no remapping exists, the
//    input name and the output name will agree.
//******************************************************************************
//******************************************************************************



//******************************************************************************
// global include files
//******************************************************************************

#include <iostream>
#include <map>
#include <string.h>

#include <lib/support/dictionary.h>
#include <lib/prof/NameMappings.hpp>


//******************************************************************************
// constants
//******************************************************************************

const char *PROGRAM_ROOT     = "<program root>";
const char *THREAD_ROOT      = "<thread root>";

const char *OMP_IDLE	       = "<omp idle>";
const char *OMP_OVERHEAD     = "<omp overhead>";
const char *OMP_BARRIER_WAIT = "<omp barrier wait>";
const char *OMP_TASK_WAIT    = "<omp task wait>"; 
const char *OMP_MUTEX_WAIT   = "<omp mutex wait>";

const char *MONITOR_DATA_FIRST_TOUCH = "<first touch>";

const char *DATACENTRIC_ROOT    = "<datacentric>";
const char *DATACENTRIC_DYNAMIC = "<heap allocation>";
const char *DATACENTRIC_STATIC  = "<static variable>";
const char *DATACENTRIC_UNKNOWN = "<unknown variable>";

const int NO_FAKE             = 0;
const int FAKE_PROCEDURE      = 1;
const int FAKE_ROOT           = 2;

//******************************************************************************
// types
//******************************************************************************

class NameMapCompare {
public:
  bool operator()(const char *n1,  const char *n2) const {
    return strcmp(n1,n2) < 0;
  }
};


typedef std::map<const char *, const char *, NameMapCompare> NameMappings_t;
typedef std::map<const char *, int> ProcedureStatusMapping_t;


typedef struct {
  const char *in;
  const char *out;
} NameMapping;


typedef struct {
  const char *name;
  const int  status;
} ProcedureStatusMapping;

//******************************************************************************
// private data
//******************************************************************************

static NameMapping renamingTable[] = { 
  { "monitor_main",           PROGRAM_ROOT     },
  { "monitor_main_fence1",    PROGRAM_ROOT     },
  { "monitor_main_fence2",    PROGRAM_ROOT     },
  { "monitor_main_fence3",    PROGRAM_ROOT     },
  { "monitor_main_fence4",    PROGRAM_ROOT     },

  { "monitor_begin_thread",   THREAD_ROOT      },
  { "monitor_thread_fence1",  THREAD_ROOT      },
  { "monitor_thread_fence2",  THREAD_ROOT      },
  { "monitor_thread_fence3",  THREAD_ROOT      },
  { "monitor_thread_fence4",  THREAD_ROOT      },

  { "ompt_idle_state",         OMP_IDLE         },
  { "ompt_idle",               OMP_IDLE         },

  { "ompt_overhead_state",     OMP_OVERHEAD     },
  { "omp_overhead",            OMP_OVERHEAD	},

  { "ompt_barrier_wait_state", OMP_BARRIER_WAIT	    },
  { "ompt_barrier_wait",       OMP_BARRIER_WAIT     },

  { "ompt_task_wait_state",    OMP_TASK_WAIT	    },
  { "ompt_task_wait",          OMP_TASK_WAIT	    },

  { "ompt_mutex_wait_state",   OMP_MUTEX_WAIT	    },
  { "ompt_mutex_wait",         OMP_MUTEX_WAIT     },

  { "monitor_data_first_touch", MONITOR_DATA_FIRST_TOUCH },

  { "DATACENTRIC"             , DATACENTRIC_ROOT },
  { "DATACENTRIC_Dynamic"     , DATACENTRIC_DYNAMIC },
  { "DATACENTRIC_Static"      , DATACENTRIC_STATIC },
  { "DATACENTRIC_Unknown"     , DATACENTRIC_UNKNOWN }
};


static ProcedureStatusMapping fakeProcedureTable[] = {
    {PROGRAM_ROOT,            FAKE_PROCEDURE},
    {THREAD_ROOT,             FAKE_PROCEDURE},
    {GUARD_NAME,              FAKE_PROCEDURE},
    {"<partial call paths>",  FAKE_PROCEDURE},
    {DATACENTRIC_ROOT,        FAKE_ROOT}, // need to be consider both root and fake
    {DATACENTRIC_DYNAMIC,     FAKE_PROCEDURE},
    {DATACENTRIC_STATIC,      FAKE_PROCEDURE},
};

static NameMappings_t renamingMap;
static ProcedureStatusMapping_t fakeProcedureMap;


//******************************************************************************
// private operations
//******************************************************************************


static void 
normalize_name_load_renamings()
{
  static int initialized = 0;
  if (initialized) return;
  initialized = 1;

  for (unsigned int i = 0; i < sizeof(renamingTable) / sizeof(NameMapping); i++) {
    renamingMap[renamingTable[i].in] =  renamingTable[i].out;
  }
  for (unsigned int i = 0; i < sizeof(fakeProcedureTable) / sizeof(const char*); i++) {
    fakeProcedureMap[fakeProcedureTable[i].name] = fakeProcedureTable[i].status;
  }
}


static const char *
normalize_name_rename(const char *in, int &fake_procedure)
{
  // check if the name has to be changed or not
  const char *name_new = in;
  NameMappings_t::iterator it = renamingMap.find(in);
    
  if (it != renamingMap.end()) {
    // it has to be renamed.
    name_new = it->second;
  }
  // here we want to mark fake procedures if they match with the list from fakeProcedureMap
  fake_procedure = NO_FAKE;

  ProcedureStatusMapping_t::iterator itf = fakeProcedureMap.find(name_new);
  if (itf != fakeProcedureMap.end()) {
    fake_procedure = itf->second;
  }

  return name_new;
}



//******************************************************************************
// interface operations
//******************************************************************************

const char *
normalize_name(const char *in, int &fake_procedure)
{
  normalize_name_load_renamings();
  return normalize_name_rename(in, fake_procedure);
}

#if 0

const char * tests[] = {
 "omp_idle_state", 
 "foo",
 "monitor_main_fence2",
 "omp_mutex_wait_state",
 "bar"
};

main()
{
  for(int i = 0; i < (sizeof(tests) / sizeof(const char *)); i++) {
    std::cout << "input name = '" << tests[i] 
              << "', output name = '" <<  normalize_name(tests[i]) 
              << "'" << std::endl;
  }
}

#endif
