#include "selective_compression_module.h"
#include "common/config.h"
#include "common/stats.h"
#include "utils/utils.h"
#include "lz4.h"

#include <iostream>
#include <cstring>
#include <csignal>

namespace cache {

SelectiveCompressionModule& SelectiveCompressionModule::getInstance() {
  static SelectiveCompressionModule instance;
  return instance;
}

bool SelectiveCompressionModule::compressible(const uint8_t *uncompressedBuf)
{
    return true;
}

}