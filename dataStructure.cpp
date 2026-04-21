#include"dataStructure.h"
#include "pathfinding.h"
#include "thirdparty/json.hpp"
#include<fstream>
#include<queue>
#include<cstring>
#include<string>
#include<cmath>
#include<climits>
#include<cstdlib>
#include<stdexcept>
#include<vector>
#include<algorithm>

using nlohmann::json;

// ========== 构造 / 析构 ==========

mapGraphBase::mapGraphBase(int nodes, int edges, mapNode* graph) {
	(void)edges;
	numOfNodes = 0;
	numOfEdges = 0;
	mainGraph = nullptr;
	edgeAttrs = nullptr;
	nextEdgeID = 0;
	m_allocatedNodeCount = 0;
	mapMinX = 0;
	mapMaxX = 10000;
	mapMinY = 0;
	mapMaxY = 10000;
	cache = new cacheNode[CACHE_SIZE];

	if (nodes > 0) {
		allocateMainGraphMemory(nodes);
	}
	if (graph != nullptr) {
		for (int i = 0; i < nodes; i++) {
			addNode(graph[i].x_coordinate, graph[i].y_coordinate);
		}
	}
}

mapGraphBase::~mapGraphBase() {
	clearMap();
	if (cache != nullptr) {
		delete[] cache;
		cache = nullptr;
	}
}

// ========== 内存管理 ==========

void mapGraphBase::allocateMainGraphMemory(int nodeCount) {
	mainGraph = new mapNode[nodeCount];
	m_allocatedNodeCount = nodeCount;
	// edgeAttrs 初始分配 nodeCount*4 容量（预估平均每节点 4 条边）
	// 实际边数可能超出，addOneWayEdge 中动态扩容
	int initialEdgeCapacity = nodeCount * 4;
	edgeAttrs = new EdgeAttr[initialEdgeCapacity];
	m_allocatedEdgeCount = initialEdgeCapacity;
}

void mapGraphBase::freeMainGraphMemory() {
	if (mainGraph != nullptr) {
		for (int i = 0; i < numOfNodes; i++) {
			if (mainGraph[i].Neighbour != nullptr) {
				delete[] mainGraph[i].Neighbour;
				mainGraph[i].Neighbour = nullptr;
			}
			if (mainGraph[i].distanceToNeighbour != nullptr) {
				delete[] mainGraph[i].distanceToNeighbour;
				mainGraph[i].distanceToNeighbour = nullptr;
			}
		}
		delete[] mainGraph;
		mainGraph = nullptr;
	}
	if (edgeAttrs != nullptr) {
		delete[] edgeAttrs;
		edgeAttrs = nullptr;
	}
}

void mapGraphBase::clearMap() {
	freeMainGraphMemory();
	numOfNodes = 0;
	numOfEdges = 0;
	m_allocatedNodeCount = 0;
	m_allocatedEdgeCount = 0;
	nextEdgeID = 0;
	mapMinX = 0;
	mapMaxX = 10000;
	mapMinY = 0;
	mapMaxY = 10000;
	if (cache != nullptr) {
		for (int i = 0; i < CACHE_SIZE; i++) {
			if (cache[i].path != nullptr) {
				delete[] cache[i].path;
				cache[i].path = nullptr;
			}
			cache[i].beginNodeID = -1;
			cache[i].endNodeID = -1;
			cache[i].distance = -1;
		}
	}
}

// ========== 节点操作 ==========

int mapGraphBase::addNode(int x, int y) {
	//用 m_allocatedNodeCount 判断是否已满,maingraph未分配或节点数达到分配上限时，返回错误
	if (mainGraph == nullptr || numOfNodes >= m_allocatedNodeCount) {
		return -1;
	}
	int newID = numOfNodes;
	mainGraph[newID].nodeID = newID;
	mainGraph[newID].x_coordinate = x;
	mainGraph[newID].y_coordinate = y;
	mainGraph[newID].numOfNeighbour = 0;
	mainGraph[newID].Neighbour = nullptr;
	mainGraph[newID].distanceToNeighbour = nullptr;
	numOfNodes++;

	if (x < mapMinX) mapMinX = x;
	if (x > mapMaxX) mapMaxX = x;
	if (y < mapMinY) mapMinY = y;
	if (y > mapMaxY) mapMaxY = y;
	return newID;
}

