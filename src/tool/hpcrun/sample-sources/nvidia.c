// -*-Mode: C++;-*- // technically C99

// * BeginRiceCopyright *****************************************************
//
// $HeadURL: https://outreach.scidac.gov/svn/hpctoolkit/trunk/src/tool/hpcrun/sample-sources/papi.c $
// $Id: papi.c 3328 2010-12-23 23:39:09Z tallent $
//
// --------------------------------------------------------------------------
// Part of HPCToolkit (hpctoolkit.org)
//
// Information about sources of support for research and development of
// HPCToolkit is at 'hpctoolkit.org' and in 'README.Acknowledgments'.
// --------------------------------------------------------------------------
//
// Copyright ((c)) 2002-2017, Rice University
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// * Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the distribution.
//
// * Neither the name of Rice University (RICE) nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// This software is provided by RICE and contributors "as is" and any
// express or implied warranties, including, but not limited to, the
// implied warranties of merchantability and fitness for a particular
// purpose are disclaimed. In no event shall RICE or contributors be
// liable for any direct, indirect, incidental, special, exemplary, or
// consequential damages (including, but not limited to, procurement of
// substitute goods or services; loss of use, data, or profits; or
// business interruption) however caused and on any theory of liability,
// whether in contract, strict liability, or tort (including negligence
// or otherwise) arising in any way out of the use of this software, even
// if advised of the possibility of such damage.
//
// ******************************************************* EndRiceCopyright *

//
// CUPTI synchronous sampling via PAPI sample source simple oo interface
//

/******************************************************************************
 * system includes
 *****************************************************************************/

#include <alloca.h>
#include <assert.h>
#include <ctype.h>
#include <papi.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ucontext.h>
#include <stdbool.h>

#include <pthread.h>

#if 0
#include <cuda.h>
#include <cupti.h>
#endif

/******************************************************************************
 * libmonitor
 *****************************************************************************/

#include <monitor.h>


/******************************************************************************
 * local includes
 *****************************************************************************/

#include "simple_oo.h"
#include "sample_source_obj.h"
#include "common.h"

#include <hpcrun/hpcrun_options.h>
#include <hpcrun/hpcrun_stats.h>
#include <hpcrun/metrics.h>
#include <hpcrun/sample_sources_registered.h>
#include <hpcrun/sample_event.h>
#include <hpcrun/thread_data.h>
#include <utilities/tokenize.h>
#include <messages/messages.h>
#include <lush/lush-backtrace.h>
#include <lib/prof-lean/hpcrun-fmt.h>



/******************************************************************************
 * macros
 *****************************************************************************/

#define FORALL_EM(macro)	\
  macro("EM_INVALID",       0)	\
  macro("EM_HTOD_BYTES",    1)	\
  macro("EM_DTOH_BYTES",    2)	\
  macro("EM_HTOA_BYTES",    3)	\
  macro("EM_ATOH_BYTES",    4)	\
  macro("EM_ATOA_BYTES",    5)	\
  macro("EM_ATOD_BYTES",    6)	\
  macro("EM_DTOA_BYTES",    7)	\
  macro("EM_DTOD_BYTES",    8)	\
  macro("EM_HTOH_BYTES",    9)	\
  macro("EM_PTOP_BYTES",   10) 

#define FORALL_EM_TIME(macro)  \
  macro("EM_TIME (us)",    11)  

#define FORALL_IM(macro)	\
  macro("IM_INVALID",       0)	\
  macro("IM_HTOD_BYTES",    1)	\
  macro("IM_DTOH_BYTES",    2)	\
  macro("IM_CPU_PF",        3)	\
  macro("IM_GPU_PF",        4)	\
  macro("IM_THRASH",        5)	\
  macro("IM_THROT",         6)	\
  macro("IM_RMAP",          7)	\
  macro("IM_DTOD_BYTES",    8)

#define FORALL_IM_TIME(macro)  \
  macro("EM_TIME (us)",     9)  

#define FORALL_STL(macro)	\
  macro("STL_INVALID",      0)	\
  macro("STL_NONE",         1)	\
  macro("STL_IFETCH",       2)	\
  macro("STL_EXC_DEP",      3)	\
  macro("STL_MEM_DEP",      4)	\
  macro("STL_TEX",          5)	\
  macro("STL_SYNC",         6)	\
  macro("STL_CMEM_DEP",     7)	\
  macro("STL_PIPE_BSY",     8)	\
  macro("STL_MEM_THR",      9)	\
  macro("STL_NOSEL",       10)	\
  macro("STL_OTHR",        11)	\
  macro("STL_SLEEP",       12)

