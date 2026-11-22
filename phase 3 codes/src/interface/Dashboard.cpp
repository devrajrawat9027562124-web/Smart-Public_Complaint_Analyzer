/*
 * Dashboard.cpp
 * Console UI utilities with ANSI colors, box drawing, tables, and progress bars.
 */

#include "../../include/interface/Dashboard.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <limits>

/* ─── ANSI Color Constants ─── */
const std::string Dashboard::RESET      = "\033[0m";
const std::string Dashboard::BOLD       = "\033[1m";
const std::string Dashboard::DIM        = "\033[2m";
const std::string Dashboard::UNDERLINE  = "\033[4m";
const std::string Dashboard::BLINK      = "\033[5m";

const std::string Dashboard::RED        = "\033[31m";
const std::string Dashboard::GREEN      = "\033[32m";
const std::string Dashboard::YELLOW     = "\033[33m";
const std::string Dashboard::BLUE       = "\033[34m";
const std::string Dashboard::MAGENTA    = "\033[35m";
const std::string Dashboard::CYAN       = "\033[36m";
const std::string Dashboard::WHITE      = "\033[37m";
const std::string Dashboard::GRAY       = "\033[90m";

const std::string Dashboard::BRIGHT_RED     = "\033[91m";
const std::string Dashboard::BRIGHT_GREEN   = "\033[92m";
const std::string Dashboard::BRIGHT_YELLOW  = "\033[93m";
const std::string Dashboard::BRIGHT_BLUE    = "\033[94m";
const std::string Dashboard::BRIGHT_MAGENTA = "\033[95m";
const std::string Dashboard::BRIGHT_CYAN    = "\033[96m";
const std::string Dashboard::BRIGHT_WHITE   = "\033[97m";

const std::string Dashboard::BG_RED     = "\033[41m";
const std::string Dashboard::BG_GREEN   = "\033[42m";
const std::string Dashboard::BG_YELLOW  = "\033[43m";
const std::string Dashboard::BG_BLUE    = "\033[44m";
const std::string Dashboard::BG_MAGENTA = "\033[45m";
const std::string Dashboard::BG_CYAN    = "\033[46m";

/* ─── Clear Screen ─── */
void Dashboard::clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

/* ─── Print Header ─── */
void Dashboard::printHeader(const std::string& title) {
    std::cout << std::endl;
    std::cout << BOLD << BRIGHT_CYAN;
    std::cout << "  ╔══════════════════════════════════════════════════════════╗" << std::endl;

    /* Center the title. A title wider than the box (a long name) is cut to
     * fit: a negative std::string length used to throw std::length_error
     * and end the program at login. */
    const int inner = 58;
    std::string shown = fitText(title, inner);
    int width = textWidth(shown);
    int left = (inner - width) / 2;
    int right = inner - width - left;
    std::cout << "  ║" << std::string(left, ' ') << BRIGHT_WHITE << shown
              << BRIGHT_CYAN << std::string(right, ' ') << "║" << std::endl;

    std::cout << "  ╚══════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << RESET << std::endl;
}

/* ─── Print Sub-Header ─── */
void Dashboard::printSubHeader(const std::string& title) {
    std::cout << std::endl;
    std::cout << BOLD << YELLOW << "  ┌─── " << title << " ";
    int remaining = 55 - textWidth(title);
    for (int i = 0; i < remaining; i++) std::cout << "─";
    std::cout << "┐" << RESET << std::endl;
}

/* ─── Print Separator ─── */
void Dashboard::printSeparator(int width) {
    std::cout << GRAY << "  ";
    for (int i = 0; i < width; i++) std::cout << "─";
    std::cout << RESET << std::endl;
}

/* ─── Print Double Separator ─── */
void Dashboard::printDoubleSeparator(int width) {
    std::cout << GRAY << "  ";
    for (int i = 0; i < width; i++) std::cout << "═";
    std::cout << RESET << std::endl;
}

/* ─── Success / Error / Warning / Info Messages ─── */
void Dashboard::printSuccess(const std::string& message) {
    std::cout << BOLD << GREEN << "  ✓ " << message << RESET << std::endl;
}

