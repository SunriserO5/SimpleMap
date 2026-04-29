
#include "dataStructure.h"
#include "pathfinding.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

static void clearInputState() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

static int readIntWithRetry(const char* prompt, int minValue, const char* ruleHint) {
    while (true) {
        int value = 0;
        std::cout << prompt;
        if (std::cin >> value) {
            if (value >= minValue) {
                return value;
            }
            std::cout << "[Error] 输入值不合法：" << ruleHint << std::endl;
        }
        else {
            std::cout << "[Error] 请输入整数。" << std::endl;
        }
        clearInputState();
        std::cout << "请重新输入。" << std::endl;
    }
}

static int readIntInRangeWithRetry(const char* prompt, int minValue, int maxValue) {
    while (true) {
        int value = 0;
        std::cout << prompt;
        if (std::cin >> value) {
            if (value >= minValue && value <= maxValue) {
                return value;
            }
            std::cout << "[Error] 输入值不合法：请输入范围 ["
                << minValue << ", " << maxValue << "] 内的整数。" << std::endl;
        }
        else {
            std::cout << "[Error] 请输入整数。" << std::endl;
        }
        clearInputState();
        std::cout << "请重新输入。" << std::endl;
    }
}

static std::string readLineWithPrompt(const char* prompt) {
    std::cout << prompt;
    std::string line;
    std::getline(std::cin >> std::ws, line);
    return line;
}

static double computePathLength(mapGraphBase& graph, const std::vector<int>& path) {
    if (path.size() < 2) {
        return 0.0;
    }

    double total = 0.0;
    for (size_t i = 1; i < path.size(); i++) {
        EdgeAttr* edge = graph.getEdgeByNodes(path[i - 1], path[i]);
        if (edge == nullptr) {
            return -1.0;
        }
        total += edge->length;
    }
    return total;
}

static void printMapSummary(mapGraphBase& graph) {
    std::cout << "[Map] nodes=" << graph.getNumOfNodes()
        << ", edges=" << graph.getNumOfEdges() << std::endl;
    std::cout << "[Map] connected=" << (graph.isMapFullyConnected() ? "true" : "false")
        << ", dataValid=" << (graph.isDataValid() ? "true" : "false") << std::endl;
}

static void printPathDetail(mapGraphBase& graph, const std::vector<int>& path) {
    if (path.empty()) {
        std::cout << "[Path] 未找到可达路径。" << std::endl;
        return;
    }

    std::cout << "[PathDetail] index  nodeID  segmentDist  cumulativeDist" << std::endl;
    double cumulative = 0.0;
    std::cout << "[PathDetail] "
        << std::setw(5) << 0 << "  "
        << std::setw(6) << path[0] << "  "
        << std::setw(11) << 0.0 << "  "
        << std::setw(14) << cumulative << std::endl;

    for (size_t i = 1; i < path.size(); i++) {
        EdgeAttr* edge = graph.getEdgeByNodes(path[i - 1], path[i]);
        double segment = (edge == nullptr) ? -1.0 : edge->length;
        if (segment >= 0.0) {
            cumulative += segment;
        }
        std::cout << "[PathDetail] "
            << std::setw(5) << i << "  "
            << std::setw(6) << path[i] << "  "
            << std::setw(11) << segment << "  "
            << std::setw(14) << cumulative << std::endl;
    }
}

static void runPathAlgorithmTest(mapGraphBase& graph, bool useAStar) {
    int nodeCount = graph.getNumOfNodes();
    if (nodeCount < 2) {
        std::cout << "[Error] 当前地图节点不足，无法进行路径测试。" << std::endl;
        return;
    }

    std::cout << "[Info] 节点 ID 有效范围: 0 ~ " << (nodeCount - 1) << std::endl;
    int startID = readIntInRangeWithRetry("请输入 startID: ", 0, nodeCount - 1);
    int endID = readIntInRangeWithRetry("请输入 endID: ", 0, nodeCount - 1);

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    std::vector<int> path = useAStar
        ? PathFinder::aStar(&graph, startID, endID, false)
        : PathFinder::dijkstra(&graph, startID, endID, false);
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

    double elapsedMs = std::chrono::duration<double, std::milli>(end - begin).count();
    double totalLength = computePathLength(graph, path);

    std::cout << "[Path] algorithm=" << (useAStar ? "A*" : "Dijkstra")
        << ", startID=" << startID
        << ", endID=" << endID
        << ", nodes=" << path.size()
        << ", length=" << totalLength
        << ", timeMs=" << elapsedMs << std::endl;

    printPathDetail(graph, path);

    std::chrono::steady_clock::time_point cacheBegin1 = std::chrono::steady_clock::now();
    int cachedLength1 = graph.getPath(startID, endID);
    std::chrono::steady_clock::time_point cacheEnd1 = std::chrono::steady_clock::now();

    std::chrono::steady_clock::time_point cacheBegin2 = std::chrono::steady_clock::now();
    int cachedLength2 = graph.getPath(startID, endID);
    std::chrono::steady_clock::time_point cacheEnd2 = std::chrono::steady_clock::now();

    double cacheMs1 = std::chrono::duration<double, std::milli>(cacheEnd1 - cacheBegin1).count();
    double cacheMs2 = std::chrono::duration<double, std::milli>(cacheEnd2 - cacheBegin2).count();

    std::cout << "[Cache] getPath firstCallLength=" << cachedLength1
        << ", secondCallLength=" << cachedLength2
        << ", firstCallMs=" << cacheMs1
        << ", secondCallMs=" << cacheMs2 << std::endl;
}

