@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d C:\Users\lenovo\Desktop\simplemap1
cl /EHsc /c dataStructure.cpp /Fo"dataStructure.obj" /utf-8
cl /EHsc /c pathfinding.cpp /Fo"pathfinding.obj" /utf-8
cl /EHsc /c mapView.cpp /Fo"mapView.obj" /D "WITH_GUI" /utf-8
cl /EHsc /c main.cpp /Fo"main.obj" /D "WITH_GUI" /utf-8
link /OUT:simplemap1_gui.exe main.obj mapView.obj dataStructure.obj pathfinding.obj /SUBSYSTEM:WINDOWS user32.lib gdi32.lib gdiplus.lib comdlg32.lib
echo Done.
if exist simplemap1_gui.exe echo SUCCESS