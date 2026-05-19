#define _CRT_SECURE_NO_WARNINGS
#include "gui.h"
#include "pathfinding.h"
#include "raylib.h"
#include "filedialog.h"

#define MAX_PATH 260
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// ========== 工具栏按钮定义 ==========

static MapGUI::Button g_defaultButtons[] = {
	{MapGUI::BTN_OPEN,       "Open",    false},
	{MapGUI::BTN_RANDOM,     "Random",  false},
	{MapGUI::BTN_GRID,       "Grid",    false},
	{MapGUI::BTN_TRAFFIC,    "Traffic", false},
	{MapGUI::BTN_FIND_PATH,  "Dijkstra",false},
	{MapGUI::BTN_ASTAR,      "A*",      false},
	{MapGUI::BTN_CLEAR,      "Clear",   false},
	{MapGUI::BTN_RESET_VIEW, "Reset",   false},
};

static Color TrafficColor(double ratio) {
	ratio = std::max(0.0, std::min(1.5, ratio));
	int r, g;
	if (ratio <= 0.5) {
		double t = ratio / 0.5;
		r = (int)(255 * t);
		g = 255;
	} else {
		double t = (ratio - 0.5) / 0.5;
		r = 255;
		g = (int)(255 * (1.0 - t));
	}
	return {(unsigned char)r, (unsigned char)g, 0, 255};
}

// ========== ViewTransform ==========

ScreenPoint ViewTransform::worldToScreen(double wx, double wy) const {
	return {(int)(wx * scale + offsetX), (int)(clientH - wy * scale + offsetY)};
}

WorldPoint ViewTransform::screenToWorld(int sx, int sy) const {
	return {(sx - offsetX) / scale, (clientH - sy + offsetY) / scale};
}

void ViewTransform::fitToMap(int mnX, int mnY, int mxX, int mxY, int cw, int ch) {
	clientW = cw;
	clientH = ch;
	if (mxX <= mnX || mxY <= mnY) {
		scale = 1.0;
		offsetX = cw / 2;
		offsetY = 0;
		return;
	}
	double s = std::min((double)cw / (mxX - mnX), (double)ch / (mxY - mnY));
	scale = s;
	offsetX = (int)(cw * 0.5 - (mnX + mxX) * 0.5 * s);
	offsetY = (int)(ch * 0.5 - (clientH - (mnY + mxY) * 0.5 * s));
}

void ViewTransform::pan(int dx, int dy) {
	offsetX += dx;
	offsetY += dy;
}

void ViewTransform::zoomAt(int sx, int sy, double factor) {
	if (factor <= 0) return;
	double ns = scale * factor;
	if (ns < 0.05 || ns > 200.0) return;
	auto wp = screenToWorld(sx, sy);
	scale = ns;
	offsetX = sx - (int)(wp.x * scale);
	offsetY = sy - (int)(clientH - wp.y * scale);
}

void ViewTransform::getVisibleRange(double& a, double& b, double& c, double& d) const {
	auto lt = screenToWorld(0, 0);
	auto rb = screenToWorld(clientW, clientH);
	a = lt.x;
	b = rb.x;
	c = rb.y;
	d = lt.y;
}

// ========== TrafficSimulator ==========

TrafficSimulator::TrafficSimulator()
	: m_graph(nullptr), m_numEdges(0), m_spawnTimer(0), m_running(false) {
}

void TrafficSimulator::init(mapGraphBase* g) {
	m_graph = g;
	m_cars.clear();
	m_edgeStates.clear();
	m_running = false;
	m_spawnTimer = 0;
	m_numEdges = g->getNumOfEdges();
	auto* e = g->getAllEdgeAttrs();
	m_edgeStates.resize(m_numEdges);
	for (int i = 0; i < m_numEdges; i++) {
		m_edgeStates[i] = {e[i].currentCars, e[i].capacity, e[i].length};
	}
}

void TrafficSimulator::start() {
	m_running = true;
	m_spawnTimer = 0;
}

void TrafficSimulator::stop() {
	m_running = false;
}

void TrafficSimulator::pause() {
	m_running = false;
}

void TrafficSimulator::resume() {
	m_running = true;
}

double TrafficSimulator::getEdgeLoadRatio(int i) const {
	if (i < 0 || i >= m_numEdges) return 0.0;
	if (m_edgeStates[i].capacity <= 0) return 0.0;
	// 容量缩小10倍让少量车也能显示颜色，车流模拟也用相同比例
	double visualCapacity = std::max(1.0, m_edgeStates[i].capacity / 10.0);
	return m_edgeStates[i].currentCars / visualCapacity;
}

void TrafficSimulator::syncToGraph() {
	if (!m_graph) return;
	EdgeAttr* edges = m_graph->getAllEdgeAttrs();
	for (int i = 0; i < m_numEdges && i < m_graph->getNumOfEdges(); i++) {
		edges[i].currentCars = m_edgeStates[i].currentCars;
	}
}

double TrafficSimulator::getEdgeCars(int i) const {
	if (i < 0 || i >= m_numEdges) return 0.0;
	return m_edgeStates[i].currentCars;
}

void TrafficSimulator::update(double dt) {
	if (!m_running || !m_graph) return;

	// 生成车辆：每次1辆，生成频率0.05秒
	m_spawnTimer += dt;
	if (m_spawnTimer > 0.05) {
		m_spawnTimer = 0.0;
		spawnCarBatch(1);
	}

	// 更新每辆车的位置
	for (int i = 0; i < (int)m_cars.size(); /* no ++ */) {
		auto& c = m_cars[i];
		auto& es = m_edgeStates[c.edgeIndex];

		if (es.length <= 0) {
			es.currentCars = std::max(0.0, es.currentCars - 1);
			m_cars.erase(m_cars.begin() + i);
			continue;
		}

		// 容量缩小10倍，少量车即可触发拥堵
		double effCapacity = std::max(1.0, es.capacity / 10.0);
		double ratio = es.currentCars / effCapacity;
		double f = (ratio <= 0.7) ? 1.0 : (1.0 + exp(ratio - 0.7));

		// 通行时间 T = c * L * f，c=0.1 使车速减慢约10倍，路段停留约10秒
		const double TRAFFIC_C = 0.1;
		double travelTime = TRAFFIC_C * es.length * f;
		double speed = (travelTime > 0) ? (es.length / travelTime) : es.length;

		c.progress += (speed / es.length) * dt;

		if (c.progress >= 1.0) {
			int nxt = findNextEdge(c);
			es.currentCars = std::max(0.0, es.currentCars - 1);

			if (nxt >= 0) {
				c.edgeIndex = nxt;
				m_edgeStates[nxt].currentCars += 1;
				c.progress = 0;
				auto* ne = m_graph->getEdgeById(nxt);
				if (ne) {
					c.fromNode = ne->fromNodeID;
					c.toNode = ne->toNodeID;
				}
				i++;
			} else {
				m_cars.erase(m_cars.begin() + i);
			}
		} else {
			i++;
		}
	}
}

