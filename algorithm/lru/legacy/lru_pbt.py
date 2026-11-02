"""LRU 的 property-based testing 演示：用抽象模型当 oracle。

核心思想：
  1. oracle（参考模型）故意写得很笨 —— 一条 list，位置就是最近性。慢但显然正确。
  2. 被测实现写得很聪明 —— hash map + 双向链表，O(1)。快但容易错。
  3. 随机生成操作序列，两边同时喂，逐步比对"可观察行为"。
  4. 发现不一致后，自动删减操作序列，给出最小反例。
"""

from dataclasses import dataclass
import random


# ---------------------------------------------------------------------------
# 1. 参考模型（oracle）—— O(n)，照抄抽象模型，只求显然正确
# ---------------------------------------------------------------------------
class RefLRU:
    def __init__(self, cap):
        self.cap = cap
        self.seq = []       # [(k, v)]，头最新，尾最旧
        self.evicted = []

    def get(self, k):
        for i, (kk, v) in enumerate(self.seq):
            if kk == k:
                self.seq.pop(i)
                self.seq.insert(0, (kk, v))
                return v
        return None

    def put(self, k, v):
        for i, (kk, _) in enumerate(self.seq):
            if kk == k:
                self.seq.pop(i)
                self.seq.insert(0, (k, v))
                return
        if len(self.seq) == self.cap:
            self.evicted.append(self.seq.pop()[0])
        self.seq.insert(0, (k, v))

    def order(self):
        return [k for k, _ in self.seq]


# ---------------------------------------------------------------------------
# 2. 被测实现 —— O(1)，hash map + 双向链表
# ---------------------------------------------------------------------------
class Node:
    __slots__ = ("k", "v", "prev", "next")

    def __init__(self, k=None, v=None):
        self.k, self.v, self.prev, self.next = k, v, None, None


class LRUCache:
    def __init__(self, cap):
        self.cap = cap
        self.map = {}
        self.evicted = []
        self.head, self.tail = Node(), Node()
        self.head.next, self.tail.prev = self.tail, self.head

    def _unlink(self, n):
        n.prev.next = n.next
        n.next.prev = n.prev

    def _push_front(self, n):
        n.prev, n.next = self.head, self.head.next
        self.head.next.prev = n
        self.head.next = n

    def get(self, k):
        n = self.map.get(k)
        if n is None:
            return None
        self._unlink(n)
        self._push_front(n)
        return n.v

    def put(self, k, v):
        n = self.map.get(k)
        if n is not None:
            n.v = v
            self._unlink(n)
            self._push_front(n)
            return
        if len(self.map) == self.cap:
            lru = self.tail.prev
            self._unlink(lru)
            del self.map[lru.k]
            self.evicted.append(lru.k)
        n = Node(k, v)
        self.map[k] = n
        self._push_front(n)

    def order(self):
        out, n = [], self.head.next
        while n is not self.tail:
            out.append(n.k)
            n = n.next
            if len(out) > 10_000:
                raise RuntimeError("linked list corrupted")
        return out


# ---------------------------------------------------------------------------
# 3. 注入 bug 的变体（都不是故意的低级错误，而是真实会犯的错）
# ---------------------------------------------------------------------------
class BugFifoOnHit(LRUCache):
    """bug: get 命中后不刷新最近性 -> LRU 退化成 FIFO。返回值全对。"""

    def get(self, k):
        n = self.map.get(k)
        return None if n is None else n.v


class BugNoRefreshOnUpdate(LRUCache):
    """bug: put 已存在的 key 时只改值，没移到队首。返回值全对。"""

    def put(self, k, v):
        n = self.map.get(k)
        if n is not None:
            n.v = v
            return
        super().put(k, v)


# ---------------------------------------------------------------------------
# 4. 比对执行器
# ---------------------------------------------------------------------------
@dataclass
class Fail:
    kind: str
    step: int
    op: tuple
    ref: object
    impl: object
    trace: list


def run_once(cap, ops, factory, observe=("get", "evicted", "order")):
    """跑一遍操作序列，返回第一个不一致点；全一致返回 None。"""
    ref, impl = RefLRU(cap), factory(cap)
    for i, (op, k, v) in enumerate(ops):
        if op == "get":
            r, m = ref.get(k), impl.get(k)
            if "get" in observe and r != m:
                return Fail("get 返回值不一致", i, (op, k, v), r, m, ops[: i + 1])
        else:
            ref.put(k, v)
            impl.put(k, v)

        if "evicted" in observe and ref.evicted != impl.evicted:
            return Fail("淘汰序列不一致", i, (op, k, v),
                        ref.evicted[:], impl.evicted[:], ops[: i + 1])

        if "order" in observe and ref.order() != impl.order():
            return Fail("缓存内部顺序不一致", i, (op, k, v),
                        ref.order(), impl.order(), ops[: i + 1])
    return None


# ---------------------------------------------------------------------------
# 5. 随机生成 + 反例最小化
# ---------------------------------------------------------------------------
def random_ops(rng, n, key_space, val_space):
    ops = []
    for _ in range(n):
        if rng.random() < 0.5:
            ops.append(("get", rng.randrange(key_space), None))
        else:
            ops.append(("put", rng.randrange(key_space), rng.randrange(val_space)))
    return ops


def shrink(cap, ops, factory, observe):
    """贪心删减：只要删掉某步后仍然失败，就永久删掉它。"""
    cur = ops
    improved = True
    while improved:
        improved = False
        for i in range(len(cur)):
            cand = cur[:i] + cur[i + 1:]
            if run_once(cap, cand, factory, observe) is not None:
                cur = cand
                improved = True
                break
    return cur


def fmt_ops(ops):
    return "\n".join(
        f"    {i}. {'get(' + str(k) + ')' if op == 'get' else f'put({k}, {v})'}"
        for i, (op, k, v) in enumerate(ops)
    )


# ---------------------------------------------------------------------------
# 6. 主流程
# ---------------------------------------------------------------------------
TIMES, CAP = 500, 2


def detect_rate(cap, factory, observe, cases):
    hit = sum(run_once(cap, c, factory, observe) is not None for c in cases)
    return hit / len(cases)


def main():
    rng = random.Random(20261101)
    bugs = {"BugFifoOnHit": BugFifoOnHit,
            "BugNoRefreshOnUpdate": BugNoRefreshOnUpdate}

    for name, factory in bugs.items():
        print(f"\n{'=' * 70}\n被测实现: {name}\n{'=' * 70}")
        print(f"{'序列长度':<8}{'只观察 get 返回值':<20}{'get + 淘汰序列':<20}")
        print("-" * 70)
        for n in (4, 5, 6, 8, 10, 14):
            cases = [random_ops(rng, n, 4, 3) for _ in range(TIMES)]
            r1 = detect_rate(CAP, factory, ("get",), cases)
            r2 = detect_rate(CAP, factory, ("get", "evicted"), cases)
            print(f"{n:<8}{r1:<20.1%}{r2:<20.1%}")

        observe = ("get", "evicted")
        cases = [random_ops(rng, 10, 4, 3) for _ in range(TIMES)]
        bad = next((c for c in cases if run_once(CAP, c, factory, observe)), None)
        small = shrink(CAP, bad, factory, observe)
        f = run_once(CAP, small, factory, observe)
        print(f"\n  反例最小化: {len(bad)} 步 -> {len(small)} 步 (cap={CAP})")
        print(fmt_ops(small))
        print(f"\n  失败类型: {f.kind} @ 第 {f.step + 1} 步 {f.op}")
        print(f"    oracle : {f.ref}")
        print(f"    实现   : {f.impl}")


if __name__ == "__main__":
    main()
