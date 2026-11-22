/*
 * DepartmentOfficer.cpp
 * Department officer — manages department complaints and assigns workers.
 */

#include "../../include/interface/DepartmentOfficer.h"
#include "../../include/interface/Dashboard.h"
#include <iostream>
#include <iomanip>
#include <cstring>
#include <limits>

DepartmentOfficer::DepartmentOfficer(const UserRecord& record) : Person(record) {}

/* ─── Dashboard ─── */
void DepartmentOfficer::showDashboard() {
    Dashboard::printHeader("Department Officer - " + fullName);

    std::cout << Dashboard::CYAN << "  Department: " << Dashboard::BOLD << Dashboard::YELLOW
              << department << Dashboard::RESET << std::endl;

    ComplaintStats stats = get_department_stats(department.c_str());
    Dashboard::printStats(stats);
}

/* ─── Menu ─── */
void DepartmentOfficer::showMenu() {
    int choice = 0;

    do {
        Dashboard::clearScreen();
        showDashboard();

        std::cout << Dashboard::BOLD << Dashboard::BRIGHT_CYAN
                  << "  ┌─── Department Officer Menu ──────────────────────────┐"
                  << Dashboard::RESET << std::endl;
        std::cout << Dashboard::CYAN << "  │" << Dashboard::BRIGHT_WHITE << "  1. " << Dashboard::WHITE << "View Department Complaints                       " << Dashboard::CYAN << "│" << Dashboard::RESET << std::endl;
        std::cout << Dashboard::CYAN << "  │" << Dashboard::BRIGHT_WHITE << "  2. " << Dashboard::WHITE << "Assign Worker to Complaint                       " << Dashboard::CYAN << "│" << Dashboard::RESET << std::endl;
        std::cout << Dashboard::CYAN << "  │" << Dashboard::BRIGHT_WHITE << "  3. " << Dashboard::WHITE << "View Worker Status                               " << Dashboard::CYAN << "│" << Dashboard::RESET << std::endl;
        std::cout << Dashboard::CYAN << "  │" << Dashboard::BRIGHT_WHITE << "  4. " << Dashboard::WHITE << "View Department Statistics                       " << Dashboard::CYAN << "│" << Dashboard::RESET << std::endl;
        std::cout << Dashboard::CYAN << "  │" << Dashboard::BRIGHT_WHITE << "  5. " << Dashboard::WHITE << "View Profile                                     " << Dashboard::CYAN << "│" << Dashboard::RESET << std::endl;
        std::cout << Dashboard::CYAN << "  │" << Dashboard::BRIGHT_RED   << "  0. " << Dashboard::RED   << "Logout                                           " << Dashboard::CYAN << "│" << Dashboard::RESET << std::endl;
        std::cout << Dashboard::BOLD << Dashboard::BRIGHT_CYAN
                  << "  └──────────────────────────────────────────────────────────┘"
                  << Dashboard::RESET << std::endl;

        std::cout << std::endl << Dashboard::BRIGHT_WHITE << "  Enter choice: " << Dashboard::RESET;
        choice = Dashboard::readInt(-1, 0);  /* invalid: -1; input closed: 0 = log out */

        switch (choice) {
            case 1: viewDepartmentComplaints(); break;
            case 2: assignWorker(); break;
            case 3: viewWorkerStatus(); break;
            case 4: viewDepartmentStats(); break;
            case 5:
                Dashboard::clearScreen();
                showProfile();
                Dashboard::pressEnterToContinue();
                break;
            case 0: break;
            default:
                Dashboard::printError("Invalid choice.");
                Dashboard::pressEnterToContinue();
        }
    } while (choice != 0);
}

/* ─── View Department Complaints ─── */
void DepartmentOfficer::viewDepartmentComplaints() {
    Dashboard::clearScreen();
    Dashboard::printHeader(department + " - Complaints");

    std::vector<Complaint> complaints(MAX_COMPLAINTS);
    int count = load_complaints_by_department(department.c_str(), complaints.data(), MAX_COMPLAINTS);

    Dashboard::printComplaintTable(complaints.data(), count);

    if (count > 0) {
        std::cout << std::endl;
        std::cout << Dashboard::CYAN << "  View details? Enter complaint ID (0 to skip): "
                  << Dashboard::WHITE;
        int view_id = Dashboard::readInt();

        if (view_id > 0) {
            for (int i = 0; i < count; i++) {
                if (complaints[i].id == view_id) {
                    std::cout << std::endl;
                    Dashboard::printComplaintCard(complaints[i]);
                    std::cout << Dashboard::CYAN << "  Description: " << Dashboard::WHITE
                              << complaints[i].description << Dashboard::RESET << std::endl;
                    break;
                }
            }
        }
    }

    Dashboard::pressEnterToContinue();
}

