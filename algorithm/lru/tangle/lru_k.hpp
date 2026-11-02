#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace lrk {

template <class K, class V>
class Cache {

	struct Entry {
		V value;
		std::deque<std::uint64_t> hist; // 最近 K 次访问的时刻，新在尾
	};

	std::size_t cap_;
	std::size_t k_;
	std::uint64_t tick_ = 0;
	std::unordered_map<K, Entry> table_;

	// 排序键：(是否攒够 K 次, 窗口里最早的那次)
	std::pair<int, std::uint64_t> rank(const Entry& e) const {
		return {e.hist.size() < k_ ? 0 : 1, e.hist.front()};
	}

	void update(Entry& e) {
		e.hist.push_back(++tick_);
		if (e.hist.size() > k_) e.hist.pop_front(); // 窗口滑动：只留最近 K 次
	}

	void evict_min() {
		auto victim = table_.begin();
		for (auto it = table_.begin(); it != table_.end(); ++it)
			if (rank(it->second) < rank(victim->second)) victim = it;
		table_.erase(victim);
	}

public:

	Cache(std::size_t cap, std::size_t k) : cap_(cap), k_(k) {
		if (cap == 0 || k == 0) throw std::invalid_argument("cap and K must be >= 1");
	}

	std::optional<V> get(const K& k) {
		auto p = table_.find(k);
		if (p == table_.end()) return std::nullopt; // 规则 4：窗口不动
		update(p->second);                          // 规则 3
		return p->second.value;
	}

	void put(const K& k, V v) {
		auto p = table_.find(k);
		if (p != table_.end()) {            // 规则 5
			p->second.value = std::move(v);
			update(p->second);
			return;
		}
		if (table_.size() == cap_) evict_min(); // 规则 7
		Entry e;
		e.value = std::move(v);
		update(e);                          // 规则 6：插入也算一次访问
		table_.emplace(k, std::move(e));
	}

	std::size_t size() const { return table_.size(); }

	std::size_t capacity() const { return cap_; }
};

} // namespace lrk
