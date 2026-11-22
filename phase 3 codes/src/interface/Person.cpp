/*
 * Person.cpp
 * Base class constructors and common methods.
 */

#include "../../include/interface/Person.h"
#include "../../include/interface/Dashboard.h"
#include <cstring>

/* Constructor 1: Manual initialization */
Person::Person(int id, const std::string& username, const std::string& fullName,
               const std::string& role)
    : id(id), username(username), fullName(fullName), role(role),
      currentLocationId(0), isAvailable(1) {}

/* Constructor 2: Initialize from C struct */
Person::Person(const UserRecord& record)
    : id(record.id),
      username(record.username),
      password(record.password),
      fullName(record.full_name),
      phone(record.phone),
      email(record.email),
      role(record.role),
      registeredDate(record.registered_date),
      department(record.department),
      currentLocationId(record.current_location_id),
      isAvailable(record.is_available) {}

/* Convert to C struct */
UserRecord Person::toUserRecord() const {
    UserRecord record;
    record.id = id;
    strncpy(record.username, username.c_str(), sizeof(record.username) - 1);
    record.username[sizeof(record.username) - 1] = '\0';
    strncpy(record.password, password.c_str(), sizeof(record.password) - 1);
    record.password[sizeof(record.password) - 1] = '\0';
    strncpy(record.full_name, fullName.c_str(), sizeof(record.full_name) - 1);
    record.full_name[sizeof(record.full_name) - 1] = '\0';
    strncpy(record.phone, phone.c_str(), sizeof(record.phone) - 1);
    record.phone[sizeof(record.phone) - 1] = '\0';
    strncpy(record.email, email.c_str(), sizeof(record.email) - 1);
    record.email[sizeof(record.email) - 1] = '\0';
    strncpy(record.role, role.c_str(), sizeof(record.role) - 1);
    record.role[sizeof(record.role) - 1] = '\0';
    strncpy(record.registered_date, registeredDate.c_str(), sizeof(record.registered_date) - 1);
    record.registered_date[sizeof(record.registered_date) - 1] = '\0';
    strncpy(record.department, department.c_str(), sizeof(record.department) - 1);
    record.department[sizeof(record.department) - 1] = '\0';
    record.current_location_id = currentLocationId;
    record.is_available = isAvailable;
    return record;
}

/* Display profile */
void Person::showProfile() {
    Dashboard::printSubHeader("My Profile");
    
    std::cout << Dashboard::CYAN << "  ID          : " << Dashboard::WHITE << id << std::endl;
    std::cout << Dashboard::CYAN << "  Username    : " << Dashboard::WHITE << username << std::endl;
    std::cout << Dashboard::CYAN << "  Full Name   : " << Dashboard::WHITE << fullName << std::endl;
    std::cout << Dashboard::CYAN << "  Phone       : " << Dashboard::WHITE << phone << std::endl;
    std::cout << Dashboard::CYAN << "  Email       : " << Dashboard::WHITE << email << std::endl;
    std::cout << Dashboard::CYAN << "  Role        : " << Dashboard::YELLOW << role << std::endl;
    std::cout << Dashboard::CYAN << "  Department  : " << Dashboard::WHITE << department << std::endl;
    std::cout << Dashboard::CYAN << "  Registered  : " << Dashboard::WHITE << registeredDate << std::endl;
    
    CityGraph graph;
    init_city_graph(&graph);
    std::cout << Dashboard::CYAN << "  Location    : " << Dashboard::WHITE 
              << get_location_name(&graph, currentLocationId) << std::endl;
    
    std::cout << Dashboard::RESET << std::endl;
}

