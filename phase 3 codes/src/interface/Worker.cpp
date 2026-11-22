/*
 * Worker.cpp
 * Field worker role — views assigned tasks, gets Dijkstra routes, completes tasks.
 */

#include "../../include/interface/Worker.h"
#include "../../include/interface/Dashboard.h"
#include <iostream>
#include <iomanip>
#include <cstring>
#include <limits>

Worker::Worker(const UserRecord& record) : Person(record) {}

/* The session keeps the record read at login, but the website or an
 * officer can change it meanwhile (the route used the file's location
 * while the screen showed the old one), so re-read it before using it. */
static void reloadRecord(Worker* worker) {
    UserRecord fresh;
    if (find_user_by_id(worker->getId(), &fresh)) {
        worker->setCurrentLocationId(fresh.current_location_id);
        worker->setIsAvailable(fresh.is_available);
    }
}

/* ─── Dashboard ─── */
void Worker::showDashboard() {
    reloadRecord(this);
    Dashboard::printHeader("Worker Dashboard - " + fullName);

    CityGraph graph;
    init_city_graph(&graph);

    std::cout << Dashboard::CYAN << "  Department : " << Dashboard::YELLOW << department << Dashboard::RESET << std::endl;
    std::cout << Dashboard::CYAN << "  Location   : " << Dashboard::WHITE
              << get_location_name(&graph, currentLocationId) << Dashboard::RESET << std::endl;
    std::cout << Dashboard::CYAN << "  Status     : "
              << (isAvailable ? Dashboard::GREEN + "Available" : Dashboard::RED + "Busy")
              << Dashboard::RESET << std::endl;

    /* Show assigned task count */
    std::vector<Complaint> complaints(MAX_COMPLAINTS);
    int count = load_complaints_by_worker(id, complaints.data(), MAX_COMPLAINTS);

    int active = 0, completed = 0;
    for (int i = 0; i < count; i++) {
        if (strcmp(complaints[i].status, "COMPLETED") == 0) completed++;
        else active++;
    }

    std::cout << std::endl;
    std::cout << Dashboard::WHITE << "  Active Tasks    : " << Dashboard::BOLD
              << Dashboard::BRIGHT_YELLOW << active << Dashboard::RESET << std::endl;
    std::cout << Dashboard::WHITE << "  Completed Tasks : " << Dashboard::BOLD
              << Dashboard::GREEN << completed << Dashboard::RESET << std::endl;
    Dashboard::printSeparator();
}

/* ─── Menu ─── */
void Worker::showMenu() {
    int choice = 0;

    do {
        Dashboard::clearScreen();
        showDashboard();

        std::cout << Dashboard::BOLD << Dashboard::BRIGHT_CYAN
                  << "  ┌─── Worker Menu ──────────────────────────────────────┐"
                  << Dashboard::RESET << std::endl;
        std::cout << Dashboard::CYAN << "  │" << Dashboard::BRIGHT_WHITE << "  1. " << Dashboard::WHITE << "View Assigned Tasks                              " << Dashboard::CYAN << "│" << Dashboard::RESET << std::endl;
        std::cout << Dashboard::CYAN << "  │" << Dashboard::BRIGHT_WHITE << "  2. " << Dashboard::WHITE << "Get Optimized Route (Dijkstra)                   " << Dashboard::CYAN << "│" << Dashboard::RESET << std::endl;
        std::cout << Dashboard::CYAN << "  │" << Dashboard::BRIGHT_WHITE << "  3. " << Dashboard::WHITE << "Mark Task as Complete                            " << Dashboard::CYAN << "│" << Dashboard::RESET << std::endl;
        std::cout << Dashboard::CYAN << "  │" << Dashboard::BRIGHT_WHITE << "  4. " << Dashboard::WHITE << "Update My Location                               " << Dashboard::CYAN << "│" << Dashboard::RESET << std::endl;
        std::cout << Dashboard::CYAN << "  │" << Dashboard::BRIGHT_WHITE << "  5. " << Dashboard::WHITE << "View Profile                                     " << Dashboard::CYAN << "│" << Dashboard::RESET << std::endl;
        std::cout << Dashboard::CYAN << "  │" << Dashboard::BRIGHT_WHITE << "  6. " << Dashboard::WHITE << "Switch Available / Busy                          " << Dashboard::CYAN << "│" << Dashboard::RESET << std::endl;
        std::cout << Dashboard::CYAN << "  │" << Dashboard::BRIGHT_RED   << "  0. " << Dashboard::RED   << "Logout                                           " << Dashboard::CYAN << "│" << Dashboard::RESET << std::endl;
        std::cout << Dashboard::BOLD << Dashboard::BRIGHT_CYAN
                  << "  └──────────────────────────────────────────────────────────┘"
                  << Dashboard::RESET << std::endl;

        std::cout << std::endl << Dashboard::BRIGHT_WHITE << "  Enter choice: " << Dashboard::RESET;
        choice = Dashboard::readInt(-1, 0);  /* invalid: -1; input closed: 0 = log out */

        switch (choice) {
            case 1: viewAssignedTasks(); break;
            case 2: getOptimizedRoute(); break;
            case 3: markTaskComplete(); break;
            case 4: updateLocation(); break;
            case 5:
                Dashboard::clearScreen();
                showProfile();
                Dashboard::pressEnterToContinue();
                break;
            case 6: toggleAvailability(); break;
            case 0: break;
            default:
                Dashboard::printError("Invalid choice.");
                Dashboard::pressEnterToContinue();
        }
    } while (choice != 0);
}

