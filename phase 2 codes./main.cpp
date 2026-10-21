
#include <iostream>
#include <string>
#include <limits>
#include <cstdlib>

#include "Complaint.h"

extern "C"
{
    #include "complaint_ds.h"
}

using namespace std;


/* Global data-structure pointers */

ComplaintNode* head = NULL;
PriorityNode* priorityFront = NULL;


/* Menu declarations */

void citizenMenu();
void adminMenu();
void clearPriorityQueue(PriorityNode** front);


/* CLEAR PRIORITY QUEUE */

void clearPriorityQueue(PriorityNode** front)
{
    PriorityNode* current = *front;

    while (current != NULL)
    {
        PriorityNode* temp = current;
        current = current->next;
        free(temp);
    }

    *front = NULL;
}


/*  BASE CLASS: USER */

class User
{
public:
    virtual void displayMenu() = 0;

    virtual ~User()
    {
    }
};


/* DERIVED CLASS: CITIZEN */

class Citizen : public User
{
public:
    void displayMenu() override
    {
        citizenMenu();
    }
};


/* DERIVED CLASS: ADMIN */

class Admin : public User
{
public:
    void displayMenu() override
    {
        adminMenu();
    }
};


/* CITIZEN MENU */

void citizenMenu()
{
    int choice;

    do
    {
        cout << "\n========== CITIZEN MENU ==========\n";
        cout << "1. Register Complaint\n";
        cout << "2. View All Complaints\n";
        cout << "3. Search Complaint\n";
        cout << "4. Back to Main Menu\n";
        cout << "Enter your choice: ";

        if (!(cin >> choice))
        {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input. Enter a number.\n";
            continue;
        }

        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        switch (choice)
        {
            case 1:
            {
                ComplaintNode* oldTail = head;

                while (oldTail != NULL && oldTail->next != NULL)
                {
                    oldTail = oldTail->next;
                }

                /*
                    C function collects complaint details
                    and automatically generates the ID.
                */
                addComplaint(&head);

                ComplaintNode* newTail = head;

                while (newTail != NULL && newTail->next != NULL)
                {
                    newTail = newTail->next;
                }

                /*
                    Add the newly registered complaint
                    to the priority queue.
                */
                if (newTail != NULL && newTail != oldTail)
                {
                    insertPriorityQueue(&priorityFront, newTail);

                    /*
                        Create a C++ Complaint object.
                    */
                    Complaint complaint(
                        newTail->complaintID,
                        newTail->citizenName,
                        newTail->category,
                        newTail->description,
                        newTail->location,
                        newTail->priority,
                        newTail->status
                    );

                    cout << "\nComplaint details confirmed.\n";
                    cout << "Complaint ID: "
                         << complaint.getComplaintID() << endl;

                    cout << "Priority: "
                         << complaint.getPriority() << endl;

                    cout << "Status: "
                         << complaint.getStatus() << endl;
                }

                break;
            }

            case 2:
                displayComplaints(head);
                break;

            case 3:
            {
                int id;

                cout << "Enter Complaint ID to search: ";

                if (!(cin >> id))
                {
                    cin.clear();
                    cin.ignore(
                        numeric_limits<streamsize>::max(), '\n'
                    );

                    cout << "Invalid complaint ID.\n";
                    break;
                }

                cin.ignore(numeric_limits<streamsize>::max(), '\n');

                searchComplaint(head, id);
                break;
            }

            case 4:
                cout << "Returning to main menu...\n";
                break;

            default:
                cout << "Invalid choice. Try again.\n";
        }

    } while (choice != 4);
}


/* ADMIN MENU */

