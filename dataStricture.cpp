#include"dataStructure.h"
#include<fstream>
#include<queue>
#include<cstring>
#include<string>
#include<cmath>
#include<stdexcept>

mapGraphBase::mapGraphBase(int nodes, int edges,mapNode* graph = nullptr) {
	numOfNodes = 0;
	numOfEdges = 0;
	mainGraph = nullptr;
	edgeAttrs = nullptr;
	nextEdges = 0;
	mapMinX = 0;
	mapMaxX = 10000;
	mapMinY = 0;
	mapMaxY = 10000;
	memset(cache, 0, sizeof(cache));
}

mapGraphBase::~mapGraphBase() {
	clearMap();
}

int mapGraphBase::addNode(int x,int y) {
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

int mapGraphBase::calculateDistance(int x1, int y1, int x2, int y2) {
	double dx = x2 - x1;
	double dy = y2 - y1;
	return (int)std::sqrt(dx * dx + dy * dy);
}
