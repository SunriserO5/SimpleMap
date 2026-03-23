#ifndef DATASTRUCTURE_H
#define DATASTRUCTURE_H
#include "parameters.h"

struct mapNode
{
    int nodeID;
    int x_coordinate;
    int y_coordinate;
    int numOfNeighbour;
    int* Neighbour;
    int* distanceToNeighbour;
    mapNode(int id=-1,int x=-1,int y=-1,int neighbours=0){
        nodeID=id;
        x_coordinate=x;
        y_coordinate=y;
        numOfNeighbour=neighbours;
        Neighbour=new int[neighbours];
        distanceToNeighbour=new int[neighbours];
    }
};

struct cacheNode{
    int beginNodeID;
    int endNodeID;
    int distance;
    int* path;
    cacheNode(int begin=-1,int end=-1,int dis=-1,int* p=nullptr){
        beginNodeID=begin;
        endNodeID=end;
        distance=dis;
        path=p;
}
};
//边及其属性
struct EdgeAttr {
    int edgeID;//边的编号
    int fromNodeID;//边的起点
    int toNodeID;//边的终点
    double length;//边的长度（计算用？）
    double speedlimit;//限速
    double capacity;//路的容量
    double congestion;//拥堵程度
    //构造函数：初始化边，默认限速 10.0，容量为长度*0.8，拥堵为 0.0
    EdgeAttr(int id = -1, int from = -1, int to = -1, double len = 0) {
        edgeID = id;
        fromNodeID = from;
        toNodeID = to;
        length = len;
        speedLimit = 10.0;
        capacity = len * 0.8;
        congestion = 0.0;
    }
};

class mapGraphBase
{
private:
    int numOfNodes;
    int numOfEdges;
    int m_allocatedNodeCount;// 【新增】记录分配的节点数组容量
    mapNode* mainGraph;
    EdgeAttr* edgeAttrs;
    int nextEdgeID;//下一个新边ID（用于自动编号？）
    int mapMinX, mapMaxX;    // 地图的 x 坐标范围（最小/最大 x）
    int mapMinY, mapMaxY;    // 地图的 y 坐标范围（最小/最大 y）
    cacheNode* cache=new cacheNode[CACHE_SIZE];
public:
    mapGraphBase(int nodes = 0, int edges = 0, mapNode* graph = nullptr);
    ~mapGraphBase();
    /*不确定要不要纯虚函数先放这吧
    virtual int getNumOfNodes() = 0;
    virtual int getNumOfEdges()=0;
    virtual mapNode* getMainGraph()=0;
    virtual int* getCoordinate(int nodeID)=0;
    mapGraphBase(int nodes=0,int edges=0,mapNode* graph=nullptr);
    ~mapGraphBase();
    virtual int getPath(int nodeID1,int nodeID2)=0;*/
    int getNumOfNodes();
    int getNumOfEdges();
    mapNode* getMainGraph();
    int* getCoordinate(int nodeID);
    int getPath(int nodeID1, int nodeID2);
    mapGraphBase(int nodes = 0, int edges = 0, mapNode* graph = nullptr);
    ~mapGraphBase();
    //地图生成
    bool generateGridMap(int mapWidth, int mapHeight, int nodeColCount, int nodeRowCount);//生成网格状地图
    bool generateRandomMap(int mapWidth, int mapHeight, int totalNodeCount);//生成随机地图

    //数据管理
    mapNode* getNodeById(int nodeID);//ID访问点
    EdgeAttr* getEdgeById(int edgeID);//ID访问边
    EdgeAttr* getEdgeByNodes(int fromID, int toID);//起点终点访问边
    EdgeAttr* getAllEdgeAttrs();
    bool updateEdgeCongestion(int edgeID, double newCongestion);//更新路的拥堵程度

    //数据持久化
    /*不知道要不要用json先放这
   
    bool saveMapToFile(const char* filePath);
    bool loadMapFromFile(const char* filePath);
    */

    // 数据验证
    bool isMapFullyConnected();//检查地图是否全连通（是否所有点都能互相到达）
    bool isDataValid();//检查地图数据是否合法（比如边的起点终点是否存在）

    // 辅助功能
    void clearMap();
private:
    int addNode(int x, int y);// 内部添加一个节点（返回新节点 ID）
    bool addTwoWayEdge(int fromID, int toID);   // 添加双向边（A→B 和 B→A 都加）
    bool addOneWayEdge(int fromID, int toID);   // 添加单向边（只加 A→B）
    int calculateDistance(int x1, int y1, int x2, int y2); // 计算两点之间的距离
    void allocateMainGraphMemory(int nodeCount); // 分配节点数组内存
    void freeMainGraphMemory();                  // 释放节点数组内存

};

#endif // DATASTRUCTURE_H