#ifndef GUI_H
#define GUI_H

#include "dataStructure.h"
#include <string>
#include <vector>
#include <map>
#include <ctime>

// ========== 坐标转换 ==========
struct WorldPoint {
    double x;
    double y;
};

struct ScreenPoint {
    int x;
    int y;
};

// ========== 视图变换 ==========
struct ViewTransform {
    double scale;
    int offsetX;
    int offsetY;
    int clientW;
    int clientH;

    ScreenPoint worldToScreen(double wx, double wy) const;
    WorldPoint screenToWorld(int sx, int sy) const;
    void fitToMap(int mapMinX, int mapMinY, int mapMaxX, int mapMaxY, int cw, int ch);
    void pan(int dx, int dy);
    void zoomAt(int sx, int sy, double factor);
    void getVisibleRange(double& visMinX, double& visMaxX,
                         double& visMinY, double& visMaxY) const;
};

// ========== 车流模拟器 ==========
class TrafficSimulator {
public:
    struct Car {
        int edgeIndex;
        double progress;
        int fromNode;
        int toNode;
    };

    struct EdgeState {
        double currentCars;
        double capacity;
        double length;
    };

private:
    mapGraphBase* m_graph;
    std::vector<Car> m_cars;
    std::vector<EdgeState> m_edgeStates;
    int m_numEdges;
    double m_spawnTimer;
    bool m_running;

public:
    TrafficSimulator();
    void init(mapGraphBase* graph);
    void start();
    void stop();
    void pause();
    void resume();
    bool isRunning() const { return m_running; }
    void update(double dt);
    void syncToGraph();  // 将 m_edgeStates 写回图的 edgeAttrs，供寻路使用
    double getEdgeLoadRatio(int edgeIndex) const;
    int getTotalCars() const { return (int)m_cars.size(); }
    const std::vector<Car>& getCars() const { return m_cars; }
    double getEdgeCars(int edgeIndex) const;

private:
    void spawnCarBatch(int count);
    void removeCar(int carIdx);
    int findNextEdge(const Car& car);
};

// ========== 参数输入对话框（raylib 绘制） ==========
struct TextField {
    char buffer[32];
    int cursorPos;
    int len;
    bool active;
};

struct InputDialog {
    bool active;
    char title[64];
    char labels[4][32];
    TextField fields[4];
    int fieldCount;
    int resultValues[4];
    bool confirmed;

    InputDialog() : active(false), fieldCount(0), confirmed(false) {
        for (int i = 0; i < 4; i++) {
            fields[i].buffer[0] = '\0';
            fields[i].cursorPos = 0;
            fields[i].len = 0;
            fields[i].active = false;
            resultValues[i] = 0;
            labels[i][0] = '\0';
        }
        title[0] = '\0';
    }
};

// ========== MapGUI ==========
class MapGUI {
public:
    enum {
        BTN_OPEN = 1, BTN_RANDOM, BTN_GRID,
        BTN_TRAFFIC, BTN_FIND_PATH, BTN_ASTAR,
        BTN_CLEAR, BTN_RESET_VIEW
    };

    struct Button {
        int id;
        const char* label;
        bool mouseOver;
    };

    struct PathCompareResult {
        std::string dijkstraText;
        std::string aStarText;
        int dijkstraNodes;
        int aStarNodes;
        double dijkstraMs;
        double aStarMs;
        bool hasResult;
        PathCompareResult() : dijkstraNodes(0), aStarNodes(0), dijkstraMs(0), aStarMs(0), hasResult(false) {}
    };

    MapGUI();
    ~MapGUI();

    void run();

private:
    // 绘制
    void drawToolbar();
    void drawEdges();
    void drawNodes();
    void drawPath();
    void drawSelectedNode(int nodeId);
    void drawF1Highlight();
    void drawCars();
    void drawTrafficLegend();
    void drawStatusBar();
    void drawGrid();

    // 交互
    void handleInput();
    int isButtonClicked(int x, int y);
    void onButtonClick(int btnId);
    void autoFit();

    // 地图加载
    bool loadMap(const char* jsonPath);
    bool loadRandomMap(int width, int height, int nodes);
    bool loadGridMap(int width, int height, int cols, int rows);

    // 路径
    void setPath(const std::vector<int>& path);
    void clearPath();
    void setPathStart(int nodeId);
    void setPathEnd(int nodeId);
    int findNearestNode(double wx, double wy);
    void findAndHighlightNearest100(double cx, double cy);
    bool isNodeF1Highlighted(int nodeId) const;
    bool isEdgeF1Related(int fromId, int toId) const;
    void setStatusText(const std::string& text);

    // 输入对话框
    void showInputDialog(const char* title, const char* labels[], int defaults[], int fieldCount);
    void drawInputDialog();
    void handleInputDialogInput();
    void execInputDialog();

    // 寻路对比（提取公共逻辑）
    void runPathfindingCompare(bool showDijkstra);

    // 布局
    static const int TOOLBAR_Y = 48;
    static const int TOOLBAR_H = 40;
    static const int BTN_W = 105;
    static const int BTN_H = 28;
    static const int BTN_MARGIN = 5;

    // 数据
    mapGraphBase m_graph;
    TrafficSimulator m_traffic;
    ViewTransform m_transform;

    // 路径
    std::vector<int> m_path;
    int m_pathStart;
    int m_pathEnd;

    // 鼠标
    bool m_dragging;
    int m_dragStartX, m_dragStartY;
    int m_hoverNode;
    int m_lastMouseX, m_lastMouseY;

    // F1
    bool m_f1Active;
    std::vector<int> m_f1Nodes;

    // UI
    std::string m_statusText;
    bool m_showTrafficLegend;
    bool m_useCongestion;
    PathCompareResult m_compareResult;

    std::vector<Button> m_buttons;

    InputDialog m_inputDlg;

    int m_screenW, m_screenH;
};

#endif // GUI_H
