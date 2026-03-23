# SimpleMap导航系统设计
车载导航系统是在城市中驾车的好帮手，不仅能够计算出到达目的地的最优路线，而且可以显示当前位置附近的一些附加信息，比如附近的加油站，餐馆等等。请设计一款导航软件，实现导航系统的核心功能。

## 目标用户
汽车司机

## 数据配置：
导航系统中的地图可以抽象为一个图，地点信息和路径信息可以抽象为图中的顶点和边。请设计算法，来产生模拟的地图数据：</br>
a)	随机产生N个二维平面上的顶点（N>=10000），每个顶点对应地图中的一个地点</br>
b)	对于每个地点x，随机建立若干个连边到x附近的地点。每条连边代表一条路径，路径的长度等于边的两个顶点之间的二维坐标距离。</br>
c)	模拟数据必须保证，产生的图是一个连通图，并且道路之间不应有不合理的交叉。</br>

## 功能要求：
F1. 地图显示功能。输入一个坐标，显示距离该坐标最近的100个顶点以及相关联的边。关于如何用vc++画图，或java画图，请在baidu或google上查找相关方法，或者借阅相关的参考书。</br>
F2. 地图缩放功能。提示：地图缩的越小，屏幕上显示的点数就越多，但是太多的点，会看不清楚。所以可以考虑只选择一个单元区域内只显示一个代表的点。</br>
F2. 任意指定两个地点A和B，能计算出A到B的最短路径。并将该最短路径经过的顶点以及连边显示出来。</br>
F3. 模拟车流。请为每条连边增加两个属性：车容量v（饱和状态下这条路所能容纳的汽车的数量）、当前在这条路上的车辆数目n（n>v时为超负荷运作）。假设该路的长度为L，则该路的通行时间可模拟为cLf(n/v)，其中c是常数；f(x)是一个分段函数, x小于等于某个常数时，f(x) = 1，当x大于该常数时，f(x) = 1+ex。每条道路的车容量v和道路的长度L为预先指定的固定参数。请模拟产生汽车在地图中行驶，为简化模型假设在同一条路上每架汽车穿越该路的时间均等于cLf(n/v)。要求实现模拟车流的动态变化，任意时刻，给定一个坐标，能在界面上显示该坐标附近的所有路径，并动态显示各个路径上的车流量的大小（可用不同颜色或其他方法区分车流量的大小级别）。</br>
F4. 综合考虑路况的最短路径。任意时刻，指定两个地点A和B，能根据当前的路况，计算出从A到B最短行车时间，以及相应的最佳路径，并在界面上，将该最短路径经过的顶点以及连边显示出来。



# 车载导航系统 - 地图与路径规划模块

基于 C++ 实现的数据结构大作业，包含完整的地图生成、存储、路径规划与车流模拟功能。

## 目录

- 项目概述
- 技术栈
- 功能特性
- 文件结构
- 核心数据结构
- 快速开始
- API 文档
- 使用示例

## 项目概述

本项目实现了一个完整的车载导航系统核心后端，包括：

- 支持生成≥10000 个节点的网格地图或无交叉随机地图（Delaunay 三角剖分）
- 基于邻接表的高效图存储
- 堆优化 Dijkstra 算法与 A * 启发式搜索算法
- JSON 格式的地图数据持久化
- 地图连通性验证与数据校验
- 预留车流模拟与动态路况接口

## 技术栈

- 编程语言：C++ (C++11 及以上)
- 开发环境：Visual Studio 2022 (MSVC)
- 第三方库：nlohmann/json (Header-Only)
- 核心算法：邻接表、堆优化 Dijkstra、A*、BFS、Delaunay 三角剖分

## 功能特性

### 地图生成
- 网格地图生成：生成均匀分布的网格状地图，100% 全连通，无交叉
- 随机地图生成：生成带最小间距约束的随机节点，通过 Delaunay 三角剖分实现无交叉连边
- 自动属性分配：自动计算道路长度、限速、容量等属性

### 图数据管理
- 邻接表存储：高效的图存储结构，支持 O(1) 节点查询与 O(k) 邻边遍历（k 为邻居数）
- 标准化接口：提供完整的节点 / 边查询、修改接口
- 内存安全：自动管理动态内存，无内存泄漏

### 路径规划
- 堆优化 Dijkstra 算法：时间复杂度 O(m log n)，支持最短距离路径查询
- A * 启发式搜索算法：基于欧氏距离启发函数，比 Dijkstra 快 3-10 倍，同样保证最优解
- 动态路况支持：支持基于拥堵系数的动态权重路径规划

### 数据持久化
- JSON 保存：将地图数据（节点、边、元数据）保存为人类可读的 JSON 文件
- JSON 加载：从 JSON 文件快速重建地图，支持测试结果复现
- 数据校验：加载后自动验证数据完整性与合理性

### 验证与辅助
- BFS 全连通验证：保证生成的地图无孤立节点 / 子图
- 数据合理性检查：自动验证节点 ID、坐标范围、距离非负等

## 文件结构
导航系统项目/
├── datastructure.h # 核心头文件，定义所有数据结构与类接口
├── datastructure.cpp # 核心源文件，实现所有功能
├── thirdparty/
│ └── json.hpp # nlohmann/json库（Header-Only）
├── main.cpp # 测试主函数
└── navi_map.json # （生成后）地图数据文件

## 核心数据结构

```cpp
// 地图节点
struct mapNode {
    int nodeID;              // 节点唯一ID
    int x_coordinate;        // X坐标
    int y_coordinate;        // Y坐标
    int numOfNeighbour;      // 邻居数量
    int* Neighbour;          // 邻居ID数组
    int* distanceToNeighbour;// 到邻居的距离数组
};

// 道路边属性
struct EdgeAttr {
    int edgeID;              // 边唯一ID
    int fromNodeID;          // 起点ID
    int toNodeID;            // 终点ID
    double length;           // 道路长度
    double speedLimit;       // 限速 (m/s)
    double capacity;         // 最大车容量
    double currentCars;      // 当前车辆数
    double congestion;       // 拥堵系数 (0-1)
};

// 核心管理类
class mapGraphBase {
public:
    // 构造与析构
    mapGraphBase();
    ~mapGraphBase();

    // 地图生成
    bool generateGridMap(int mapWidth, int mapHeight, int nodeColCount, int nodeRowCount);
    bool generateRandomMap(int mapWidth, int mapHeight, int totalNodeCount);

    // 数据查询
    int getNumOfNodes();
    int getNumOfEdges();
    mapNode* getNodeById(int nodeID);
    EdgeAttr* getEdgeById(int edgeID);

    // 路径规划
    std::vector<int> findShortestPath_Dijkstra(int startId, int endId, bool useCongestion = false);
    std::vector<int> findShortestPath_AStar(int startId, int endId, bool useCongestion = false);

    // 数据持久化
    bool saveMapToFile(const char* filePath);
    bool loadMapFromFile(const char* filePath);

    // 验证
    bool isMapFullyConnected();
    bool isDataValid();

    // 辅助
    void clearMap();
};



