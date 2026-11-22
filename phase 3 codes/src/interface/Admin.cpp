/*
 * Admin.cpp
 * System administrator role — reviews, escalates, assigns, and manages.
 */

#include "../../include/interface/Admin.h"
#include "../../include/interface/Dashboard.h"
#include <iostream>
#include <iomanip>
#include <cstring>
#include <limits>

Admin::Admin(const UserRecord& record) : Person(record) {}

/* ─── Dashboard ─── */
void Admin::showDashboard() {
    Dashboard::printHeader("Admin Dashboard - " + fullName);

    ComplaintStats stats = get_complaint_stats();
    Dashboard::printStats(stats);
}

/* ─── Menu ─── */
void Admin::showMenu() {
    int choice = 0;

    do {
        Dashboard::clearScreen();
        showDashboard();

        std::cout << Dashboard::BOLD << Dashboard::BRIGHT_CYAN
                  << "  ┌─── Admin Menu ────────────────────────────────────────┐"
                  << Dashboard::RESET << std::endl;
        std::cout << Dashboard::CYAN << "  │" << Dashboard::BRIGHT_WHITE << "  1. " << Dashboard::WHITE << "Review Pending Complaints (Priority Queue)       " << Dashboard::CYAN << "│" << Dashboard::RESET << std::endl;
        std::cout << Dashboard::CYAN << "  │" << Dashboard::BRIGHT_WHITE << "  2. " << Dashboard::WHITE << "View All Complaints                              " << Dashboard::CYAN << "│" << Dashboard::RESET << std::endl;
        std::cout << Dashboard::CYAN << "  │" << Dashboard::BRIGHT_WHITE << "  3. " << Dashboard::WHITE << "Escalate Complaint Severity                      " << Dashboard::CYAN << "│" << Dashboard::RESET << std::endl;
        std::cout << Dashboard::CYAN << "  │" << Dashboard::BRIGHT_WHITE << "  4. " << Dashboard::WHITE << "Assign Complaint to Department                   " << Dashboard::CYAN << "│" << Dashboard::RESET << std::endl;
        std::cout << Dashboard::CYAN << "  │" << Dashboard::BRIGHT_WHITE << "  5. " << Dashboard::WHITE << "View Statistics                                  " << Dashboard::CYAN << "│" << Dashboard::RESET << std::endl;
        std::cout << Dashboard::CYAN << "  │" << Dashboard::BRIGHT_WHITE << "  6. " << Dashboard::WHITE << "Manage Departments                               " << Dashboard::CYAN << "│" << Dashboard::RESET << std::endl;
        std::cout << Dashboard::CYAN << "  │" << Dashboard::BRIGHT_WHITE << "  7. " << Dashboard::WHITE << "Manage Workers                                   " << Dashboard::CYAN << "│" << Dashboard::RESET << std::endl;
        std::cout << Dashboard::CYAN << "  │" << Dashboard::BRIGHT_WHITE << "  8. " << Dashboard::WHITE << "View City Map                                    " << Dashboard::CYAN << "│" << Dashboard::RESET << std::endl;
        std::cout << Dashboard::CYAN << "  │" << Dashboard::BRIGHT_WHITE << "  9. " << Dashboard::WHITE << "View Profile                                     " << Dashboard::CYAN << "│" << Dashboard::RESET << std::endl;
        std::cout << Dashboard::CYAN << "  │" << Dashboard::BRIGHT_RED   << "  0. " << Dashboard::RED   << "Logout                                           " << Dashboard::CYAN << "│" << Dashboard::RESET << std::endl;
        std::cout << Dashboard::BOLD << Dashboard::BRIGHT_CYAN
                  << "  └──────────────────────────────────────────────────────────┘"
                  << Dashboard::RESET << std::endl;

        std::cout << std::endl << Dashboard::BRIGHT_WHITE << "  Enter choice: " << Dashboard::RESET;
        choice = Dashboard::readInt(-1, 0);  /* invalid: -1; input closed: 0 = log out */

        switch (choice) {
            case 1: reviewComplaints(); break;
            case 2: viewAllComplaints(); break;
            case 3: escalateComplaint(); break;
            case 4: assignToDepartment(); break;
            case 5: viewStatistics(); break;
            case 6: manageDepartments(); break;
            case 7: manageWorkers(); break;
            case 8: viewCityMap(); break;
            case 9:
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

/* ─── Review Complaints (Priority Queue) ─── */
void Admin::reviewComplaints() {
    Dashboard::clearScreen();
    Dashboard::printHeader("Review Complaints - Priority Queue");

    /* Load all pending complaints into priority queue */
    std::vector<Complaint> all(MAX_COMPLAINTS);
    int total = load_complaints(all.data(), MAX_COMPLAINTS);

    PriorityQueue pq;
    pq_init(&pq, MAX_COMPLAINTS);

    int pending_count = 0;
    for (int i = 0; i < total; i++) {
        if (strcmp(all[i].status, "PENDING") == 0) {
            pq_insert(&pq, all[i]);
            pending_count++;
        }
    }

    if (pending_count == 0) {
        Dashboard::printInfo("No pending complaints to review.");
        pq_free(&pq);
        Dashboard::pressEnterToContinue();
        return;
    }

    Dashboard::printInfo("Showing " + std::to_string(pending_count) +
                        " pending complaints (highest severity first):");
    std::cout << std::endl;

    /* Get sorted complaints */
    std::vector<Complaint> sorted(MAX_COMPLAINTS);
    int sorted_count = pq_get_sorted(&pq, sorted.data(), pending_count);

    for (int i = 0; i < sorted_count; i++) {
        Dashboard::printComplaintCard(sorted[i]);

        std::cout << Dashboard::CYAN << "  Review this complaint? (y/n/q to quit): "
                  << Dashboard::WHITE;
        /* One whole line per answer: "yes" used to answer this complaint
         * with 'y' and then the next two with 'e' and 's' */
        char review = Dashboard::readChoice('n', 'q');  /* Enter = skip; input closed = quit */

        if (review == 'q') break;

        if (review == 'y') {
            /* Mark as reviewed (shared workflow) */
            WfResult result = wf_review(sorted[i].id, username.c_str(), "");
            if (result.code == WF_OK) Dashboard::printSuccess(result.message);
            else Dashboard::printError(result.message);
        }
        std::cout << std::endl;
    }

    pq_free(&pq);
    Dashboard::pressEnterToContinue();
}

/* ─── View All Complaints ─── */
void Admin::viewAllComplaints() {
    Dashboard::clearScreen();
    Dashboard::printHeader("All Complaints");

    std::vector<Complaint> complaints(MAX_COMPLAINTS);
    int count = load_complaints(complaints.data(), MAX_COMPLAINTS);

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

/* ─── Escalate Complaint Severity ─── */
void Admin::escalateComplaint() {
    Dashboard::clearScreen();
    Dashboard::printHeader("Escalate Complaint Severity");

    std::cout << Dashboard::CYAN << "  Enter Complaint ID: " << Dashboard::WHITE;
    int complaint_id = Dashboard::readInt();

    std::vector<Complaint> complaints(MAX_COMPLAINTS);
    int count = load_complaints(complaints.data(), MAX_COMPLAINTS);

    for (int i = 0; i < count; i++) {
        if (complaints[i].id == complaint_id) {
            Dashboard::printComplaintCard(complaints[i]);

            std::cout << Dashboard::CYAN << "  Current Severity: "
                      << Dashboard::getSeverityColor(complaints[i].severity)
                      << complaints[i].severity << "/10" << Dashboard::RESET << std::endl;
            std::cout << Dashboard::CYAN << "  New Severity (1-10): " << Dashboard::WHITE;

            int new_sev = Dashboard::readInt();

            WfResult result = wf_escalate(complaint_id, new_sev, username.c_str(), "");
            if (result.code == WF_OK) Dashboard::printSuccess(result.message);
            else Dashboard::printError(result.message);

            Dashboard::pressEnterToContinue();
            return;
        }
    }

    Dashboard::printError("Complaint not found.");
    Dashboard::pressEnterToContinue();
}

/* ─── Assign to Department ─── */
void Admin::assignToDepartment() {
    Dashboard::clearScreen();
    Dashboard::printHeader("Assign Complaint to Department");

    std::cout << Dashboard::CYAN << "  Enter Complaint ID: " << Dashboard::WHITE;
    int complaint_id = Dashboard::readInt();

    std::vector<Complaint> complaints(MAX_COMPLAINTS);
    int count = load_complaints(complaints.data(), MAX_COMPLAINTS);

    for (int i = 0; i < count; i++) {
        if (complaints[i].id == complaint_id) {
            Dashboard::printComplaintCard(complaints[i]);

            /* Show departments */
            Department depts[MAX_DEPARTMENTS];
            int dept_count = load_departments(depts, MAX_DEPARTMENTS);

            Dashboard::printSubHeader("Available Departments");
            for (int d = 0; d < dept_count; d++) {
                std::cout << Dashboard::CYAN << "    " << (d + 1) << ". "
                          << Dashboard::WHITE << depts[d].name
                          << Dashboard::GRAY << " (Officer: " << depts[d].officer_username << ")"
                          << Dashboard::RESET << std::endl;
            }

            std::cout << std::endl << Dashboard::CYAN << "  Select department (1-"
                      << dept_count << "): " << Dashboard::WHITE;
            int dept_choice = Dashboard::readInt();

            if (dept_choice >= 1 && dept_choice <= dept_count) {
                /* Sets department + category, status and counters (shared workflow) */
                WfResult result = wf_assign_department(complaint_id, depts[dept_choice - 1].id,
                                                       username.c_str(), "");
                if (result.code == WF_OK) Dashboard::printSuccess(result.message);
                else Dashboard::printError(result.message);
            } else {
                Dashboard::printError("Invalid department selection.");
            }

            Dashboard::pressEnterToContinue();
            return;
        }
    }

    Dashboard::printError("Complaint not found.");
    Dashboard::pressEnterToContinue();
}

/* ─── View Statistics ─── */
void Admin::viewStatistics() {
    Dashboard::clearScreen();
    Dashboard::printHeader("System Statistics");

    ComplaintStats overall = get_complaint_stats();
    Dashboard::printStats(overall);

    /* Per-department stats */
    Department depts[MAX_DEPARTMENTS];
    int dept_count = load_departments(depts, MAX_DEPARTMENTS);

    for (int d = 0; d < dept_count; d++) {
        ComplaintStats dept_stats = get_department_stats(depts[d].name);
        if (dept_stats.total > 0) {
            Dashboard::printSubHeader(std::string(depts[d].name));
            Dashboard::printStats(dept_stats);
        }
    }

    Dashboard::pressEnterToContinue();
}

/* ─── Manage Departments ─── */
void Admin::manageDepartments() {
    Dashboard::clearScreen();
    Dashboard::printHeader("Department Management");

    refresh_department_counts();  /* counters always reflect complaints.txt */
    Department depts[MAX_DEPARTMENTS];
    int dept_count = load_departments(depts, MAX_DEPARTMENTS);

    std::cout << std::endl;
    std::cout << Dashboard::BOLD << Dashboard::CYAN
              << "  ┌──────┬─────────────────────┬──────────────────┬───────────┬──────────┐" << Dashboard::RESET << std::endl;
    std::cout << Dashboard::BOLD << Dashboard::CYAN
              << "  │  ID  │ Department           │ Officer          │ Complaints│ Resolved │" << Dashboard::RESET << std::endl;
    std::cout << Dashboard::BOLD << Dashboard::CYAN
              << "  ├──────┼─────────────────────┼──────────────────┼───────────┼──────────┤" << Dashboard::RESET << std::endl;

    for (int i = 0; i < dept_count; i++) {
        std::string name(depts[i].name);
        if (name.length() > 19) name = name.substr(0, 16) + "...";
        std::string officer(depts[i].officer_username);
        if (officer.length() > 16) officer = officer.substr(0, 13) + "...";

        std::cout << Dashboard::CYAN << "  │ " << Dashboard::WHITE << std::setw(4) << depts[i].id
                  << Dashboard::CYAN << " │ " << Dashboard::YELLOW << std::setw(19) << std::left << name
                  << Dashboard::CYAN << " │ " << Dashboard::WHITE << std::setw(16) << std::left << officer
                  << Dashboard::CYAN << " │ " << Dashboard::WHITE << std::setw(9) << std::right << depts[i].complaint_count
                  << Dashboard::CYAN << " │ " << Dashboard::GREEN << std::setw(8) << std::right << depts[i].resolved_count
                  << Dashboard::CYAN << " │" << Dashboard::RESET << std::endl;
    }

    std::cout << Dashboard::BOLD << Dashboard::CYAN
              << "  └──────┴─────────────────────┴──────────────────┴───────────┴──────────┘" << Dashboard::RESET << std::endl;

    Dashboard::pressEnterToContinue();
}

/* ─── Manage Workers ─── */
void Admin::manageWorkers() {
    Dashboard::clearScreen();
    Dashboard::printHeader("Worker Management");

    UserRecord all_users[MAX_USERS];
    int total = load_users(all_users, MAX_USERS);

    CityGraph graph;
    init_city_graph(&graph);

    std::cout << std::endl;
    std::cout << Dashboard::BOLD << Dashboard::CYAN
              << "  ┌──────┬──────────────────┬──────────────────┬─────────────────┬───────────┐" << Dashboard::RESET << std::endl;
    std::cout << Dashboard::BOLD << Dashboard::CYAN
              << "  │  ID  │ Username         │ Department       │ Location        │ Available │" << Dashboard::RESET << std::endl;
    std::cout << Dashboard::BOLD << Dashboard::CYAN
              << "  ├──────┼──────────────────┼──────────────────┼─────────────────┼───────────┤" << Dashboard::RESET << std::endl;

    for (int i = 0; i < total; i++) {
        if (strcmp(all_users[i].role, "WORKER") != 0) continue;

        std::string uname(all_users[i].username);
        if (uname.length() > 16) uname = uname.substr(0, 13) + "...";
        std::string dept(all_users[i].department);
        if (dept.length() > 16) dept = dept.substr(0, 13) + "...";
        std::string loc(get_location_name(&graph, all_users[i].current_location_id));
        if (loc.length() > 15) loc = loc.substr(0, 12) + "...";

        std::string avail = all_users[i].is_available ? "Yes" : "No";
        std::string avail_color = all_users[i].is_available ? Dashboard::GREEN : Dashboard::RED;

        std::cout << Dashboard::CYAN << "  │ " << Dashboard::WHITE << std::setw(4) << all_users[i].id
                  << Dashboard::CYAN << " │ " << Dashboard::WHITE << std::setw(16) << std::left << uname
                  << Dashboard::CYAN << " │ " << Dashboard::YELLOW << std::setw(16) << std::left << dept
                  << Dashboard::CYAN << " │ " << Dashboard::WHITE << std::setw(15) << std::left << loc
                  << Dashboard::CYAN << " │ " << avail_color << std::setw(9) << std::left << avail
                  << Dashboard::CYAN << " │" << Dashboard::RESET << std::endl;
    }

    std::cout << Dashboard::BOLD << Dashboard::CYAN
              << "  └──────┴──────────────────┴──────────────────┴─────────────────┴───────────┘" << Dashboard::RESET << std::endl;

    Dashboard::pressEnterToContinue();
}

/* ─── View City Map ─── */
void Admin::viewCityMap() {
    Dashboard::clearScreen();
    Dashboard::printHeader("Dehradun City Map");

    CityGraph graph;
    init_city_graph(&graph);

    /* Print all locations */
    Dashboard::printSubHeader("Locations");
    for (int i = 0; i < graph.num_locations; i++) {
        std::cout << Dashboard::CYAN << "    [" << std::setw(2) << i << "] "
                  << Dashboard::WHITE << graph.locations[i].name << Dashboard::RESET << std::endl;
    }

    /* Print adjacency information */
    Dashboard::printSubHeader("Road Connections");
    for (int i = 0; i < graph.num_locations; i++) {
        for (int j = i + 1; j < graph.num_locations; j++) {
            if (graph.adj_matrix[i][j] > 0) {
                std::cout << Dashboard::WHITE << "    " << graph.locations[i].name
                          << Dashboard::CYAN << " ←→ "
                          << Dashboard::WHITE << graph.locations[j].name
                          << Dashboard::YELLOW << " (" << graph.adj_matrix[i][j] << " km)"
                          << Dashboard::RESET << std::endl;
            }
        }
    }

    /* Test a route */
    std::cout << std::endl;
    std::cout << Dashboard::CYAN << "  Test route? Enter source ID (-1 to skip): " << Dashboard::WHITE;
    int src = Dashboard::readInt();

    if (src >= 0 && src < graph.num_locations) {
        std::cout << Dashboard::CYAN << "  Enter destination ID: " << Dashboard::WHITE;
        int dst = Dashboard::readInt();

        if (dst >= 0 && dst < graph.num_locations) {
            RouteResult result;
            if (dijkstra(&graph, src, dst, &result)) {
                Dashboard::printSubHeader("Shortest Route");
                print_route(&graph, &result, src, dst);
            } else {
                Dashboard::printError("No route found.");
            }
        }
    }

    Dashboard::pressEnterToContinue();
}

