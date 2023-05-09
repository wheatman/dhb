#pragma once

#include "dhb/block.h"
#include "dhb/integer_log2.h"

#include <array>
#include <list>
#include <set>
#include <unordered_set>
#include <vector>

namespace dhb {

#ifndef DHB_SYSTEM_ALLOCATOR

struct BlockBucket {
    struct AgeLess {
        bool operator()(BlockArray* lhs, BlockArray* rhs) const { return lhs->age() < rhs->age(); }
    };

    std::mutex mutex;
    unsigned int next_age = 1;
    std::unordered_set<BlockArray*> all;
    std::set<BlockArray*, AgeLess> available;
};

struct BlockCache {
    std::array<BlockBucket, 48> buckets;
};

#endif // DHB_SYSTEM_ALLOCATOR

class BlockManager {
  public:
    BlockManager(unsigned int bytes_per_entry);

    BlockManager(const BlockManager&) = delete;

    BlockManager(BlockManager&& other) = delete;

    ~BlockManager();

    BlockManager& operator=(const BlockManager& other) = delete;

    BlockHandle allocate_block(Degree degree);

    void free_block(BlockHandle bptr);

  private:
    unsigned int m_bytes_per_entry;

#ifndef DHB_SYSTEM_ALLOCATOR
    BlockCache* get_own_cache();

    // Stores the block cache of each thread.
    pthread_key_t m_cache;

    // Protectes access to m_all_caches.
    std::mutex m_meta_mutex;

    // Stores all thread caches.
    // TODO: Add some mechanism (std::shared_ptr?) to deal with exiting threads.
    // We need caches with stable addresses; simply use a std::list here.
    std::list<BlockCache> m_all_caches;
#endif

    size_t m_min_bsize{4};
};

inline dhb::BlockManager::BlockManager(unsigned int bytes_per_entry) : m_bytes_per_entry(bytes_per_entry) {
#ifndef DHB_SYSTEM_ALLOCATOR
    pthread_key_create(&m_cache, nullptr);
#endif
}

inline BlockManager::~BlockManager() {
#ifndef DHB_SYSTEM_ALLOCATOR
    while (!m_all_caches.empty()) {
        auto cache = &m_all_caches.front();
        for (size_t i = 0; i < cache->buckets.size(); ++i) {
            auto bucket = &cache->buckets[i];
            for (BlockArray* ba : bucket->all)
                delete ba;
        }
        m_all_caches.pop_front();
    }

    pthread_key_delete(m_cache);
#endif
}

inline BlockHandle BlockManager::allocate_block(Degree degree) {
    // the max() operation will make sure that we return at least BA of bsize 4 / index 2,
    // no matter the degree of the vertex
    size_t const index = std::max(2u, integer_log2_ceil(degree));
    size_t const bsize_requested = 1 << index;
    assert(bsize_requested >= m_min_bsize);

#ifndef DHB_SYSTEM_ALLOCATOR
    auto cache = get_own_cache();
    if (index >= cache->buckets.size())
        throw std::runtime_error("bucket size is too large");
    auto bucket = &cache->buckets[index];

    std::unique_lock<std::mutex> lock{bucket->mutex};
    [&] {
        // Fast path if a BA is available.
        unsigned int age;
        if (!bucket->available.empty())
            return;
        age = bucket->next_age++;

        // Allocate the BA with locks dropped.
        lock.unlock();
        auto ba = new BlockArray(m_bytes_per_entry, bsize_requested, cache, age);
        lock.lock();

        bucket->all.insert(ba);
        bucket->available.insert(ba);
    }();
    assert(!bucket->available.empty());

    auto ba = *bucket->available.begin();
    assert(!ba->occupied());

    auto bhandle = ba->take();

    if (ba->occupied()) {
        if (ba->mostly_free()) {
            ba->recycle();
        } else {
            bucket->available.erase(bucket->available.begin());
        }
    }

    return bhandle;
#else
    void* ptr_entries;
    void* ptr_htab = nullptr;
    ptr_entries = operator new(m_bytes_per_entry* bsize_requested);
    if (uses_htab(bsize_requested))
        ptr_htab = operator new(sizeof(index_type) * 2 * bsize_requested);
    return BlockHandle(bsize_requested, ptr_entries, reinterpret_cast<index_type*>(ptr_htab));
#endif
}

inline void BlockManager::free_block(BlockHandle bhandle) {
#ifndef DHB_SYSTEM_ALLOCATOR
    if (!bhandle)
        return;

    auto ba = bhandle.block_array();
    auto index = integer_log2_ceil(ba->bsize());
    auto cache = ba->m_cache;
    auto bucket = &cache->buckets[index];

    std::unique_lock<std::mutex> lock{bucket->mutex};

    ba->reclaim(bhandle);

    if (ba->occupied()) {
        if (ba->mostly_free()) {
            ba->recycle();
            bucket->available.insert(ba);
        }
    }
#else
    operator delete(bhandle.access_entries(), m_bytes_per_entry* bhandle.bsize());
    operator delete(bhandle.access_htab(), sizeof(index_type) * 2 * bhandle.bsize());
#endif
}

#ifndef DHB_SYSTEM_ALLOCATOR
inline BlockCache* BlockManager::get_own_cache() {
    void* p = pthread_getspecific(m_cache);
    if (!p) {
        std::unique_lock<std::mutex> meta_lock{m_meta_mutex};

        m_all_caches.emplace_back();
        p = &m_all_caches.back();
        if (pthread_setspecific(m_cache, p))
            std::terminate();
    }
    return static_cast<BlockCache*>(p);
}
#endif

} // namespace dhb
