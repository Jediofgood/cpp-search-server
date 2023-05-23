#pragma once

#include<map>
#include<set>
#include<mutex>
#include<vector>

using namespace std::literals;

template <typename Key, typename Value>
class ConcurrentMap {
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);

    struct Bucket {
        std::map<Key, Value> the_map;
        std::mutex the_mutex;
    };

    struct Access {
        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;

        Access(const Key& key, Bucket& bucket)
            :guard(bucket.the_mutex), ref_to_value(bucket.the_map[key])
        {}
    };

    explicit ConcurrentMap()
        :buckets_(3){}

    explicit ConcurrentMap(size_t bucket_count)
        :buckets_(bucket_count)
    {
    }

    Access operator[](const Key& key) {
        Bucket& bucket = buckets_[static_cast<uint64_t>(key) % buckets_.size()];
        return { key, bucket };
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> to_return;
        for (Bucket& bucket : buckets_) {
            std::lock_guard<std::mutex> guard(bucket.the_mutex);
            to_return.insert(bucket.the_map.begin(), bucket.the_map.end());
        }
        return to_return;
    }

    void erase(const Key& key) {
        buckets_[static_cast<uint64_t>(key) % buckets_.size()].the_map.erase(key);
    }


private:
    std::vector<Bucket> buckets_;
};