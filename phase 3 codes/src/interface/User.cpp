/*
 * User.cpp
 * Regular citizen role implementation — submit, view, and track complaints.
 */

#include "../../include/interface/User.h"
#include "../../include/interface/Dashboard.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <cstring>
#include <limits>

User::User(const UserRecord& record) : Person(record) {}

User::User(int id, const std::string& username, const std::string& fullName)
    : Person(id, username, fullName, "USER") {}

/* ─── Dashboard ─── */
void User::showDashboard() {
    Dashboard::printHeader("User Dashboard - " + fullName);

    /* Show user's complaint summary */
    std::vector<Complaint> complaints(MAX_COMPLAINTS);
    int count = load_complaints_by_user(id, complaints.data(), MAX_COMPLAINTS);

    int pending = 0, in_progress = 0, completed = 0;
    for (int i = 0; i < count; i++) {
        if (strcmp(complaints[i].status, "PENDING") == 0 ||
            strcmp(complaints[i].status, "REVIEWED") == 0) pending++;
        else if (strcmp(complaints[i].status, "ASSIGNED") == 0 ||
                 strcmp(complaints[i].status, "IN_PROGRESS") == 0) in_progress++;
        else if (strcmp(complaints[i].status, "COMPLETED") == 0) completed++;
    }

    std::cout << Dashboard::BOLD << Dashboard::WHITE
              << "  Your Complaints Summary:" << Dashboard::RESET << std::endl;
    std::cout << Dashboard::YELLOW << "  ● Pending      : " << pending << Dashboard::RESET << std::endl;
    std::cout << Dashboard::CYAN   << "  ● In Progress  : " << in_progress << Dashboard::RESET << std::endl;
    std::cout << Dashboard::GREEN  << "  ● Completed    : " << completed << Dashboard::RESET << std::endl;
    std::cout << Dashboard::WHITE  << "  ● Total        : " << count << Dashboard::RESET << std::endl;
    Dashboard::printSeparator();
}

/* ─── Menu ─── */
void User::showMenu() {
    int choice = 0;

    do {
        Dashboard::clearScreen();
        showDashboard();

        std::cout << Dashboard::BOLD << Dashboard::BRIGHT_CYAN
                  << "  ┌─── Menu ───────────────────────────────────────────────┐"
                  << Dashboard::RESET << std::endl;
        std::cout << Dashboard::CYAN  << "  │" << Dashboard::BRIGHT_WHITE << "  1. " << Dashboard::WHITE << "Submit New Complaint                              " << Dashboard::CYAN << "│" << Dashboard::RESET << std::endl;
        std::cout << Dashboard::CYAN  << "  │" << Dashboard::BRIGHT_WHITE << "  2. " << Dashboard::WHITE << "View My Complaints                                " << Dashboard::CYAN << "│" << Dashboard::RESET << std::endl;
        std::cout << Dashboard::CYAN  << "  │" << Dashboard::BRIGHT_WHITE << "  3. " << Dashboard::WHITE << "Track a Complaint                                 " << Dashboard::CYAN << "│" << Dashboard::RESET << std::endl;
        std::cout << Dashboard::CYAN  << "  │" << Dashboard::BRIGHT_WHITE << "  4. " << Dashboard::WHITE << "View Profile                                      " << Dashboard::CYAN << "│" << Dashboard::RESET << std::endl;
        std::cout << Dashboard::CYAN  << "  │" << Dashboard::BRIGHT_RED   << "  0. " << Dashboard::RED   << "Logout                                            " << Dashboard::CYAN << "│" << Dashboard::RESET << std::endl;
        std::cout << Dashboard::BOLD << Dashboard::BRIGHT_CYAN
                  << "  └──────────────────────────────────────────────────────────┘"
                  << Dashboard::RESET << std::endl;

        std::cout << std::endl << Dashboard::BRIGHT_WHITE << "  Enter choice: " << Dashboard::RESET;
        choice = Dashboard::readInt(-1, 0);  /* invalid: -1; input closed: 0 = log out */

        switch (choice) {
            case 1: submitComplaint(); break;
            case 2: viewMyComplaints(); break;
            case 3: trackComplaint(); break;
            case 4: viewProfile(); break;
            case 0: break;
            default:
                Dashboard::printError("Invalid choice. Please try again.");
                Dashboard::pressEnterToContinue();
        }
    } while (choice != 0);
}

