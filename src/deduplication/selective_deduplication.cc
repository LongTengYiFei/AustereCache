#include "selective_deduplication.h"
#include "common/stats.h"
#include "utils/utils.h"

namespace cache {

  SelectiveDeduplicationModule::SelectiveDeduplicationModule() = default;
  std::map<std::string, int> SelectiveDeduplicationModule::GFFI;

  bool SelectiveDeduplicationModule::isHighFreq(Chunk &chunk)
  {
    std::string tmp;
    tmp.resize(20);
    for(int i=0; i<=19; i++)
        tmp[i] = chunk.fingerprint_[i];
    
    if(GFFI.count(tmp) == 0){
        GFFI[tmp] = 1;
    }else{
        GFFI[tmp] += 1;
    }

    if(GFFI[tmp] >= SELECTIVE_DEDUPLICATION_TRESHOULD){
        return true;
    }
        
    return false;
  }
}