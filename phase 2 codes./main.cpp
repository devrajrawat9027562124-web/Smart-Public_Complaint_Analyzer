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

    if (head != NULL)
    {
        ComplaintNode* current = head;

        while (current->next != NULL)
        {
            current = current->next;
        }

        insertPriorityQueue(&priorityFront, current);
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
        cout << "7. Admin Dashboard & Statistics" << endl;
        cout << "8. Filter Complaints" << endl;
        cout << "9. Back to Main Menu" << endl;
   

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
    displayComplaintStatistics(head);
    break;

case 8:
{
    int filterChoice;

    cout << "\n===== FILTER COMPLAINTS =====\n";
    cout << "1. Filter by Status\n";
    cout << "2. Filter by Category\n";
    cout << "3. Show All Complaints\n";
    cout << "Enter choice: ";
    cin >> filterChoice;

    if (filterChoice == 1)
    {
        int statusChoice;

        cout << "\n1. Pending\n";
        cout << "2. In Progress\n";
        cout << "3. Resolved\n";
        cout << "Enter status choice: ";
        cin >> statusChoice;

        if (statusChoice == 1)
            filterComplaintsByStatus(head, "Pending");
        else if (statusChoice == 2)
            filterComplaintsByStatus(head, "In Progress");
        else if (statusChoice == 3)
            filterComplaintsByStatus(head, "Resolved");
        else
            cout << "Invalid status choice.\n";
    }
    else if (filterChoice == 2)
    {
        int categoryChoice;

        cout << "\n1. Road\n";
        cout << "2. Electricity\n";
        cout << "3. Water Supply\n";
        cout << "4. Garbage\n";
        cout << "5. Other\n";
        cout << "Enter category choice: ";
        cin >> categoryChoice;

        if (categoryChoice == 1)
            filterComplaintsByCategory(head, "Road");
        else if (categoryChoice == 2)
            filterComplaintsByCategory(head, "Electricity");
        else if (categoryChoice == 3)
            filterComplaintsByCategory(head, "Water Supply");
        else if (categoryChoice == 4)
            filterComplaintsByCategory(head, "Garbage");
        else if (categoryChoice == 5)
            filterComplaintsByCategory(head, "Other");
        else
            cout << "Invalid category choice.\n";
      
    }
    else if (filterChoice == 3)
    {
        displayComplaints(head);
    }
    else
    {
        cout << "Invalid filter choice.\n";
    }

    break;
}


case 9:
    return;


default:
    cout << "Invalid choice." << endl;
        }
    }
}

int main()
{
    int choice;
     loadComplaints(&head);

    ComplaintNode* current = head;

    while (current != NULL)
    {
        insertPriorityQueue(&priorityFront, current);
        current = current->next;
    }

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