/* ─── View Assigned Tasks ─── */
void Worker::viewAssignedTasks() {
    Dashboard::clearScreen();
    Dashboard::printHeader("My Assigned Tasks");

    std::vector<Complaint> complaints(MAX_COMPLAINTS);
    int count = load_complaints_by_worker(id, complaints.data(), MAX_COMPLAINTS);

    if (count == 0) {
        Dashboard::printInfo("No tasks assigned to you currently.");
        Dashboard::pressEnterToContinue();
        return;
    }

    /* Show active tasks first, then completed */
    Dashboard::printSubHeader("Active Tasks");
    int active = 0;
    for (int i = 0; i < count; i++) {
        if (strcmp(complaints[i].status, "COMPLETED") != 0) {
            Dashboard::printComplaintCard(complaints[i]);
            active++;
        }
    }
    if (active == 0) {
        Dashboard::printInfo("No active tasks.");
    }

    Dashboard::printSubHeader("Completed Tasks");
    int completed = 0;
    for (int i = 0; i < count; i++) {
        if (strcmp(complaints[i].status, "COMPLETED") == 0) {
            std::cout << Dashboard::GREEN << "  ✓ Complaint #" << complaints[i].id
                      << ": " << complaints[i].title
                      << " (completed: " << complaints[i].completed_date << ")"
                      << Dashboard::RESET << std::endl;
            completed++;
        }
    }
    if (completed == 0) {
        Dashboard::printInfo("No completed tasks yet.");
    }

    Dashboard::pressEnterToContinue();
}

/* ─── Get Optimized Route ─── */
void Worker::getOptimizedRoute() {
    Dashboard::clearScreen();
    Dashboard::printHeader("Optimized Route - Dijkstra's Algorithm");
    reloadRecord(this);

    CityGraph graph;
    init_city_graph(&graph);

    std::cout << Dashboard::CYAN << "  Your current location: " << Dashboard::WHITE
              << get_location_name(&graph, currentLocationId) << Dashboard::RESET << std::endl;

    /* Show assigned tasks with locations */
    std::vector<Complaint> complaints(MAX_COMPLAINTS);
    int count = load_complaints_by_worker(id, complaints.data(), MAX_COMPLAINTS);

    int active_count = 0;
    std::cout << std::endl;
    Dashboard::printSubHeader("Active Task Locations");
    for (int i = 0; i < count; i++) {
        if (strcmp(complaints[i].status, "COMPLETED") != 0) {
            std::cout << Dashboard::CYAN << "  [" << complaints[i].id << "] "
                      << Dashboard::WHITE << complaints[i].title
                      << Dashboard::YELLOW << " @ " << complaints[i].location
                      << " (ID: " << complaints[i].location_id << ")"
                      << Dashboard::RESET << std::endl;
            active_count++;
        }
    }

    if (active_count == 0) {
        Dashboard::printInfo("No active tasks to route to.");
        Dashboard::pressEnterToContinue();
        return;
    }

    std::cout << std::endl;
    std::cout << Dashboard::CYAN << "  Enter Complaint ID to route to: " << Dashboard::WHITE;
    int target_id = Dashboard::readInt();

    /* Find the complaint */
    for (int i = 0; i < count; i++) {
        if (complaints[i].id == target_id && strcmp(complaints[i].status, "COMPLETED") != 0) {
            RouteResult result;
            /* Marks the task IN_PROGRESS and computes the route (shared workflow) */
            WfResult started = wf_start_task(target_id, id, "", &result);
            if (started.code == WF_OK) {
                std::cout << std::endl;
                Dashboard::printSubHeader("Shortest Route Found");

                std::cout << Dashboard::BRIGHT_GREEN << "  ";
                for (int p = 0; p < result.path_length; p++) {
                    std::cout << get_location_name(&graph, result.path[p]);
                    if (p < result.path_length - 1) {
                        std::cout << Dashboard::CYAN << " → " << Dashboard::BRIGHT_GREEN;
                    }
                }
                std::cout << Dashboard::RESET << std::endl;

                std::cout << std::endl;
                std::cout << Dashboard::BOLD << Dashboard::YELLOW
                          << "  Total Distance: " << result.total_distance << " km"
                          << Dashboard::RESET << std::endl;

                /* Show step-by-step distances */
                Dashboard::printSubHeader("Step-by-Step");
                for (int p = 0; p < result.path_length - 1; p++) {
                    int from = result.path[p];
                    int to = result.path[p + 1];
                    std::cout << Dashboard::WHITE << "  " << (p + 1) << ". "
                              << get_location_name(&graph, from)
                              << Dashboard::CYAN << " → "
                              << Dashboard::WHITE << get_location_name(&graph, to)
                              << Dashboard::YELLOW << " (" << graph.adj_matrix[from][to] << " km)"
                              << Dashboard::RESET << std::endl;
                }

                Dashboard::printSuccess(started.message);
            } else {
                Dashboard::printError(started.message);
            }

            Dashboard::pressEnterToContinue();
            return;
        }
    }

    Dashboard::printError("Complaint not found in your assigned tasks.");
    Dashboard::pressEnterToContinue();
}

