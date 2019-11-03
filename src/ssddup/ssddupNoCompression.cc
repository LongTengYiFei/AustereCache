#include "ssddup.h"

#if defined(CACHE_DEDUP) && (defined(DLRU) || defined(DARC) || defined(BUCKETDLRU))
namespace cache {
  void SSDDup::internalRead(Chunk &chunk)
  {
    DeduplicationModule::lookup(chunk);
    Stats::getInstance().addReadLookupStatistics(chunk);
    // printf("TEST: %s, manageModule_ read\n", __func__);
    ManageModule::getInstance().read(chunk);

    if (chunk.lookupResult_ == NOT_HIT) {
      chunk.computeFingerprint();
      DeduplicationModule::dedup(chunk);
      ManageModule::getInstance().updateMetadata(chunk);
      if (chunk.dedupResult_ == NOT_DUP) {
        ManageModule::getInstance().write(chunk);
      }

      Stats::getInstance().add_read_post_dedup_stat(chunk);
    } else {
      ManageModule::getInstance().updateMetadata(chunk);
    }
  }

  void SSDDup::internalWrite(Chunk &chunk)
  {
    chunk.computeFingerprint();
    DeduplicationModule::dedup(chunk);
    ManageModule::getInstance().updateMetadata(chunk);
    ManageModule::getInstance().write(chunk);
#if defined(WRITE_BACK_CACHE)
    DirtyList::getInstance().addLatestUpdate(chunk.addr_, chunk.cachedataLocation_, chunk.len_);
#endif

    Stats::getInstance().add_write_stat(chunk);
  }
}
#endif