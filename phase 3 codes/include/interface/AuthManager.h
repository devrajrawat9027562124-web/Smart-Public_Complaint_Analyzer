/*
 * AuthManager.h
 * Static authentication utility — login, signup, and role-based factory.
 */

#ifndef AUTH_MANAGER_H
#define AUTH_MANAGER_H

#include "Person.h"
#include <string>

class AuthManager {
public:
    /* Factory method: creates correct subclass based on role */
    static Person* login(const std::string& username, const std::string& password);
    
    /* Register a new user (always creates USER role) */
    static bool signup(const std::string& username, const std::string& password,
                       const std::string& fullName, const std::string& phone,
                       const std::string& email);
};

#endif /* AUTH_MANAGER_H */

