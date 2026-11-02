# 深挖 LRU —— 迁移包

CodeBuddy 会话（2026-11-01）的完整导出 + 项目计划书。原环境 CodeBuddy 崩溃，此包用于迁移到工作站继续执行。

## 文件说明

| 文件 | 用途 |
|---|---|
| `计划书_LRU深挖.md` | **先读这个**：项目背景、L1–L5 分层大纲、工具链安装清单、neoweb 骨架计划、执行顺序、概念索引、迁移 checklist |
| `聊天记录_01_LRU主会话.md` | LRU 深挖主会话完整记录（44 条消息），含 TLA+ 规约、Alloy 模型、PBT/MBT 讲解原文 |
| `聊天记录_02_neoweb构建.md` | neoweb 本地构建记录（67 条消息） |
| `lru_pbt.py` | PBT 可跑模板（参考实现 + hypothesis 状态机） |
| `lru.nw` | 书的开头草稿 |

## 快速开始（工作站上）

```bash
# 1. neoweb
git clone https://github.com/random25160765-collab/neoweb.git && cd neoweb && make

# 2. 工具链（详见计划书 §3）
sudo apt install -y openjdk-21-jre-headless
mkdir -p <深挖LRU>/tools && cd <深挖LRU>/tools
curl -LO https://github.com/tlaplus/tlaplus/releases/latest/download/tla2tools.jar
curl -LO https://github.com/AlloyTools/org.alloytools.alloy/releases/latest/download/org.alloytools.alloy.dist.jar
cd <深挖LRU> && python3 -m venv .venv && .venv/bin/pip install pytest hypothesis matplotlib numpy
```

然后按计划书 §5 的顺序：搭 neoweb 骨架 → L1 章开始。