void TrafficSimulator::spawnCarBatch(int count) {
	if (!m_graph || m_numEdges == 0) return;

	// 找一条未满载的路段，放一批车上去
	int ei = rand() % m_numEdges;
	auto* e = m_graph->getAllEdgeAttrs();

	for (int k = 0; k < count; k++) {
		if (m_edgeStates[ei].currentCars >= m_edgeStates[ei].capacity * 1.5) {
			break;
		}
		m_cars.push_back({ei, 0.0, e[ei].fromNodeID, e[ei].toNodeID});
		m_edgeStates[ei].currentCars += 1;
	}
}

void TrafficSimulator::removeCar(int i) {
	if (i < 0 || i >= (int)m_cars.size()) return;
	m_edgeStates[m_cars[i].edgeIndex].currentCars = std::max(0.0, m_edgeStates[m_cars[i].edgeIndex].currentCars - 1);
	m_cars.erase(m_cars.begin() + i);
}

int TrafficSimulator::findNextEdge(const Car& c) {
	auto* n = m_graph->getNodeById(c.toNode);
	if (!n || n->numOfNeighbour == 0) return -1;

	std::vector<int> cand;
	for (int i = 0; i < n->numOfNeighbour; i++) {
		if (n->Neighbour[i] != c.fromNode) {
			cand.push_back(i);
		}
	}

	auto* e = m_graph->getAllEdgeAttrs();

	if (cand.empty()) {
		// 只能原路返回
		for (int i = 0; i < n->numOfNeighbour; i++) {
			for (int j = 0; j < m_numEdges; j++) {
				if (e[j].fromNodeID == c.toNode && e[j].toNodeID == n->Neighbour[i]) {
					return j;
				}
			}
		}
		return -1;
	}

	int nb = n->Neighbour[cand[rand() % cand.size()]];
	for (int j = 0; j < m_numEdges; j++) {
		if (e[j].fromNodeID == c.toNode && e[j].toNodeID == nb) {
			return j;
		}
	}
	return -1;
}

// ========== MapGUI ==========

MapGUI::MapGUI()
	: m_pathStart(-1), m_pathEnd(-1), m_dragging(false),
	  m_dragStartX(0), m_dragStartY(0),
	  m_hoverNode(-1), m_lastMouseX(0), m_lastMouseY(0),
	  m_f1Active(false),
	  m_showTrafficLegend(true), m_useCongestion(false),
	  m_screenW(0), m_screenH(0) {
	for (auto& b : g_defaultButtons) {
		m_buttons.push_back(b);
	}
	m_transform.scale = 1.0; // 避免 drawGrid 无限循环
	srand((unsigned)time(nullptr));
}

MapGUI::~MapGUI() {}

void MapGUI::setStatusText(const std::string& t) {
	m_statusText = t;
}

bool MapGUI::loadMap(const char* p) {
	bool ok = m_graph.loadMapFromFile(p);
	if (ok) {
		m_traffic.init(&m_graph);
		autoFit();
	}
	return ok;
}

bool MapGUI::loadRandomMap(int w, int h, int n) {
	bool ok = m_graph.generateRandomMap(w, h, n);
	if (ok) {
		m_traffic.init(&m_graph);
		autoFit();
	}
	return ok;
}

bool MapGUI::loadGridMap(int w, int h, int c, int r) {
	bool ok = m_graph.generateGridMap(w, h, c, r);
	if (ok) {
		m_traffic.init(&m_graph);
		autoFit();
	}
	return ok;
}

void MapGUI::setPath(const std::vector<int>& p) {
	m_path = p;
}

void MapGUI::clearPath() {
	m_path.clear();
}

void MapGUI::setPathStart(int n) {
	m_pathStart = n;
}

void MapGUI::setPathEnd(int n) {
	m_pathEnd = n;
}

int MapGUI::findNearestNode(double wx, double wy) {
	int n = m_graph.getNumOfNodes();
	if (!n) return -1;
	auto* nodes = m_graph.getMainGraph();
	int best = -1;
	double bd = 1e18;
	for (int i = 0; i < n; i++) {
		double dx = nodes[i].x_coordinate - wx;
		double dy = nodes[i].y_coordinate - wy;
		double d = dx * dx + dy * dy;
		if (d < bd) {
			bd = d;
			best = i;
		}
	}
	return best;
}

