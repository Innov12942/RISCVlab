#ifndef CACHE_CACHE_H_
#define CACHE_CACHE_H_

#include <stdint.h>
#include "storage.h"

#define CACHE_B 6
#define BLOCK_SIZE (1 << CACHE_B)

typedef struct CacheConfig_ {
  int size;
  int associativity;
  int set_num; // Number of cache sets
  int write_through; // 0|1 for back|through
  int write_allocate; // 0|1 for no-alc|alc
} CacheConfig;

typedef  struct
{
  uint64_t tag;
  uint64_t set;
  uint64_t timeStamp;  //0 is the newest 
  char data[BLOCK_SIZE];
  bool valid;
  bool dirty;
}CacheEntry;

class Cache: public Storage {
 public:
  Cache();
  ~Cache();

  // Sets & Gets
  void SetConfig(CacheConfig cc);
  void GetConfig(CacheConfig cc);
  void SetLower(Storage *ll) { lower_ = ll; }
  // Main access process
  void HandleRequest(uint64_t addr, int bytes, int read,
                     char *content, int &hit, int &time);

  void buildContent();

 private:
  // Bypassing
  int BypassDecision();
  // Partitioning
  void PartitionAlgorithm();
  // Replacement
  int ReplaceDecision(uint64_t addr);
  void ReplaceAlgorithm(uint64_t set_id, int read, int &time);
  // Prefetching
  int PrefetchDecision();
  void PrefetchAlgorithm();

  /*
   *Used for LRU,  
   *make every cahce entry aging except for new_id
  */
  void Aging(int new_id);

  CacheConfig config_;
  Storage *lower_;

  CacheEntry *cache_content;
  int entry_num;

  DISALLOW_COPY_AND_ASSIGN(Cache);
};

#endif //CACHE_CACHE_H_ 

