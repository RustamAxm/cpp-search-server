//
// Created by rustam on 19.04.22.
//
#pragma once

#include <cstdlib>
#include <future>
#include <map>
#include <numeric>
#include <vector>
#include <mutex>


template <typename Key, typename Value>
class ConcurrentMap {

private:
    struct MutexMap{
        std::mutex m_;
        std::map<Key, Value> map_;
    };
    std::vector<MutexMap> vector_;

public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys");

    struct Access {
        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;

        Access(const Key& key, MutexMap& map) :
                guard(map.m_),
                ref_to_value(map.map_[key]) {
        }
    };

    explicit ConcurrentMap(size_t bucket_count) : vector_(bucket_count){
    }

    Access operator[](const Key& key){
        auto& part = vector_[static_cast<uint32_t>(key) % vector_.size()];
        return {key, part};
    }

    void Erase(const Key& key){
        auto& [mutex, map] = vector_[static_cast<uint32_t>(key) % vector_.size()];
        std::lock_guard guard(mutex);
        map.erase(key);
    }

    std::map<Key, Value> BuildOrdinaryMap(){
        std::map<Key, Value> result;
        for (auto& [mutex, map] : vector_) {
            std::lock_guard guard(mutex);
            result.insert(map.begin(), map.end());
        }
        return result;
    }

};