void MapGUI::findAndHighlightNearest100(double cx, double cy) {
	int n = m_graph.getNumOfNodes();
	if (!n) return;
	auto* nodes = m_graph.getMainGraph();

	struct D {
		int id;
		double d;
	};
	std::vector<D> ds;
	ds.reserve(n);
	for (int i = 0; i < n; i++) {
		double dx = nodes[i].x_coordinate - cx;
		double dy = nodes[i].y_coordinate - cy;
		ds.push_back({i, sqrt(dx * dx + dy * dy)});
	}
	std::sort(ds.begin(), ds.end(), [](const D& a, const D& b) {
		return a.d < b.d;
	});

	int cnt = (n < 100) ? n : 100;
	m_f1Nodes.clear();
	for (int i = 0; i < cnt; i++) {
		m_f1Nodes.push_back(ds[i].id);
	}
	m_f1Active = true;

	if (cnt > 0) {
		int mnX = nodes[m_f1Nodes[0]].x_coordinate;
		int mxX = mnX;
		int mnY = nodes[m_f1Nodes[0]].y_coordinate;
		int mxY = mnY;
		for (int i = 1; i < cnt; i++) {
			int id = m_f1Nodes[i];
			if (nodes[id].x_coordinate < mnX) mnX = nodes[id].x_coordinate;
			if (nodes[id].x_coordinate > mxX) mxX = nodes[id].x_coordinate;
			if (nodes[id].y_coordinate < mnY) mnY = nodes[id].y_coordinate;
			if (nodes[id].y_coordinate > mxY) mxY = nodes[id].y_coordinate;
		}
		m_transform.fitToMap(mnX - 50, mnY - 50, mxX + 50, mxY + 50, m_screenW, m_screenH);
	}

	char buf[128];
	snprintf(buf, 128, "F1: nearest %d nodes @(%.0f,%.0f) | ESC=cancel", cnt, cx, cy);
	setStatusText(buf);
}

bool MapGUI::isNodeF1Highlighted(int id) const {
	if (!m_f1Active) return false;
	for (int n : m_f1Nodes) {
		if (n == id) return true;
	}
	return false;
}

bool MapGUI::isEdgeF1Related(int f, int t) const {
	return m_f1Active && isNodeF1Highlighted(f) && isNodeF1Highlighted(t);
}

void MapGUI::autoFit() {
	int n = m_graph.getNumOfNodes();
	if (!n) return;
	auto* nodes = m_graph.getMainGraph();

	int mnX = nodes[0].x_coordinate;
	int mxX = mnX;
	int mnY = nodes[0].y_coordinate;
	int mxY = mnY;
	for (int i = 1; i < n; i++) {
		if (nodes[i].x_coordinate < mnX) mnX = nodes[i].x_coordinate;
		if (nodes[i].x_coordinate > mxX) mxX = nodes[i].x_coordinate;
		if (nodes[i].y_coordinate < mnY) mnY = nodes[i].y_coordinate;
		if (nodes[i].y_coordinate > mxY) mxY = nodes[i].y_coordinate;
	}
	m_transform.fitToMap(mnX - 20, mnY - 20, mxX + 20, mxY + 20, m_screenW, m_screenH);
}

// ========== 寻路对比 ==========

void MapGUI::runPathfindingCompare(bool showDijkstra) {
	if (m_pathStart < 0 || m_pathEnd < 0) {
		setStatusText("Click to set start, Shift+click to set end");
		return;
	}

	auto t1 = std::chrono::steady_clock::now();
	auto dPath = PathFinder::dijkstra(&m_graph, m_pathStart, m_pathEnd, m_useCongestion);
	auto t2 = std::chrono::steady_clock::now();
	double dMs = std::chrono::duration<double, std::milli>(t2 - t1).count();

	auto t3 = std::chrono::steady_clock::now();
	auto aPath = PathFinder::aStar(&m_graph, m_pathStart, m_pathEnd, m_useCongestion);
	auto t4 = std::chrono::steady_clock::now();
	double aMs = std::chrono::duration<double, std::milli>(t4 - t3).count();

	if (showDijkstra) setPath(dPath);
	else              setPath(aPath);

	double dL = 0, dT = 0;
	for (size_t i = 1; i < dPath.size(); i++) {
		auto* e = m_graph.getEdgeByNodes(dPath[i - 1], dPath[i]);
		if (e) {
			dL += e->length;
			dT += PathFinder::getTravelTime(&m_graph, dPath[i - 1], dPath[i], m_useCongestion);
		}
	}

	double aL = 0, aT = 0;
	for (size_t i = 1; i < aPath.size(); i++) {
		auto* e = m_graph.getEdgeByNodes(aPath[i - 1], aPath[i]);
		if (e) {
			aL += e->length;
			aT += PathFinder::getTravelTime(&m_graph, aPath[i - 1], aPath[i], m_useCongestion);
		}
	}

	m_compareResult.hasResult = true;
	m_compareResult.dijkstraNodes = (int)dPath.size();
	 m_compareResult.aStarNodes = (int)aPath.size();
	m_compareResult.dijkstraMs = dMs;
	m_compareResult.aStarMs = aMs;

	char dB[256], aB[256];
	snprintf(dB, 256, "Dijkstra: %d nodes | len %.0f | time %.2f | cpu %.2fms",
		(int)dPath.size(), dL, dT, dMs);
	snprintf(aB, 256, "A*:      %d nodes | len %.0f | time %.2f | cpu %.2fms",
		(int)aPath.size(), aL, aT, aMs);
	m_compareResult.dijkstraText = dB;
	m_compareResult.aStarText = aB;

	char cb[520];
	snprintf(cb, 520, "%s  ||  %s", dB, aB);
	setStatusText(cb);
}

// ========== 按钮检测与点击 ==========

int MapGUI::isButtonClicked(int x, int y) {
	if (y < TOOLBAR_Y || y > TOOLBAR_Y + TOOLBAR_H) return 0;
	int bx = BTN_MARGIN;
	int by = TOOLBAR_Y + (TOOLBAR_H - BTN_H) / 2;
	for (auto& b : m_buttons) {
		if (x >= bx && x <= bx + BTN_W && y >= by && y <= by + BTN_H) {
			return b.id;
		}
		bx += BTN_W + BTN_MARGIN;
	}
	return 0;
}