static void runPathTestSubMenu(mapGraphBase& graph) {
    while (true) {
        std::cout << std::endl;
        std::cout << "--- 路径测试子菜单 ---" << std::endl;
        std::cout << "1. 测试 Dijkstra" << std::endl;
        std::cout << "2. 测试 A*" << std::endl;
        std::cout << "3. 返回上一级" << std::endl;

        int choice = readIntInRangeWithRetry("输入选项编号 (1/2/3): ", 1, 3);
        if (choice == 1) {
            runPathAlgorithmTest(graph, false);
        }
        else if (choice == 2) {
            runPathAlgorithmTest(graph, true);
        }
        else {
            break;
        }
    }
}

static bool runGenerateRandomMap(mapGraphBase& graph) {
    std::cout << "\n[Hint] generateRandomMap 参数说明:" << std::endl;
    std::cout << "  1) mapWidth       地图宽度（正整数，建议 1200，示例 1200）" << std::endl;
    std::cout << "  2) mapHeight      地图高度（正整数，建议 800，示例 800）" << std::endl;
    std::cout << "  3) totalNodeCount 节点数量（>=2，建议 300~1000，示例 300）" << std::endl;

    int mapWidth = readIntWithRetry("请输入 mapWidth: ", 1, "mapWidth 必须是正整数");
    int mapHeight = readIntWithRetry("请输入 mapHeight: ", 1, "mapHeight 必须是正整数");
    int totalNodeCount = readIntWithRetry("请输入 totalNodeCount: ", 2, "totalNodeCount 必须 >= 2");

    std::cout << "[Info] 已接收参数: width=" << mapWidth
        << ", height=" << mapHeight
        << ", nodes=" << totalNodeCount << std::endl;

    if (!graph.generateRandomMap(mapWidth, mapHeight, totalNodeCount)) {
        std::cout << "[Error] generateRandomMap 执行失败。" << std::endl;
        return false;
    }
    return true;
}

int main() {
    mapGraphBase graph;

    std::cout << "============================" << std::endl;
    std::cout << "      SimpleMap CLI" << std::endl;
    std::cout << "============================" << std::endl;

    while (true) {
        std::cout << std::endl;
        std::cout << "请选择操作:" << std::endl;
        std::cout << "1. 导入已有地图数据" << std::endl;
        std::cout << "2. 生成随机地图并测试" << std::endl;
        std::cout << "3. 退出地图" << std::endl;
        std::cout << "输入选项编号 (1/2/3): ";

        int choice = 0;
        if (!(std::cin >> choice)) {
            clearInputState();
            std::cout << "[Error] 请输入数字选项。" << std::endl;
            continue;
        }

        if (choice == 1) {
            std::string filePath = readLineWithPrompt("请输入地图 JSON 文件路径 (示例: sample_map.json): ");
            if (filePath.empty()) {
                std::cout << "[Error] 文件路径不能为空。" << std::endl;
                continue;
            }

            if (!graph.loadMapFromFile(filePath.c_str())) {
                std::cout << "[Error] 地图导入失败，请检查文件路径和格式。" << std::endl;
                continue;
            }

            std::cout << "[Info] 地图导入成功。" << std::endl;
            printMapSummary(graph);
            runPathTestSubMenu(graph);
        }
        else if (choice == 2) {
            if (runGenerateRandomMap(graph)) {
                std::cout << "[Info] 随机地图生成成功。" << std::endl;
                printMapSummary(graph);

                // ---------- 新增：保存地图 ----------
                std::string savePath = readLineWithPrompt("请输入保存路径 (示例: my_map.json，留空跳过): ");
                if (!savePath.empty()) {
                    if (graph.saveMapToFile(savePath.c_str())) {
                        std::cout << "[Info] 地图已保存到: " << savePath << std::endl;
                    }
                    else {
                        std::cout << "[Error] 保存失败。" << std::endl;
                    }
                }
                // ------------------------------------
                runPathTestSubMenu(graph);
            }
        }
        else if (choice == 3) {
            std::cout << "[Info] 已退出地图。" << std::endl;
            break;
        }
        else {
            std::cout << "[Error] 无效选项，请输入 1/2/3。" << std::endl;
        }
    }

    return 0;
}