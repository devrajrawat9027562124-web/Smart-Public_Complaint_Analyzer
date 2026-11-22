/*
 * MenuSystem.h
 * Main application controller — handles the top-level menu loop.
 */

#ifndef MENU_SYSTEM_H
#define MENU_SYSTEM_H

#include "Person.h"
#include "Dashboard.h"

class MenuSystem {
private:
    Person* currentUser;
    bool running;

public:
    MenuSystem();
    ~MenuSystem();
    
    /* Run the main application loop */
    void run();
    
private:
    void showMainMenu();
    void handleLogin();
    void handleSignup();
    void handleLogout();
};

#endif /* MENU_SYSTEM_H */

