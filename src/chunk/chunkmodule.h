#ifndef __CHUNK_H__
#define __CHUNK_H__
#include <cstdint>

namespace cache {

  struct Chunk {
    uint32_t _addr;
    uint32_t _len;
    uint32_t _mode; // read, write, or read-modify-write
    //uint32_t _fingerprint;
    uint8_t *_buf;

    uint32_t _lba_hash;
    uint32_t _ca_hash;
    uint8_t  _ca[16];

    void fingerprinting();
    inline bool is_end() { return _len == 0; }
    bool is_partial();
    Chunk construct_read_chunk(uint8_t *buf);
    void merge(const Chunk &c);
  };

  class Chunker {
   public:
    // initialize a chunking iterator
    // read is possibly triggered to deal with unaligned situation
    Chunker(uint8_t *buf, uint32_t len, uint32_t addr);
    // obtain next chunk, addr, len, and buf
    bool next(Chunk &c);
   protected:
    uint32_t _addr;
    uint32_t _len;
    uint8_t *_buf;
    uint32_t _chunk_size;
    uint32_t _mode;
  };

  class ChunkModule {
   public:
     ChunkModule();
     Chunker create_chunker(uint32_t addr, uint32_t len, uint8_t *buf);
  };
}

#endif