void MapGUI::onButtonClick(int id) {
	switch (id) {
		case BTN_OPEN: {
			char fn[MAX_PATH] = "";
			if (OpenFileDialog(GetWindowHandle(), fn, MAX_PATH)) {
				if (loadMap(fn)) {
					char b[320];
					snprintf(b, 320, "Loaded: %s", fn);
					setStatusText(b);
				} else {
					setStatusText("Failed to load map!");
				}
			}
			break;
		}
		case BTN_RANDOM: {
			const char* l[] = {"Map Width:", "Map Height:", "Node Count:"};
			int d[] = {2000, 1500, 300};
			showInputDialog("Generate Random Map", l, d, 3);
			break;
		}
		case BTN_GRID: {
			const char* l[] = {"Columns:", "Rows:"};
			int d[] = {20, 15};
			showInputDialog("Generate Grid Map", l, d, 2);
			break;
		}
		case BTN_TRAFFIC: {
			if (m_traffic.isRunning()) {
				m_traffic.pause();
				m_useCongestion = false;
			} else {
				m_traffic.resume();
				m_useCongestion = true;
			}
			setStatusText(m_traffic.isRunning() ? "Traffic ON" : "Traffic OFF");
			break;
		}
		case BTN_CLEAR: {
			clearPath();
			setPathStart(-1);
			setPathEnd(-1);
			m_f1Active = false;
			m_f1Nodes.clear();
			m_compareResult.hasResult = false;
			setStatusText("Cleared");
			break;
		}
		case BTN_FIND_PATH:
			runPathfindingCompare(true);
			break;
		case BTN_ASTAR:
			runPathfindingCompare(false);
			break;
		case BTN_RESET_VIEW: {
			autoFit();
			setStatusText("View reset");
			break;
		}
	}
}

// ========== 输入对话框 ==========

void MapGUI::showInputDialog(const char* title, const char* labels[], int defaults[], int fc) {
	m_inputDlg.active = true;
	m_inputDlg.confirmed = false;
	m_inputDlg.fieldCount = fc;
	strncpy(m_inputDlg.title, title, 63);
	m_inputDlg.title[63] = 0;
	for (int i = 0; i < fc && i < 4; i++) {
		strncpy(m_inputDlg.labels[i], labels[i], 31);
		m_inputDlg.labels[i][31] = 0;
		snprintf(m_inputDlg.fields[i].buffer, 32, "%d", defaults[i]);
		m_inputDlg.fields[i].len = (int)strlen(m_inputDlg.fields[i].buffer);
		m_inputDlg.fields[i].cursorPos = m_inputDlg.fields[i].len;
		m_inputDlg.fields[i].active = false;
		m_inputDlg.resultValues[i] = defaults[i];
	}
	m_inputDlg.fields[0].active = true;
}

void MapGUI::drawInputDialog() {
	if (!m_inputDlg.active) return;
	int dw = 320;
	int rh = 40;
	int dh = 60 + m_inputDlg.fieldCount * rh + 50;
	int dx = (m_screenW - dw) / 2;
	int dy = (m_screenH - dh) / 2;

	DrawRectangle(0, 0, m_screenW, m_screenH, {0, 0, 0, 100});
	DrawRectangle(dx, dy, dw, dh, {50, 54, 60, 255});
	DrawRectangleLines(dx, dy, dw, dh, {80, 84, 92, 255});
	DrawText(m_inputDlg.title, dx + 10, dy + 8, 16, WHITE);

	for (int i = 0; i < m_inputDlg.fieldCount; i++) {
		int fy = dy + 36 + i * rh;
		DrawText(m_inputDlg.labels[i], dx + 10, fy + 8, 14, LIGHTGRAY);

		Color bc = m_inputDlg.fields[i].active
			? Color{0, 120, 255, 255}
			: Color{70, 74, 80, 255};
		DrawRectangle(dx + 170, fy + 2, 130, 28, {30, 34, 40, 255});
		DrawRectangleLines(dx + 170, fy + 2, 130, 28, bc);
		DrawText(m_inputDlg.fields[i].buffer, dx + 174, fy + 5, 14, WHITE);
	}

	int by = dy + dh - 42;
	DrawRectangle(dx + 50, by, 90, 30, {0, 120, 255, 255});
	DrawText("OK", dx + 80, by + 5, 14, WHITE);
	DrawRectangle(dx + 180, by, 90, 30, {60, 64, 72, 255});
	DrawRectangleLines(dx + 180, by, 90, 30, {80, 84, 92, 255});
	DrawText("Cancel", dx + 195, by + 5, 14, LIGHTGRAY);
}

void MapGUI::execInputDialog() {
	if (strcmp(m_inputDlg.title, "Generate Random Map") == 0) {
		int w = std::max(100, m_inputDlg.resultValues[0]);
		int h = std::max(100, m_inputDlg.resultValues[1]);
		int n = std::max(10, m_inputDlg.resultValues[2]);
		loadRandomMap(w, h, n);
		char b[128];
		snprintf(b, 128, "Random map: %dx%d, %d nodes", w, h, m_graph.getNumOfNodes());
		setStatusText(b);
	} else if (strcmp(m_inputDlg.title, "Generate Grid Map") == 0) {
		int c = std::max(2, m_inputDlg.resultValues[0]);
		int r = std::max(2, m_inputDlg.resultValues[1]);
		loadGridMap(2000, 1500, c, r);
		char b[128];
		snprintf(b, 128, "Grid map: %dx%d, %d nodes", c, r, m_graph.getNumOfNodes());
		setStatusText(b);
	}
}

