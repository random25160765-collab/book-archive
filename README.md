# book-archive

文学编程工程归档。每个子目录是一本书：`book/*.nw` 是人类写的唯一真相，`tangle/` 是它展开出来的代码。

```
algorithm/lru/    对 LRU 的深挖，以及缓存淘汰与准入策略（C++，10 个头文件）
language/cpp/     C++ 标准库书稿（尚未开始）
```

## 重建

所有书都由 [neoweb](https://github.com/random25160765-collab/neoweb) 展开，先装引擎：

```sh
git clone https://github.com/random25160765-collab/neoweb.git
cd neoweb && make && make install     # 装到 ~/.local/bin/nw
```

然后进任一工程，用 `nw` 重新展开（`tangle/` 是产物，不要手改；改 `book/*.nw` 后重跑同一条命令即可覆盖）：

```sh
cd algorithm/lru

nw tangle --check book/lru.nw        # 只检查：chunk 77 个，file 10 个，警告 0 条
nw tangle book/lru.nw -o tangle      # 重建 tangle/ 下 10 个 .hpp
nw weave  book/lru.nw -o weave       # 生成 HTML（index.html / lru.html / tree.html）
```

`language/cpp` 的 `book/*.nw` 目前是空文件，书稿还没开始写；开写之后重建命令与上面相同，换篇名即可：

```sh
cd language/cpp
nw tangle book/list.nw -o tangle
```

## 工具链（可选，只在跑 LRU 那本书的模型与实验时需要）

```sh
cd algorithm/lru
mkdir -p tools && cd tools
curl -LO https://github.com/tlaplus/tlaplus/releases/latest/download/tla2tools.jar
curl -LO https://github.com/AlloyTools/org.alloytools.alloy/releases/latest/download/org.alloytools.alloy.dist.jar
cd .. && python3 -m venv .venv && .venv/bin/pip install pytest hypothesis matplotlib numpy
.venv/bin/pytest legacy/lru_pbt.py
```

两条 jar 需要一个 JRE：`sudo apt install -y openjdk-21-jre-headless`。

## 未入库的目录

以下内容体积大、可重新获取，已在 `.gitignore` 里排除，clone 下来不会有：

| 路径 | 体积 | 怎么拿回来 |
|---|---|---|
| `algorithm/lru/.venv/` | 209M | 上面「工具链」里的 `python3 -m venv` |
| `algorithm/lru/tools/*.jar` | 23M | 上面「工具链」里的两条 `curl` |
| `language/cpp/reference/` | 401M | cppreference 的 Doxygen tag/HTML 抓取，以及 `git clone https://github.com/microsoft/STL` |
