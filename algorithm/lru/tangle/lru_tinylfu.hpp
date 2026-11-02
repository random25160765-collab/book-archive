#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <list>
#include <optional>
#include <unordered_map>
#include <utility>

namespace tlf {

template <class K, class V>
class Cache {

	static constexpr std::size_t W = 64;   // 每行格数
	static constexpr std::size_t D = 4;    // 行数，即哈希函数个数
	static constexpr std::size_t AGE = 8;  // 每多少次访问做一次全局减半

	std::array<std::array<std::uint8_t, W>, D> c_{}; // 4 位计数，这里用 uint8 存
	std::size_t cap_;
	std::size_t ops_ = 0;
	std::list<std::pair<K, V>> order_;               // 主结构：LRU
	std::unordered_map<K, typename std::list<std::pair<K, V>>::iterator> pos_;
	std::hash<K> h_;

	std::uint64_t hs(const K& k) const {
		std::uint64_t x = static_cast<std::uint64_t>(h_(k));
		x += 0x9e3779b97f4a7c15ull;                  // splitmix64：把弱哈希搅开
		x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ull;
		x = (x ^ (x >> 27)) * 0x94d049bb133111ebull;
		return x ^ (x >> 31);
	}

	std::size_t ix(std::uint64_t h, std::size_t i) const { return (h >> (i * 13)) % W; }

	void bump(std::uint64_t h) {
		for (std::size_t i = 0; i < D; ++i) {
			auto& v = c_[i][ix(h, i)];
			if (v < 15) ++v;                 // 4 位饱和
		}
	}

	int est(std::uint64_t h) const {
		int m = 15;
		for (std::size_t i = 0; i < D; ++i) m = std::min<int>(m, c_[i][ix(h, i)]);
		return m;
	}

	void tick() {
		if (++ops_ % AGE) return;
		for (auto& row : c_)
			for (auto& v : row) v = static_cast<std::uint8_t>(v >> 1); // 全体减半
	}

public:
	explicit Cache(std::size_t cap) : cap_(cap) {}

	std::optional<V> get(const K& k) {
		tick();
		bump(hs(k));                       // 未命中也记一笔
		auto p = pos_.find(k);
		if (p == pos_.end()) return std::nullopt; // 规则 4 变了：结构不动，计数动
		order_.splice(order_.begin(), order_, p->second);
		return p->second->second;
	}

	void put(const K& k, V v) {
		tick();
		bump(hs(k));
		auto p = pos_.find(k);
		if (p != pos_.end()) {             // 规则 5
			p->second->second = std::move(v);
			order_.splice(order_.begin(), order_, p->second);
			return;
		}
		if (order_.size() == cap_) {       // 规则 7 变成“比较”
			const K victim = order_.back().first;
			if (est(hs(k)) <= est(hs(victim))) return; // 准入被拒：什么都不做
			pos_.erase(victim);
			order_.pop_back();
		}
		order_.push_front({k, std::move(v)}); // 规则 6
		pos_[k] = order_.begin();
	}

	std::size_t size() const { return order_.size(); }
	std::size_t capacity() const { return cap_; }
};

} // namespace tlf
