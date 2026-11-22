/*
 * Admin.h
 * System administrator role — reviews, escalates, assigns complaints.
 */

#ifndef ADMIN_H
#define ADMIN_H

#include "Person.h"

class Admin : public Person {
public:
    Admin(const UserRecord& record);
    
    void showDashboard() override;
    void showMenu() override;
    
    void reviewComplaints();
    void viewAllComplaints();
    void escalateComplaint();
    void assignToDepartment();
    void viewStatistics();
    void manageDepartments();
    void manageWorkers();
    void viewCityMap();
};

#endif /* ADMIN_H */

