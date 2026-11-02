# C++ 标准库

从 `list` 与 `unordered_map` 入手，把标准库容器的约定、节点布局、迭代器与失效规则写成书。`book/*.nw` 是唯一真相，`tangle/` 是它展开出来的产物。

```
book/list.nw           书稿（尚未开始，空文件）
book/unordered_map.nw  书稿（尚未开始，空文件）
book/*.cpp             准备收进书里的原始代码片段
reference/             写书时查的第三方素材，未入库，见下
```

## 重建

展开由 [neoweb](https://github.com/random25160765-collab/neoweb) 完成，先装引擎：

```sh
git clone https://github.com/random25160765-collab/neoweb.git
cd neoweb && make && make install     # 装到 ~/.local/bin/nw
```

之后按篇展开：

```sh
nw tangle --check book/list.nw
nw tangle book/list.nw -o tangle
nw weave  book/list.nw -o weave
```

目前 `book/*.nw` 还是空文件，跑上面任意一条都会得到 `没有 book 定义：一本书从 book 开始` —— 书稿开写之后这几条才成立。

## 未入库的目录

`reference/` 是写书用的素材（cppreference 的 Doxygen tag 与 HTML 抓取、微软 STL 源码），401M，已被 `.gitignore` 排除。需要时重新取：

```sh
git clone https://github.com/microsoft/STL
# cppreference 的 tag.xml 与 html-book-*.tar.xz 发布于 cppreference 的 Doxygen 抓取页
```
