/*
 * AuthManager.cpp
 * Authentication — login factory pattern and user registration.
 */

#include "../../include/interface/AuthManager.h"
#include "../../include/interface/User.h"
#include "../../include/interface/Admin.h"
#include "../../include/interface/Worker.h"
#include "../../include/interface/DepartmentOfficer.h"
#include "../../include/interface/Dashboard.h"
#include <cstring>
#include <iostream>

/* ─── Factory Method: Login ─── */
Person* AuthManager::login(const std::string& username, const std::string& password) {
    UserRecord record;

    int success = authenticate_user(username.c_str(), password.c_str(),
                                     FILE_USERS, &record);

    if (!success) {
        return nullptr;
    }

    /* Factory decision based on role */
    if (strcmp(record.role, "ADMIN") == 0) {
        return new Admin(record);
    } else if (strcmp(record.role, "DEPT_OFFICER") == 0) {
        return new DepartmentOfficer(record);
    } else if (strcmp(record.role, "WORKER") == 0) {
        return new Worker(record);
    } else {
        return new User(record);
    }
}

/* ─── Signup (creates USER role; validation is shared with the website) ─── */
bool AuthManager::signup(const std::string& username, const std::string& password,
                         const std::string& fullName, const std::string& phone,
                         const std::string& email) {
    WfResult result = wf_register_user(username.c_str(), password.c_str(), fullName.c_str(),
                                       phone.c_str(), email.c_str(), "", NULL);
    if (result.code != WF_OK) {
        Dashboard::printError(result.message);
        return false;
    }
    return true;
}