void Dashboard::printError(const std::string& message) {
    std::cout << BOLD << RED << "  ✗ " << message << RESET << std::endl;
}

void Dashboard::printWarning(const std::string& message) {
    std::cout << BOLD << YELLOW << "  ⚠ " << message << RESET << std::endl;
}

void Dashboard::printInfo(const std::string& message) {
    std::cout << BOLD << BLUE << "  ℹ " << message << RESET << std::endl;
}

/* ─── Press Enter to Continue ─── */
void Dashboard::pressEnterToContinue() {
    if (!std::cin) return;  /* input closed: don't block */
    std::cout << std::endl << GRAY << "  Press Enter to continue..." << RESET;
    std::string ignored;
    readLine(ignored);
}

/* ─── Line-based input ─── */
bool Dashboard::readLine(std::string& out) {
    out.clear();
    if (!std::getline(std::cin, out)) return false;
    if (!out.empty() && out[out.size() - 1] == '\r') out.erase(out.size() - 1);  /* CRLF input */
    return true;
}

static std::string trimmed(const std::string& s) {
    size_t start = s.find_first_not_of(" \t");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t");
    return s.substr(start, end - start + 1);
}

int Dashboard::readInt(int fallback, int endValue) {
    std::string line;
    if (!readLine(line)) return endValue;
    std::istringstream in(trimmed(line));
    int value;
    char extra;
    if (in >> value && !(in >> extra)) return value;
    return fallback;
}

