@echo off
chcp 65001 >nul
title SimpleMap GUI 编译 (raylib)

echo 正在加载Visual Studio环境...
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

cd /d "D:\SimpleMap"

echo.
echo ========== 编译 SimpleMap GUI (raylib) ==========
echo.

set RAYLIB_DIR=thirdparty\raylib\raylib-6.0_win64_msvc16

echo [1/5] 编译 filedialog.cpp ...
cl /EHsc /c filedialog.cpp /Fo"filedialog.obj" /W4 /utf-8
if %ERRORLEVEL% neq 0 goto :error

echo [2/5] 编译 gui.cpp ...
cl /EHsc /c gui.cpp /Fo"gui.obj" /I"%RAYLIB_DIR%\include" /W4 /D "WITH_GUI" /utf-8
if %ERRORLEVEL% neq 0 goto :error

echo [3/5] 编译 dataStructure.cpp ...
cl /EHsc /c dataStructure.cpp /Fo"dataStructure.obj" /W4 /utf-8
if %ERRORLEVEL% neq 0 goto :error

echo [4/5] 编译 pathfinding.cpp 和 main.cpp ...
cl /EHsc /c pathfinding.cpp /Fo"pathfinding.obj" /W4 /utf-8
if %ERRORLEVEL% neq 0 goto :error

cl /EHsc /c main.cpp /Fo"main.obj" /I"%RAYLIB_DIR%\include" /W4 /D "WITH_GUI" /utf-8
if %ERRORLEVEL% neq 0 goto :error

echo [5/5] 链接...
link /OUT:simplemap1_gui.exe main.obj gui.obj filedialog.obj dataStructure.obj pathfinding.obj /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup /INCREMENTAL:NO "%RAYLIB_DIR%\lib\raylibdll.lib" user32.lib gdi32.lib comdlg32.lib winmm.lib
if %ERRORLEVEL% neq 0 goto :error

echo.
echo ========== 编译成功! ==========
echo 可执行文件: simplemap1_gui.exe
echo.
echo 复制 raylib.dll 到输出目录...
copy /Y "%RAYLIB_DIR%\lib\raylib.dll" . >nul
echo.
echo 按任意键启动程序，或关闭窗口退出...
pause >nul
start simplemap1_gui.exe
exit

:error
echo.
echo ========== 编译失败! ==========
echo 请检查代码错误，按任意键退出...
pause >nul
