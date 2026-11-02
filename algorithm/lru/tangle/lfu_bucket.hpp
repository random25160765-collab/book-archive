#pragma once

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <list>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace lfu {

template <class K, class V>
class Cache {

	struct Node {
		K key;
		V value;
		std::uint64_t freq = 1;
	};

	using Bucket = std::list<Node>;
	using Iter   = typename Bucket::iterator;

	std::size_t cap_;
	std::size_t size_ = 0;
	std::uint64_t minfreq_ = 1;                         // 当前最低频次；空表时无意义
	std::unordered_map<std::uint64_t, Bucket> buckets_; // freq -> 桶，头新尾旧
	std::unordered_map<K, Iter> index_;                 // key -> 桶内位置

	void touch(Iter it) {
		const std::uint64_t f = it->freq;
		Bucket& from = buckets_[f];
		Bucket& to   = buckets_[f + 1];  // 没有就现建一个空桶
		to.splice(to.begin(), from, it); // 搬到高一档的链头
		it->freq = f + 1;
		if (from.empty()) {
			buckets_.erase(f);           // 空桶要删，否则桶表随频次无限变长
			if (minfreq_ == f) ++minfreq_; // 最低档空了，抬一格
		}
	}

	void evict_one() {
		Bucket& b = buckets_[minfreq_];
		Iter victim = std::prev(b.end()); // 桶尾：同频里最久没用的
		index_.erase(victim->key);        // 规则 8
		b.erase(victim);
		if (b.empty()) buckets_.erase(minfreq_);
		--size_;
	}

public:

	explicit Cache(std::size_t cap) : cap_(cap) {
		if (cap == 0) throw std::invalid_argument("capacity must be >= 1");
	}

	std::optional<V> get(const K& k) {
		auto p = index_.find(k);
		if (p == index_.end()) return std::nullopt; // 规则 4：计数不动
		touch(p->second);                           // 规则 3
		return p->second->value;                    // 搬完家，它仍指着同一个节点
	}

	void put(const K& k, V v) {
		auto p = index_.find(k);
		if (p != index_.end()) {           // 规则 5
			p->second->value = std::move(v);
			touch(p->second);
			return;
		}
		if (size_ == cap_) evict_one();    // 规则 7
		Bucket& b = buckets_[1];           // 规则 6：新条目落在 1 档
		b.push_front(Node{k, std::move(v), 1});
		index_.emplace(k, b.begin());
		minfreq_ = 1;                      // 新条目必是最低档
		++size_;
	}

	std::size_t size() const { return size_; }

	std::size_t capacity() const { return cap_; }
};

} // namespace lfu
