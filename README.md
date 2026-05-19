# SimpleMap

一个基于 C++ 的地图生成与路径规划项目，带有 **raylib GUI 可视化界面**。

- 可控地图生成（网格 / 随机）
- 图结构管理与校验
- 最短路径计算（Dijkstra / A\*）
- 车流模拟 + 拥堵感知路径规划

## 项目结构

```
SimpleMap/
├── .vscode/               ← VSCode 配置（直接 F5 编译运行）
├── thirdparty/            ← raylib 库（编译必需）
├── gui.cpp/h              ← raylib GUI 主代码
├── filedialog.cpp/h       ← 文件对话框
├── dataStructure.cpp/h    ← 核心图数据结构
├── pathfinding.cpp/h      ← Dijkstra / A\* 路径规划
├── main.cpp               ← 程序入口
├── parameters.h           ← 全局参数
├── _compile_gui.bat       ← 一键编译运行脚本
├── simplemap1.sln/.vcxproj  ← Visual Studio 项目
└── .gitignore
```

## 编译

### VSCode（Windows + MSVC）

1. 安装 VS2022 Build Tools 或 Visual Studio 2022（含 C++ 工作负载）
2. 安装 VSCode 扩展：**C/C++ Extension Pack**
3. 打开项目文件夹，按 `F5` 或 `Ctrl+Shift+B` 选择 **Build (MSVC)** 任务

### Visual Studio 2022

双击 `simplemap1.sln` 打开，`F5` 编译运行。

### 命令行

```bat
_compile_gui.bat
```

## 操作

| 操作 | 功能 |
|------|------|
| 左键点击节点 | 设起点 |
| Shift + 左键 | 设终点 |
| 右键 | F1 查找附近 100 节点 |
| 滚轮 | 缩放 |
| 拖拽 | 平移 |
| O | 打开地图 |
| G | 随机地图 |
| T | 车流开关 |
| P / F | Dijkstra / A\* 寻路 |
| R | 重置视图 |
| S | 保存地图 |
| ESC | 清除 |
