#include "pathfinding.h"
#include <queue>
#include <vector>
#include <climits>
#include <cmath>
#include <algorithm>
#include <functional>

// 通行时间公式参数（来自 README）
// travelTime = c * L * f(n/v)
// f(x) = 1           当 x <= THRESHOLD
// f(x) = 1 + E * x   当 x > THRESHOLD
static constexpr double TRAFFIC_C = 1.0;
static constexpr double TRAFFIC_THRESHOLD = 0.7;
static constexpr double TRAFFIC_E = 0.5;

double PathFinder::getTravelTime(mapGraphBase* graph, int fromID, int toID, bool useCongestion) {
	EdgeAttr* edge = graph->getEdgeByNodes(fromID, toID);
	if (edge == nullptr) return 1e18; // 不存在的边返回极大值

	if (!useCongestion) {
		return edge->length;
	}

	double L = edge->length;
	double v = edge->capacity;
	double n = edge->currentCars;

	if (v <= 0) return L; // 防止除零

	double x = n / v;
	double f;
	if (x <= TRAFFIC_THRESHOLD) {
		f = 1.0;
	} else {
		f = 1.0 + TRAFFIC_E * x;
	}

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

	// 启发函数：欧氏距离
	auto heuristic = [&](int nodeID) -> double {
		double dx = nodes[nodeID].x_coordinate - endX;
		double dy = nodes[nodeID].y_coordinate - endY;
		return std::sqrt(dx * dx + dy * dy);
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