/* ─── Mark Task as Complete ─── */
void Worker::markTaskComplete() {
    Dashboard::clearScreen();
    Dashboard::printHeader("Mark Task Complete");

    std::vector<Complaint> complaints(MAX_COMPLAINTS);
    int count = load_complaints_by_worker(id, complaints.data(), MAX_COMPLAINTS);

    /* Only started tasks can be completed: the workflow is
     * ASSIGNED -> IN_PROGRESS (Get Route starts a task) -> COMPLETED */
    int active = 0;
    for (int i = 0; i < count; i++) {
        if (strcmp(complaints[i].status, "IN_PROGRESS") == 0) {
            std::cout << Dashboard::CYAN << "  [" << complaints[i].id << "] "
                      << Dashboard::WHITE << complaints[i].title
                      << Dashboard::YELLOW << " - " << complaints[i].status
                      << Dashboard::RESET << std::endl;
            active++;
        }
    }

    if (active == 0) {
        Dashboard::printInfo("No task in progress. Start one first with 'Get Optimized Route'.");
        Dashboard::pressEnterToContinue();
        return;
    }

    std::cout << std::endl;
    std::cout << Dashboard::CYAN << "  Enter Complaint ID to mark complete: " << Dashboard::WHITE;
    int target_id = Dashboard::readInt();

    for (int i = 0; i < count; i++) {
        if (complaints[i].id == target_id && strcmp(complaints[i].status, "IN_PROGRESS") == 0) {
            WfResult result = wf_complete_task(target_id, id, "");
            if (result.code == WF_OK) Dashboard::printSuccess(result.message);
            else Dashboard::printError(result.message);
            Dashboard::pressEnterToContinue();
            return;
        }
    }

    Dashboard::printError("Complaint not found among your tasks in progress.");
    Dashboard::pressEnterToContinue();
}

/* ─── Update Location ─── */
void Worker::updateLocation() {
    Dashboard::clearScreen();
    Dashboard::printHeader("Update My Location");

    CityGraph graph;
    init_city_graph(&graph);

    std::cout << Dashboard::CYAN << "  Current: " << Dashboard::WHITE
              << get_location_name(&graph, currentLocationId) << Dashboard::RESET << std::endl;
    std::cout << std::endl;

    Dashboard::printSubHeader("Available Locations");
    for (int i = 0; i < graph.num_locations; i++) {
        std::cout << Dashboard::CYAN << "    " << std::setw(2) << i << ". "
                  << Dashboard::WHITE << graph.locations[i].name << Dashboard::RESET << std::endl;
    }

    std::cout << std::endl << Dashboard::CYAN << "  New Location ID (0-"
              << graph.num_locations - 1 << "): " << Dashboard::WHITE;
    /* -1 for both bad input and closed input: 0 is a real place (Clock
     * Tower), and closed input used to move the worker there */
    int new_loc = Dashboard::readInt(-1, -1);
    if (new_loc < 0 || new_loc >= graph.num_locations) {
        Dashboard::printError("Location not changed: please enter a number from the list.");
        Dashboard::pressEnterToContinue();
        return;
    }

    WfResult result = wf_set_location(id, new_loc, "");
    if (result.code == WF_OK) {
        currentLocationId = new_loc;
        Dashboard::printSuccess(result.message);
    } else {
        Dashboard::printError(result.message);
    }

    Dashboard::pressEnterToContinue();
}

/* ─── Switch Available / Busy ───
 * Availability is the worker's own switch: officers can only assign new
 * tasks to available workers. (It used to be set nowhere, so every worker
 * always showed "Available", whatever they were doing.) */
void Worker::toggleAvailability() {
    Dashboard::clearScreen();
    Dashboard::printHeader("Availability");
    reloadRecord(this);

    WfResult result = wf_set_availability(id, isAvailable ? 0 : 1, "");
    if (result.code == WF_OK) {
        isAvailable = !isAvailable;
        Dashboard::printSuccess(result.message);
    } else {
        Dashboard::printError(result.message);
    }
    Dashboard::pressEnterToContinue();
}