// ========== 边操作 ==========

bool mapGraphBase::addOneWayEdge(int fromID, int toID) {
	if (fromID < 0 || fromID >= numOfNodes || toID < 0 || toID >= numOfNodes) {
		return false;
	}

	mapNode& fromNode = mainGraph[fromID];
	// 扩容 neighbor 列表：新数组 = 旧数组拷贝 + 新元素
	int newNeighbourCount = fromNode.numOfNeighbour + 1;
	int* newNeighbours = new int[newNeighbourCount];
	int* newDistances = new int[newNeighbourCount];

	for (int i = 0; i < fromNode.numOfNeighbour; i++) {
		newNeighbours[i] = fromNode.Neighbour[i];
		newDistances[i] = fromNode.distanceToNeighbour[i];
	}

	int dist = calculateDistance(
		fromNode.x_coordinate, fromNode.y_coordinate,
		mainGraph[toID].x_coordinate, mainGraph[toID].y_coordinate
	);
	newNeighbours[fromNode.numOfNeighbour] = toID;
	newDistances[fromNode.numOfNeighbour] = dist;

	delete[] fromNode.Neighbour;
	delete[] fromNode.distanceToNeighbour;
	fromNode.Neighbour = newNeighbours;
	fromNode.distanceToNeighbour = newDistances;
	fromNode.numOfNeighbour = newNeighbourCount;

	// 创建 EdgeAttr
	// 如果边数组已满，扩容
	if (numOfEdges >= m_allocatedEdgeCount) {
		int newCapacity = m_allocatedEdgeCount * 2;
		EdgeAttr* newEdges = new EdgeAttr[newCapacity];
		for (int i = 0; i < numOfEdges; i++) {
			newEdges[i] = edgeAttrs[i];
		}
		delete[] edgeAttrs;
		edgeAttrs = newEdges;
		m_allocatedEdgeCount = newCapacity;
	}

	edgeAttrs[numOfEdges] = EdgeAttr(nextEdgeID, fromID, toID, dist);
	numOfEdges++;
	nextEdgeID++;

	return true;
}

bool mapGraphBase::addTwoWayEdge(int fromID, int toID) {
	bool ok1 = addOneWayEdge(fromID, toID);
	bool ok2 = addOneWayEdge(toID, fromID);
	return ok1 && ok2;
}

// ========== 查询方法 ==========

int mapGraphBase::getNumOfNodes() {
	return numOfNodes;
}

int mapGraphBase::getNumOfEdges() {
	return numOfEdges;
}

mapNode* mapGraphBase::getMainGraph() {
	return mainGraph;
}

int* mapGraphBase::getCoordinate(int nodeID) {
	if (nodeID < 0 || nodeID >= numOfNodes) return nullptr;
	int* coord = new int[2];
	coord[0] = mainGraph[nodeID].x_coordinate;
	coord[1] = mainGraph[nodeID].y_coordinate;
	return coord;
}

