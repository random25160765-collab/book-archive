#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace ts {

template <class K, class V>
class Cache {

	struct Slot { // 这里内容和时间戳被捆在一起
		V value;
		std::uint64_t recency; // 最后一次被使用的时刻
	};

	std::size_t cap_;                   // 缓存容量
	std::unordered_map<K, Slot> table_; // key -> 条目
	std::uint64_t tick_ = 0;            // 全局时钟

	void evict_min() {
		auto victim = table_.begin();
		for (auto it = table_.begin(); it != table_.end(); ++it)
			if (it->second.recency < victim->second.recency) victim = it; // 找到 recency 最小的那位
		table_.erase(victim); // 删除条目
	}

public:

	explicit Cache(std::size_t cap) : cap_(cap) {
		if (cap == 0) throw std::invalid_argument("capacity must be >= 1");
	}

	std::optional<V> get(const K& k) {
		auto p = table_.find(k);
		if (p == table_.end()) return std::nullopt; // 如果没找到，就不动 tick_
		p->second.recency = ++tick_;                // 规则 3
		return p->second.value;
	}

	void put(const K& k, V v) {
		auto p = table_.find(k);                    // 在哈希表里找
		if (p != table_.end()) {                    // 如果没找到
			p->second.value = std::move(v);         // 换内容，走移动赋值
			p->second.recency = ++tick_;            // 重记时刻，即 update(k)
			return;                                 // 条目数不变，不淘汰
		}
		std::uint64_t stamp = ++tick_;              // 先取时刻，再淘汰
		if (table_.size() == cap_) evict_min();     // 规则 7
		table_.emplace(k, Slot{std::move(v), stamp}); // 规则 6
	}

	std::size_t size() const { return table_.size(); }

	std::size_t capacity() const { return cap_; }
};

} // namespace ts
