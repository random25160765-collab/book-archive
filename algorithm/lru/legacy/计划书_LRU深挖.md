# 深挖 LRU —— 项目计划书

> 来源：CodeBuddy 会话「LRU Cache 讲解」（2026-11-01，已随本目录附上完整聊天记录）
> 状态：原环境（WSL）CodeBuddy 崩溃，本计划书用于迁移到工作站继续执行

---

## 1. 背景与目标

### 起点

本来只想复习 LRU（算法题常考），但在说「建模」这个词之后，展开出了一条完整的链条：

```
"LRU 是最久未使用的淘汰"              ← 一句话定义
        ↓ 这句话有歧义（访问含不含写？满了才淘汰？容量有界吗？）
"需要精确化"                          ← 建模
        ↓ 精确化 = 从自然语言到形式规约
"写成状态 + 转移 + 不变式"
        ↓ 规约写出来了，怎么知道它对？
"要么穷举（TLA+），要么拿它当判据测实现（oracle/MBT）"
```

每一步都是上一步的必然结果。这条链的起点是算法题，终点是「如何保证一个有状态系统正确」——这正是形式化方法 / 模型检验的领域。

### 为什么值得深挖（而非只刷题）

从工作区内容看（`CXLMemSim`、`cosim-gpu`、`mgpusim`、`little-gpu-cmodel`、`XiangShanLab`），缓存替换策略、内存层次、TLB/页置换、模拟器状态一致性，正是这条链下游的主场：

- **L3（替换策略语义）是刚需**：L2/L3 cache 用什么替换策略、set-associative 下的近似 LRU 怎么做，直接影响模拟结果的真实性。Redis 的采样近似 LRU 思想在硬件 cache 里到处都是。
- **L5（形式化 / 模型检验）是刚需**：体系结构模拟器最怕「模拟器行为和规格不一致」却查不出来。这正是 MBT/oracle 比对的场景——SUT 天生有状态、有合法操作序列（流水线状态、一致性协议状态机、MMU 状态），朴素 PBT 不够用，必须 MBT。

对「算法题」这个目标，形式化超纲；但对实际工作，这些恰好是底层的语言。

### 产出形式

在 `/home/rd/codefield/深挖LRU`（工作站上路径自定）用 **neoweb**（文学化编程工具，一份 `.nw` 源同时 tangle 出可编译代码、weave 出书页）写一本以 LRU 为切入点的分层深挖书，按 L1–L5 逐章推进，每一层讲透一个「LRU 视角下的新领域」。

---

## 2. 分层大纲（L1–L5）

| 层 | 主题 | 书里演示什么 | 工具 | 目标程度 |
|---|---|---|---|---|
| **L1 算法** | 核心实现 | `dict + 双向链表`，`get`/`put` 各 20 行，O(1)；对比 `std::list`、`OrderedDict` | g++ | 100%，手写到肌肉记忆 |
| **L2 工程** | 工程质量 | ASan 查悬空指针 bug、微基准对比、泄漏检测；线程安全/分片/TTL/命中率统计（用到再查） | g++ `-fsanitize`、hyperfine、valgrind、libbenchmark | 了解工具即可 |
| **L3 语义** | 变体与现实实现 | 精确 vs 近似（Redis 采样）、实测 Redis 近似 LRU、命中率曲线、LRU-K / 2Q / ARC、硬件 cache 里的近似 LRU | redis-server、matplotlib | **刚需**（模拟器工作直接相关） |
| **L4 正确性** | 测试方法论 | oracle 比对、property-based testing（hypothesis 状态机 + shrink）、model-based testing（guard/precondition）、覆盖率 | pytest、hypothesis、lcov | 选修 → 实践中会用 |
| **L5 形式化** | 形式化验证 | TLA+ 规约 + TLC 跑反例、Alloy 不变式（SAT 有界验证）、精化关系、ioco | JDK 21、TLC jar、Alloy jar | 选修，但一致性协议工作是刚需 |

**L1 的收益占 90%，成本是 20 分钟。** L4/L5 的收益在算法题里是零，在体系结构模拟器里可能是刚需——所以这本书的定位不是「课程」，是**索引**：把每个概念的入口、工具、示例留在书里，需要时回来查。