int mapGraphBase::getPath(int nodeID1, int nodeID2) {
	if (nodeID1 < 0 || nodeID1 >= numOfNodes || nodeID2 < 0 || nodeID2 >= numOfNodes) {
		return -1;
	}

	if (cache != nullptr) {
		for (int i = 0; i < CACHE_SIZE; i++) {
			if (cache[i].beginNodeID == nodeID1 && cache[i].endNodeID == nodeID2 && cache[i].distance >= 0) {
				return cache[i].distance;
			}
		}
	}

	std::vector<int> path = PathFinder::dijkstra(this, nodeID1, nodeID2, false);
	if (path.empty()) {
		return -1;
	}

	double totalLength = 0.0;
	for (size_t i = 1; i < path.size(); i++) {
		EdgeAttr* edge = getEdgeByNodes(path[i - 1], path[i]);
		if (edge == nullptr) {
			return -1;
		}
		totalLength += edge->length;
	}

	int resultDistance = static_cast<int>(totalLength);

	if (cache != nullptr) {
		int targetSlot = -1;
		for (int i = 0; i < CACHE_SIZE; i++) {
			if (cache[i].distance < 0) {
				targetSlot = i;
				break;
			}
		}
		if (targetSlot < 0) {
			targetSlot = (nodeID1 * 131 + nodeID2) % CACHE_SIZE;
			if (targetSlot < 0) {
				targetSlot += CACHE_SIZE;
			}
		}

		if (cache[targetSlot].path != nullptr) {
			delete[] cache[targetSlot].path;
			cache[targetSlot].path = nullptr;
		}

		cache[targetSlot].beginNodeID = nodeID1;
		cache[targetSlot].endNodeID = nodeID2;
		cache[targetSlot].distance = resultDistance;
	}

	return resultDistance;
}

mapNode* mapGraphBase::getNodeById(int nodeID) {
	if (nodeID < 0 || nodeID >= numOfNodes) return nullptr;
	return &mainGraph[nodeID];
}

EdgeAttr* mapGraphBase::getEdgeById(int edgeID) {
	for (int i = 0; i < numOfEdges; i++) {
		if (edgeAttrs[i].edgeID == edgeID) return &edgeAttrs[i];
	}
	return nullptr;
}

EdgeAttr* mapGraphBase::getEdgeByNodes(int fromID, int toID) {
	for (int i = 0; i < numOfEdges; i++) {
		if (edgeAttrs[i].fromNodeID == fromID && edgeAttrs[i].toNodeID == toID) {
			return &edgeAttrs[i];
		}
	}
	return nullptr;
}

EdgeAttr* mapGraphBase::getAllEdgeAttrs() {
	return edgeAttrs;
}

bool mapGraphBase::updateEdgeCongestion(int edgeID, double newCongestion) {
	EdgeAttr* edge = getEdgeById(edgeID);
	if (edge == nullptr) return false;
	edge->congestion = newCongestion;
	return true;
}

bool mapGraphBase::saveMapToFile(const char* filePath) {
	if (filePath == nullptr || std::strlen(filePath) == 0) {
		return false;
	}

	try {
		json root;
		root["metadata"] = {
			{ "numOfNodes", numOfNodes },
			{ "numOfEdges", numOfEdges },
			{ "nextEdgeID", nextEdgeID },
			{ "mapMinX", mapMinX },
			{ "mapMaxX", mapMaxX },
			{ "mapMinY", mapMinY },
			{ "mapMaxY", mapMaxY }
		};

		root["nodes"] = json::array();
		for (int i = 0; i < numOfNodes; i++) {
			root["nodes"].push_back({
				{ "nodeID", mainGraph[i].nodeID },
				{ "x_coordinate", mainGraph[i].x_coordinate },
				{ "y_coordinate", mainGraph[i].y_coordinate }
			});
		}

		root["edges"] = json::array();
		for (int i = 0; i < numOfEdges; i++) {
			root["edges"].push_back({
				{ "edgeID", edgeAttrs[i].edgeID },
				{ "fromNodeID", edgeAttrs[i].fromNodeID },
				{ "toNodeID", edgeAttrs[i].toNodeID },
				{ "length", edgeAttrs[i].length },
				{ "speedLimit", edgeAttrs[i].speedLimit },
				{ "capacity", edgeAttrs[i].capacity },
				{ "currentCars", edgeAttrs[i].currentCars },
				{ "congestion", edgeAttrs[i].congestion }
			});
		}

		std::ofstream out(filePath);
		if (!out.is_open()) {
			return false;
		}
		out << root.dump(2);
		return true;
	}
	catch (...) {
		return false;
	}
}

