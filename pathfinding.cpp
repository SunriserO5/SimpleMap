#include "pathfinding.h"
#include <queue>
#include <vector>
#include <climits>
#include <cmath>
#include <algorithm>
#include <functional>

// F4 通行时间公式参数（题目要求）
// travelTime = c * L * f(n/v)
// f(x) = 1                   当 x <= THRESHOLD
// f(x) = 1 + e^(x - THRESHOLD) 当 x > THRESHOLD
static constexpr double TRAFFIC_C = 0.1;      // 常数c（与 TrafficSimulator 保持一致）
static constexpr double TRAFFIC_THRESHOLD = 0.7; // n/v 阈值

double PathFinder::getTravelTime(mapGraphBase* graph, int fromID, int toID, bool useCongestion) {
	EdgeAttr* edge = graph->getEdgeByNodes(fromID, toID);
	if (edge == nullptr) return 1e18; // 不存在的边返回极大值

	// 不考虑拥堵时，直接返回道路长度
	if (!useCongestion) {
		return edge->length;
	}

	double L = edge->length;
	double v = std::max(1.0, edge->capacity / 10.0);  // 容量缩小10倍，与 TrafficSimulator 保持一致
	double n = edge->currentCars;

	double x = n / v;  // n/v 负载率
	double f;
	if (x <= TRAFFIC_THRESHOLD) {
		// 低负载：f(x) = 1
		f = 1.0;
	} else {
		// 高负载：f(x) = 1 + e^(x - threshold)
		f = 1.0 + std::exp(x - TRAFFIC_THRESHOLD);
	}

	// F4: 通行时间 = c * L * f(n/v)
	return TRAFFIC_C * L * f;
}

std::vector<int> PathFinder::dijkstra(mapGraphBase* graph, int startId, int endId, bool useCongestion) {
	int n = graph->getNumOfNodes();
	std::vector<int> result;

	if (startId < 0 || startId >= n || endId < 0 || endId >= n) return result;
	if (n == 0) return result;

	// 距离数组
	std::vector<double> dist(n, 1e18);
	// 前驱数组
	std::vector<int> prev(n, -1);
	// 已访问标记
	std::vector<bool> visited(n, false);

	dist[startId] = 0;

	// 小顶堆: (距离, 节点ID)
	using pii = std::pair<double, int>;

	//todo 可以改写成手搓的优先队列
	std::priority_queue<pii, std::vector<pii>, std::greater<pii>> pq;
	pq.push({0.0, startId});

	mapNode* nodes = graph->getMainGraph();

	while (!pq.empty()) {
		int u = pq.top().second;
		pq.pop();

		if (visited[u]) continue;
		visited[u] = true;

		if (u == endId) break;

		mapNode& node = nodes[u];
		for (int i = 0; i < node.numOfNeighbour; i++) {
			int v = node.Neighbour[i];
			if (visited[v]) continue;

			double w = getTravelTime(graph, u, v, useCongestion);
			if (dist[u] + w < dist[v]) {
				dist[v] = dist[u] + w;
				prev[v] = u;
				pq.push({dist[v], v});
			}
		}
	}

	// 回溯路径
	if (dist[endId] >= 1e18) return result; // 不可达

	for (int cur = endId; cur != -1; cur = prev[cur]) {
		result.push_back(cur);
	}
	std::reverse(result.begin(), result.end());

	return result;
}

std::vector<int> PathFinder::aStar(mapGraphBase* graph, int startId, int endId, bool useCongestion) {
	int n = graph->getNumOfNodes();
	std::vector<int> result;

	if (startId < 0 || startId >= n || endId < 0 || endId >= n) return result;
	if (n == 0) return result;

	mapNode* nodes = graph->getMainGraph();
	int endX = nodes[endId].x_coordinate;
	int endY = nodes[endId].y_coordinate;

	// 启发函数：欧氏距离（按边权缩放因子校准）
	// 拥堵模式下边权 = TRAFFIC_C * L * f(n/v)，最小为 TRAFFIC_C * L
	// 因此启发式也需缩放 TRAFFIC_C 倍，保证可采纳性
	double hScale = useCongestion ? TRAFFIC_C : 1.0;
	auto heuristic = [&](int nodeID) -> double {
		double dx = nodes[nodeID].x_coordinate - endX;
		double dy = nodes[nodeID].y_coordinate - endY;
		return hScale * std::sqrt(dx * dx + dy * dy);
	};

	// gScore: 从起点到当前节点的实际代价
	std::vector<double> gScore(n, 1e18);
	// fScore: gScore + heuristic
	std::vector<double> fScore(n, 1e18);
	// 前驱
	std::vector<int> prev(n, -1);
	// 已访问
	std::vector<bool> closed(n, false);

	gScore[startId] = 0;
	fScore[startId] = heuristic(startId);

	using pii = std::pair<double, int>;
	std::priority_queue<pii, std::vector<pii>, std::greater<pii>> openSet;
	openSet.push({fScore[startId], startId});

	while (!openSet.empty()) {
		int u = openSet.top().second;
		openSet.pop();

		if (closed[u]) continue;
		closed[u] = true;

		if (u == endId) break;

		mapNode& node = nodes[u];
		for (int i = 0; i < node.numOfNeighbour; i++) {
			int v = node.Neighbour[i];
			if (closed[v]) continue;

			double w = getTravelTime(graph, u, v, useCongestion);
			double tentativeG = gScore[u] + w;

			if (tentativeG < gScore[v]) {
				prev[v] = u;
				gScore[v] = tentativeG;
				fScore[v] = gScore[v] + heuristic(v);
				openSet.push({fScore[v], v});
			}
		}
	}

	// 回溯路径
	if (gScore[endId] >= 1e18) return result;

	for (int cur = endId; cur != -1; cur = prev[cur]) {
		result.push_back(cur);
	}
	std::reverse(result.begin(), result.end());

	return result;
}