void MapGUI::handleInputDialogInput() {
	if (!m_inputDlg.active) return;
	auto m = GetMousePosition();

	if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
		int dw = 320;
		int rh = 40;
		int dh = 60 + m_inputDlg.fieldCount * rh + 50;
		int dx = (m_screenW - dw) / 2;
		int dy = (m_screenH - dh) / 2;

		// 点击输入框切换焦点
		for (int i = 0; i < m_inputDlg.fieldCount; i++) {
			int fy = dy + 36 + i * rh;
			if (m.x >= dx + 170 && m.x <= dx + 300
			 && m.y >= fy + 2 && m.y <= fy + 30) {
				for (int j = 0; j < m_inputDlg.fieldCount; j++) {
					m_inputDlg.fields[j].active = false;
				}
				m_inputDlg.fields[i].active = true;
				return;
			}
		}

		int by = dy + dh - 42;
		// OK 按钮
		if (m.x >= dx + 50 && m.x <= dx + 140 && m.y >= by && m.y <= by + 30) {
			for (int i = 0; i < m_inputDlg.fieldCount; i++) {
				m_inputDlg.resultValues[i] = atoi(m_inputDlg.fields[i].buffer);
			}
			m_inputDlg.active = false;
			execInputDialog();
			return;
		}
		// Cancel 按钮
		if (m.x >= dx + 180 && m.x <= dx + 270 && m.y >= by && m.y <= by + 30) {
			m_inputDlg.active = false;
			return;
		}
	}

	// 键盘输入
	for (int i = 0; i < m_inputDlg.fieldCount; i++) {
		if (!m_inputDlg.fields[i].active) continue;
		auto& f = m_inputDlg.fields[i];
		int ch;
		while ((ch = GetCharPressed()) > 0) {
			if (ch >= '0' && ch <= '9' && f.len < 31) {
				f.buffer[f.len++] = (char)ch;
				f.buffer[f.len] = 0;
				f.cursorPos = f.len;
			}
		}
		if (IsKeyPressed(KEY_BACKSPACE) && f.len > 0) {
			f.buffer[--f.len] = 0;
			f.cursorPos = f.len;
		}
	}
	if (IsKeyPressed(KEY_ESCAPE)) {
		m_inputDlg.active = false;
	}
	if (IsKeyPressed(KEY_ENTER)) {
		for (int i = 0; i < m_inputDlg.fieldCount; i++) {
			m_inputDlg.resultValues[i] = atoi(m_inputDlg.fields[i].buffer);
		}
		m_inputDlg.active = false;
		execInputDialog();
	}
}

// ========== 交互处理 ==========

void MapGUI::handleInput() {
	if (m_inputDlg.active) {
		handleInputDialogInput();
		return;
	}

	if (IsKeyPressed(KEY_ESCAPE)) {
		m_f1Active = false;
		m_f1Nodes.clear();
		clearPath();
		setPathStart(-1);
		setPathEnd(-1);
		m_compareResult.hasResult = false;
		setStatusText("Cleared");
	}
	if (IsKeyPressed(KEY_ONE)) {
		auto c = m_transform.screenToWorld(m_screenW / 2, m_screenH / 2);
		findAndHighlightNearest100(c.x, c.y);
	}
	if (IsKeyPressed(KEY_T)) {
		if (m_traffic.isRunning()) {
			m_traffic.pause();
			m_useCongestion = false;
		} else {
			m_traffic.resume();
			m_useCongestion = true;
		}
		setStatusText(m_traffic.isRunning() ? "Traffic ON" : "Traffic OFF");
	}
	if (IsKeyPressed(KEY_P)) runPathfindingCompare(true);
	if (IsKeyPressed(KEY_F)) runPathfindingCompare(false);
	if (IsKeyPressed(KEY_R)) {
		autoFit();
		setStatusText("View reset");
	}
	if (IsKeyPressed(KEY_O)) {
		char fn[MAX_PATH] = "";
		if (OpenFileDialog(GetWindowHandle(), fn, MAX_PATH)) {
			if (loadMap(fn)) {
				char b[320];
				snprintf(b, 320, "Loaded: %s", fn);
				setStatusText(b);
			} else {
				setStatusText("Failed to load map!");
			}
		}
	}
	if (IsKeyPressed(KEY_G)) {
		const char* gl[] = {"Map Width:", "Map Height:", "Node Count:"};
		int gd[] = {2000, 1500, 300};
		showInputDialog("Generate Random Map", gl, gd, 3);
	}
	if (IsKeyPressed(KEY_S)) {
		if (m_graph.getNumOfNodes() == 0) {
			setStatusText("No map to save");
		} else {
			char fn[MAX_PATH] = "map.json";
			if (SaveFileDialog(GetWindowHandle(), fn, MAX_PATH, "map.json")) {
				if (m_graph.saveMapToFile(fn)) {
					char b[320];
					snprintf(b, 320, "Map saved: %s", fn);
					setStatusText(b);
				} else {
					setStatusText("Failed to save map!");
				}
			}
		}
	}

	auto m = GetMousePosition();
	int mx = (int)m.x;
	int my = (int)m.y;

	double w = GetMouseWheelMove();
	if (w != 0) {
		m_transform.zoomAt(mx, my, (w > 0) ? 1.15 : 1.0 / 1.15);
	}

	if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
		int bid = isButtonClicked(mx, my);
		if (bid > 0) {
			onButtonClick(bid);
			return;
		}

		m_dragging = true;
		m_dragStartX = mx;
		m_dragStartY = my;

		auto wp = m_transform.screenToWorld(mx, my);
		int nid = findNearestNode(wp.x, wp.y);
		if (nid >= 0) {
			auto* nd = m_graph.getNodeById(nid);
			if (nd) {
				double d = sqrt(
					(nd->x_coordinate - wp.x) * (nd->x_coordinate - wp.x)
					+ (nd->y_coordinate - wp.y) * (nd->y_coordinate - wp.y)
				);
				if (d < 30.0 / m_transform.scale) {
					if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
						setPathEnd(nid);
						m_compareResult.hasResult = false;
						char b[256];
						snprintf(b, 256, "End: node%d (%d,%d)", nid, nd->x_coordinate, nd->y_coordinate);
						setStatusText(b);
						if (m_pathStart >= 0 && m_pathEnd >= 0) {
							runPathfindingCompare(true);
						}
					} else {
						setPathStart(nid);
						clearPath();
						m_compareResult.hasResult = false;
						char b[256];
						snprintf(b, 256, "Start: node%d (%d,%d) | Shift+click to set end",
							nid, nd->x_coordinate, nd->y_coordinate);
						setStatusText(b);
					}
					m_hoverNode = nid;
				}
			}
		}
	}

	if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
		m_dragging = false;
	}

	if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && m_dragging) {
		m_transform.pan(mx - m_dragStartX, my - m_dragStartY);
		m_dragStartX = mx;
		m_dragStartY = my;
	}

	m_lastMouseX = mx;
	m_lastMouseY = my;

	// 按钮悬停检测
	if (my >= TOOLBAR_Y && my <= TOOLBAR_Y + TOOLBAR_H) {
		int bx = BTN_MARGIN;
		int by = TOOLBAR_Y + (TOOLBAR_H - BTN_H) / 2;
		for (auto& b : m_buttons) {
			bool no = (mx >= bx && mx <= bx + BTN_W && my >= by && my <= by + BTN_H);
			if (b.mouseOver != no) b.mouseOver = no;
			bx += BTN_W + BTN_MARGIN;
		}
	} else {
		for (auto& b : m_buttons) b.mouseOver = false;
	}

	// 节点悬停高亮
	if (!m_dragging) {
		auto wp = m_transform.screenToWorld(mx, my);
		int nid = findNearestNode(wp.x, wp.y);
		if (nid >= 0) {
			auto* nd = m_graph.getNodeById(nid);
			if (nd) {
				double dist = sqrt(
					(nd->x_coordinate - wp.x) * (nd->x_coordinate - wp.x)
					+ (nd->y_coordinate - wp.y) * (nd->y_coordinate - wp.y)
				);
				m_hoverNode = (dist < 30.0 / m_transform.scale) ? nid : -1;
			}
		} else {
			m_hoverNode = -1;
		}
	}

	if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
		auto wp = m_transform.screenToWorld(m_lastMouseX, m_lastMouseY);
		findAndHighlightNearest100(wp.x, wp.y);
	}
}