bool mapGraphBase::loadMapFromFile(const char* filePath) {
	if (filePath == nullptr || std::strlen(filePath) == 0) {
		return false;
	}

	try {
		std::ifstream in(filePath);
		if (!in.is_open()) {
			return false;
		}

		json root;
		in >> root;

		if (!root.contains("nodes") || !root["nodes"].is_array()) {
			return false;
		}
		if (!root.contains("edges") || !root["edges"].is_array()) {
			return false;
		}

		struct NodeRaw {
			int nodeID;
			int x;
			int y;
		};

		std::vector<NodeRaw> nodes;
		nodes.reserve(root["nodes"].size());

		for (size_t i = 0; i < root["nodes"].size(); i++) {
			const json& nodeObj = root["nodes"][i];
			if (!nodeObj.is_object()) {
				return false;
			}
			if (!nodeObj.contains("nodeID") || !nodeObj.contains("x_coordinate") || !nodeObj.contains("y_coordinate")) {
				return false;
			}

			NodeRaw node;
			node.nodeID = nodeObj["nodeID"].get<int>();
			node.x = nodeObj["x_coordinate"].get<int>();
			node.y = nodeObj["y_coordinate"].get<int>();
			nodes.push_back(node);
		}

		std::sort(nodes.begin(), nodes.end(), [](const NodeRaw& a, const NodeRaw& b) {
			return a.nodeID < b.nodeID;
		});

		for (size_t i = 0; i < nodes.size(); i++) {
			if (nodes[i].nodeID != static_cast<int>(i)) {
				return false;
			}
		}

		clearMap();

		if (!nodes.empty()) {
			allocateMainGraphMemory(static_cast<int>(nodes.size()));
			for (size_t i = 0; i < nodes.size(); i++) {
				int newID = addNode(nodes[i].x, nodes[i].y);
				if (newID != static_cast<int>(i)) {
					clearMap();
					return false;
				}
			}
		}

		int maxEdgeID = -1;
		for (size_t i = 0; i < root["edges"].size(); i++) {
			const json& edgeObj = root["edges"][i];
			if (!edgeObj.is_object()) {
				clearMap();
				return false;
			}
			if (!edgeObj.contains("fromNodeID") || !edgeObj.contains("toNodeID")) {
				clearMap();
				return false;
			}

			int fromID = edgeObj["fromNodeID"].get<int>();
			int toID = edgeObj["toNodeID"].get<int>();
			if (!addOneWayEdge(fromID, toID)) {
				clearMap();
				return false;
			}

			EdgeAttr& edge = edgeAttrs[numOfEdges - 1];
			edge.edgeID = edgeObj.value("edgeID", edge.edgeID);
			edge.length = edgeObj.value("length", edge.length);
			edge.speedLimit = edgeObj.value("speedLimit", edge.speedLimit);
			edge.capacity = edgeObj.value("capacity", edge.capacity);
			edge.currentCars = edgeObj.value("currentCars", edge.currentCars);
			edge.congestion = edgeObj.value("congestion", edge.congestion);

			if (edge.edgeID > maxEdgeID) {
				maxEdgeID = edge.edgeID;
			}
		}

		nextEdgeID = maxEdgeID + 1;
		if (root.contains("metadata") && root["metadata"].is_object()) {
			int savedNextEdgeID = root["metadata"].value("nextEdgeID", nextEdgeID);
			if (savedNextEdgeID > nextEdgeID) {
				nextEdgeID = savedNextEdgeID;
			}
		}

		return isDataValid();
	}
	catch (...) {
		clearMap();
		return false;
	}
}

// ========== 辅助 ==========

int mapGraphBase::calculateDistance(int x1, int y1, int x2, int y2) {
	double dx = x2 - x1;
	double dy = y2 - y1;
	return (int)std::sqrt(dx * dx + dy * dy);
}

// ========== 地图生成 ==========

