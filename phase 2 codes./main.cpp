#include <iostream>
#include "Complaint.h"
#include "complaint_ds.h"

using namespace std;

ComplaintNode* head = NULL;
PriorityNode* priorityFront = NULL;

void citizenMenu()
{
    int choice;

    while (true)
    {
        cout << endl;
        cout << "========== CITIZEN MENU ==========" << endl;
        cout << "1. Register Complaint" << endl;
        cout << "2. View Complaints" << endl;
        cout << "3. Search Complaint" << endl;
        cout << "4. Back to Main Menu" << endl;

        cout << endl;
        cout << "Enter your choice: ";
        cin >> choice;

        switch (choice)
        {
            case 1:
{
    addComplaint(&head);

    ComplaintNode* current = head;

    while (current->next != NULL)
    {
        current = current->next;
    }

    insertPriorityQueue(&priorityFront, current);

    break;
}

            case 2:
                displayComplaints(head);
                break;

            case 3:
            {
                int id;

                cout << "Enter Complaint ID to search: ";
                cin >> id;

                searchComplaint(head, id);

                break;
            }

            case 4:
                return;

            default:
                cout << "Invalid choice." << endl;
        }
    }
}

void adminMenu()
{
    int choice;

    while (true)
    {
        cout << endl;
        cout << "========== ADMIN MENU ==========" << endl;
        cout << "1. View All Complaints" << endl;
        cout << "2. Update Complaint Status" << endl;
        cout << "3. Assign Department" << endl;
        cout << "4. Assign Employee" << endl;
        cout << "5. View Priority Queue" << endl;
        cout << "6. Sort Complaints by Priority" << endl;
        cout << "7. Back to Main Menu" << endl;

        cout << endl;
        cout << "Enter your choice: ";
        cin >> choice;

        switch (choice)
        {
            case 1:
                displayComplaints(head);
                break;

            case 2:
{
    int id;
    int statusChoice;

    cout << endl;
    cout << "Enter Complaint ID: ";
    cin >> id;

    cout << endl;
    cout << "Select New Status:" << endl;
    cout << "1. Pending" << endl;
    cout << "2. In Progress" << endl;
    cout << "3. Resolved" << endl;

    cout << "Enter your choice: ";
    cin >> statusChoice;

    if (statusChoice == 1)
    {
        updateStatus(head, id, "Pending");
    }
    else if (statusChoice == 2)
    {
        updateStatus(head, id, "In Progress");
    }
    else if (statusChoice == 3)
    {
        updateStatus(head, id, "Resolved");
    }
    else
    {
        cout << "Invalid status choice." << endl;
    }

    break;
}
case 3:
{
    int id;
    int departmentChoice;

    cout << endl;
    cout << "Enter Complaint ID: ";
    cin >> id;

    cout << endl;
    cout << "Select Department:" << endl;
    cout << "1. Public Works" << endl;
    cout << "2. Electricity" << endl;
    cout << "3. Water Supply" << endl;
    cout << "4. Sanitation" << endl;

    cout << "Enter your choice: ";
    cin >> departmentChoice;

    if (departmentChoice == 1)
    {
        assignDepartment(head, id, "Public Works");
    }
    else if (departmentChoice == 2)
    {
        assignDepartment(head, id, "Electricity");
    }
    else if (departmentChoice == 3)
    {
        assignDepartment(head, id, "Water Supply");
    }
    else if (departmentChoice == 4)
    {
        assignDepartment(head, id, "Sanitation");
    }
    else
    {
        cout << "Invalid department choice." << endl;
    }

    break;
}

           case 4:
{
    int id;
    int employeeChoice;

    cout << endl;
    cout << "Enter Complaint ID: ";
    cin >> id;

    cout << endl;
    cout << "Select Employee:" << endl;
    cout << "1. Rahul Sharma" << endl;
    cout << "2. Aman Verma" << endl;
    cout << "3. Priya Singh" << endl;
    cout << "4. Neha Gupta" << endl;

    cout << "Enter your choice: ";
    cin >> employeeChoice;

    if (employeeChoice == 1)
    {
        assignEmployee(head, id, "Rahul Sharma");
    }
    else if (employeeChoice == 2)
    {
        assignEmployee(head, id, "Aman Verma");
    }
    else if (employeeChoice == 3)
    {
        assignEmployee(head, id, "Priya Singh");
    }
    else if (employeeChoice == 4)
    {
        assignEmployee(head, id, "Neha Gupta");
    }
    else
    {
        cout << "Invalid employee choice." << endl;
    }

    break;
}

  case 5:
    displayPriorityQueue(priorityFront);
    break;

case 6:
    sortComplaintsByPriority(head);
    break;

case 7:
    return;

default:
    cout << "Invalid choice." << endl;
        }
    }
}

int main()
{
    int choice;

    while (true)
    {
        cout << endl;
        cout << "========================================" << endl;
        cout << "       SMART PUBLIC COMPLAINT ANALYZER" << endl;
        cout << "========================================" << endl;

        cout << endl;
        cout << "1. Citizen" << endl;
        cout << "2. Admin" << endl;
        cout << "3. Exit" << endl;

        cout << endl;
        cout << "Enter your choice: ";
        cin >> choice;

        switch (choice)
        {
            case 1:
                citizenMenu();
                break;

            case 2:
                adminMenu();
                break;

            case 3:
                cout << "Exiting program..." << endl;
                return 0;

            default:
                cout << "Invalid choice." << endl;
        }
    }

    return 0;
}
