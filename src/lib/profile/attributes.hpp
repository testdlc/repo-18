// -*-Mode: C++;-*-

// * BeginRiceCopyright *****************************************************
//
// $HeadURL$
// $Id$
//
// --------------------------------------------------------------------------
// Part of HPCToolkit (hpctoolkit.org)
//
// Information about sources of support for research and development of
// HPCToolkit is at 'hpctoolkit.org' and in 'README.Acknowledgments'.
// --------------------------------------------------------------------------
//
// Copyright ((c)) 2019-2020, Rice University
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

#ifndef HPCTOOLKIT_PROFILE_ATTRIBUTES_H
#define HPCTOOLKIT_PROFILE_ATTRIBUTES_H

#include "util/ragged_vector.hpp"

#include <functional>
#include <unordered_map>
#include "stdshim/filesystem.hpp"
#include "stdshim/optional.hpp"

namespace hpctoolkit {

/// Attributes unique to a particular thread within a profile. Whether this is
/// a thread (as in thd_create) or a process (as in an MPI process), this
/// roughly represents a single serial execution within the profile.
class ThreadAttributes {
public:
  /// Default (empty) constructor. No news is no news.
  ThreadAttributes();
  ThreadAttributes(const ThreadAttributes& o) = default;
  ~ThreadAttributes() = default;

  /// Get or set the ID of the host that ran this Thread.
  // MT: Externally Synchronized
  const stdshim::optional<uint32_t>& hostid() const noexcept { return m_hostid; }
  void hostid(uint32_t);

  /// Get or set the MPI rank of this Thread.
  // MT: Externally Synchronized
  const stdshim::optional<unsigned long>& mpirank() const noexcept { return m_mpirank; }
  void mpirank(unsigned long);

  /// Get or set the thread id of this Thread.
  // MT: Externally Synchronized
  const stdshim::optional<unsigned long>& threadid() const noexcept { return m_threadid; }
  void threadid(unsigned long);

  /// Get or set the process id of this Thread.
  // MT: Externally Synchronized
  const stdshim::optional<unsigned long>& procid() const noexcept { return m_procid; }
  void procid(unsigned long);

  /// Get or set the number of timepoints emitted that are local to this Thread.
  // MT: Externally Synchronized
  const stdshim::optional<unsigned long long>& timepointCnt() const noexcept { return m_timepointCnt; }
  void timepointCnt(unsigned long long);

private:
  stdshim::optional<uint32_t> m_hostid;
  stdshim::optional<unsigned long> m_mpirank;
  stdshim::optional<unsigned long> m_threadid;
  stdshim::optional<unsigned long> m_procid;
  stdshim::optional<unsigned long long> m_timepointCnt;
};

/// A single Thread within a Profile. Or something like that.
class Thread {
public:
  using ud_t = util::ragged_vector<const Thread&>;
  using met_t = util::ragged_map<>;

  Thread(ud_t::struct_t& rs)
    : Thread(rs, ThreadAttributes()) {};
  Thread(ud_t::struct_t& rs, const ThreadAttributes& attr)
    : Thread(rs, ThreadAttributes(attr)) {};
  Thread(ud_t::struct_t& rs, ThreadAttributes&& attr)
    : userdata(rs, std::cref(*this)), attributes(std::move(attr)) {};
  Thread(Thread&& o)
    : userdata(std::move(o.userdata), std::cref(*this)),
      attributes(std::move(o.attributes)) {};

  mutable ud_t userdata;

  // Attributes accociated with this Thread.
  ThreadAttributes attributes;

  /// Thread-local data that is temporary and should be removed quickly.
  class Temporary {
  public:
    // Access to the backing thread.
    operator Thread&() noexcept { return m_thread; }
    operator const Thread&() const noexcept { return m_thread; }
    Thread& thread() noexcept { return m_thread; }
    const Thread& thread() const noexcept { return m_thread; }

    // Movable, not copyable
    Temporary(const Temporary&) = delete;
    Temporary(Temporary&&) = default;

  private:
    Thread& m_thread;

    friend class ProfilePipeline;
    Temporary(Thread& t, met_t::struct_t& ms) : m_thread(t), data(ms) {};

    friend class Metric;
    met_t data;
  };
};

/// Attributes unique to a particular profile. Since a profile represents an
/// entire single execution (potentially on multiple processors), all of these
/// should be constant within a profile. If it isn't, we just issue warnings.
class ProfileAttributes {
public:
  /// Default (empty) constructor. No news is no news.
  ProfileAttributes();
  ~ProfileAttributes() = default;

  /// Get or set the name of the program being executed. Usually this is the
  /// basename of the path, but just in case it isn't always...
  // MT: Externally Synchronized
  const stdshim::optional<std::string>& name() const noexcept { return m_name; }
  void name(const std::string& s) { name(std::string(s)); };
  void name(std::string&&);

  /// Get or set the path to the program that was profiled.
  // MT: Externally Synchronized
  const stdshim::optional<stdshim::filesystem::path>& path() const noexcept { return m_path; }
  void path(const stdshim::filesystem::path& p) {
    path(stdshim::filesystem::path(p));
  }
  void path(stdshim::filesystem::path&&);

  /// Get or set the job number corrosponding to the profiled program.
  /// If the job number is not known, has_job() will return false and job()
  /// will throw an error.
  // MT: Externally Synchronized
  const stdshim::optional<unsigned long>& job() const noexcept { return m_job; }
  void job(unsigned long);

  /// Get or set individual environment variables. Note that the getter may
  /// return stdshim::nullopt if the variable is not known to be set.
  // MT: Externally Synchronized
  const stdshim::optional<const std::string&> environment(const std::string& var) const noexcept;
  void environment(const std::string& var, const std::string& val);

  /// Access the entire environment for this profile.
  // MT: Unstable (const)
  const std::unordered_map<std::string, std::string>& environment() const noexcept {
    return m_env;
  }

  /// Merge another PAttr into this one. Uses the given callback to issue
  /// human-readable warnings (at least for now). Returns true if the process
  /// proceeded without errors.
  // MT: Externally Synchronized
  bool merge(const ProfileAttributes&);

private:
  stdshim::optional<std::string> m_name;
  stdshim::optional<unsigned long> m_job;
  stdshim::optional<stdshim::filesystem::path> m_path;
  std::unordered_map<std::string, std::string> m_env;
};

}

#endif  // HPCTOOLKIT_PROFILE_ATTRIBUTES_H
