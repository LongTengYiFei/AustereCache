#ifndef __SELECTIVECOMPRESSIONMODULE_H__
#define __SELECTIVECOMPRESSIONMODULE_H__

#include "common/common.h"
#include "chunking/chunk_module.h"

/*
    Chunk whose compression ratio above this threshould is considered compression unfriendly,
    will filtered by selective compression module.
    This threshould is same with FAST'13 Harnik.
*/
#define COMPRESSION_RATIO_TRESHOULD 0.8

namespace cache {
class SelectiveCompressionModule {
    SelectiveCompressionModule() = default;
    public:
        static SelectiveCompressionModule& getInstance();
        static bool compressible(const uint8_t *uncompressedBuf);
};
}

#endif //__SELECTIVECOMPRESSIONMODULE_H__