#pragma once

#include <cstddef>
#include <list>
#include <optional>
#include <unordered_map>
#include <utility>

namespace tq {

template <class K, class V>
class Cache {

	using A1    = std::list<std::pair<K, V>>; // 候车：FIFO
	using A1It  = typename A1::iterator;
	using Am    = std::list<K>;               // 热区：LRU
	using AmIt  = typename Am::iterator;
	using Out   = std::list<K>;               // 影子：只有 key
	using OutIt = typename Out::iterator;

	std::size_t cap_, ki_, kout_;
	A1 a1_;
	std::unordered_map<K, A1It> a1p_;
	Am am_;
	std::unordered_map<K, AmIt> amp_;
	std::unordered_map<K, V> amv_;
	Out out_;
	std::unordered_map<K, OutIt> outp_;

	void trim_a1() {
		while (a1_.size() > ki_) {
			K k = std::move(a1_.front().first);
			a1p_.erase(k);
			a1_.pop_front();
			out_.push_back(k);              // 出去的不销毁，进影子
			outp_[k] = std::prev(out_.end());
			while (out_.size() > kout_) {   // 影子也有界
				outp_.erase(out_.front());
				out_.pop_front();
			}
		}
	}

	void trim_am() {
		while (am_.size() + ki_ > cap_) {   // Am 的配额是 cap - Ki
			K k = std::move(am_.back());
			amp_.erase(k);
			amv_.erase(k);
			am_.pop_back();
		}
	}

	void enter_am(const K& k, V v) {
		amv_[k] = std::move(v);
		am_.push_front(k);
		amp_[k] = am_.begin();
		trim_am();
	}

	void touch_am(const K& k) { am_.splice(am_.begin(), am_, amp_.at(k)); }

public:
	Cache(std::size_t cap, std::size_t ki, std::size_t kout)
	    : cap_(cap), ki_(ki), kout_(kout) {}

	std::optional<V> get(const K& k) {
		if (amp_.count(k)) { touch_am(k); return amv_.at(k); } // 规则 3：热区，挪头
		auto p = a1p_.find(k);
		if (p == a1p_.end()) return std::nullopt;              // 规则 4
		V v = p->second->second;                               // 规则 3：二次访问
		a1_.erase(p->second);
		a1p_.erase(p);
		enter_am(k, v);                                        // 升到 Am
		return v;
	}

	void put(const K& k, V v) {
		auto pa = amp_.find(k);
		if (pa != amp_.end()) {            // 规则 5：已在热区
			amv_[k] = std::move(v);
			touch_am(k);
			return;
		}
		auto p1 = a1p_.find(k);
		if (p1 != a1p_.end()) {            // 规则 6：同一 key 第二次写，升档
			a1_.erase(p1->second);
			a1p_.erase(p1);
			enter_am(k, std::move(v));
			return;
		}
		auto po = outp_.find(k);
		if (po != outp_.end()) {           // 准入：影子命中，直接进热区
			out_.erase(po->second);
			outp_.erase(po);
			enter_am(k, std::move(v));
			return;
		}
		a1_.push_back({k, std::move(v)});  // 全新：进候车室
		a1p_[k] = std::prev(a1_.end());
		trim_a1();
	}

	std::size_t size() const { return am_.size() + a1_.size(); }

	std::size_t capacity() const { return cap_; }
};

} // namespace tq
