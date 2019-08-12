#include <fstream>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "utils/utils.h"
#include "utils/gen_zipf.h"
#include "ssddup/ssddup.h"
#include "workload_conf.h"

#include <vector>
#include <thread>
#include <atomic>
#define DIRECT_IO

namespace cache {

class RunSystem {
 public:
  RunSystem() {
    _multi_thread = 0;
    _num_workers = 1;
    genzipf::rand_val(2);
  }
  ~RunSystem() {
    free(_workload_chunks);
    for (int i = 0; i < _num_workers; ++i)
      free(_read_data[i]);
  }

  void parse_argument(int argc, char **argv)
  {
    char *param, *value;
    for (int i = 1; i < argc; i += 2) {
      param = argv[i];
      value = argv[i + 1];
      if (value == nullptr) {
        std::cout << "RunSystem::parse_argument invalid argument!" << std::endl;
        print_help();
        exit(1);
      }
      if (strcmp(param, "--trace") == 0) {
        // read workload configuration
        int fd = open(value, O_RDONLY);
        int n = read(fd, &_workload_conf, sizeof(cache::WorkloadConfiguration));
        if (n != sizeof(cache::WorkloadConfiguration)) {
          std::cout << "RunSystem:parse_argument read configuration failed!" << std::endl;
          exit(1);
        }
        // parse workload configuration
        _chunk_size = _workload_conf._chunk_size;
        _num_unique_chunks = _workload_conf._num_unique_chunks;
        _num_chunks = _workload_conf._num_chunks;
        _working_set_size = _chunk_size * _num_chunks;
        _distribution = _workload_conf._distribution;
        _skewness = _workload_conf._skewness;
        std::cout << _chunk_size << " " << _num_unique_chunks << " " << _num_chunks << " " << _working_set_size << " " << _distribution << " " << _skewness << std::endl;

        // read working set
#ifdef DIRECT_IO
        _workload_chunks = (char *)aligned_alloc(512, _working_set_size);
        _unique_chunks = (char *)aligned_alloc(512, _num_unique_chunks * _chunk_size);
#else
        _workload_chunks = reinterpret_cast<char*>(malloc(_working_set_size));
        _unique_chunks = reinterpret_cast<char*>(malloc(_num_unique_chunks * _chunk_size));
#endif
        n = read(fd, _workload_chunks, _working_set_size);
        if (n != _working_set_size) {
          std::cout << "RunSystem: read working set failed!" << std::endl; 
          exit(1);
        }
        n = read(fd, _unique_chunks, _num_unique_chunks * _chunk_size);
      } else if (strcmp(param, "--wr-ratio") == 0) {
        _wr_ratio = atof(value);
      } else if (strcmp(param, "--ca-bits") == 0) {
        Config::get_configuration().set_ca_bucket_no_len(atoi(value));
      } else if (strcmp(param, "--lba-bits") == 0) {
        Config::get_configuration().set_lba_bucket_no_len(atoi(value));
      } else if (strcmp(param, "--multi-thread") == 0) {
        _multi_thread = atoi(value);
      } else if (strcmp(param, "--num-workers") == 0) {
        _num_workers = atoi(value);
      } else if (strcmp(param, "--fingerprint-algorithm") == 0) {
        Config::get_configuration().set_fingerprint_algorithm(atoi(value));
      } else if (strcmp(param, "--fingerprint-computation-method") == 0) {
        Config::get_configuration().set_fingerprint_computation_method(atoi(value));
      } else if (strcmp(param, "--write-buffer-size") == 0) {
        Config::get_configuration().set_write_buffer_size(atoi(value));
      } else if (strcmp(param, "--direct-io") == 0) {
        Config::get_configuration().set_direct_io(atoi(value));
      } else if (strcmp(param, "--access-pattern") == 0) {
        FILE *f = fopen(value, "r");
        int chunk_id, length;
        if (f == nullptr) {
          std::cout << "Access pattern file does not exist!" << std::endl;
          exit(-1);
        }
        while (fscanf(f, "%d %d", &chunk_id, &length) != -1) {
          _accesses.push_back(std::make_pair(chunk_id, length));
        }
      }
    }

    if (_workload_chunks == nullptr) {
      std::cout << "RunSystem::parse_argument no input file given!" << std::endl;
      print_help();
      exit(1);
    }
    {
      _read_data = (char**)malloc(sizeof(char*) * _num_workers);
      for (int i = 0; i < _num_workers; i++) {
        _read_data[i] = (char *)aligned_alloc(512, _working_set_size);
      }
    }
    _workload_conf.print_current_parameters();
    //Config::get_configuration().set_cache_device_name("./cache_device");
    //Config::get_configuration().set_primary_device_name("./primary_device");
    //Config::get_configuration().set_cache_device_name("./ramdisk/cache_device");
    //Config::get_configuration().set_primary_device_name("./primary_device");
    //Config::get_configuration().set_primary_device_name("./ramdisk/primary_device");
    //Config::get_configuration().set_cache_device_name("./ramdisk/cache_device");
    //Config::get_configuration().set_primary_device_name("/dev/sdb");
    //Config::get_configuration().set_cache_device_name("/dev/sda");
    _ssddup = std::make_unique<SSDDup>();
  }