/* ─── Submit Complaint ─── */
void User::submitComplaint() {
    Dashboard::clearScreen();
    Dashboard::printHeader("Submit New Complaint");

    Complaint complaint;
    memset(&complaint, 0, sizeof(Complaint));
    complaint.user_id = id;

    /* Get title and description. If input ends part-way (stdin closed),
     * nothing is saved: a half-typed complaint used to be submitted. */
    std::cout << Dashboard::CYAN << "  Title: " << Dashboard::WHITE;
    std::string title;
    bool haveTitle = Dashboard::readLine(title);

    std::cout << Dashboard::CYAN << "  Description: " << Dashboard::WHITE;
    std::string desc;
    if (!haveTitle || !Dashboard::readLine(desc)) {
        std::cout << std::endl;
        Dashboard::printError("Input ended - complaint not submitted.");
        return;
    }
    strncpy(complaint.title, title.c_str(), MAX_TITLE_LEN - 1);
    strncpy(complaint.description, desc.c_str(), MAX_DESC_LEN - 1);

    if (title.find_first_not_of(" \t") == std::string::npos ||
        desc.find_first_not_of(" \t") == std::string::npos) {
        Dashboard::printError("Title and description are both needed. Complaint not submitted.");
        Dashboard::pressEnterToContinue();
        return;
    }

    /* NLP Analysis (severity, spam check, category suggestion) */
    NLPAnalyzer analyzer;
    nlp_init(&analyzer);
    char suggested_category[MAX_CATEGORY_LEN];
    wf_analyze_draft(&analyzer, &complaint, suggested_category);

    if (complaint.is_irrelevant) {
        Dashboard::printWarning("Your complaint has been flagged as potentially irrelevant/spam.");
        Dashboard::printInfo("It will still be recorded but may be reviewed by an admin.");
    }

    int severity = complaint.severity;

    std::cout << std::endl;
    std::cout << Dashboard::BOLD << Dashboard::WHITE << "  NLP Analysis Result:" << Dashboard::RESET << std::endl;
    std::cout << "  Computed Severity: " << Dashboard::getSeverityColor(severity)
              << severity << "/10 (" << Dashboard::getSeverityLabel(severity) << ")"
              << Dashboard::RESET << std::endl;

    /* Category: always one of the five departments. When the analysis finds
     * no category it suggests "General", which no department handles (that
     * is how complaint #502 ended up as "General"), so the user must pick. */
    const char* cats[] = {"Water Supply", "Electricity", "Roads & Transport",
                          "Sanitation", "Public Safety"};
    bool suggestionUsable = false;
    for (int i = 0; i < 5; i++)
        if (strcmp(suggested_category, cats[i]) == 0) suggestionUsable = true;

    if (suggestionUsable) {
        std::cout << Dashboard::CYAN << "  Suggested Category: " << Dashboard::YELLOW
                  << suggested_category << Dashboard::RESET << std::endl;
        std::cout << Dashboard::CYAN << "  Accept suggestion? (Y/n): " << Dashboard::WHITE;
        char accept = Dashboard::readChoice('y', 'x');  /* Enter = yes */
        if (accept == 'x') {
            std::cout << std::endl;
            Dashboard::printError("Input ended - complaint not submitted.");
            nlp_free(&analyzer);
            return;
        }
        if (accept == 'y') strncpy(complaint.category, suggested_category, MAX_CATEGORY_LEN - 1);
    } else {
        Dashboard::printWarning("No category could be worked out from the text - please choose one.");
    }

    if (complaint.category[0] == '\0') {
        std::cout << std::endl;
        std::cout << Dashboard::CYAN << "  Available Categories:" << Dashboard::RESET << std::endl;
        for (int i = 0; i < 5; i++)
            std::cout << Dashboard::WHITE << "    " << i + 1 << ". " << cats[i] << std::endl;
        std::cout << Dashboard::RESET;

        int cat_choice = -1;
        while (cat_choice < 1 || cat_choice > 5) {
            std::cout << Dashboard::CYAN << "  Select (1-5): " << Dashboard::WHITE;
            cat_choice = Dashboard::readInt(-1, -2);
            if (cat_choice == -2) {
                std::cout << std::endl;
                Dashboard::printError("Input ended - complaint not submitted.");
                nlp_free(&analyzer);
                return;
            }
            if (cat_choice < 1 || cat_choice > 5) Dashboard::printError("Please enter a number from 1 to 5.");
        }
        strncpy(complaint.category, cats[cat_choice - 1], MAX_CATEGORY_LEN - 1);
    }

    /* Select location */
    CityGraph graph;
    init_city_graph(&graph);

    std::cout << std::endl;
    Dashboard::printSubHeader("Select Location");
    for (int i = 0; i < graph.num_locations; i++) {
        std::cout << Dashboard::CYAN << "    " << std::setw(2) << i << Dashboard::WHITE
                  << ". " << graph.locations[i].name << Dashboard::RESET << std::endl;
    }

    /* Ask again on a wrong number: it used to fall back to Clock Tower
     * silently, which put complaints at the wrong place on the map */
    int loc_id = -1;
    while (loc_id < 0 || loc_id >= graph.num_locations) {
        std::cout << std::endl << Dashboard::CYAN << "  Location ID (0-"
                  << graph.num_locations - 1 << "): " << Dashboard::WHITE;
        loc_id = Dashboard::readInt(-1, -2);
        if (loc_id == -2) {
            std::cout << std::endl;
            Dashboard::printError("Input ended - complaint not submitted.");
            nlp_free(&analyzer);
            return;
        }
        if (loc_id < 0 || loc_id >= graph.num_locations)
            Dashboard::printError("Please enter a location number from the list.");
    }
    complaint.location_id = loc_id;

    /* Duplicate check, ID, dates, save and log (shared workflow) */
    WfResult result = wf_finalize_submission(&complaint, username.c_str(), "", NULL, NULL);

    if (result.code == WF_OK) {
        if (complaint.is_duplicate) {
            Dashboard::printWarning("This complaint appears to be a DUPLICATE of Complaint #" +
                                   std::to_string(complaint.duplicate_of));
            Dashboard::printInfo("It will be flagged accordingly but still recorded.");
        }
        Dashboard::printSuccess(result.message);

        /* Show the complaint card */
        std::cout << std::endl;
        Dashboard::printComplaintCard(complaint);
    } else {
        Dashboard::printError(result.message);
    }

    nlp_free(&analyzer);
    Dashboard::pressEnterToContinue();
}