// ========== 绘制 ==========

void MapGUI::drawGrid() {
	if (m_transform.scale <= 0) return;
	double a, b, c, d;
	m_transform.getVisibleRange(a, b, c, d);
	double gs = 100;
	while (gs * m_transform.scale < 40) gs *= 2;
	while (gs * m_transform.scale > 200) gs /= 2;
	int sy = TOOLBAR_Y + TOOLBAR_H;

	double sx = floor(a / gs) * gs;
	for (double wx = sx; wx <= b; wx += gs) {
		Color co = (fmod(wx, gs * 2) < 0.1)
			? Color{200, 200, 200, 255}
			: Color{220, 220, 220, 255};
		auto p1 = m_transform.worldToScreen(wx, c);
		auto p2 = m_transform.worldToScreen(wx, d);
		if (p2.y > std::max(p1.y, sy)) {
			DrawLine(p1.x, std::max(p1.y, sy), p2.x, p2.y, co);
		}
	}

	double sfy = floor(c / gs) * gs;
	for (double wy = sfy; wy <= d; wy += gs) {
		Color co = (fmod(wy, gs * 2) < 0.1)
			? Color{200, 200, 200, 255}
			: Color{220, 220, 220, 255};
		auto p1 = m_transform.worldToScreen(a, wy);
		auto p2 = m_transform.worldToScreen(b, wy);
		if (p2.y > std::max(p1.y, sy)) {
			DrawLine(p1.x, std::max(p1.y, sy), p2.x, p2.y, co);
		}
	}
}

void MapGUI::drawEdges() {
	int n = m_graph.getNumOfNodes();
	int m = m_graph.getNumOfEdges();
	if (!n || !m) return;

	auto* nodes = m_graph.getMainGraph();
	auto* edges = m_graph.getAllEdgeAttrs();

	// 路径边集合（快速判断）
	std::map<std::pair<int, int>, bool> pe;
	for (size_t i = 0; i + 1 < m_path.size(); i++) {
		int a = m_path[i];
		int b = m_path[i + 1];
		pe[{a, b}] = true;
		pe[{b, a}] = true;
	}

	double a, b, c, d;
	m_transform.getVisibleRange(a, b, c, d);

	// 绘制非路径边（带车流颜色）
	for (int i = 0; i < m; i++) {
		int f = edges[i].fromNodeID;
		int t = edges[i].toNodeID;
		if (pe.count({f, t})) continue;

		auto& na = nodes[f];
		auto& nb = nodes[t];

		// 可见性裁剪
		if ((na.x_coordinate < a && nb.x_coordinate < a)
		 || (na.x_coordinate > b && nb.x_coordinate > b)
		 || (na.y_coordinate < c && nb.y_coordinate < c)
		 || (na.y_coordinate > d && nb.y_coordinate > d)) {
			continue;
		}

		// F1 模式：只高亮 F1 相关边
		if (m_f1Active && !(isNodeF1Highlighted(f) && isNodeF1Highlighted(t))) {
			auto p = m_transform.worldToScreen(na.x_coordinate, na.y_coordinate);
			auto q = m_transform.worldToScreen(nb.x_coordinate, nb.y_coordinate);
			DrawLine((float)p.x, (float)p.y, (float)q.x, (float)q.y, {220, 220, 220, 255});
			continue;
		}

		auto sa = m_transform.worldToScreen(na.x_coordinate, na.y_coordinate);
		auto sb = m_transform.worldToScreen(nb.x_coordinate, nb.y_coordinate);

		Color col;
		int lw;
		if (m_traffic.isRunning()) {
			col = TrafficColor(m_traffic.getEdgeLoadRatio(i));
			lw = (m_transform.scale > 3) ? 8 : ((m_transform.scale > 1.5) ? 6 : 5);
		} else {
			col = {140, 150, 165, 255};
			lw = (m_transform.scale > 3) ? 5 : ((m_transform.scale > 1.5) ? 4 : 3);
		}
		DrawLineEx({(float)sa.x, (float)sa.y}, {(float)sb.x, (float)sb.y}, (float)lw, col);
	}

	// 绘制路径边（青色高亮）
	for (size_t i = 0; i + 1 < m_path.size(); i++) {
		int pa = m_path[i];
		int pb = m_path[i + 1];
		for (int j = 0; j < m; j++) {
			if (edges[j].fromNodeID == pa && edges[j].toNodeID == pb) {
				auto& e = edges[j];
				auto& na = nodes[e.fromNodeID];
				auto& nb = nodes[e.toNodeID];
				auto sa = m_transform.worldToScreen(na.x_coordinate, na.y_coordinate);
				auto sb = m_transform.worldToScreen(nb.x_coordinate, nb.y_coordinate);
				float lw = (float)((m_transform.scale > 2) ? 7 : 5);
				DrawLineEx({(float)sa.x, (float)sa.y}, {(float)sb.x, (float)sb.y}, lw, {0, 188, 212, 255});
				break;
			}
		}
	}
}

