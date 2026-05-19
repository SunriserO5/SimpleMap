@echo off
cd /d "C:\Users\lenovo\Desktop\simplemap1"
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" simplemap1.vcxproj /p:Configuration=Debug /p:Platform=x64 /t:Rebuild /v:detailed > build.log 2>&1
type build.log
