
#ifndef COMPLAINT_DS_H
#define COMPLAINT_DS_H

/* Structure for storing complaint details */
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

/* Structure for priority queue */
typedef struct PriorityNode
{
    ComplaintNode* complaint;
    struct PriorityNode* next;

} PriorityNode;

/* Complaint management functions */
void addComplaint(ComplaintNode** head);
void displayComplaints(ComplaintNode* head);
void searchComplaint(ComplaintNode* head, int id);

/* Status and assignment functions */
void updateStatus(ComplaintNode* head, int id,const char newStatus[]);

void assignDepartment(ComplaintNode* head, int id,const char newDepartment[]);

void assignEmployee(ComplaintNode* head, int id,const char newEmployee[]);

/* Priority queue functions */
void insertPriorityQueue(PriorityNode** front,ComplaintNode* complaint);
void displayPriorityQueue(PriorityNode* front);
void sortComplaintsByPriority(ComplaintNode* head);
/* Priority calculation */
void assignPriority(ComplaintNode* complaint);
/* File handling */
void saveComplaints(ComplaintNode* head);
void loadComplaints(ComplaintNode** head);
void saveStatusHistory(int complaintID,const char oldStatus[],const char newStatus[]);
/* Statistics and filters */
void displayComplaintStatistics(ComplaintNode* head);

void filterComplaintsByStatus(ComplaintNode* head,const char status[]);

void filterComplaintsByCategory(ComplaintNode* head,const char category[]);

#endif