bool mapGraphBase::generateGridMap(int mapWidth, int mapHeight, int nodeColCount, int nodeRowCount) {
	if (nodeColCount < 2 || nodeRowCount < 2) return false;

	clearMap();
	int totalNodes = nodeColCount * nodeRowCount;
	allocateMainGraphMemory(totalNodes);

	double dx = (nodeColCount > 1) ? (double)mapWidth / (nodeColCount - 1) : 0;
	double dy = (nodeRowCount > 1) ? (double)mapHeight / (nodeRowCount - 1) : 0;

	// 添加节点
	for (int row = 0; row < nodeRowCount; row++) {
		for (int col = 0; col < nodeColCount; col++) {
			int x = (int)(col * dx);
			int y = (int)(row * dy);
			addNode(x, y);
		}
	}

	// 添加边：每个节点连接右邻居和下邻居
	for (int row = 0; row < nodeRowCount; row++) {
		for (int col = 0; col < nodeColCount; col++) {
			int nodeID = row * nodeColCount + col;
			// 右邻居
			if (col + 1 < nodeColCount) {
				addTwoWayEdge(nodeID, nodeID + 1);
			}
			// 下邻居
			if (row + 1 < nodeRowCount) {
				addTwoWayEdge(nodeID, nodeID + nodeColCount);
			}
		}
	}

	return true;
}

bool mapGraphBase::generateRandomMap(int mapWidth, int mapHeight, int totalNodeCount) {
	if (totalNodeCount < 2 || mapWidth <= 0 || mapHeight <= 0) return false;

	clearMap();
	allocateMainGraphMemory(totalNodeCount);

	// 生成随机节点，保证最小间距
	const int MIN_DISTANCE = 10; // 最小节点间距
	srand(42); // 固定种子，方便复现

	int attempts = 0;
	const int MAX_ATTEMPTS = totalNodeCount * 100;

	while (numOfNodes < totalNodeCount && attempts < MAX_ATTEMPTS) {
		int x = rand() % mapWidth;
		int y = rand() % mapHeight;

		// 检查与已有节点的最小间距
		bool tooClose = false;
		for (int i = 0; i < numOfNodes; i++) {
			if (calculateDistance(x, y, mainGraph[i].x_coordinate, mainGraph[i].y_coordinate) < MIN_DISTANCE) {
				tooClose = true;
				break;
			}
		}

		if (!tooClose) {
			addNode(x, y);
		}
		attempts++;
	}

	if (numOfNodes < 2) return false;

	// K 近邻连边
	const int K = 4; // 每个节点连接最近的 K 个邻居
	for (int i = 0; i < numOfNodes; i++) {
		// 找到距离最近的 K 个节点
		// 使用简单选择：维护一个大小为 K 的最近邻数组
		int* nearestIDs = new int[K];
		int* nearestDists = new int[K];
		int nearestCount = 0;

		for (int j = 0; j < numOfNodes; j++) {
			if (j == i) continue;
			int dist = calculateDistance(mainGraph[i].x_coordinate, mainGraph[i].y_coordinate,
				mainGraph[j].x_coordinate, mainGraph[j].y_coordinate);

			if (nearestCount < K) {
				nearestIDs[nearestCount] = j;
				nearestDists[nearestCount] = dist;
				nearestCount++;
			} else {
				// 找到当前最远的那个，如果新距离更近则替换
				int maxIdx = 0;
				for (int k = 1; k < K; k++) {
					if (nearestDists[k] > nearestDists[maxIdx]) {
						maxIdx = k;
					}
				}
				if (dist < nearestDists[maxIdx]) {
					nearestIDs[maxIdx] = j;
					nearestDists[maxIdx] = dist;
				}
			}
		}

		// 对最近邻节点添加双向边
		for (int k = 0; k < nearestCount; k++) {
			// 检查是否已存在该边（避免重复添加）
			bool exists = false;
			for (int n = 0; n < mainGraph[i].numOfNeighbour; n++) {
				if (mainGraph[i].Neighbour[n] == nearestIDs[k]) {
					exists = true;
					break;
				}
			}
			if (!exists) {
				addTwoWayEdge(i, nearestIDs[k]);
			}
		}

		delete[] nearestIDs;
		delete[] nearestDists;
	}

	// 去交叉：检测并删除交叉边
	removeCrossingEdges();

	// BFS 验证连通性，不连通则补充连边
	ensureConnectivity();

	return true;
}