#define FORALL_GPU_INST(macro)  \
  macro("GPU_ISAMP",        13)  

#define COUNT_FORALL_CLAUSE(a,b) + 1
#define NUM_CLAUSES(forall_macro) 0 forall_macro(COUNT_FORALL_CLAUSE)

#if 0
#define OMPT_MEMORY_EXPLICIT  "dev_ex_memcpy"
#define OMPT_MEMORY_IMPLICIT  "dev_im_memcpy"
#define OMPT_KERNEL_EXECUTION "dev_kernel"
#endif

#define OMPT_NVIDIA "nvidia" 


/******************************************************************************
 * forward declarations 
 *****************************************************************************/

/******************************************************************************
 * local variables 
 *****************************************************************************/

static kind_info_t* ke_kind; // kernel execution
static kind_info_t* em_kind; // explicit memory copies
static kind_info_t* im_kind; // implicit memory events


int stall_metric_id[NUM_CLAUSES(FORALL_STL)+1];
int gpu_inst_metric_id;

int em_metric_id[NUM_CLAUSES(FORALL_EM)+1];
int em_time_metric_id;

int im_metric_id[NUM_CLAUSES(FORALL_IM)+1];
int im_time_metric_id;


/******************************************************************************
 * interface operations
 *****************************************************************************/

static void
METHOD_FN(init)
{
  self->state = INIT;
}

static void
METHOD_FN(thread_init)
{
  TMSG(CUDA, "thread_init");
}

static void
METHOD_FN(thread_init_action)
{
  TMSG(CUDA, "thread_init_action");
}
static void
METHOD_FN(start)
{
  TMSG(CUDA, "start");
}

static void
METHOD_FN(thread_fini_action)
{
  TMSG(CUDA, "thread_fini_action");
}

static void
METHOD_FN(stop)
{
  thread_data_t *td = hpcrun_get_thread_data();

  TD_GET(ss_state)[self->sel_idx] = STOP;
}

static void
METHOD_FN(shutdown)
{
  self->state = UNINIT;
}


static bool
METHOD_FN(supports_event, const char *ev_str)
{
  return hpcrun_ev_is(ev_str, OMPT_NVIDIA);

#if 0
    hpcrun_ev_is(ev_str, OMPT_MEMORY_EXPLICIT) ||
    hpcrun_ev_is(ev_str, OMPT_MEMORY_IMPLICIT) ||
    hpcrun_ev_is(ev_str, OMPT_KERNEL_EXECUTION);
#endif
}
 
static void
METHOD_FN(process_event_list, int lush_metrics)
{
  int nevents = (self->evl).nevents;

  TMSG(CUDA,"nevents = %d", nevents);

#define getindex(name, index) index
#define declare_stall_metric(name, index) \
  stall_metric_id[index] = hpcrun_set_new_metric_info(name);

  ke_kind = hpcrun_metrics_new_kind();
  kind_info_t* incoming_kind = hpcrun_metrics_switch_kind(ke_kind);

  FORALL_STL(declare_stall_metric);	
  FORALL_GPU_INST(declare_stall_metric);
  gpu_inst_metric_id = stall_metric_id[FORALL_GPU_INST(getindex)];
  hpcrun_finalize_metrics();

#define declare_im_metric(name, index) \
  im_metric_id[index] = hpcrun_set_new_metric_info(name);

  im_kind = hpcrun_metrics_new_kind();
  hpcrun_metrics_switch_kind(im_kind);

  FORALL_IM(declare_im_metric);	
  FORALL_IM_TIME(declare_im_metric);
  im_time_metric_id = im_metric_id[FORALL_IM_TIME(getindex)];
  hpcrun_finalize_metrics();

#define declare_em_metric(name, index) \
  em_metric_id[index] = hpcrun_set_new_metric_info(name);

  em_kind = hpcrun_metrics_new_kind();
  hpcrun_metrics_switch_kind(em_kind);

  FORALL_EM(declare_em_metric);	
  FORALL_EM_TIME(declare_em_metric);
  em_time_metric_id = em_metric_id[FORALL_EM_TIME(getindex)];
  hpcrun_finalize_metrics();

  hpcrun_metrics_switch_kind(incoming_kind);
}

static void
METHOD_FN(gen_event_set,int lush_metrics)
{
}

static void
METHOD_FN(display_events)
{
  printf("\n");
}

/***************************************************************************
 * object
 ***************************************************************************/

#define ss_name ompt_nvidia_gpu
#define ss_cls SS_HARDWARE

#include "ss_obj.h"

/******************************************************************************
 * private operations 
 *****************************************************************************/






