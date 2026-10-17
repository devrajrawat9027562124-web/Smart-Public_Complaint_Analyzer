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
#endif
    
    MenuSystem app;
    app.run();
    
    return 0;
}
