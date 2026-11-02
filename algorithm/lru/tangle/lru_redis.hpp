#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <random>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace rds {

template <class K, class V>
class Cache {

	static constexpr std::uint32_t MASK = 0xFFFFFFu; // 24 位时钟，约 194 天回绕
	static constexpr std::size_t POOL = 16;         // 候选池：跨调用累积

	struct Entry {
		V value;
		std::uint32_t lru; // 最后一次使用时的时钟读数
	};

	struct Pick {
		K key;
		std::uint32_t idle;
	};

	std::size_t cap_;
	std::uint32_t now_ = 0;      // 秒级时钟，由 tick() 推进
	std::size_t samples_ = 5;    // 每次抓几个
	std::unordered_map<K, Entry> table_;
	std::array<Pick, POOL> pool_{}; // 按 idle 升序；尾部就是当前最该走的
	std::size_t pool_n_ = 0;
	std::mt19937 rng_{1};

	std::uint32_t idle_of(const Entry& e) const {
		return (now_ - e.lru) & MASK; // 环形差：回绕过也不会算成负数
	}

	void offer(const K& key, std::uint32_t idle) {
		std::size_t i = 0;
		while (i < pool_n_ && pool_[i].idle < idle) ++i;
		if (i == pool_n_ && pool_n_ == POOL) return; // 比池里所有都新：不收
		if (pool_n_ < POOL) ++pool_n_;
		for (std::size_t j = pool_n_ - 1; j > i; --j) pool_[j] = pool_[j - 1];
		pool_[i] = Pick{key, idle};
	}

	void evict_one() {
		std::size_t n = table_.size();
		auto it = table_.begin();
		std::advance(it, rng_() % n);        // 随机起点
		std::size_t stride = n / samples_ ? n / samples_ : 1;
		for (std::size_t i = 0; i < samples_ && n > 0; ++i) {
			offer(it->first, idle_of(it->second));
			for (std::size_t s = 0; s < stride; ++s) {
				if (++it == table_.end()) it = table_.begin();
			}
		}
		const K victim = pool_[pool_n_ - 1].key; // 池尾：idle 最大
		--pool_n_;                               // 其余的留在池里，下一轮接着用
		table_.erase(victim);
	}

public:

	void tick() { now_ = (now_ + 1) & MASK; }

	explicit Cache(std::size_t cap, std::uint32_t seed = 1) : cap_(cap), rng_(seed) {
		if (cap == 0) throw std::invalid_argument("capacity must be >= 1");
	}

	std::optional<V> get(const K& k) {
		auto p = table_.find(k);
		if (p == table_.end()) return std::nullopt; // 规则 4：时钟不动
		p->second.lru = now_;                       // 规则 3
		return p->second.value;
	}

	void put(const K& k, V v) {
		auto p = table_.find(k);
		if (p != table_.end()) {            // 规则 5
			p->second.value = std::move(v);
			p->second.lru = now_;
			return;
		}
		if (table_.size() == cap_) evict_one(); // 规则 7
		table_.emplace(k, Entry{std::move(v), now_}); // 规则 6
	}

	std::size_t size() const { return table_.size(); }

	std::size_t capacity() const { return cap_; }
};

} // namespace rds