### 每层要回答的核心问题（写作提纲）

- **L1**：什么操作算「访问」？（读/写/含不含失败路径）容量有界吗？满了才淘汰还是提前淘汰？——语义歧义清单
- **L2**：怎么证明「O(1)」？（复杂度不是 TLA+ 能表达的，要靠计数器/基准）
- **L3**：为什么真实系统几乎不用严格 LRU？（扫描污染、内存开销、近似采样）
- **L4**：怎么写一个「笨版参考实现」当 oracle？guard 怎么定？
- **L5**：TLA+ 管时序/并发，Alloy 管结构/关系；什么时候哪把锤子

---

## 3. 工具链安装清单

> 原环境已确认就位（无需装）：g++（支持 C++20/ASan/UBSan/TSan）、make、python3、graphviz dot、node/npm、neoweb 依赖（libcmark-gfm + extensions + source-highlight）。
> **迁移到工作站时先按下面清单补齐，再用自检命令验证。**

### A. 必需（开工就装，约 2 分钟）

```bash
# 只为 L5 的形式化工具（L1–L4 完全不需要 Java）
sudo apt update
sudo apt install -y openjdk-21-jre-headless

# Python 侧必须进 venv（Debian/Ubuntu 新版 pip 有 PEP 668 限制）
cd <深挖LRU目录>
python3 -m venv .venv
.venv/bin/pip install -U pip
.venv/bin/pip install pytest hypothesis matplotlib numpy
```

matplotlib 若在新 Python 上没有 wheel，改用系统包 + `--system-site-packages`：

```bash
sudo apt install -y python3-matplotlib python3-numpy
python3 -m venv --system-site-packages .venv
```

### B. 两个 jar 包（不用安装，下载放目录即可）

```bash
mkdir -p <深挖LRU目录>/tools && cd <深挖LRU目录>/tools
curl -LO https://github.com/tlaplus/tlaplus/releases/latest/download/tla2tools.jar
curl -LO https://github.com/AlloyTools/org.alloytools.alloy/releases/latest/download/org.alloytools.alloy.dist.jar
```

### C. 按章补装（写到哪章装哪样）

```bash
sudo apt install -y redis-server redis-tools   # L3 对照真实 Redis
sudo apt install -y lcov                        # L4 C++ 覆盖率
sudo apt install -y valgrind hyperfine          # L2 泄漏检测 / 命令行基准
sudo apt install -y libbenchmark-dev            # L2 C++ 微基准（统计严谨版）
```

### D. neoweb 依赖

```bash
sudo apt-get install build-essential pkg-config \
  libcmark-gfm-dev libcmark-gfm-extensions-dev \
  libsource-highlight-dev python3
# 软依赖：graphviz（dot 围栏）、KaTeX（make deps 取的，公式渲染）
```

### 装完自检

```bash
g++ -fsanitize=address,undefined -x c++ - -o /tmp/t <<<'int main(){int*p=new int[2];p[3]=0;}' && /tmp/t; echo "ASan exit=$?"
java -version
java -cp tools/tla2tools.jar tlc2.TLC -help | head -3
.venv/bin/python -c "import pytest,hypothesis,matplotlib,numpy;print('py OK')"
redis-server --version 2>/dev/null || echo "redis 未装（L3 再说）"
```

第一行故意越界写入，ASan 报错才算配置成功。

---

## 4. neoweb 骨架搭建

### 获取与构建

```bash
git clone https://github.com/random25160765-collab/neoweb.git
cd neoweb && make        # bootstrap.py 先破点编译出 nw，之后自举
```

### 语法要点（从构建会话提炼）

- `book/part/essay` 块：定阅读顺序（目录即代码）
- `dir/file` 块：定输出树（tangle 铺盘的位置）
- `<<chunk>>= @`：代码块定义；同一 chunk 可以多处续写
- `tangle` 抽出实现，`weave` 抽出文档——同一份源、两种综合结果

### 骨架计划（原会话最后一步，未执行）