/* ─── Assign Worker ─── */
void DepartmentOfficer::assignWorker() {
    Dashboard::clearScreen();
    Dashboard::printHeader("Assign Worker to Complaint");

    /* Show unassigned department complaints */
    std::vector<Complaint> dept_complaints(MAX_COMPLAINTS);
    int dept_count = load_complaints_by_department(department.c_str(), dept_complaints.data(), MAX_COMPLAINTS);

    /* Waiting for a worker = routed here by an admin (ASSIGNED) and no
     * worker yet. PENDING/REVIEWED complaints used to be listed too, which
     * let an officer skip the admin's review. */
    Dashboard::printSubHeader("Waiting for a Worker");
    int unassigned = 0;
    for (int i = 0; i < dept_count; i++) {
        if (dept_complaints[i].worker_id <= 0 &&
            strcmp(dept_complaints[i].status, "ASSIGNED") == 0) {
            std::cout << Dashboard::CYAN << "  [" << dept_complaints[i].id << "] "
                      << Dashboard::WHITE << dept_complaints[i].title
                      << Dashboard::YELLOW << " @ " << dept_complaints[i].location
                      << " (Severity: " << Dashboard::getSeverityColor(dept_complaints[i].severity)
                      << dept_complaints[i].severity << Dashboard::YELLOW << ")"
                      << Dashboard::RESET << std::endl;
            unassigned++;
        }
    }

    if (unassigned == 0) {
        Dashboard::printInfo("No complaint is waiting for a worker. Complaints appear here once an admin routes them to your department.");
        Dashboard::pressEnterToContinue();
        return;
    }

    std::cout << std::endl;
    std::cout << Dashboard::CYAN << "  Enter Complaint ID: " << Dashboard::WHITE;
    int complaint_id = Dashboard::readInt();

    /* Find the complaint */
    int comp_idx = -1;
    for (int i = 0; i < dept_count; i++) {
        if (dept_complaints[i].id == complaint_id && dept_complaints[i].worker_id <= 0 &&
            strcmp(dept_complaints[i].status, "ASSIGNED") == 0) {
            comp_idx = i;
            break;
        }
    }

    if (comp_idx < 0) {
        Dashboard::printError("Complaint not found or already assigned.");
        Dashboard::pressEnterToContinue();
        return;
    }

    /* Show available workers */
    UserRecord workers[MAX_WORKERS];
    int worker_count = load_workers_by_department(department.c_str(), workers, MAX_WORKERS);

    CityGraph graph;
    init_city_graph(&graph);

    Dashboard::printSubHeader("Available Workers");
    int available = 0;
    for (int w = 0; w < worker_count; w++) {
        std::string avail = workers[w].is_available ? "Available" : "Busy";
        std::string avail_color = workers[w].is_available ? Dashboard::GREEN : Dashboard::RED;

        std::cout << Dashboard::CYAN << "  [" << workers[w].id << "] "
                  << Dashboard::WHITE << workers[w].full_name
                  << Dashboard::GRAY << " (" << workers[w].username << ")"
                  << Dashboard::YELLOW << " @ " << get_location_name(&graph, workers[w].current_location_id)
                  << " - " << avail_color << avail
                  << Dashboard::RESET << std::endl;

        if (workers[w].is_available) available++;
    }

    if (available == 0) {
        Dashboard::printWarning("No workers available in this department.");
        Dashboard::pressEnterToContinue();
        return;
    }

    std::cout << std::endl;
    std::cout << Dashboard::CYAN << "  Enter Worker ID: " << Dashboard::WHITE;
    int worker_id = Dashboard::readInt();

    /* Find the worker */
    int worker_idx = -1;
    for (int w = 0; w < worker_count; w++) {
        if (workers[w].id == worker_id && workers[w].is_available) {
            worker_idx = w;
            break;
        }
    }

    if (worker_idx < 0) {
        Dashboard::printError("Worker not found or not available.");
        Dashboard::pressEnterToContinue();
        return;
    }

    /* Assign worker (shared workflow: validates department and availability, logs) */
    WfResult assign = wf_assign_worker(complaint_id, worker_id, username.c_str(), "");
    if (assign.code != WF_OK) {
        Dashboard::printError(assign.message);
        Dashboard::pressEnterToContinue();
        return;
    }

    /* Show route preview */
    RouteResult result;
    int src = workers[worker_idx].current_location_id;
    int dst = dept_complaints[comp_idx].location_id;

    if (dijkstra(&graph, src, dst, &result)) {
        Dashboard::printSubHeader("Route Preview");
        std::cout << Dashboard::BRIGHT_GREEN << "  ";
        for (int p = 0; p < result.path_length; p++) {
            std::cout << get_location_name(&graph, result.path[p]);
            if (p < result.path_length - 1) {
                std::cout << Dashboard::CYAN << " → " << Dashboard::BRIGHT_GREEN;
            }
        }
        std::cout << std::endl;
        std::cout << Dashboard::YELLOW << "  Distance: " << result.total_distance << " km"
                  << Dashboard::RESET << std::endl;
    }

    Dashboard::printSuccess("Worker " + std::string(workers[worker_idx].full_name) +
                           " assigned to Complaint #" + std::to_string(complaint_id));
    Dashboard::pressEnterToContinue();
}

