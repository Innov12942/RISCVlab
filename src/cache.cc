#include "cache.h"
#include "def.h"

/*x should be 2^k, return k*/
inline int log2(int x){
    int s = 0;
    for(int i = 0; i < 32; i++)
        if(x & (1 << i)){
            s = i;
            break;
    }
    return s;
}

void Cache::HandleRequest(uint64_t addr, int bytes, int read,
                          char *content, int &hit, int &time) {

    ASSERT(cache_content != NULL);
    hit = 0;
    time = 0;
    stats_.access_counter ++;
    // Find which set correspond to addr
    int s = log2(config_.set_num);
    uint64_t set_id = (addr >> CACHE_B) & ((1 << s) - 1);
    uint64_t addr_tag = (addr >> (CACHE_B + s));

    // Get offset in the target block
    int offset = addr & ((1 << CACHE_B) - 1);
    ASSERT(offset < BLOCK_SIZE);
    ASSERT(offset + bytes <= BLOCK_SIZE);

  // Bypass?
  if (!BypassDecision()) {
    PartitionAlgorithm();
    // Miss?
    if (ReplaceDecision(addr)) {
        stats_.miss_num ++;
      // Choose victim
      ReplaceAlgorithm(set_id, read, time);
    } else {
      // return hit & time
      hit = 1;
      time += latency_.bus_latency + latency_.hit_latency;
      stats_.access_time += time;

      //copy data
      for(int k = 0; k < entry_num; k++)
        if( cache_content[k].valid &&
            cache_content[k].set == set_id && 
            cache_content[k].tag == addr_tag){
            Aging(k);
            if(read)
                for(int bitr = 0; bitr < bytes; bitr++)
                    content[bitr] = cache_content[k].data[bitr + offset];

            else{   //write
                if(config_.write_through){
                    for(int bitr = 0; bitr < bytes; bitr++){
                        cache_content[k].data[bitr + offset] = content[bitr];
                        cache_content[k].dirty = FALSE;
                    }

                    int lower_hit, lower_time;
                    lower_->HandleRequest(addr, bytes, read, content,
                                          lower_hit, lower_time);
                    time += lower_time;
                    stats_.fetch_num ++;
                }
                else{   //write_back
                    for(int bitr = 0; bitr < bytes; bitr++){
                        cache_content[k].data[bitr + offset] = content[bitr];
                        cache_content[k].dirty = TRUE;
                    }
                }
            }
            break;
        }
      return;
    }
  }
  // Prefetch?
  if (PrefetchDecision()) {
    PrefetchAlgorithm();
  } else {
        // Fetch from lower layer
        hit = 0;

        // Find a cache entry to store data from lower layer
        if(read == 1)
            for(int i = 0; i < entry_num; i++){
                if(cache_content[i].set == set_id && cache_content[i].valid == FALSE){
                    //set valid
                    cache_content[i].valid = TRUE;
                    //read data
                    char block_buf[BLOCK_SIZE + 2];
                    int readHit, readTime;
                    uint64_t addr_startOfBlock = addr - offset;
                    lower_->HandleRequest(addr_startOfBlock, BLOCK_SIZE, 1, block_buf,
                              readHit, readTime);
                    for(int j = 0; j < BLOCK_SIZE; j++)
                        cache_content[i].data[j] = block_buf[j];

                    for(int bitr = 0; bitr < bytes; bitr++)
                        content[bitr] = cache_content[i].data[bitr + offset];

                    time += latency_.bus_latency + readTime;
                    stats_.access_time += latency_.bus_latency;
                    stats_.fetch_num ++;
                    //set time stamp, 0 represent a new entry
                    cache_content[i].timeStamp = 0;
                    //set tag
                    cache_content[i].tag = addr_tag;
                    //set dirty
                    cache_content[i].dirty = FALSE;

                    //aging, all other cache entries should grow older
                    Aging(i);

                    break;
                }
            }

        if(read == 0){
            if(config_.write_allocate){
                for(int i = 0; i < entry_num; i++){
                    if(cache_content[i].set == set_id && cache_content[i].valid == FALSE){
                        int lower_hit, lower_time;
                        lower_->HandleRequest(addr, bytes, read, content,
                                              lower_hit, lower_time);
                        time += lower_time;

                        int lowHit, lowTime;
                        char readBuf[BLOCK_SIZE + 2];
                        uint64_t addr_startOfBlock = addr - offset;
                        lower_->HandleRequest(addr_startOfBlock, BLOCK_SIZE, 1, readBuf,
                              lowHit, lowTime);
                        time += lowTime + latency_.bus_latency;
                        stats_.access_time += latency_.bus_latency;
                        stats_.fetch_num ++;

                        //set valid
                        cache_content[i].valid = TRUE;
                        //read data
                        for(int j = 0; j < BLOCK_SIZE; j++)
                            cache_content[i].data[j] = readBuf[j];
                        //set time stamp, 0 represent a new entry
                        cache_content[i].timeStamp = 0;
                        //set tag
                        cache_content[i].tag = addr_tag;
                        //set dirty
                        cache_content[i].dirty = FALSE;

                        //aging, all other cache entries should grow older
                        Aging(i);

                        break;
                    }
                }
            }
            else{
                //Non-allocate -- no bus_latency for cache
                int lower_hit, lower_time;
                lower_->HandleRequest(addr, bytes, read, content,
                                      lower_hit, lower_time);
                time += lower_time;
                stats_.access_time += 0;
                stats_.fetch_num ++;
            }
        }
    }
}

