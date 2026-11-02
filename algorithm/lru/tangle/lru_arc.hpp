#pragma once

#include <cstddef>
#include <list>
#include <optional>
#include <unordered_map>
#include <utility>

namespace arc {

template <class K, class V>
class Cache {

	using List = std::list<K>;
	using Iter = typename List::iterator;

	std::size_t cap_;
	std::size_t p_ = 0;                  // T1 的目标大小，唯一的动态参数
	std::unordered_map<K, V> val_;
	List t1_, t2_, b1_, b2_;
	std::unordered_map<K, Iter> t1p_, t2p_, b1p_, b2p_;

	bool full() const { return t1_.size() + t2_.size() >= cap_; }

	void replace(bool in_b2) {
		if (!t1_.empty() && (t1_.size() > p_ || (in_b2 && t1_.size() == p_))) {
			K k = t1_.back();            // 从 T1 尾砍：它是一次访问的
			t1_.pop_back();
			t1p_.erase(k);
			val_.erase(k);
			b1_.push_front(k);           // 只留 key 进影子
			b1p_[k] = b1_.begin();
		} else if (!t2_.empty()) {
			K k = t2_.back();            // 从 T2 尾砍：它是多次访问的
			t2_.pop_back();
			t2p_.erase(k);
			val_.erase(k);
			b2_.push_front(k);
			b2p_[k] = b2_.begin();
		}
	}

	void push_t2(const K& k, V v) {
		val_[k] = std::move(v);
		t2_.push_front(k);
		t2p_[k] = t2_.begin();
	}

public:
	explicit Cache(std::size_t cap) : cap_(cap) {}

	std::optional<V> get(const K& k) {
		if (auto p = t1p_.find(k); p != t1p_.end()) {  // T1 命中 → 升 T2
			V v = val_.at(k);
			t1_.erase(p->second);
			t1p_.erase(p);
			push_t2(k, v);
			return v;
		}
		if (auto p = t2p_.find(k); p != t2p_.end()) {
			t2_.splice(t2_.begin(), t2_, p->second);
			return val_.at(k);
		}
		return std::nullopt;                            // 影子命中不在这里
	}

	void put(const K& k, V v) {
		if (auto p = t1p_.find(k); p != t1p_.end()) {
			t1_.erase(p->second);
			t1p_.erase(p);
			push_t2(k, std::move(v));
			return;
		}
		if (auto p = t2p_.find(k); p != t2p_.end()) {
			val_[k] = std::move(v);
			t2_.splice(t2_.begin(), t2_, p->second);
			return;
		}
		if (b1p_.count(k)) {                 // B1 命中：T1 该更大
			if (p_ < cap_) ++p_;
			b1_.erase(b1p_.at(k));
			b1p_.erase(k);
			while (full()) replace(false);
			push_t2(k, std::move(v));
			return;
		}
		if (b2p_.count(k)) {                 // B2 命中：T2 该更大
			if (p_ > 0) --p_;
			b2_.erase(b2p_.at(k));
			b2p_.erase(k);
			while (full()) replace(true);
			push_t2(k, std::move(v));
			return;
		}
		while (full()) replace(false);       // 全新条目
		val_[k] = std::move(v);
		t1_.push_front(k);
		t1p_[k] = t1_.begin();
	}

	std::size_t size() const { return t1_.size() + t2_.size(); }
	std::size_t capacity() const { return cap_; }
};

} // namespace arc