char Dashboard::readChoice(char emptyValue, char endValue) {
    std::string line;
    if (!readLine(line)) return endValue;
    line = trimmed(line);
    if (line.empty()) return emptyValue;
    char c = line[0];
    return (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c;
}

/* ─── UTF-8 aware width (one column per character) ─── */
int Dashboard::textWidth(const std::string& text) {
    int width = 0;
    for (size_t i = 0; i < text.size(); i++)
        if (((unsigned char)text[i] & 0xC0) != 0x80) width++;  /* skip continuation bytes */
    return width;
}

std::string Dashboard::fitText(const std::string& text, int width) {
    if (textWidth(text) <= width) return text;
    if (width <= 3) return std::string(width > 0 ? width : 0, '.');
    /* Keep width-3 characters, then "..." */
    int kept = 0;
    size_t i = 0;
    for (; i < text.size(); i++) {
        if (((unsigned char)text[i] & 0xC0) != 0x80) {
            if (kept == width - 3) break;
            kept++;
        }
    }
    return text.substr(0, i) + "...";
}

/* ─── Severity Color ─── */
std::string Dashboard::getSeverityColor(int severity) {
    if (severity >= 8) return BOLD + BRIGHT_RED;
    if (severity >= 6) return BOLD + RED;
    if (severity >= 4) return BOLD + YELLOW;
    if (severity >= 2) return BOLD + GREEN;
    return BOLD + GRAY;
}

std::string Dashboard::getSeverityLabel(int severity) {
    if (severity >= 9) return "CRITICAL";
    if (severity >= 7) return "HIGH";
    if (severity >= 5) return "MEDIUM";
    if (severity >= 3) return "LOW";
    return "MINOR";
}

/* ─── Status Color ─── */
std::string Dashboard::getStatusColor(const std::string& status) {
    if (status == "PENDING") return YELLOW;
    if (status == "REVIEWED") return BLUE;
    if (status == "ASSIGNED") return MAGENTA;
    if (status == "IN_PROGRESS") return CYAN;
    if (status == "COMPLETED") return GREEN;
    return WHITE;
}

/* ─── Progress Bar ─── */
void Dashboard::printProgressBar(int current, int total, int width) {
    if (total == 0) total = 1;
    double ratio = (double)current / total;
    int filled = (int)(ratio * width);

    std::cout << CYAN << "  [";
    for (int i = 0; i < width; i++) {
        if (i < filled) std::cout << BRIGHT_GREEN << "█";
        else std::cout << GRAY << "░";
    }
    std::cout << CYAN << "] " << BRIGHT_WHITE << current << "/" << total
              << " (" << (int)(ratio * 100) << "%)" << RESET << std::endl;
}

/* ─── Print Complaint Card ─── */
void Dashboard::printComplaintCard(const Complaint& complaint) {
    std::string sevColor = getSeverityColor(complaint.severity);
    std::string statColor = getStatusColor(complaint.status);

    std::cout << BOLD << CYAN << "  ┌────────────────────────────────────────────────────────┐" << RESET << std::endl;
    std::cout << CYAN << "  │ " << BOLD << BRIGHT_WHITE << "Complaint #" << complaint.id
              << "                                               " << CYAN << "│" << RESET << std::endl;
    std::cout << CYAN << "  ├────────────────────────────────────────────────────────┤" << RESET << std::endl;

    std::cout << CYAN << "  │ " << WHITE << "Title     : " << BRIGHT_WHITE << complaint.title << RESET << std::endl;
    std::cout << CYAN << "  │ " << WHITE << "Category  : " << BRIGHT_YELLOW << complaint.category << RESET << std::endl;
    std::cout << CYAN << "  │ " << WHITE << "Location  : " << BRIGHT_CYAN << complaint.location << RESET << std::endl;
    std::cout << CYAN << "  │ " << WHITE << "Severity  : " << sevColor << complaint.severity
              << "/10 (" << getSeverityLabel(complaint.severity) << ")" << RESET << std::endl;
    std::cout << CYAN << "  │ " << WHITE << "NLP Score : " << BRIGHT_MAGENTA << complaint.nlp_score << RESET << std::endl;
    std::cout << CYAN << "  │ " << WHITE << "Status    : " << statColor << complaint.status << RESET << std::endl;
    std::cout << CYAN << "  │ " << WHITE << "Dept      : " << WHITE << complaint.department << RESET << std::endl;

    if (complaint.is_duplicate) {
        std::cout << CYAN << "  │ " << RED << "⚠ DUPLICATE of Complaint #" << complaint.duplicate_of << RESET << std::endl;
    }
    if (complaint.is_irrelevant) {
        std::cout << CYAN << "  │ " << RED << "⚠ FLAGGED AS IRRELEVANT/SPAM" << RESET << std::endl;
    }

    std::cout << CYAN << "  │ " << WHITE << "Submitted : " << GRAY << complaint.submitted_date << RESET << std::endl;
    if (strlen(complaint.completed_date) > 1 && strcmp(complaint.completed_date, "N/A") != 0) {
        std::cout << CYAN << "  │ " << WHITE << "Completed : " << GREEN << complaint.completed_date << RESET << std::endl;
    }
    if (complaint.worker_id > 0) {
        std::cout << CYAN << "  │ " << WHITE << "Worker ID : " << BRIGHT_BLUE << complaint.worker_id << RESET << std::endl;
    }

    std::cout << CYAN << "  └────────────────────────────────────────────────────────┘" << RESET << std::endl;
}

/* A table cell: text cut and padded to width by character count, not
 * bytes, so titles in Hindi (3 bytes per letter) keep the columns straight. */
static std::string padCell(const std::string& s, int width) {
    std::string fitted = Dashboard::fitText(s, width);
    return fitted + std::string(width - Dashboard::textWidth(fitted), ' ');
}

/* ─── Print Complaint Table ─── */
void Dashboard::printComplaintTable(Complaint* complaints, int count) {
    if (count == 0) {
        printInfo("No complaints found.");
        return;
    }

    /* Header */
    std::cout << std::endl;
    std::cout << BOLD << CYAN
              << "  ┌──────┬─────────────────────────┬───────────────┬──────┬─────────────┐" << RESET << std::endl;
    std::cout << BOLD << CYAN
              << "  │  ID  │ Title                   │ Category      │ Sev  │ Status      │" << RESET << std::endl;
    std::cout << BOLD << CYAN
              << "  ├──────┼─────────────────────────┼───────────────┼──────┼─────────────┤" << RESET << std::endl;

    for (int i = 0; i < count; i++) {
        std::string sevColor = getSeverityColor(complaints[i].severity);
        std::string statColor = getStatusColor(complaints[i].status);

        /* std::right is set explicitly on every number: std::left is sticky
         * on std::cout and used to misalign every row after the first */
        std::cout << CYAN << "  │ " << WHITE << std::right << std::setw(4) << complaints[i].id << CYAN << " │ "
                  << WHITE << padCell(complaints[i].title, 23) << CYAN << " │ "
                  << YELLOW << padCell(complaints[i].category, 13) << CYAN << " │ "
                  << sevColor << std::right << std::setw(4) << complaints[i].severity << RESET << CYAN << " │ "
                  << statColor << padCell(complaints[i].status, 11) << RESET << CYAN << " │"
                  << RESET << std::endl;
    }

    std::cout << BOLD << CYAN
              << "  └──────┴─────────────────────────┴───────────────┴──────┴─────────────┘" << RESET << std::endl;
    std::cout << GRAY << "  Total: " << count << " complaints" << RESET << std::endl;
}

/* ─── Print Stats ─── */
void Dashboard::printStats(const ComplaintStats& stats) {
    printSubHeader("Complaint Statistics");

    std::cout << WHITE << "  Total       : " << BOLD << BRIGHT_WHITE << stats.total << RESET << std::endl;
    std::cout << YELLOW << "  Pending     : " << BOLD << stats.pending << RESET << "  ";
    printProgressBar(stats.pending, stats.total > 0 ? stats.total : 1);

    std::cout << BLUE << "  Reviewed    : " << BOLD << stats.reviewed << RESET << "  ";
    printProgressBar(stats.reviewed, stats.total > 0 ? stats.total : 1);

    std::cout << MAGENTA << "  Assigned    : " << BOLD << stats.assigned << RESET << "  ";
    printProgressBar(stats.assigned, stats.total > 0 ? stats.total : 1);

    std::cout << CYAN << "  In Progress : " << BOLD << stats.in_progress << RESET << "  ";
    printProgressBar(stats.in_progress, stats.total > 0 ? stats.total : 1);

    std::cout << GREEN << "  Completed   : " << BOLD << stats.completed << RESET << "  ";
    printProgressBar(stats.completed, stats.total > 0 ? stats.total : 1);

    std::cout << std::endl;
}

/* ─── Welcome Banner ─── */

void Dashboard::printWelcomeBanner() {
    clearScreen();

    std::cout << std::endl;
    std::cout << BOLD << BRIGHT_CYAN;

    std::cout << "  +--------------------------------------------------------------+" << std::endl;
    std::cout << "  |                                                              |" << std::endl;

    std::cout << BRIGHT_WHITE;
    std::cout << "  |   SSSS  M   M   AAA   RRRR   TTTTT                          |" << std::endl;
    std::cout << "  |  S      MM MM  A   A  R   R    T                            |" << std::endl;
    std::cout << "  |   SSS   M M M  AAAAA  RRRR     T                            |" << std::endl;
    std::cout << "  |      S  M   M  A   A  R  R     T                            |" << std::endl;
    std::cout << "  |  SSSS   M   M  A   A  R   R    T                            |" << std::endl;

    std::cout << BRIGHT_CYAN;
    std::cout << "  |                                                              |" << std::endl;
    std::cout << "  +--------------------------------------------------------------+" << std::endl;

    std::cout << std::endl;
    std::cout << BOLD << BRIGHT_YELLOW;
    std::cout << "             PUBLIC COMPLAINT ANALYZER SYSTEM" << std::endl;

    std::cout << GRAY;
    std::cout << "                Dehradun Smart City Initiative" << std::endl;

    std::cout << RESET << std::endl;
}


/* ─── Login Banner ─── */
void Dashboard::printLoginBanner() {
    std::cout << std::endl;
    std::cout << BOLD << BRIGHT_BLUE;
    std::cout << "  ┌──────────────────────────────────────────────────────────┐" << std::endl;
    std::cout << "  │                    AUTHENTICATION                       │" << std::endl;
    std::cout << "  └──────────────────────────────────────────────────────────┘" << std::endl;
    std::cout << RESET << std::endl;
}

