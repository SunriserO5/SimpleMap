#ifndef PATHFINDING_H
#define PATHFINDING_H
#include "dataStructure.h"
#include <vector>

class PathFinder {
public:
    // 堆优化 Dijkstra 最短路径
    // useCongestion=false: 用 length 作权重
    // useCongestion=true: 用通行时间 c*L*f(n/v) 作权重
    static std::vector<int> dijkstra(mapGraphBase* graph, int startId, int endId, bool useCongestion = false);

    // A* 搜索算法（欧氏距离启发函数）
    static std::vector<int> aStar(mapGraphBase* graph, int startId, int endId, bool useCongestion = false);

    // 计算边的通行时间（考虑拥堵）
    static double getTravelTime(mapGraphBase* graph, int fromID, int toID, bool useCongestion);
};

#endif // PATHFINDING_H