1. `menu.nw`：书的总目录（book/part/essay 结构，按 L1–L5 排章次）
2. `skeleton.nw`：写作骨架约定（每章结构：动机 → 模型 → 实现 → 验证 → 延伸）
3. 章节目录：`L1-算法/`、`L2-工程/`、`L3-语义/`、`L4-正确性/`、`L5-形式化/`
4. `examples/`：各章代码示例的 tangle 目标与编译规则（Makefile 集成）

已有草稿：`lru.nw`（本目录已附）——书的开头：LRU 定义段落。

---

## 5. 执行顺序建议

1. **先装 A + venv**（约 2 分钟），`pytest`、`hypothesis`、`java` 三样到齐即开工
2. **搭 neoweb 骨架**，从 L1 开始逐章写
3. **L1 优先**：20 分钟写完，用 `dict + 双向链表` 20 行版（不要用聊天记录里那个 60 行的教学版——面试要的是 20 行）
4. L2 → L3 → L4 → L5 逐章推进，每章收尾时把对应工具装好（见 §3.C）
5. 每章产出双份：`.nw` 源（进书） + `examples/` 下可编译运行的代码

---

## 6. 关键概念索引（术语表雏形）

| 概念 | 一句话 | 在聊天记录 01 中的位置 |
|---|---|---|
| **建模** | 把自然语言规约精确化为「状态 + 转移 + 不变式」 | 00:33 |
| **oracle** | 「笨版参考实现」，作为判据与真实实现对拍 | 00:43 |
| **guard / precondition** | 操作合法性前置条件；有 guard 才需要 MBT | 00:54 |
| **PBT** | 随机生成操作序列，用性质当判据（不给证明，只降概率） | 00:43 |
| **MBT** | 模型既当判据又参与生成（`RuleBasedStateMachine` + `precondition`） | 00:54 |
| **TLA+** | Lamport 时序逻辑规约语言，管并发/分布式状态机；TLC 显式枚举可达状态 | 00:40 |
| **Alloy** | MIT 关系逻辑建模语言，管结构/关系不变式；SAT 后端，**scope 内完备**（bounded verification） | 00:59 |
| **Small Scope Hypothesis** | 绝大多数设计 bug 在很小实例上就能触发（scope ≤ 4） | 00:59 |
| **shrink** | hypothesis 把失败反例最小化的过程 | 00:43 |
| **三者分工** | TLA+/TLC 抓并发不变式；PBT+oracle 抓逻辑 + 复杂度；单元测试抓边界回归 | 00:43 |

**关键结论（反复出现）**：LRU 的核心难点之一是 O(1) 复杂度，而 TLA+ 对此完全无话可说——它验证逻辑正确性，不验证「双向链表选对了」。用错单向链表，规约照样全绿。复杂度要用计数器断言（PBT）+ 基准（L2）来抓。

---

## 7. 随包资产清单

| 文件 | 内容 |
|---|---|
| `聊天记录_01_LRU主会话.md` | LRU 深挖主会话完整导出（44 条消息）：建模过程、TLA+ 规约示例、Alloy 模型示例、PBT/MBT 讲解、L1–L5 分层、安装清单 |
| `聊天记录_02_neoweb构建.md` | neoweb 本地构建会话（67 条消息）：依赖确认、bootstrap 构建、旧 tangle 产物处理 |
| `lru_pbt.py` | PBT 可跑模板：`RefLRU` 参考实现 + hypothesis 状态机。以后任何有状态组件（cache、TLB、buffer、状态机）把 `RefLRU` 换成它的「笨版规格」，其余照抄 |
| `lru.nw` | 书的开头草稿（LRU 定义） |

---

## 8. 工作站迁移 checklist

- [ ] clone neoweb 并构建（§4）
- [ ] 新建 `<深挖LRU>` 目录，把本包所有文件放进去
- [ ] 安装工具链 §3.A + §3.B（必需），跑自检
- [ ] 搭 neoweb 骨架（menu.nw / skeleton.nw / 章节目录 / examples）
- [ ] L1 章：20 行实现 + 歧义清单
- [ ] 之后按 §5 顺序逐章推进，§3.C 按章补装
- [ ] 写不下去或概念忘了 → 查聊天记录 01 对应时间段（§6 表格有索引）
