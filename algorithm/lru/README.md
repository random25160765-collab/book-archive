# 对 LRU 的深挖，以及缓存淘汰与准入策略

从一道 LRU 算法题出发，把 recency 序与 frequency 序、序列/时间戳/CLOCK 模型、LFU、LRU-K、2Q、TinyLFU、W-TinyLFU、ARC 一路写下来。`book/lru.nw` 是唯一真相，`tangle/` 是它展开出来的 10 个 C++ 头文件。

```
book/lru.nw     书稿（散文 + 代码块，文学编程）
tangle/*.hpp    产物，10 个模型实现，勿手改
legacy/         从 CodeBuddy 会话迁移过来的计划书、聊天记录、PBT 模板
tools/          建模工具（TLA+ / Alloy），见下
```

## 重建

展开由 [neoweb](https://github.com/random25160765-collab/neoweb) 完成，先装引擎：

```sh
git clone https://github.com/random25160765-collab/neoweb.git
cd neoweb && make && make install     # 装到 ~/.local/bin/nw
```

回来后用 `nw` 重新展开（改 `book/lru.nw` 后重跑同一条命令即可覆盖 `tangle/`）：

```sh
nw tangle --check book/lru.nw        # 只检查：chunk 77 个，file 10 个，警告 0 条
nw tangle book/lru.nw -o tangle      # 重建 tangle/ 下 10 个 .hpp
nw weave  book/lru.nw -o weave       # 生成 HTML（index.html / lru.html / tree.html）
```

## 工具链（只有跑模型和实验时才需要）

```sh
sudo apt install -y openjdk-21-jre-headless

mkdir -p tools && cd tools
curl -LO https://github.com/tlaplus/tlaplus/releases/latest/download/tla2tools.jar
curl -LO https://github.com/AlloyTools/org.alloytools.alloy/releases/latest/download/org.alloytools.alloy.dist.jar
cd ..

python3 -m venv .venv && .venv/bin/pip install pytest hypothesis matplotlib numpy
.venv/bin/pytest legacy/lru_pbt.py
```

## 未入库的目录

体积大、可重新获取，已被 `.gitignore` 排除，clone 下来不会有：

| 路径 | 体积 | 怎么拿回来 |
|---|---|---|
| `.venv/` | 209M | 上面「工具链」里的 `python3 -m venv` |
| `tools/*.jar` | 23M | 上面「工具链」里的两条 `curl` |
