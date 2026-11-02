#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <list>
#include <optional>
#include <unordered_map>
#include <utility>

namespace wtl {

template <class K, class V>
class Cache {

	using List = std::list<std::pair<K, V>>;
	using Iter = typename List::iterator;

	std::size_t cap_, win_cap_, pcap_, qcap_;
	std::size_t ops_ = 0;
	std::array<std::array<std::uint8_t, 64>, 4> c_{}; // 和 TinyLFU 同一张 sketch
	List win_, prob_, prot_;
	std::unordered_map<K, Iter> wp_, pp_, rp_;
	std::hash<K> h_;

	std::uint64_t hs(const K& k) const {
		std::uint64_t x = static_cast<std::uint64_t>(h_(k));
		x += 0x9e3779b97f4a7c15ull;
		x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ull;
		x = (x ^ (x >> 27)) * 0x94d049bb133111ebull;
		return x ^ (x >> 31);
	}

	std::size_t ix(std::uint64_t h, std::size_t i) const { return (h >> (i * 13)) % 64; }

	void bump(std::uint64_t h) {
		for (std::size_t i = 0; i < 4; ++i) {
			auto& v = c_[i][ix(h, i)];
			if (v < 15) ++v;
		}
	}

	int est(std::uint64_t h) const {
		int m = 15;
		for (std::size_t i = 0; i < 4; ++i) m = std::min<int>(m, c_[i][ix(h, i)]);
		return m;
	}

	void tick() {
		if (++ops_ % 8) return;
		for (auto& row : c_)
			for (auto& v : row) v = static_cast<std::uint8_t>(v >> 1);
	}

	void admit(std::pair<K, V> cand) {
		if (prob_.size() < pcap_) {          // 试用区没满，直接进
			prob_.push_front(std::move(cand));
			pp_[prob_.front().first] = prob_.begin();
			return;
		}
		const K vic = prob_.back().first;    // 满则比较：候选 vs 试用区尾部
		if (est(hs(cand.first)) <= est(hs(vic))) return; // 打不过就丢
		pp_.erase(vic);
		prob_.pop_back();
		prob_.push_front(std::move(cand));
		pp_[prob_.front().first] = prob_.begin();
	}

	void demote() {
		while (prot_.size() > qcap_) {       // 保护区超编，降回试用区
			auto cand = std::move(prot_.back());
			prot_.pop_back();
			rp_.erase(cand.first);
			prob_.push_front(std::move(cand));
			pp_[prob_.front().first] = prob_.begin();
		}
	}

public:
	Cache(std::size_t cap, std::size_t win, std::size_t pcap)
	    : cap_(cap), win_cap_(win), pcap_(pcap),
	      qcap_(cap > win + pcap ? cap - win - pcap : 0) {}

	std::optional<V> get(const K& k) {
		tick();
		bump(hs(k));
		if (auto p = wp_.find(k); p != wp_.end()) {   // 窗口：纯 LRU
			win_.splice(win_.begin(), win_, p->second);
			return p->second->second;
		}
		if (auto p = pp_.find(k); p != pp_.end()) {   // 试用区命中 → 升保护
			V v = p->second->second;
			prob_.erase(p->second);
			pp_.erase(p);
			prot_.push_front({k, v});
			rp_[k] = prot_.begin();
			demote();
			return v;
		}
		if (auto p = rp_.find(k); p != rp_.end()) {   // 保护区：挪头
			prot_.splice(prot_.begin(), prot_, p->second);
			return p->second->second;
		}
		return std::nullopt;
	}

	void put(const K& k, V v) {
		tick();
		bump(hs(k));
		if (auto p = wp_.find(k); p != wp_.end()) {
			p->second->second = std::move(v);
			win_.splice(win_.begin(), win_, p->second);
			return;
		}
		if (auto p = pp_.find(k); p != pp_.end()) { p->second->second = std::move(v); return; }
		if (auto p = rp_.find(k); p != rp_.end()) {
			p->second->second = std::move(v);
			prot_.splice(prot_.begin(), prot_, p->second);
			return;
		}
		win_.push_front({k, std::move(v)});           // 规则 6 第一段：无条件进窗口
		wp_[k] = win_.begin();
		while (win_.size() > win_cap_) {              // 窗口溢出才走准入
			auto cand = std::move(win_.back());
			win_.pop_back();
			wp_.erase(cand.first);
			admit(std::move(cand));
		}
	}

	std::size_t size() const { return win_.size() + prob_.size() + prot_.size(); }
	std::size_t capacity() const { return cap_; }
};

} // namespace wtl
