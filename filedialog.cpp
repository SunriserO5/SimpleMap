#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <cstring>
#include "filedialog.h"

bool OpenFileDialog(void* hwnd, char* outPath, int maxLen) {
    OPENFILENAMEA ofn;
    char fileName[MAX_PATH] = "";
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = (HWND)hwnd;
    ofn.lpstrFilter = "JSON Map Files\0*.json\0All Files\0*.*\0";
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = "Open Map";
    ofn.Flags = OFN_FILEMUSTEXIST;
    if (GetOpenFileNameA(&ofn)) {
        strncpy(outPath, fileName, maxLen - 1);
        outPath[maxLen - 1] = '\0';
        return true;
    }
    return false;
}

bool SaveFileDialog(void* hwnd, char* outPath, int maxLen, const char* defaultName) {
    OPENFILENAMEA ofn;
    char fileName[MAX_PATH];
    strncpy(fileName, defaultName ? defaultName : "map.json", MAX_PATH - 1);
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = (HWND)hwnd;
    ofn.lpstrFilter = "JSON Map Files\0*.json\0All Files\0*.*\0";
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = "Save Map As";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    if (GetSaveFileNameA(&ofn)) {
        strncpy(outPath, fileName, maxLen - 1);
        outPath[maxLen - 1] = '\0';
        return true;
    }
    return false;
}
