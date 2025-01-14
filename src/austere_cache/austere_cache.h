#ifndef __AUSTERECACHE_H__
#define __AUSTERECACHE_H__
#include <cstdint>
#include <memory>
#include "chunking/chunk_module.h"
#include "deduplication/deduplication_module.h"
#include "compression/compression_module.h"
#include "manage/manage_module.h"
#include "utils/thread_pool.h"
#include <set>
#include <string>
#include <mutex>
#include <unistd.h>
#include <string>
#include <ios>
#include <iostream>
#include <fstream>

namespace cache {
class AustereCache {
 public:
  AustereCache();
  ~AustereCache();
  void read(uint64_t addr, void *buf, uint32_t len);
  void write(uint64_t addr, void *buf, uint32_t len, double cb);
  inline void resetStatistics() { stats_->reset(); }
  inline void dumpStatistics() { stats_->dump(); }
  void dumpMemoryUsage(double& vm_usage, double& resident_set)
  {
    using std::ios_base;
    using std::ifstream;
    using std::string;

    vm_usage     = 0.0;
    resident_set = 0.0;

    // 'file' stat seems to give the most reliable results
    //
    ifstream stat_stream("/proc/self/stat",ios_base::in);

    // dummy vars for leading entries in stat that we don't care about
    //
    string pid, comm, state, ppid, pgrp, session, tty_nr;
    string tpgid, flags, minflt, cminflt, majflt, cmajflt;
    string utime, stime, cutime, cstime, priority, nice;
    string O, itrealvalue, starttime;

    // the two fields we want
    //
    unsigned long vsize;
    long rss;

    stat_stream >> pid >> comm >> state >> ppid >> pgrp >> session >> tty_nr
      >> tpgid >> flags >> minflt >> cminflt >> majflt >> cmajflt
      >> utime >> stime >> cutime >> cstime >> priority >> nice
      >> O >> itrealvalue >> starttime >> vsize >> rss; // don't care about the rest

    stat_stream.close();

    long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024; // in case x86-64 is configured to use 2MB pages
    vm_usage     = vsize / 1024.0;
    resident_set = rss * page_size_kb;
  }

 private:
  void internalRead(Chunk &chunk);
  void internalWrite(Chunk &chunk);

  // Statistics
  Stats* stats_;
};
}


#endif
