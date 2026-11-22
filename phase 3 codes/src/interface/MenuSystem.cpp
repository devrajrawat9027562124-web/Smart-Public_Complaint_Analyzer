/*
 * MenuSystem.cpp
 * Main application controller — top-level menu loop with login/signup.
 */

#include "../../include/interface/MenuSystem.h"
#include "../../include/interface/AuthManager.h"
#include <iostream>
#include <string>
#include <limits>

MenuSystem::MenuSystem() : currentUser(nullptr), running(true) {
    /* Initialize data files (seed defaults if first run) */
    init_data_files();
}

MenuSystem::~MenuSystem() {
    if (currentUser) {
        delete currentUser;
        currentUser = nullptr;
    }
}

/* ─── Main Application Loop ─── */
void MenuSystem::run() {
    while (running) {
        if (currentUser == nullptr) {
            showMainMenu();
        } else {
            /* Polymorphic dispatch — calls the correct role's menu */
            currentUser->showMenu();
            handleLogout();
        }
    }

    Dashboard::clearScreen();
    std::cout << std::endl;
    std::cout << Dashboard::BOLD << Dashboard::BRIGHT_CYAN;
    std::cout << "  ╔══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "  ║                                                          ║" << std::endl;
    std::cout << "  ║" << Dashboard::BRIGHT_WHITE << "          Thank you for using SMART Complaint            " << Dashboard::BRIGHT_CYAN << "║" << std::endl;
    std::cout << "  ║" << Dashboard::BRIGHT_WHITE << "          Analyzer System! Goodbye!                      " << Dashboard::BRIGHT_CYAN << "║" << std::endl;
    std::cout << "  ║                                                          ║" << std::endl;
    std::cout << "  ╚══════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << Dashboard::RESET << std::endl;
}

/* ─── Main Menu (before login) ─── */
void MenuSystem::showMainMenu() {
    Dashboard::printWelcomeBanner();

    std::cout << Dashboard::BOLD << Dashboard::BRIGHT_CYAN;
    std::cout << "  ┌─── Main Menu ──────────────────────────────────────────┐" << std::endl;
    std::cout << Dashboard::CYAN << "  │" << Dashboard::BRIGHT_WHITE << "  1. " << Dashboard::WHITE << "Login                                                " << Dashboard::CYAN << "│" << Dashboard::RESET << std::endl;
    std::cout << Dashboard::CYAN << "  │" << Dashboard::BRIGHT_WHITE << "  2. " << Dashboard::WHITE << "Sign Up (New User)                                   " << Dashboard::CYAN << "│" << Dashboard::RESET << std::endl;
    std::cout << Dashboard::CYAN << "  │" << Dashboard::BRIGHT_RED   << "  0. " << Dashboard::RED   << "Exit                                                 " << Dashboard::CYAN << "│" << Dashboard::RESET << std::endl;
    std::cout << Dashboard::BOLD << Dashboard::BRIGHT_CYAN;
    std::cout << "  └──────────────────────────────────────────────────────────┘" << std::endl;
    std::cout << Dashboard::RESET << std::endl;

    std::cout << Dashboard::BRIGHT_WHITE << "  Enter choice: " << Dashboard::RESET;
    int choice = Dashboard::readInt(-1, 0);  /* invalid: -1; input closed: 0 = exit */

    switch (choice) {
        case 1: handleLogin(); break;
        case 2: handleSignup(); break;
        case 0: running = false; break;
        default:
            Dashboard::printError("Invalid choice. Please try again.");
            Dashboard::pressEnterToContinue();
    }
}

/* ─── Handle Login ─── */
void MenuSystem::handleLogin() {
    Dashboard::clearScreen();
    Dashboard::printLoginBanner();

    std::string username, password;

    /* Whole lines, like signup: a password containing a space used to be
     * cut at the space here, so such an account could never log in */
    std::cout << Dashboard::CYAN << "  Username: " << Dashboard::WHITE;
    if (!Dashboard::readLine(username)) return;
    std::cout << Dashboard::CYAN << "  Password: " << Dashboard::WHITE;
    if (!Dashboard::readLine(password)) return;
    size_t first = username.find_first_not_of(" \t"), last = username.find_last_not_of(" \t");
    username = (first == std::string::npos) ? "" : username.substr(first, last - first + 1);

    std::cout << std::endl;
    std::cout << Dashboard::DIM << "  Authenticating";
    for (int i = 0; i < 3; i++) {
        std::cout << "." << std::flush;
    }
    std::cout << Dashboard::RESET << std::endl;

    Person* user = AuthManager::login(username, password);

    if (user) {
        currentUser = user;

        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg), "User '%s' logged in (Role: %s)",
                 username.c_str(), user->getRole().c_str());
        log_activity(log_msg);

        Dashboard::printSuccess("Welcome, " + user->getFullName() + "! (Role: " +
                               user->getRole() + ")");
        Dashboard::pressEnterToContinue();
    } else {
        Dashboard::printError("Invalid username or password.");
        Dashboard::pressEnterToContinue();
    }
}

/* ─── Handle Signup ─── */
void MenuSystem::handleSignup() {
    Dashboard::clearScreen();
    Dashboard::printHeader("Create New Account");

    std::string username, password, fullName, phone, email;

    std::cout << Dashboard::CYAN << "  Username (min 3 chars)  : " << Dashboard::WHITE;
    bool complete = Dashboard::readLine(username);

    std::cout << Dashboard::CYAN << "  Password (min 4 chars)  : " << Dashboard::WHITE;
    complete = complete && Dashboard::readLine(password);

    std::cout << Dashboard::CYAN << "  Full Name               : " << Dashboard::WHITE;
    complete = complete && Dashboard::readLine(fullName);

    std::cout << Dashboard::CYAN << "  Phone Number            : " << Dashboard::WHITE;
    complete = complete && Dashboard::readLine(phone);

    std::cout << Dashboard::CYAN << "  Email                   : " << Dashboard::WHITE;
    complete = complete && Dashboard::readLine(email);

    std::cout << std::endl;
    if (!complete) {
        Dashboard::printError("Input ended - account not created.");
        return;
    }

    if (AuthManager::signup(username, password, fullName, phone, email)) {
        Dashboard::printSuccess("Account created successfully! You can now login.");
    } else {
        Dashboard::printError("Failed to create account.");
    }

    Dashboard::pressEnterToContinue();
}

/* ─── Handle Logout ─── */
void MenuSystem::handleLogout() {
    if (currentUser) {
        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg), "User '%s' logged out",
                 currentUser->getUsername().c_str());
        log_activity(log_msg);

        Dashboard::printInfo("Logged out successfully.");

        delete currentUser;
        currentUser = nullptr;
    }
}