  void warm_up()
  {
    std::cout << sizeof(WorkloadConfiguration) << std::endl;
    //_ssddup->read(0, _read_data[0], _workload_conf._working_set_size);
    _ssddup->write(0, _workload_chunks, _working_set_size);
    _ssddup->reset_stats();
    sync();
    //_ssddup->sync();
    DEBUG("finish warm up");
  }

  void work(int n_requests, std::atomic<uint64_t> &total_bytes)
  {
    std::vector<std::thread> workers;
    if (_accesses.size() != 0) n_requests = _accesses.size();
    for (int thread_id = 0; thread_id < _num_workers; thread_id++) {
      workers.push_back(std::thread( [&, thread_id]()
        {
          srand(thread_id);
          std::cout << _accesses.size() << std::endl;
          for (uint32_t i = 0; i < _accesses.size(); i++) {
            uint32_t begin, len;

            begin = _accesses[i].first;
            len = _accesses[i].second;

            uint32_t op = rand() % 100;
            total_bytes.fetch_add(len * _chunk_size, std::memory_order_relaxed);
            if (op < 0) {
            //if (op < _wr_ratio / (_wr_ratio + 1) * 100) {
              modify_chunk(begin, len);
              _ssddup->write(begin * _chunk_size, _workload_chunks + begin * _chunk_size, len * _chunk_size);
            } else {
              _ssddup->read(begin * _chunk_size, _read_data[thread_id] + begin * _chunk_size, len * _chunk_size);
              int result = -1;
              result = compare_array(
                  _workload_chunks + begin * _chunk_size, _read_data[thread_id] + begin * _chunk_size, 
                  len * _chunk_size
                  );
              if (result != len * _chunk_size) {
                std::cout << "RunSystem:work content not match for address: "
                  << (begin * _chunk_size) << ", length: " << (len * _chunk_size)
                  << " matched length: " << result << std::endl;
                std::cout << "The request num: " << i << std::endl;
                exit(1);
              }
            }
          }
        }
      ));
    }
    for (auto &worker : workers) {
      worker.join();
    }
    sync();
  }

 private: 
  int compare_array(void *a, void *b, uint32_t length)
  {
    int count = 0;
    for (uint32_t i = 0; i < length; i++) {
      if (((char*)a)[i] == ((char*)b)[i]) count++;
      else return count;
    }
    return count;
  }

  void print_help()
  {
    std::cout << "--wr-ratio: the ratio of write chunks and read chunks" << std::endl
      << "--trace: input workload file" << std::endl
      << "--wr-ratio: write-read ratio of the workload float number (0 ~ 1)" << std::endl
      << "--multi-thread: enable multi-thread or not, 1 means enable, 0 means disable" << std::endl
      << "--ca-bits: number of bits of ca index" << std::endl
      << "--num-workers: number of workers to issue read requests" << std::endl
      << "--fingerprint-algorithm: main fingerprint algorithm, 0 - SHA1, 1 - murmurhash" << std::endl
      << "--fingerprint-computation-method: indicate the fingerprint computation optimization, 0 - SHA1, 1 - weak hash + memcmp, 2 - weak hash + SHA1" << std::endl;
  }

  void modify_chunk(uint32_t chunk_id, uint32_t len)
  {
    uint64_t rd = 0;
    for (int i = 0; i < len; ++i) {
      if (_distribution == 0) {
        rd = rand();
      } else {
        rd = genzipf::zipf(_skewness, _num_unique_chunks);
      }
      rd %= _num_unique_chunks;
      memcpy(_workload_chunks + ((uint64_t)(chunk_id + i)) * _chunk_size,
          _unique_chunks + rd * _chunk_size,
          _chunk_size);
    }
  }

  char *_workload_chunks, *_unique_chunks, **_read_data;

  WorkloadConfiguration _workload_conf; 
  uint32_t _working_set_size;
  uint32_t _chunk_size;
  uint32_t _num_unique_chunks;
  uint32_t _num_chunks;
  uint32_t _distribution;
  float _skewness;

  bool _multi_thread;
  int _num_workers;
  double _wr_ratio;

  std::unique_ptr<SSDDup> _ssddup;
  std::vector<std::pair<uint32_t, uint32_t>> _accesses;
};

}
int main(int argc, char **argv)
{
  srand(0);
  cache::RunSystem run_system;
  run_system.parse_argument(argc, argv);
  run_system.warm_up();

  std::atomic<uint64_t> total_bytes(0);
  int elapsed = 0;
  PERF_FUNCTION(elapsed, run_system.work, 4096, total_bytes);
  std::cout << (double)total_bytes / (1024 * 1024) << std::endl;
  std::cout << elapsed << " ms" << std::endl;
  std::cout << "Throughput: " << (double)total_bytes / elapsed << " MBytes/s" << std::endl;
}

