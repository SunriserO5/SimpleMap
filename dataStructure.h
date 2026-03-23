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

class mapGraphBase
{
    int numOfNodes;
    int numOfEdges;
    mapNode* mainGraph;
    cacheNode* cache=new cacheNode[CACHE_SIZE];
public:
    virtual int getNumOfNodes()=0;
    virtual int getNumOfEdges()=0;
    virtual mapNode* getMainGraph()=0;
    virtual int* getCoordinate(int nodeID)=0;
    mapGraphBase(int nodes=0,int edges=0,mapNode* graph=nullptr);
    ~mapGraphBase();
    virtual int getPath(int nodeID1,int nodeID2)=0;

};

#endif // DATASTRUCTURE_H