void mapGraphBase::removeCrossingEdges() {
	// 线段交叉检测：遍历所有边对，检测非共享端点的交叉
	// 两条线段 (p1,p2) 和 (p3,p4) 交叉条件：跨立实验
	for (int i = 0; i < numOfEdges; i++) {
		for (int j = i + 1; j < numOfEdges; j++) {
			int a = edgeAttrs[i].fromNodeID;
			int b = edgeAttrs[i].toNodeID;
			int c = edgeAttrs[j].fromNodeID;
			int d = edgeAttrs[j].toNodeID;

			// 共享端点的边不算交叉
			if (a == c || a == d || b == c || b == d) continue;

			if (segmentsIntersect(
				mainGraph[a].x_coordinate, mainGraph[a].y_coordinate,
				mainGraph[b].x_coordinate, mainGraph[b].y_coordinate,
				mainGraph[c].x_coordinate, mainGraph[c].y_coordinate,
				mainGraph[d].x_coordinate, mainGraph[d].y_coordinate)) {
				// 删除较长的边（保留短边更有意义）
				int edgeToRemove = (edgeAttrs[i].length > edgeAttrs[j].length) ? i : j;
				removeEdge(edgeToRemove);
				// 被删除边之后的所有边索引前移，需要调整循环
				j--;
			}
		}
	}
}

void mapGraphBase::removeEdge(int edgeIndex) {
	if (edgeIndex < 0 || edgeIndex >= numOfEdges) return;

	int fromID = edgeAttrs[edgeIndex].fromNodeID;
	int toID = edgeAttrs[edgeIndex].toNodeID;

	// 从 fromNode 的邻居列表中删除 toID
	removeNeighbour(fromID, toID);

	// 从 edgeAttrs 数组中删除该边（用最后一个元素填补）
	if (edgeIndex < numOfEdges - 1) {
		edgeAttrs[edgeIndex] = edgeAttrs[numOfEdges - 1];
	}
	numOfEdges--;
}

void mapGraphBase::removeNeighbour(int nodeID, int neighbourID) {
	if (nodeID < 0 || nodeID >= numOfNodes) return;
	mapNode& node = mainGraph[nodeID];

	for (int i = 0; i < node.numOfNeighbour; i++) {
		if (node.Neighbour[i] == neighbourID) {
			// 用最后一个元素填补
			node.Neighbour[i] = node.Neighbour[node.numOfNeighbour - 1];
			node.distanceToNeighbour[i] = node.distanceToNeighbour[node.numOfNeighbour - 1];
			node.numOfNeighbour--;
			return;
		}
	}
}

bool mapGraphBase::segmentsIntersect(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4) {
	// 跨立实验：判断两线段是否相交
	auto cross = [](int ox, int oy, int ax, int ay, int bx, int by) -> long long {
		return (long long)(ax - ox) * (by - oy) - (long long)(ay - oy) * (bx - ox);
	};

	long long d1 = cross(x3, y3, x4, y4, x1, y1);
	long long d2 = cross(x3, y3, x4, y4, x2, y2);
	long long d3 = cross(x1, y1, x2, y2, x3, y3);
	long long d4 = cross(x1, y1, x2, y2, x4, y4);

	if (((d1 > 0 && d2 < 0) || (d1 < 0 && d2 > 0)) &&
		((d3 > 0 && d4 < 0) || (d3 < 0 && d4 > 0))) {
		return true;
	}
	return false;
}