void MapGUI::drawNodes() {
	int n = m_graph.getNumOfNodes();
	if (!n) return;

	auto* nodes = m_graph.getMainGraph();
	double a, b, c, d;
	m_transform.getVisibleRange(a, b, c, d);
	int mr = 20;

	int r = (m_transform.scale > 2) ? 7
		: (m_transform.scale > 1) ? 6
		: (m_transform.scale > 0.5) ? 5
		: (m_transform.scale > 0.2) ? 4 : 3;

	// 普通节点
	for (int i = 0; i < n; i++) {
		double nx = nodes[i].x_coordinate;
		double ny = nodes[i].y_coordinate;
		if (nx < a - mr || nx > b + mr || ny < c - mr || ny > d + mr) continue;
		if (i == m_pathStart || i == m_pathEnd || isNodeF1Highlighted(i) || i == m_hoverNode) continue;

		auto sp = m_transform.worldToScreen(nx, ny);
		DrawCircle(sp.x, sp.y, (float)(r + 1), WHITE);
		DrawCircle(sp.x, sp.y, (float)r, {66, 133, 244, 255});
	}

	// F1 高亮节点（橙色）
	for (int i = 0; i < n; i++) {
		if (!isNodeF1Highlighted(i) || i == m_pathStart || i == m_pathEnd) continue;
		double nx = nodes[i].x_coordinate;
		double ny = nodes[i].y_coordinate;
		if (nx < a - mr || nx > b + mr || ny < c - mr || ny > d + mr) continue;

		auto sp = m_transform.worldToScreen(nx, ny);
		DrawCircle(sp.x, sp.y, 10, WHITE);
		DrawCircle(sp.x, sp.y, 8, {255, 152, 0, 255});
	}

	// 悬停节点（绿色）
	if (m_hoverNode >= 0 && m_hoverNode != m_pathStart && m_hoverNode != m_pathEnd) {
		auto sp = m_transform.worldToScreen(
			nodes[m_hoverNode].x_coordinate,
			nodes[m_hoverNode].y_coordinate
		);
		DrawCircleLines(sp.x, sp.y, 11, {76, 175, 80, 255});
		DrawCircle(sp.x, sp.y, 8, {76, 175, 80, 255});
	}
}

void MapGUI::drawPath() {
	if (m_path.empty()) return;
	auto* nodes = m_graph.getMainGraph();
	for (int nid : m_path) {
		auto sp = m_transform.worldToScreen(nodes[nid].x_coordinate, nodes[nid].y_coordinate);
		DrawCircle(sp.x, sp.y, 8, WHITE);
		DrawCircle(sp.x, sp.y, 7, {0, 188, 212, 255});
	}
}

void MapGUI::drawSelectedNode(int nid) {
	if (nid < 0) return;
	auto sp = m_transform.worldToScreen(
		m_graph.getMainGraph()[nid].x_coordinate,
		m_graph.getMainGraph()[nid].y_coordinate
	);
	bool isS = (nid == m_pathStart);
	Color mc = isS ? Color{0, 150, 136, 255} : Color{244, 67, 54, 255};
	Color dc = isS ? Color{0, 105, 92, 255} : Color{198, 40, 40, 255};

	DrawCircleLines(sp.x, sp.y, 16, WHITE);
	DrawCircle(sp.x, sp.y, 14, mc);
	DrawCircleLines(sp.x, sp.y, 14, dc);
	DrawCircle(sp.x, sp.y, 10, WHITE);

	int fs = 16;
	const char* label = isS ? "S" : "E";
	int tw = MeasureText(label, fs);
	DrawText(label, sp.x - tw / 2, sp.y - fs / 2, fs, dc);
}

void MapGUI::drawF1Highlight() {
	if (!m_f1Active || m_f1Nodes.empty()) return;

	auto* nodes = m_graph.getMainGraph();
	double cx = 0, cy = 0;
	for (int n : m_f1Nodes) {
		cx += nodes[n].x_coordinate;
		cy += nodes[n].y_coordinate;
	}
	cx /= m_f1Nodes.size();
	cy /= m_f1Nodes.size();

	auto ct = m_transform.worldToScreen(cx, cy);

	double md = 0;
	for (int n : m_f1Nodes) {
		double d = sqrt(
			(nodes[n].x_coordinate - cx) * (nodes[n].x_coordinate - cx)
			+ (nodes[n].y_coordinate - cy) * (nodes[n].y_coordinate - cy)
		);
		if (d > md) md = d;
	}

	int sr = (int)(md * m_transform.scale) + 20;
	DrawCircleLines(ct.x, ct.y, (float)sr, {255, 180, 0, 255});
	DrawCircle(ct.x, ct.y, (float)sr, {255, 200, 100, 50});

	char b[64];
	snprintf(b, 64, "F1: %d node @(%.0f,%.0f)", (int)m_f1Nodes.size(), cx, cy);
	int tw = MeasureText(b, 11);
	DrawText(b, ct.x - tw / 2, ct.y - 20, 11, {255, 120, 0, 255});
}

void MapGUI::drawCars() {
	if (!m_traffic.isRunning() || m_graph.getNumOfNodes() == 0) return;
	auto& cars = m_traffic.getCars();
	if (cars.empty()) return;

	auto* edges = m_graph.getAllEdgeAttrs();
	auto* nodes = m_graph.getMainGraph();

	for (auto& c : cars) {
		if (c.edgeIndex < 0 || c.edgeIndex >= m_graph.getNumOfEdges()) continue;
		auto& e = edges[c.edgeIndex];
		auto& fn = nodes[e.fromNodeID];
		auto& tn = nodes[e.toNodeID];

		double px = fn.x_coordinate + (tn.x_coordinate - fn.x_coordinate) * c.progress;
		double py = fn.y_coordinate + (tn.y_coordinate - fn.y_coordinate) * c.progress;
		auto sp = m_transform.worldToScreen(px, py);

		int r = std::max(3, (int)(5 * std::min(1.0, m_transform.scale / 2)));
		DrawCircle(sp.x, sp.y, (float)r, {255, 80, 80, 255});
	}
}

