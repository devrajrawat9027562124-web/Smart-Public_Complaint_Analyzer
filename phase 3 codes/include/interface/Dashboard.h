/*
 * Dashboard.h
 * Static console UI utilities — ANSI colors, box drawing, tables, progress bars.
 */

#ifndef DASHBOARD_H
#define DASHBOARD_H

#include <string>

extern "C" {
    #include "../../include/engine/complaint_types.h"
}

class Dashboard {
public:
    /* ─── ANSI Color Codes ─── */
    static const std::string RESET;
    static const std::string BOLD;
    static const std::string DIM;
    static const std::string UNDERLINE;
    static const std::string BLINK;

    /* Foreground Colors */
    static const std::string RED;
    static const std::string GREEN;
    static const std::string YELLOW;
    static const std::string BLUE;
    static const std::string MAGENTA;
    static const std::string CYAN;
    static const std::string WHITE;
    static const std::string GRAY;

    /* Bright Colors */
    static const std::string BRIGHT_RED;
    static const std::string BRIGHT_GREEN;
    static const std::string BRIGHT_YELLOW;
    static const std::string BRIGHT_BLUE;
    static const std::string BRIGHT_MAGENTA;
    static const std::string BRIGHT_CYAN;
    static const std::string BRIGHT_WHITE;

    /* Background Colors */
    static const std::string BG_RED;
    static const std::string BG_GREEN;
    static const std::string BG_YELLOW;
    static const std::string BG_BLUE;
    static const std::string BG_MAGENTA;
    static const std::string BG_CYAN;

    /* ─── UI Utilities ─── */
    static void clearScreen();
    static void printHeader(const std::string& title);
    static void printSubHeader(const std::string& title);
    static void printSeparator(int width = 60);
    static void printDoubleSeparator(int width = 60);
    static void printSuccess(const std::string& message);
    static void printError(const std::string& message);
    static void printWarning(const std::string& message);
    static void printInfo(const std::string& message);
    static void pressEnterToContinue();

    /* ─── Input: every prompt reads exactly one whole line ───
     * so nothing typed for one prompt can leak into the next one. */

    /* One line (without the newline). False when input has ended. */
    static bool readLine(std::string& out);

    /* A line holding one whole number. Anything else (empty, "abc",
     * "2 Electricity") gives fallback; end of input gives endValue. */
    static int readInt(int fallback = -1, int endValue = 0);

    /* The first letter typed, in lowercase (for y/n/q answers).
     * An empty line gives emptyValue; end of input gives endValue. */
    static char readChoice(char emptyValue, char endValue);

    /* Text width in characters (UTF-8 aware), and cutting text to a width
     * without splitting a character: used to keep boxes and tables aligned. */
    static int textWidth(const std::string& text);
    static std::string fitText(const std::string& text, int width);

    /* Severity color based on level */
    static std::string getSeverityColor(int severity);
    static std::string getSeverityLabel(int severity);

    /* Status color based on status string */
    static std::string getStatusColor(const std::string& status);

    /* Progress bar */
    static void printProgressBar(int current, int total, int width = 30);

    /* Complaint display */
    static void printComplaintCard(const Complaint& complaint);
    static void printComplaintTable(Complaint* complaints, int count);

    /* Stats display */
    static void printStats(const ComplaintStats& stats);

    /* Welcome banner */
    static void printWelcomeBanner();
    static void printLoginBanner();
};

#endif /* DASHBOARD_H */

