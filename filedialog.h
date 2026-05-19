#ifndef FILEDIALOG_H
#define FILEDIALOG_H

// 纯 C 风格文件对话框接口，不依赖 windows.h 或 raylib.h
// 在需要文件对话框的 .cpp 中包含此文件即可

bool OpenFileDialog(void* hwnd, char* outPath, int maxLen);
bool SaveFileDialog(void* hwnd, char* outPath, int maxLen, const char* defaultName);

#endif // FILEDIALOG_H
