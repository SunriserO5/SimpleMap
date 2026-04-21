# SimpleMap

一个基于 C++ 的地图生成与路径规划课程项目，核心目标是为车载导航场景提供：

- 可控地图生成（网格/随机）
- 图结构管理与校验
- 最短路径计算（Dijkstra / A*）
- 面向路况权重的扩展能力

## 当前状态总览

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
| 可视化界面 | 未完成 | 当前仓库仅后端算法与数据结构 |
| 自动化测试 | 未完成 | 暂无测试工程 |

## 本轮已收尾内容（2026-04-21）

1. C++11 兼容性修复
- 将 `parameters.h` 中缓存常量改为 C++11 可用定义。
- 将 `pathfinding.cpp` 中结构化绑定改为 C++11 写法。

2. 编译告警收敛
- 处理了可见的未使用参数告警，减少编译噪音。

3. `getPath` 占位逻辑补全
- 不再返回直线距离占位值。
- 改为调用 Dijkstra 并累加路径边长，不可达时返回 `-1`。

4. 可运行演示入口
- 新增 `main.cpp`，串联随机地图生成、Dijkstra/A*、JSON 保存与加载验证。

5. CLI 交互体验增强
- 主菜单支持：导入地图、生成随机地图并测试、退出。
- 路径测试子菜单支持：测试 Dijkstra / 测试 A* / 返回。
- 路径结果支持逐节点输出：每段距离与累计距离。

6. 路径缓存接入
- `getPath` 已接入缓存查询与回填逻辑。
- 在 CLI 中可观察同一路径重复查询的耗时下降。

## 目录结构

```text
SimpleMap/
├── dataStructure.h      # 核心数据结构与图管理接口
├── dataStructure.cpp    # 图管理、地图生成、连通性与校验实现
├── pathfinding.h        # Dijkstra / A* 接口
├── pathfinding.cpp      # 路径规划与路况权重实现
├── parameters.h         # 全局参数（CACHE_SIZE）
├── main.cpp             # CLI 主入口（主菜单 + 路径测试子菜单）
├── test_data_generator.cpp  # 测试数据生成器（默认可生成 1000 节点地图）
├── thirdparty/
│   └── json.hpp         # nlohmann/json 头文件
└── README.md
```

## 核心设计

### 1) 图存储模型

- 节点数组：`mapNode* mainGraph`
- 每个节点维护动态邻接数组：`Neighbour` / `distanceToNeighbour`
- 边属性数组：`EdgeAttr* edgeAttrs`

这是一种“节点数组 + 邻接数组”的轻量实现，便于课程场景下理解与调试。

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

## 关键接口速览

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

1. 新建空 C++ 项目。
2. 将仓库中的 `.h/.cpp` 添加到项目。
3. 语言标准设为 C++11 或更高。
4. 编译运行。

## 方案 B：命令行快速语法检查

```bash
clang++ -std=c++11 -Wall -Wextra -pedantic -c dataStructure.cpp pathfinding.cpp
```

说明：该命令仅做编译检查，不生成完整演示可执行文件。

## 方案 C：运行演示程序

```bash
/usr/bin/g++ -std=c++11 -Wall -Wextra -pedantic -g dataStructure.cpp pathfinding.cpp main.cpp -o simplemap_demo
./simplemap_demo
```

CLI 中可以：

1. 导入已有地图 JSON 后做路径测试。
2. 生成随机地图后进入路径测试子菜单。
3. 输出 Dijkstra/A* 的路径节点详情、分段距离、累计距离与耗时。

运行过程中如果调用保存流程，会在项目根目录生成 `sample_map.json`。

## 方案 D：生成测试数据（1000 节点示例）

```bash
/usr/bin/g++ -std=c++11 -Wall -Wextra -pedantic -g dataStructure.cpp pathfinding.cpp test_data_generator.cpp -o test_data_generator
./test_data_generator --nodes 1000 --output test_map_1000.json
```

## 已知限制

1. 当前没有 UI 层，实现重点在数据结构和算法。
2. `getPath` 返回“路径总长度（int）”，缓存的目标是距离值而非完整节点序列；如需节点序列请使用 `PathFinder`。

## 建议下一步（未执行）

1. 基于现有 CLI 增加更多回归用例（异常输入、不可达场景、边界节点）。
2. 增加 JSON 兼容性与异常文件处理测试（字段缺失、类型错误、版本兼容）。
3. 增加大规模（>=10000 节点）性能与内存基准测试。
