# SimpleMap

一个基于 C++ 的地图生成与路径规划课程项目，核心目标是为车载导航场景提供：

- 可控地图生成（网格/随机）
- 图结构管理与校验
- 最短路径计算（Dijkstra / A*）
- 面向路况权重的扩展能力

## 目标一览

| 模块 | 状态 | 说明 |
|---|---|---|
| 图数据结构 | 已完成 | `mapNode` + `EdgeAttr` + `mapGraphBase` 基本可用 |
| 网格地图生成 | 已完成 | 支持按行列生成并自动连边 |
| 随机地图生成 | 已完成 | 随机点 + K 近邻连边 + 去交叉 + 连通性补边 |
| 路径规划（距离） | 已完成 | `PathFinder::dijkstra` 与 `PathFinder::aStar` |
| 路况权重路径 | 已完成 | 支持按 `c * L * f(n/v)` 计算边权 |
| 基础数据校验 | 已完成 | `isMapFullyConnected`、`isDataValid` |
| JSON 持久化 | 已完成 | 支持保存/加载节点、边和元数据 |
| CLI 交互界面 | 已完成 | 主菜单 + 路径测试子菜单 + 导入/生成双流程 |


## 目录结构

```text
SimpleMap/
├── dataStructure.h      # 核心数据结构与图管理接口
├── dataStructure.cpp    # 图管理、地图生成、连通性与校验实现
├── pathfinding.h        # Dijkstra / A* 接口
├── pathfinding.cpp      # 路径规划与路况权重实现
├── parameters.h         # 全局参数（CACHE_SIZE）(我也不知道为啥要放在单独文件里，反正就放了)
├── main.cpp             # CLI 主入口（主菜单 + 路径测试子菜单）
├── test_data_generator.cpp  # 测试数据生成器（默认可生成 1000 节点地图,本文件算法由ai生成）
├── thirdparty/
│   └── json.hpp         # nlohmann/json 头文件
└── README.md
```

## 核心设计

### 1) 图存储模型

- 节点数组：`mapNode* mainGraph`
- 每个节点维护动态邻接数组：`Neighbour` / `distanceToNeighbour`
- 边属性数组：`EdgeAttr* edgeAttrs`

这是一种“节点数组 + 邻接数组”的轻量实现，便于调试。

### 2) 地图生成策略

- 网格地图：规则坐标生成 + 右/下邻居双向连边
- 随机地图：
  - 最小间距随机采样
  - K 近邻连边
  - 线段相交检测并删边
  - BFS 连通分量检测与补边

### 3) 路径规划策略

- Dijkstra：用于全局最短路
- A*：启发函数为欧氏距离
- 可切换权重：
  - 纯长度模式：`length`
  - 路况模式：`c * L * f(n/v)`

## 关键接口

### mapGraphBase

- 生成地图
  - `generateGridMap(int mapWidth, int mapHeight, int nodeColCount, int nodeRowCount)`
  - `generateRandomMap(int mapWidth, int mapHeight, int totalNodeCount)`
- 查询与修改
  - `getNodeById(int nodeID)`
  - `getEdgeById(int edgeID)`
  - `getEdgeByNodes(int fromID, int toID)`
  - `updateEdgeCongestion(int edgeID, double newCongestion)`
- 校验
  - `isMapFullyConnected()`
  - `isDataValid()`
- 路径长度
  - `getPath(int nodeID1, int nodeID2)`

### PathFinder

- `dijkstra(mapGraphBase* graph, int startId, int endId, bool useCongestion = false)`
- `aStar(mapGraphBase* graph, int startId, int endId, bool useCongestion = false)`
- `getTravelTime(mapGraphBase* graph, int fromID, int toID, bool useCongestion)`

## 构建说明

## 方案 A：Visual Studio 2022（推荐）

1. 新建空 C++ 项目
2. 将仓库中的 `.h/.cpp` 添加到项目
3. 语言标准设为 C++11 或更高
4. 编译运行

## 方案 B：仓库自带的Makefile（Linux/Mac）

```bash
make
```

## 方案 C：运行演示程序

```bash
/usr/bin/g++ -std=c++11 -Wall -Wextra -pedantic -g dataStructure.cpp pathfinding.cpp main.cpp -o simplemap_demo
./simplemap_demo
```

CLI 中可以：

1. 导入已有地图 JSON 后做路径测试
2. 生成随机地图后进入路径测试子菜单
3. 输出 Dijkstra/A* 的路径节点详情、分段距离、累计距离与耗时

运行过程中如果调用保存流程，会在项目根目录生成 json文件

## 注意

1. 当前没有 UI 层，实现重点在数据结构和算法
2. `getPath` 返回“路径总长度（int）”，缓存的目标是距离值而非完整节点序列；如需节点序列请使用 `PathFinder`

## 小组成员
刘宜东 郑云祥
2026.5
