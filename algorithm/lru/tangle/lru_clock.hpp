#pragma once

#include <cstddef>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

namespace clk {

template <class K, class V>
class Cache {

	struct Slot {
		K key;
		V value;
		bool ref = true; // 引用位：本轮还有没有第二次机会
	};

	std::size_t cap_;                          // 环上的格子数
	std::size_t size_ = 0;                     // 已占格数
	std::size_t hand_ = 0;                     // 指针：下一次从这里开始看
	std::vector<std::optional<Slot>> ring_;    // 环，空位用 nullopt
	std::unordered_map<K, std::size_t> index_; // key -> 格子下标

	void evict_one() {
		for (;;) {
			auto& s = *ring_[hand_];
			if (!s.ref) {                  // 第二次机会用完了
				index_.erase(s.key);       // 规则 8
				ring_[hand_].reset();      // 清空该格
				--size_;
				return;
			}
			s.ref = false;                 // 第一圈：先记一笔，再往前走
			hand_ = (hand_ + 1) % cap_;
		}
	}

	std::size_t claim_slot() {
		while (ring_[hand_].has_value()) hand_ = (hand_ + 1) % cap_;
		return hand_;
	}

public:

	explicit Cache(std::size_t cap) : cap_(cap), ring_(cap) {
		if (cap == 0) throw std::invalid_argument("capacity must be >= 1");
	}

	std::optional<V> get(const K& k) {
		auto p = index_.find(k);
		if (p == index_.end()) return std::nullopt; // 规则 4：位和指针都不动
		auto& s = *ring_[p->second];
		s.ref = true;                               // 规则 3
		return s.value;
	}

	void put(const K& k, V v) {
		auto p = index_.find(k);
		if (p != index_.end()) {            // 规则 5
			auto& s = *ring_[p->second];
			s.value = std::move(v);
			s.ref = true;
			return;
		}
		if (size_ == cap_) evict_one();     // 规则 7
		std::size_t i = claim_slot();       // 规则 6
		ring_[i] = Slot{k, std::move(v), true};
		index_.emplace(k, i);
		++size_;
		hand_ = (i + 1) % cap_;
	}

	std::size_t size() const { return size_; }

	std::size_t capacity() const { return cap_; }
};

} // namespace clk
