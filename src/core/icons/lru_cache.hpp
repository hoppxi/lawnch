#pragma once

#include <list>
#include <map>
#include <mutex>
#include <optional>

namespace Lawnch::Core::Icons {

template <typename Key, typename Value> class LRUCache {
public:
  explicit LRUCache(size_t max_bytes)
      : max_bytes(max_bytes), current_bytes(0) {}

  void put(const Key &key, const Value &value, size_t size_bytes) {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = cache_map.find(key);
    if (it != cache_map.end()) {
      current_bytes -= it->second->size_bytes;
      it->second->second = value;
      it->second->size_bytes = size_bytes;
      current_bytes += size_bytes;

      cache_list.splice(cache_list.begin(), cache_list, it->second);
      prune();
      return;
    }

    cache_list.push_front({key, value, size_bytes});
    cache_map[key] = cache_list.begin();
    current_bytes += size_bytes;
    prune();
  }

  std::optional<Value> get(const Key &key) {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = cache_map.find(key);
    if (it == cache_map.end()) {
      return std::nullopt;
    }

    cache_list.splice(cache_list.begin(), cache_list, it->second);
    return it->second->second;
  }

  bool exists(const Key &key) {
    std::lock_guard<std::mutex> lock(mutex);
    return cache_map.find(key) != cache_map.end();
  }

  void clear() {
    std::lock_guard<std::mutex> lock(mutex);
    cache_list.clear();
    cache_map.clear();
    current_bytes = 0;
  }

private:
  struct Entry {
    Key first;
    Value second;
    size_t size_bytes;
  };

  void prune() {
    while (current_bytes > max_bytes && !cache_list.empty()) {
      auto last = cache_list.end();
      last--;
      current_bytes -= last->size_bytes;
      cache_map.erase(last->first);
      cache_list.pop_back();
    }
  }

  size_t max_bytes;
  size_t current_bytes;
  std::list<Entry> cache_list;
  std::map<Key, typename std::list<Entry>::iterator> cache_map;
  std::mutex mutex;
};

} // namespace Lawnch::Core::Icons