void MapGUI::drawTrafficLegend() {
	if (!m_showTrafficLegend) return;
	int lx = m_screenW - 130;
	int ly = TOOLBAR_Y + TOOLBAR_H + 10;
	int bw = 115;
	int bh = 85;

	DrawRectangle(lx + 2, ly + 2, bw, bh, {220, 220, 220, 255});
	DrawRectangle(lx, ly, bw, bh, WHITE);
	DrawRectangleLines(lx, ly, bw, bh, {200, 200, 200, 255});
	DrawText("Traffic", lx + 8, ly + 5, 12, {60, 60, 60, 255});

	int bx = lx + 5;
	int by = ly + 22;
	for (int i = 0; i < 45; i++) {
		double t = 1.0 - (double)i / 44;
		Color c = TrafficColor(t);
		DrawLine(bx, by + i, bx + 12, by + i, c);
	}
	DrawText("Free", bx + 15, by, 10, {120, 120, 120, 255});
	DrawText("Jam", bx + 15, by + 35, 10, {120, 120, 120, 255});
	DrawText("Mid", bx + 15, by + 17, 10, {120, 120, 120, 255});

	char b[32];
	snprintf(b, 32, "%d cars", m_traffic.getTotalCars());
	DrawText(b, lx + 5, ly + bh - 15, 10, {255, 140, 0, 255});
}

void MapGUI::drawStatusBar() {
	int bh = m_compareResult.hasResult ? 56 : 36;
	int by = m_screenH - bh;

	DrawRectangle(0, by, m_screenW, bh, {33, 37, 41, 255});
	DrawLine(0, by, m_screenW, by, {0, 150, 136, 255});

	if (m_compareResult.hasResult) {
		DrawText(m_compareResult.dijkstraText.c_str(), 12, by + 4, 14, {100, 180, 255, 255});
		DrawText(m_compareResult.aStarText.c_str(), 12, by + 30, 14, {100, 230, 150, 255});
	} else {
		Color tc;
		if (m_statusText.find("Dijkstra:") != std::string::npos
		 || m_statusText.find("A*:") != std::string::npos) {
			tc = {100, 230, 120, 255};
		} else if (m_statusText.find("failed") != std::string::npos
		        || m_statusText.find("No") != std::string::npos) {
			tc = {255, 100, 100, 255};
		} else {
			tc = {220, 220, 220, 255};
		}
		DrawText(m_statusText.c_str(), 12, by + 10, 14, tc);
	}

	char zs[32], inf[64];
	snprintf(zs, 32, "Zoom: %.0f%%", m_transform.scale * 100);
	snprintf(inf, 64, "Nodes:%d Edges:%d", m_graph.getNumOfNodes(), m_graph.getNumOfEdges());
	DrawText(zs, m_screenW - 270, by + bh - 20, 12, {160, 170, 190, 255});
	DrawText(inf, m_screenW - 165, by + bh - 20, 12, {140, 145, 155, 255});
}

void MapGUI::drawToolbar() {
	DrawRectangle(0, TOOLBAR_Y, m_screenW, TOOLBAR_H, {44, 48, 54, 255});
	DrawLine(0, TOOLBAR_Y, m_screenW, TOOLBAR_Y, {60, 64, 70, 255});

	int x = BTN_MARGIN;
	int y = TOOLBAR_Y + (TOOLBAR_H - BTN_H) / 2;
	for (auto& b : m_buttons) {
		bool act = (b.id == BTN_TRAFFIC && m_traffic.isRunning());
		Color bg = b.mouseOver
			? Color{0, 123, 255, 255}
			: (act ? Color{40, 167, 69, 255} : Color{60, 64, 72, 255});
		Color tc = (b.mouseOver || act) ? WHITE : Color{200, 205, 210, 255};

		DrawRectangleRounded({(float)x, (float)y, (float)BTN_W, (float)BTN_H}, 0.3f, 6, bg);
		int tw = MeasureText(b.label, 13);
		DrawText(b.label, x + (BTN_W - tw) / 2, y + (BTN_H - 13) / 2, 13, tc);
		x += BTN_W + BTN_MARGIN;
	}

	const char* congLabel = m_useCongestion
		? "[Congestion-aware ON]"
		: "[Congestion-aware OFF]";
	Color congColor = m_useCongestion
		? Color{40, 167, 69, 255}
		: Color{200, 200, 200, 255};
	DrawText(congLabel, m_screenW - 210, TOOLBAR_Y + (TOOLBAR_H - 13) / 2, 13, congColor);
}

// ========== 主循环 ==========

void MapGUI::run() {
	InitWindow(1200, 800, "SimpleMap - raylib");
	SetTargetFPS(60);

	m_screenW = GetScreenWidth();
	m_screenH = GetScreenHeight();
	m_transform.clientW = m_screenW;
	m_transform.clientH = m_screenH;

	setStatusText("O=Open, G=Random, T=Traffic, P=Dijkstra, F=A*, R=Reset, S=Save, "
		"ESC=Clear | Click=start, Shift+click=end, Right=F1, Scroll=zoom");

	while (!WindowShouldClose()) {
		double dt = GetFrameTime();

		if (m_traffic.isRunning()) {
			m_traffic.update(dt);
			m_traffic.syncToGraph();  // 同步车流数据到图，供寻路使用
		}

		handleInput();

		BeginDrawing();
		ClearBackground({245, 247, 250, 255});

		drawToolbar();
		drawStatusBar();

		// 地图区域剪裁：工具栏底部 ~ 状态栏顶部
		int statusH = m_compareResult.hasResult ? 56 : 36;
		BeginScissorMode(0, TOOLBAR_Y + TOOLBAR_H, m_screenW,
			m_screenH - TOOLBAR_Y - TOOLBAR_H - statusH);

		drawGrid();
		drawEdges();
		drawNodes();
		drawPath();

		if (!m_path.empty()) {
			drawSelectedNode(m_path.front());
			drawSelectedNode(m_path.back());
		}

		drawF1Highlight();
		drawCars();
		drawTrafficLegend();

		EndScissorMode();

		if (m_inputDlg.active) {
			drawInputDialog();
		}

		EndDrawing();
	}

	CloseWindow();
}