/* ─── View My Complaints ─── */
void User::viewMyComplaints() {
    Dashboard::clearScreen();
    Dashboard::printHeader("My Complaints");

    std::vector<Complaint> complaints(MAX_COMPLAINTS);
    int count = load_complaints_by_user(id, complaints.data(), MAX_COMPLAINTS);

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

                    /* Show description */
                    std::cout << Dashboard::CYAN << "  Description: " << Dashboard::WHITE
                              << complaints[i].description << Dashboard::RESET << std::endl;
                    break;
                }
            }
        }
    }

    Dashboard::pressEnterToContinue();
}

/* ─── Track Complaint ─── */
void User::trackComplaint() {
    Dashboard::clearScreen();
    Dashboard::printHeader("Track Complaint");

    std::cout << Dashboard::CYAN << "  Enter Complaint ID: " << Dashboard::WHITE;
    int complaint_id = Dashboard::readInt();

    std::vector<Complaint> complaints(MAX_COMPLAINTS);
    int count = load_complaints_by_user(id, complaints.data(), MAX_COMPLAINTS);

    bool found = false;
    for (int i = 0; i < count; i++) {
        if (complaints[i].id == complaint_id) {
            found = true;
            std::cout << std::endl;
            Dashboard::printComplaintCard(complaints[i]);

            /* Show status timeline */
            Dashboard::printSubHeader("Status Timeline");

            const char* statuses[] = {"PENDING", "REVIEWED", "ASSIGNED", "IN_PROGRESS", "COMPLETED"};
            int current_step = 0;
            for (int s = 0; s < 5; s++) {
                if (strcmp(complaints[i].status, statuses[s]) == 0) {
                    current_step = s;
                    break;
                }
            }

            for (int s = 0; s < 5; s++) {
                if (s <= current_step) {
                    std::cout << Dashboard::GREEN << "  ● " << statuses[s] << Dashboard::RESET;
                } else {
                    std::cout << Dashboard::GRAY << "  ○ " << statuses[s] << Dashboard::RESET;
                }
                if (s < 4) std::cout << " → ";
            }
            std::cout << std::endl;
            break;
        }
    }

    if (!found) {
        Dashboard::printError("Complaint not found or doesn't belong to you.");
    }

    Dashboard::pressEnterToContinue();
}

/* ─── View Profile ─── */
void User::viewProfile() {
    Dashboard::clearScreen();
    showProfile();
    Dashboard::pressEnterToContinue();
}

