/*
 * User.h
 * Regular citizen role — submits and tracks complaints.
 */

#ifndef USER_H
#define USER_H

#include "Person.h"

class User : public Person {
public:
    User(const UserRecord& record);
    User(int id, const std::string& username, const std::string& fullName);
    
    void showDashboard() override;
    void showMenu() override;
    
    void submitComplaint();
    void viewMyComplaints();
    void trackComplaint();
    void viewProfile();
};

#endif /* USER_H */

