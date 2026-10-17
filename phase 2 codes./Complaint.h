#ifndef COMPLAINT_H
#define COMPLAINT_H

#include <string>
using namespace std;

class Complaint
{
public:
    int complaintID;
    string citizenName;
    string category;
    string description;
    string location;
    string priority;
    string status;
};

#endif