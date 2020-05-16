//************************* System Include Files ****************************

#include <iostream>
#include <fstream>
#include <sstream>

#include <climits>
#include <cstring>
#include <cstdio>

#include <bitset>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <algorithm>
#include <iomanip>
#include <queue>
#include <limits>

#include <sys/stat.h>

//*************************** User Include Files ****************************

#include <include/uint.h>
#include <include/gcc-attr.h>
#include <include/gpu-metric-names.h>

#include "GPUAdvisor.hpp"
#include "../MetricNameProfMap.hpp"
#include "../CCTGraph.hpp"

using std::string;

#include <lib/prof/CCT-Tree.hpp>
#include <lib/prof/Struct-Tree.hpp>
#include <lib/prof/Metric-Mgr.hpp>
#include <lib/prof/Metric-ADesc.hpp>

#include <lib/profxml/XercesUtil.hpp>
#include <lib/profxml/PGMReader.hpp>

#include <lib/prof-lean/hpcrun-metric.h>

#include <lib/binutils/LM.hpp>
#include <lib/binutils/VMAInterval.hpp>

#include <lib/cuda/DotCFG.hpp>

#include <lib/xml/xml.hpp>

#include <lib/support/diagnostics.h>
#include <lib/support/Logic.hpp>
#include <lib/support/IOUtil.hpp>
#include <lib/support/StrUtil.hpp>

#define DEBUG_GPUADVISOR 1
#define DEBUG_GPUADVISOR_DETAILS 1

#define WARP_SIZE 32

