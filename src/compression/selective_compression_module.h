#ifndef __SELECTIVECOMPRESSIONMODULE_H__
#define __SELECTIVECOMPRESSIONMODULE_H__

#include "common/common.h"
#include "chunking/chunk_module.h"

/*
    Chunk whose compression ratio above this threshould is considered compression friendly,
    the lower is unfriendly will filtered by selective compression module.
    The threshold is some different with FAST'13 Harnik, 
    define by original chunk size divide compressed chunk size
*/
#define COMPRESSION_RATIO_TRESHOULD 1.5

namespace cache {
class SelectiveCompressionModule {
    SelectiveCompressionModule() = default;
    public:
        static SelectiveCompressionModule& getInstance();
        bool compressible(const Chunk & chunk);
};
}

#endif //__SELECTIVECOMPRESSIONMODULE_H__