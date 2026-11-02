#pragma once

#include <cstddef>
#include <list>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace seq {

template <class K, class V>
class Cache {

	using Entry = std::pair<K, V>; // 条目定义为键值对
	using List  = std::list<Entry>; // 条目是双向链表的节点
	List order_; // 条目序列
	using Iter  = typename List::iterator; // 节点句柄
	std::unordered_map<K, Iter> index_; // key -> 节点句柄的哈希映射
	std::size_t cap_; // 缓存容量

	void update(Iter it) { order_.splice(order_.begin(), order_, it); }

public:

	explicit Cache(std::size_t cap) : cap_(cap) {
		if (cap == 0) throw std::invalid_argument("capacity must be >= 1");
	}

	std::optional<V> get(const K& k) {
		auto p = index_.find(k);
		if (p == index_.end()) return std::nullopt;
		update(p->second);
		return p->second->second;
	}

	void put(const K& k, V v) {
		auto p = index_.find(k); // 在哈希表里找（_index 是哈希表）
		if (p != index_.end()) { // 如果找到了
			p->second->second = std::move(v); // 把那个条目的值换掉
			update(p->second); // 更新条目
			return;
		}
		if (order_.size() == cap_) { // 如果缓存满了
			index_.erase(order_.back().first); // 把尾节点的引用从哈希表里删掉
			order_.pop_back(); // 删掉尾节点
		}
		order_.emplace_front(k, std::move(v)); // 在头节点就地构造一个新节点，返回 void
		index_.emplace(order_.front().first, order_.begin()); // 向哈希表插入一项
	}

	std::size_t size() const     { return order_.size(); }

	std::size_t capacity() const { return cap_; }
};

} // namespace seq
