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

bool SelectiveCompressionModule::compressible(const Chunk & chunk)
{
  if(chunk.compressibility > COMPRESSION_RATIO_TRESHOULD)
    return true;
  return false;
}

}