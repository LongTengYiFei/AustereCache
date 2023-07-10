#ifndef __SDEDUP_H__
#define __SDEDUP_H__
#include <memory>
#include <map>
#include "common/common.h"
#include "metadata/metadata_module.h"
#include "compression/compression_module.h"
#include "chunking/chunk_module.h"

#define SELECTIVE_DEDUPLICATION_TRESHOULD 10

namespace cache {
  class SelectiveDeduplicationModule {
    static std::map<std::string, int> GFFI;
    SelectiveDeduplicationModule();
    public:
    // check whether chunk c is high frequency or not.
    static bool isHighFreq(Chunk &chunk);
  };
}

#endif