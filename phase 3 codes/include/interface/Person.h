/*
 * Person.h
 * Abstract base class for all user roles.
 * Uses pure virtual functions for polymorphic dispatch.
 */

#ifndef PERSON_H
#define PERSON_H

#include <string>
#include <iostream>
#include <vector>
#include <vector>

extern "C" {
    #include "../../include/engine/complaint_types.h"
    #include "../../include/engine/file_manager.h"
    #include "../../include/engine/nlp_analyzer.h"
    #include "../../include/engine/duplicate_detector.h"
    #include "../../include/engine/priority_queue.h"
    #include "../../include/engine/route_optimizer.h"
    #include "../../include/engine/hash_table.h"
    #include "../../include/engine/workflow.h"
    #include "../../include/engine/workflow.h"
}

class Person {
protected:
    int id;
    std::string username;
    std::string password;
    std::string fullName;
    std::string phone;
    std::string email;
    std::string role;
    std::string registeredDate;
    std::string department;
    int currentLocationId;
    int isAvailable;

public:
    /* Constructor 1: Manual initialization */
    Person(int id, const std::string& username, const std::string& fullName,
           const std::string& role);

    /* Constructor 2: Initialize from C struct */
    Person(const UserRecord& record);

    /* Virtual destructor for proper polymorphic cleanup */
    virtual ~Person() = default;

    /* Pure virtual functions — must be implemented by subclasses */
    virtual void showDashboard() = 0;
    virtual void showMenu() = 0;

    /* Getters */
    int getId() const { return id; }
    std::string getUsername() const { return username; }
    std::string getFullName() const { return fullName; }
    std::string getRole() const { return role; }
    std::string getDepartment() const { return department; }
    int getCurrentLocationId() const { return currentLocationId; }
    int getIsAvailable() const { return isAvailable; }

    /* Setters */
    void setDepartment(const std::string& dept) { department = dept; }
    void setCurrentLocationId(int locId) { currentLocationId = locId; }
    void setIsAvailable(int avail) { isAvailable = avail; }

    /* Convert to C struct */
    UserRecord toUserRecord() const;

    /* Display profile */
    void showProfile();
};

#endif /* PERSON_H */