void mapGraphBase::ensureConnectivity() {
	// BFS 找到所有连通分量，用最短边把分量连起来
	bool* visited = new bool[numOfNodes];
	int* component = new int[numOfNodes]; // 每个节点所属的连通分量编号
	for (int i = 0; i < numOfNodes; i++) {
		visited[i] = false;
		component[i] = -1;
	}

	int compID = 0;
	for (int start = 0; start < numOfNodes; start++) {
		if (visited[start]) continue;
		// BFS
		std::queue<int> q;
		q.push(start);
		visited[start] = true;
		component[start] = compID;

		while (!q.empty()) {
			int cur = q.front();
			q.pop();
			for (int i = 0; i < mainGraph[cur].numOfNeighbour; i++) {
				int nb = mainGraph[cur].Neighbour[i];
				if (!visited[nb]) {
					visited[nb] = true;
					component[nb] = compID;
					q.push(nb);
				}
			}
		}
		compID++;
	}

	// 如果只有一个连通分量，无需操作
	if (compID <= 1) {
		delete[] visited;
		delete[] component;
		return;
	}

	// 对每对连通分量，找到最短的跨分量边并连接
	for (int c1 = 0; c1 < compID - 1; c1++) {
		int bestDist = INT_MAX;
		int bestFrom = -1, bestTo = -1;

		for (int i = 0; i < numOfNodes; i++) {
			if (component[i] != c1) continue;
			for (int j = 0; j < numOfNodes; j++) {
				if (component[j] == c1) continue;
				int dist = calculateDistance(
					mainGraph[i].x_coordinate, mainGraph[i].y_coordinate,
					mainGraph[j].x_coordinate, mainGraph[j].y_coordinate);
				if (dist < bestDist) {
					bestDist = dist;
					bestFrom = i;
					bestTo = j;
				}
			}
		}

		if (bestFrom >= 0 && bestTo >= 0) {
			addTwoWayEdge(bestFrom, bestTo);
			// 更新 component：将 bestTo 所属分量合并到 c1
			int oldComp = component[bestTo];
			for (int i = 0; i < numOfNodes; i++) {
				if (component[i] == oldComp) component[i] = c1;
			}
		}
	}

	delete[] visited;
	delete[] component;
}

// ========== 数据验证 ==========

bool mapGraphBase::isMapFullyConnected() {
	if (numOfNodes == 0) return true;

	bool* visited = new bool[numOfNodes];
	for (int i = 0; i < numOfNodes; i++) visited[i] = false;

	std::queue<int> q;
	q.push(0);
	visited[0] = true;
	int visitedCount = 1;

	while (!q.empty()) {
		int cur = q.front();
		q.pop();
		for (int i = 0; i < mainGraph[cur].numOfNeighbour; i++) {
			int nb = mainGraph[cur].Neighbour[i];
			if (!visited[nb]) {
				visited[nb] = true;
				visitedCount++;
				q.push(nb);
			}
		}
	}

	delete[] visited;
	return visitedCount == numOfNodes;
}

bool mapGraphBase::isDataValid() {
	// 检查节点数据合法性
	for (int i = 0; i < numOfNodes; i++) {
		if (mainGraph[i].nodeID != i) return false;
		// 检查邻居 ID 合法性
		for (int j = 0; j < mainGraph[i].numOfNeighbour; j++) {
			int nb = mainGraph[i].Neighbour[j];
			if (nb < 0 || nb >= numOfNodes) return false;
			if (mainGraph[i].distanceToNeighbour[j] < 0) return false;
		}
	}

	// 检查边数据合法性
	for (int i = 0; i < numOfEdges; i++) {
		if (edgeAttrs[i].fromNodeID < 0 || edgeAttrs[i].fromNodeID >= numOfNodes) return false;
		if (edgeAttrs[i].toNodeID < 0 || edgeAttrs[i].toNodeID >= numOfNodes) return false;
		if (edgeAttrs[i].length < 0) return false;
		if (edgeAttrs[i].capacity < 0) return false;
	}

	return true;
}
