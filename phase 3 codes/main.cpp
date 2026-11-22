/*
 * main.cpp
 * Entry point for the Smart Public Complaint Analyzer.
 * Creates a MenuSystem instance and runs the application.
 */

#include "include/interface/MenuSystem.h"

#ifdef _WIN32
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#endif

int main() {
    /* Enable ANSI color support on Windows */
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }
    /* Set console code page to UTF-8 for box-drawing characters */
    SetConsoleOutputCP(65001);

    /*
     * Normalize CWD to the project root (parent of bin/).
     * The exe lives at bin/complaint_analyzer.exe, so we strip the
     * filename and then go up one directory. This ensures "data/..."
     * paths work correctly regardless of how the exe is launched.
     */
    char exePath[MAX_PATH];
    if (GetModuleFileNameA(NULL, exePath, MAX_PATH) > 0) {
        /* Strip the executable filename to get the bin/ directory */
        char* lastSlash = strrchr(exePath, '\\');
        if (!lastSlash) lastSlash = strrchr(exePath, '/');
        if (lastSlash) {
            *lastSlash = '\0';
            /* Go up one directory: bin/ -> project root */
            char* parentSlash = strrchr(exePath, '\\');
            if (!parentSlash) parentSlash = strrchr(exePath, '/');
            if (parentSlash) {
                *parentSlash = '\0';
                SetCurrentDirectoryA(exePath);
            }
        }
    }
#endif
    
    MenuSystem app;
    app.run();
    
    return 0;
}