void adminMenu()
{
    int choice;

    do
    {
        cout << "\n========== ADMIN MENU ==========\n";
        cout << "1. View All Complaints\n";
        cout << "2. Update Complaint Status\n";
        cout << "3. Assign Department\n";
        cout << "4. Assign Employee\n";
        cout << "5. View Priority Queue\n";
        cout << "6. Sort Complaints by Priority\n";
        cout << "7. Dashboard / Statistics\n";
        cout << "8. Filter Complaints\n";
        cout << "9. Back to Main Menu\n";
        cout << "Enter your choice: ";

        if (!(cin >> choice))
        {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input. Enter a number.\n";
            continue;
        }

        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        switch (choice)
        {
            case 1:
                displayComplaints(head);
                break;

            case 2:
            {
                int id;
                string status;

                cout << "Enter Complaint ID: ";

                if (!(cin >> id))
                {
                    cin.clear();
                    cin.ignore(
                        numeric_limits<streamsize>::max(), '\n'
                    );

                    cout << "Invalid complaint ID.\n";
                    break;
                }

                cin.ignore(numeric_limits<streamsize>::max(), '\n');

                cout << "Enter new status "
                     << "(Pending/In Progress/Resolved): ";
                getline(cin, status);

                updateStatus(head, id, status.c_str());
                break;
            }

            case 3:
            {
                int id;
                string department;

                cout << "Enter Complaint ID: ";

                if (!(cin >> id))
                {
                    cin.clear();
                    cin.ignore(
                        numeric_limits<streamsize>::max(), '\n'
                    );

                    cout << "Invalid complaint ID.\n";
                    break;
                }

                cin.ignore(numeric_limits<streamsize>::max(), '\n');

                cout << "Enter Department: ";
                getline(cin, department);

                assignDepartment(head, id, department.c_str());
                break;
            }

            case 4:
            {
                int id;
                string employee;

                cout << "Enter Complaint ID: ";

                if (!(cin >> id))
                {
                    cin.clear();
                    cin.ignore(
                        numeric_limits<streamsize>::max(), '\n'
                    );

                    cout << "Invalid complaint ID.\n";
                    break;
                }

                cin.ignore(numeric_limits<streamsize>::max(), '\n');

                cout << "Enter Employee Name: ";
                getline(cin, employee);

                assignEmployee(head, id, employee.c_str());
                break;
            }

            case 5:
                displayPriorityQueue(priorityFront);
                break;

            case 6:
                sortComplaintsByPriority(head);

                /*
                    Rebuild the priority queue after sorting.
                */
                clearPriorityQueue(&priorityFront);

                {
                    ComplaintNode* current = head;

                    while (current != NULL)
                    {
                        insertPriorityQueue(
                            &priorityFront, current
                        );

                        current = current->next;
                    }
                }

                cout << "Complaints sorted by priority.\n";
                break;

            case 7:
                displayComplaintStatistics(head);
                break;

            case 8:
            {
                int filterChoice;

                cout << "\n========== FILTER MENU ==========\n";
                cout << "1. Filter by Status\n";
                cout << "2. Filter by Category\n";
                cout << "3. Back\n";
                cout << "Enter your choice: ";

                if (!(cin >> filterChoice))
                {
                    cin.clear();
                    cin.ignore(
                        numeric_limits<streamsize>::max(), '\n'
                    );

                    cout << "Invalid input.\n";
                    break;
                }

                cin.ignore(numeric_limits<streamsize>::max(), '\n');

                switch (filterChoice)
                {
                    case 1:
                    {
                        string status;

                        cout << "Enter status to filter: ";
                        getline(cin, status);

                        filterComplaintsByStatus(
                            head, status.c_str()
                        );

                        break;
                    }

                    case 2:
                    {
                        string category;

                        cout << "Enter category to filter: ";
                        getline(cin, category);

                        filterComplaintsByCategory(
                            head, category.c_str()
                        );

                        break;
                    }

                    case 3:
                        break;

                    default:
                        cout << "Invalid filter choice.\n";
                }

                break;
            }

            case 9:
                cout << "Returning to main menu...\n";
                break;

            default:
                cout << "Invalid choice. Try again.\n";
        }

    } while (choice != 9);
}


/* MAIN FUNCTION */

int main()
{
    int choice;

    /*
        Load saved complaints.
    */
    loadComplaints(&head);

    /*
        Rebuild the priority queue from saved complaints.
    */
    ComplaintNode* current = head;

    while (current != NULL)
    {
        insertPriorityQueue(&priorityFront, current);
        current = current->next;
    }

    Citizen citizen;
    Admin admin;

    /*
        Base-class pointer demonstrates runtime polymorphism.
    */
    User* currentUser = NULL;

    do
    {
        cout << "\n====================================\n";
        cout << " SMART PUBLIC COMPLAINT ANALYZER\n";
        cout << "====================================\n";
        cout << "1. Citizen\n";
        cout << "2. Admin\n";
        cout << "3. Exit\n";
        cout << "Enter your choice: ";

        if (!(cin >> choice))
        {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input. Enter a number.\n";
            continue;
        }

        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        switch (choice)
        {
            case 1:
                currentUser = &citizen;
                currentUser->displayMenu();
                break;

            case 2:
                currentUser = &admin;
                currentUser->displayMenu();
                break;

            case 3:
                cout << "Exiting Smart Public Complaint Analyzer.\n";
                break;

            default:
                cout << "Invalid choice. Try again.\n";
        }

    } while (choice != 3);

    /*
        Free priority queue nodes.
    */
    clearPriorityQueue(&priorityFront);

    /*
        Free complaint linked-list nodes.
    */
    current = head;

    while (current != NULL)
    {
        ComplaintNode* nextNode = current->next;
        free(current);
        current = nextNode;
    }

    head = NULL;

    return 0;
}