int Cache::BypassDecision() {
  return FALSE;
}

void Cache::PartitionAlgorithm() {
}

// return value : true--miss, false--hit 
int Cache::ReplaceDecision(uint64_t addr) {
    int s = log2(config_.set_num);
    uint64_t set_id = (addr >> CACHE_B) & ((1 << s) - 1);
    uint64_t addr_tag = (addr >> (CACHE_B + s));

    for(int i = 0; i < entry_num; i++){
        if(cache_content[i].set == set_id && 
            cache_content[i].valid == TRUE &&
            cache_content[i].tag == addr_tag)
            return FALSE;
    }
  return TRUE;
  //return FALSE;
}

void Cache::ReplaceAlgorithm(uint64_t set_id, int read, int &time){
    //if write-miss and non-allocate, no need to replace 
    if(!read && !config_.write_allocate)
        return;

    for(int i = 0; i < entry_num; i++)
        if(cache_content[i].valid == FALSE && 
            cache_content[i].set == set_id)
            return;
    // Find a victim
    int which_lru = 0;
    for(int i = 0; i < entry_num; i++)
        if(cache_content[i].set == set_id){
            which_lru = i;
            break;
        }
    for(int i = 0; i < entry_num; i++)
        if(cache_content[i].set == set_id &&
            cache_content[i].timeStamp > cache_content[which_lru].timeStamp){
            which_lru = i;
        }
    cache_content[which_lru].valid = FALSE;

    //if dirty, flush to lower layer
    if(cache_content[which_lru].dirty){
        int lower_hit, lower_time;
        uint64_t tag_t = cache_content[which_lru].tag;
        int s = log2(config_.set_num);
        uint64_t addr = (tag_t << (CACHE_B + s)) | (set_id << CACHE_B);
    //    printf("cache dirty flush, addr:%lx\n", addr);
        lower_->HandleRequest(addr, BLOCK_SIZE, 0, 
                                cache_content[which_lru].data,
                                lower_hit, lower_time);

        /*Does cpu waits for dirty cache entry to flush*/
        //time += lower_time;
    }

    stats_.replace_num ++;
}

int Cache::PrefetchDecision() {
  return FALSE;
}

void Cache::PrefetchAlgorithm() {
}

void Cache::buildContent(){
    entry_num = config_.size / BLOCK_SIZE;
    cache_content = new CacheEntry[entry_num];
    for(int i = 0; i < entry_num; i++){
        cache_content[i].valid = FALSE;
        cache_content[i].set = i % config_.set_num;       
    }
}

Cache::Cache(){
    cache_content = NULL;
}

Cache::~Cache(){
    if(cache_content){
        delete []cache_content;
    }
}

void Cache::SetConfig(CacheConfig cc){
    if(cache_content != NULL){
        printf("cache config already exists\n");
        ASSERT(FALSE);
    }
    config_ = cc;
    if(cc.size % BLOCK_SIZE != 0)
        ASSERT(FALSE);

    int entryNum = cc.size / BLOCK_SIZE;
    if(entryNum % cc.set_num != 0)
        ASSERT(FALSE);

    buildContent();
}

void Cache::Aging(int new_id){
    int maxAge = 99999;
    for(int k = 0; k < entry_num; k++)
        if(new_id != k){
            if(cache_content[k].timeStamp < maxAge)
                cache_content[k].timeStamp ++;
        }
}