namespace Analysis {

/*
 * Interface methods
 */

void GPUAdvisor::init() {
  if (_inst_stall_metrics.size() != 0) {
    // Init already
    return;
  }

  // Init individual metrics
  _issue_metric = GPU_INST_METRIC_NAME":STL_NONE";
  _stall_metric = GPU_INST_METRIC_NAME":STL_ANY";
  _inst_metric = GPU_INST_METRIC_NAME;

  _invalid_stall_metric = GPU_INST_METRIC_NAME":STL_INV";
  _tex_stall_metric = GPU_INST_METRIC_NAME":STL_TMEM";
  _ifetch_stall_metric = GPU_INST_METRIC_NAME":STL_IFET";
  _pipe_bsy_stall_metric = GPU_INST_METRIC_NAME":STL_PIPE";
  _mem_thr_stall_metric = GPU_INST_METRIC_NAME":STL_MTHR";
  _nosel_stall_metric = GPU_INST_METRIC_NAME":STL_NSEL";
  _other_stall_metric = GPU_INST_METRIC_NAME":STL_OTHR";
  _sleep_stall_metric = GPU_INST_METRIC_NAME":STL_SLP";
  _cmem_stall_metric = GPU_INST_METRIC_NAME":STL_CMEM";
  
  _inst_stall_metrics.insert(_invalid_stall_metric);
  _inst_stall_metrics.insert(_tex_stall_metric);
  _inst_stall_metrics.insert(_ifetch_stall_metric);
  _inst_stall_metrics.insert(_pipe_bsy_stall_metric);
  _inst_stall_metrics.insert(_mem_thr_stall_metric);
  _inst_stall_metrics.insert(_nosel_stall_metric);
  _inst_stall_metrics.insert(_other_stall_metric);
  _inst_stall_metrics.insert(_sleep_stall_metric);
  _inst_stall_metrics.insert(_cmem_stall_metric);

  _exec_dep_stall_metric = GPU_INST_METRIC_NAME":STL_IDEP";
  _exec_dep_dep_stall_metric = GPU_INST_METRIC_NAME":STL_IDEP_DEP";
  _exec_dep_sche_stall_metric = GPU_INST_METRIC_NAME":STL_IDEP_SCHE";
  _exec_dep_smem_stall_metric = GPU_INST_METRIC_NAME":STL_IDEP_SMEM";
  _exec_dep_war_stall_metric = GPU_INST_METRIC_NAME":STL_IDEP_WAR";
  _mem_dep_stall_metric = GPU_INST_METRIC_NAME":STL_GMEM";
  _mem_dep_gmem_stall_metric = GPU_INST_METRIC_NAME":STL_GMEM_GMEM";
  _mem_dep_lmem_stall_metric = GPU_INST_METRIC_NAME":STL_GMEM_LMEM";
  _sync_stall_metric = GPU_INST_METRIC_NAME":STL_SYNC";

  _dep_stall_metrics.insert(_exec_dep_stall_metric);
  _dep_stall_metrics.insert(_exec_dep_dep_stall_metric);
  _dep_stall_metrics.insert(_exec_dep_sche_stall_metric);
  _dep_stall_metrics.insert(_exec_dep_smem_stall_metric);
  _dep_stall_metrics.insert(_exec_dep_war_stall_metric);
  _dep_stall_metrics.insert(_mem_dep_stall_metric);
  _dep_stall_metrics.insert(_mem_dep_gmem_stall_metric);
  _dep_stall_metrics.insert(_mem_dep_lmem_stall_metric);
  _dep_stall_metrics.insert(_sync_stall_metric);

  for (auto &s : _inst_stall_metrics) {
    _metric_name_prof_map->add("BLAME " + s);
  }

  for (auto &s : _dep_stall_metrics) {
    _metric_name_prof_map->add("BLAME " + s);
  }

  // Init optimizers
  _loop_unroll_optimizer = GPUOptimizerFactory(LOOP_UNROLL);
  _strength_reduction_optimizer = GPUOptimizerFactory(STRENGTH_REDUCTION);
  _code_reorder_optimizer = GPUOptimizerFactory(CODE_REORDER);

  _code_optimizers.push_back(_loop_unroll_optimizer);
  _code_optimizers.push_back(_strength_reduction_optimizer);
  _code_optimizers.push_back(_code_reorder_optimizer);

  // Static struct
  auto *struct_root = _prof->structure()->root();
  Prof::Struct::ANodeIterator struct_iter(struct_root, NULL/*filter*/, true/*leavesOnly*/,
    IteratorStack::PreOrder);
  for (Prof::Struct::ANode *n = NULL; (n = struct_iter.current()); ++struct_iter) {
    if (n->type() == Prof::Struct::ANode::TyStmt) {
      auto *stmt = dynamic_cast<Prof::Struct::Stmt *>(n);
      for (auto &vma_interval : stmt->vmaSet()) {
        auto vma = vma_interval.beg();
        _vma_struct_map[vma] = stmt;
      }
    }
  }
}

// 1. Init static instruction information in vma_prop_map
// 2. Init an instruction dependency graph
void GPUAdvisor::configInst(const std::vector<CudaParse::Function *> &functions) {
  _vma_prop_map.clear();
  _inst_dep_graph.clear();
  _function_offset.clear();

  // Property map
  for (auto *function : functions) {
    _function_offset[function->address] = function->index;

    for (auto *block : function->blocks) {
      for (auto *_inst : block->insts) {
        auto *inst = _inst->inst_stat;

        VMAProperty property;

        property.inst = inst;
        // Ensure inst->pc has been relocated
        property.vma = inst->pc;
        property.function = function;
        property.block = block;

        _vma_prop_map[property.vma] = property;
      }
    }
  }

  // Instruction Graph
  for (auto &iter : _vma_prop_map) {
    auto *inst = iter.second.inst;
    _inst_dep_graph.addNode(inst);

    // Add latency dependencies
    for (auto &inst_iter : inst->assign_pcs) {
      for (auto pc : inst_iter.second) {
        auto *dep_inst = _vma_prop_map[pc].inst;
        _inst_dep_graph.addEdge(dep_inst, inst);
      }
    }

    for (auto &inst_iter : inst->passign_pcs) {
      for (auto pc : inst_iter.second) {
        auto *dep_inst = _vma_prop_map[pc].inst;
        _inst_dep_graph.addEdge(dep_inst, inst);
      }
    }

    for (auto &inst_iter : inst->bassign_pcs) {
      for (auto pc : inst_iter.second) {
        auto *dep_inst = _vma_prop_map[pc].inst;
        _inst_dep_graph.addEdge(dep_inst, inst);
      }
    }

    for (auto pc : inst->predicate_assign_pcs) {
      auto *dep_inst = _vma_prop_map[pc].inst;
      _inst_dep_graph.addEdge(dep_inst, inst);
    }
  }
  
  if (DEBUG_GPUADVISOR_DETAILS) {
    std::cout << "Instruction dependency graph: " << std::endl;
    debugInstDepGraph();
    std::cout << std::endl;
  }
}


void GPUAdvisor::configGPURoot(Prof::CCT::ADynNode *gpu_root) {
  // Update current root
  this->_gpu_root = gpu_root;

  // TODO(Keren): Update kernel characteristics

  // TODO(Keren): Find device tag under the root and use the corresponding archtecture
  // Problem: currently we only have device tags for call instructions
  this->_arch = new V100(); 

  // Update vma->latency mapping
  for (auto &iter : _vma_prop_map) {
    auto &prop = iter.second;
    auto *inst = prop.inst;

    auto latency = _arch->latency(inst->op);
    prop.latency_lower = latency.first;
    prop.latency_upper = latency.second;
    prop.latency_issue = _arch->issue(inst->op);

    // Clear previous vma->prof mapping
    prop.prof_node = NULL;
  }

  // Update vma->prof mapping
  Prof::CCT::ANodeIterator prof_it(_gpu_root, NULL/*filter*/, true/*leavesOnly*/,
    IteratorStack::PreOrder);
  for (Prof::CCT::ANode *n = NULL; (n = prof_it.current()); ++prof_it) {
    Prof::CCT::ADynNode* n_dyn = dynamic_cast<Prof::CCT::ADynNode*>(n);
    if (n_dyn) {
      auto vma = n_dyn->lmIP();
      if (_vma_prop_map.find(vma) != _vma_prop_map.end()) {
        _vma_prop_map[vma].prof_node = n_dyn;
      }
    }
  }
}

}  // namespace Analysis
