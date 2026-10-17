#ifndef COMPLAINT_DS_H
#define COMPLAINT_DS_H
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ComplaintNode
{
    int complaintID;
    char citizenName[100];
    char category[50];
    char description[200];
    char location[100];
    char priority[20];
    int priorityValue;
    char status[20];
    char department[50];
    char employee[50];

    struct ComplaintNode* next;

} ComplaintNode;


typedef struct PriorityNode
{
    ComplaintNode* complaint;
    struct PriorityNode* next;

} PriorityNode;


void addComplaint(ComplaintNode** head);
void saveComplaints(ComplaintNode* head);
void loadComplaints(ComplaintNode** head);
void makeLowercase(const char *source, char *destination, size_t size);
int containsKeyword(const char *description,const char *keywords[], int count);
void assignPriority(ComplaintNode* complaint);
void displayComplaints(ComplaintNode* head);
void searchComplaint(ComplaintNode* head, int id);
void updateStatus(ComplaintNode* head, int id, const char newStatus[]);
void assignDepartment(ComplaintNode* head, int id, const char newDepartment[]);
void assignEmployee(ComplaintNode* head, int id, const char newEmployee[]);
void insertPriorityQueue(PriorityNode** front, ComplaintNode* complaint);
void displayPriorityQueue(PriorityNode* front);
void sortComplaintsByPriority(ComplaintNode* head);
void displayComplaintStatistics(ComplaintNode* head);
void saveStatusHistory(int complaintID, const char oldStatus[],const char newStatus[]);
void filterComplaintsByStatus(ComplaintNode* head, const char status[]);
void filterComplaintsByCategory(ComplaintNode* head, const char category[]);



#ifdef __cplusplus
}
#endif

#endif