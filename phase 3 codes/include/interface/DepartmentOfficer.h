/*
 * DepartmentOfficer.h
 * Department officer role — manages department complaints and assigns workers.
 */

#ifndef DEPARTMENT_OFFICER_H
#define DEPARTMENT_OFFICER_H

#include "Person.h"

class DepartmentOfficer : public Person {
public:
    DepartmentOfficer(const UserRecord& record);
    
    void showDashboard() override;
    void showMenu() override;
    
    void viewDepartmentComplaints();
    void assignWorker();
    void viewWorkerStatus();
    void viewDepartmentStats();
};

#endif /* DEPARTMENT_OFFICER_H */