/* ─── View Worker Status ─── */
void DepartmentOfficer::viewWorkerStatus() {
    Dashboard::clearScreen();
    Dashboard::printHeader(department + " - Workers");

    UserRecord workers[MAX_WORKERS];
    int count = load_workers_by_department(department.c_str(), workers, MAX_WORKERS);

    CityGraph graph;
    init_city_graph(&graph);

    for (int i = 0; i < count; i++) {
        std::string avail = workers[i].is_available ? "Available" : "Busy";
        std::string avail_color = workers[i].is_available ? Dashboard::GREEN : Dashboard::RED;

        std::cout << Dashboard::CYAN << "  ┌──────────────────────────────────────────┐" << Dashboard::RESET << std::endl;
        std::cout << Dashboard::CYAN << "  │ " << Dashboard::BOLD << Dashboard::WHITE
                  << workers[i].full_name << Dashboard::RESET << std::endl;
        std::cout << Dashboard::CYAN << "  │ " << Dashboard::GRAY << "Username : "
                  << Dashboard::WHITE << workers[i].username << Dashboard::RESET << std::endl;
        std::cout << Dashboard::CYAN << "  │ " << Dashboard::GRAY << "Location : "
                  << Dashboard::WHITE << get_location_name(&graph, workers[i].current_location_id)
                  << Dashboard::RESET << std::endl;
        std::cout << Dashboard::CYAN << "  │ " << Dashboard::GRAY << "Status   : "
                  << avail_color << avail << Dashboard::RESET << std::endl;

        /* Count assigned tasks */
        std::vector<Complaint> worker_tasks(MAX_COMPLAINTS);  /* heap: fine inside a loop */
        int task_count = load_complaints_by_worker(workers[i].id, worker_tasks.data(), MAX_COMPLAINTS);
        int active = 0;
        for (int t = 0; t < task_count; t++) {
            if (strcmp(worker_tasks[t].status, "COMPLETED") != 0) active++;
        }
        std::cout << Dashboard::CYAN << "  │ " << Dashboard::GRAY << "Active   : "
                  << Dashboard::YELLOW << active << " task(s)" << Dashboard::RESET << std::endl;
        std::cout << Dashboard::CYAN << "  └──────────────────────────────────────────┘" << Dashboard::RESET << std::endl;
    }

    if (count == 0) {
        Dashboard::printInfo("No workers in this department.");
    }

    Dashboard::pressEnterToContinue();
}

/* ─── View Department Statistics ─── */
void DepartmentOfficer::viewDepartmentStats() {
    Dashboard::clearScreen();
    Dashboard::printHeader(department + " - Statistics");

    ComplaintStats stats = get_department_stats(department.c_str());
    Dashboard::printStats(stats);

    Dashboard::pressEnterToContinue();
}

