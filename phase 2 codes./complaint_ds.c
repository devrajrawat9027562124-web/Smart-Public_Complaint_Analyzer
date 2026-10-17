
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "complaint_ds.h"

/* Convert text to lowercase for case-insensitive matching */
void makeLowercase(const char *source, char *destination, size_t size)
{
    size_t i = 0;
     if (size == 0)
        return;
     while (source[i] != '\0' && i < size - 1)
    {
        destination[i] = (char)tolower((unsigned char)source[i]);
        i++;
    }

    destination[i] = '\0';
}

/* Check whether any keyword occurs in the complaint description */
int containsKeyword(const char *description,const char *keywords[],int count)
{
    char lowerDescription[200];
    int i;

    makeLowercase(description, lowerDescription,
                  sizeof(lowerDescription));

    for (i = 0; i < count; i++)
    {
        if (strstr(lowerDescription, keywords[i]) != NULL)
            return 1;
    }

    return 0;
}
/* HIGH priority keywords: emergencies and immediate hazards */
const char *highKeywords[] = {
    "fire", "wildfire", "electrocution", "electric shock",
    "exposed wire", "live wire", "sparking wire", "gas leak",
    "severe flooding", "flash flood", "building collapse",
    "wall collapse", "bridge collapse", "road collapse",
    "sinkhole", "major accident", "fallen electric pole",
    "fallen power line", "transformer explosion",
    "electrical fire", "short circuit", "water main burst",
    "burst pipeline", "chemical spill", "toxic fumes",
    "open manhole", "serious injury", "risk of explosion",
    "fuel leak", "unstable building", "road washed away",
    "overflowing sewer", "water contamination", "major landslide",
    "dangerous road obstruction", "industrial gas leak",
    "life threatening", "electrical hazard", "live electrical cable",
    "gas cylinder leak", "building about to collapse",
    "flood water entering homes", "broken high voltage wire",
    "fallen tree on power line", "dangerous chemical leak",
    "fire spreading", "manhole causing accident",
    "electricity pole fallen", "electric wire fallen",
    "high voltage cable", "severe sewage overflow",
    "burst water main", "major water pipe burst",
    "dangerous electrical sparks", "smoke from transformer",
    "building leaning dangerously", "road cave in",
    "flooded electrical room", "electric shock hazard",
    "gas escaping", "large fuel spill", "toxic gas release",
    "dangerous structural damage", "ceiling collapse",
    "roof collapse", "retaining wall collapse",
    "flashover", "large sinkhole", "severe erosion near building",
    "contaminated drinking water", "electrical pole burning",
    "major pipeline rupture", "fire in public building",
    "flooding near electrical equipment", "fallen live cable",
    "dangerous exposed electrical connection",
    "severe road subsidence", "emergency evacuation",
    "explosion risk", "major structural crack"
};

const int highCount =
    sizeof(highKeywords) / sizeof(highKeywords[0]);


/* MEDIUM priority keywords: common civic problems */
const char *mediumKeywords[] = {
    "pothole", "broken road", "damaged road", "cracked road",
    "broken footpath", "blocked drain", "clogged drain",
    "waterlogging", "stagnant water", "pipe leak",
    "water shortage", "low water pressure", "garbage pile",
    "garbage accumulation", "uncollected garbage",
    "construction debris", "blocked sewer", "sewer blockage",
    "broken streetlight", "streetlight not working",
    "broken traffic light", "damaged traffic sign",
    "broken railing", "broken park bench",
    "damaged playground equipment", "broken public tap",
    "public toilet blockage", "water seepage", "roof leakage",
    "cracked wall", "broken manhole cover",
    "mosquito breeding", "dead animal on road",
    "water supply interruption", "clogged roadside gutter",
    "overflowing dustbin", "damaged pavement",
    "broken drainage pipe", "road erosion",
    "damaged road divider", "blocked public drain",
    "water pipe damage", "sewer smell", "stray animal problem",
    "broken bus stop", "damaged public toilet",
    "broken water pump", "damaged playground",
    "waste collection missed", "drain water overflow",
    "damaged road marking", "blocked roadside drain",
    "irregular water supply", "dirty drinking water",
    "leaking public tap", "broken water tank",
    "garbage not collected", "waste dumping",
    "illegal garbage dumping", "overflowing public bin",
    "drain cover missing", "drain cover broken",
    "sewer water on road", "road surface damaged",
    "uneven road surface", "broken speed breaker",
    "damaged footpath", "loose paving stones",
    "broken street sign", "traffic signal failure",
    "streetlight failure", "dark street",
    "nonfunctional streetlight", "broken park fence",
    "damaged public bench", "broken playground slide",
    "broken swing", "public toilet not working",
    "blocked toilet", "water pipe leakage",
    "water leaking from pipe", "low water supply",
    "water supply stopped", "drainage problem",
    "drain overflowing", "sewage leakage",
    "sewer line blockage", "bad road condition",
    "damaged culvert", "broken roadside barrier",
    "fallen branch blocking road", "stray cattle on road",
    "stray dogs causing problems", "damaged bus shelter",
    "broken public hand pump", "contaminated public tap",
    "water accumulation on road", "roadside garbage",
    "public bin damaged", "damaged road shoulder"
};

const int mediumCount =
    sizeof(mediumKeywords) / sizeof(mediumKeywords[0]);


/* LOW priority keywords: cosmetic and routine maintenance */
const char *lowKeywords[] = {
    "paint peeling", "faded paint", "minor paint damage",
    "small wall crack", "hairline crack", "chipped paint",
    "faded signboard", "scratched signboard", "minor litter",
    "scattered wrappers", "fallen leaves", "dry leaves",
    "minor garden weeds", "overgrown flowers", "dry grass",
    "faded park bench", "scratched bench", "squeaky swing",
    "outdated notice board", "missing decorative tile",
    "chipped floor tile", "minor plaster damage",
    "dusty bus shelter", "faded park sign",
    "loose fence latch", "squeaky gate", "loose door handle",
    "scratched desk", "loose chair screw", "small ceiling stain",
    "faded public mural", "scratched waste bin",
    "torn poster", "minor wall scuff", "small paint patch",
    "routine park maintenance", "routine cleaning request",
    "faded road marking", "minor cosmetic damage",
    "dirty signboard", "dusty public bench",
    "small decorative damage", "minor garden maintenance",
    "faded wall painting", "small litter near park",
    "scratched notice board", "minor fence damage",
    "dusty public area", "faded building paint",
    "loose cabinet handle", "minor appearance issue",
    "small paint scratch", "faded informational poster",
    "minor tile discoloration", "dusty street sign",
    "small graffiti mark", "faded boundary paint",
    "minor bench scratches", "loose notice board screw",
    "worn park sign", "small garden cleanup",
    "minor pavement staining", "faded directional sign",
    "cosmetic fence damage", "minor paint touch up",
    "small decorative crack", "dusty public notice board",
    "faded playground markings", "minor surface scratches",
    "routine grass trimming", "small fallen twig",
    "minor signboard cleaning", "dusty seating area",
    "minor wall discoloration", "routine beautification"
};

const int lowCount =
    sizeof(lowKeywords) / sizeof(lowKeywords[0]);



/* Save all complaints to a text file */
void saveComplaints(ComplaintNode* head)
{
    FILE* file = fopen("complaints.txt", "w");
    ComplaintNode* current = head;

    if (file == NULL)
    {
        printf("Error saving complaints!\n");
        return;
    }

    while (current != NULL)
    {
        fprintf(file, "%d|%s|%s|%s|%s|%s|%d|%s|%s|%s\n",
                current->complaintID,
                current->citizenName,
                current->category,
                current->description,
                current->location,
                current->priority,
                current->priorityValue,
                current->status,
                current->department,
                current->employee);

        current = current->next;
    }

    fclose(file);
}


/* Load saved complaints from the text file */
void loadComplaints(ComplaintNode** head)
{
    FILE* file = fopen("complaints.txt", "r");
    char line[1000];
    ComplaintNode* current;
    ComplaintNode* newNode;

    if (file == NULL)
        return;

    while (fgets(line, sizeof(line), file) != NULL)
    {
        newNode = (ComplaintNode*)malloc(sizeof(ComplaintNode));

        if (newNode == NULL)
        {
            printf("Memory allocation failed while loading!\n");
            break;
        }

        int fields = sscanf(
            line,
            "%d|%99[^|]|%49[^|]|%199[^|]|%99[^|]|%19[^|]|%d|%19[^|]|%49[^|]|%49[^\n]",
            &newNode->complaintID,
            newNode->citizenName,
            newNode->category,
            newNode->description,
            newNode->location,
            newNode->priority,
            &newNode->priorityValue,
            newNode->status,
            newNode->department,
            newNode->employee
        );

        if (fields != 10)
        {
            free(newNode);
            continue;
        }

        newNode->next = NULL;

        if (*head == NULL)
        {
            *head = newNode;
        }
        else
        {
            current = *head;

            while (current->next != NULL)
                current = current->next;

            current->next = newNode;
        }
    }

    fclose(file);
}

/* Register a new complaint */
void addComplaint(ComplaintNode** head)
{
    ComplaintNode* newNode;
    ComplaintNode* temp;

    newNode = (ComplaintNode*)malloc(sizeof(ComplaintNode));

    if (newNode == NULL)
    {
        printf("Memory allocation failed!\n");
        return;
    }

    printf("Enter Complaint ID: ");
    scanf("%d", &newNode->complaintID);
    getchar();

    printf("Enter Citizen Name: ");
    fgets(newNode->citizenName, sizeof(newNode->citizenName), stdin);
    newNode->citizenName[strcspn(newNode->citizenName, "\n")] = '\0';

    printf("Enter Category (Road/Water/Electricity/Garbage/Other): ");
    fgets(newNode->category, sizeof(newNode->category), stdin);
    newNode->category[strcspn(newNode->category, "\n")] = '\0';

    printf("Enter Description: ");
    fgets(newNode->description, sizeof(newNode->description), stdin);
    newNode->description[strcspn(newNode->description, "\n")] = '\0';

    printf("Enter Location: ");
    fgets(newNode->location, sizeof(newNode->location), stdin);
    newNode->location[strcspn(newNode->location, "\n")] = '\0';

    assignPriority(newNode);

    printf("\nSystem Assigned Priority: %s\n", newNode->priority);

    strcpy(newNode->status, "Pending");
    strcpy(newNode->department, "Not Assigned");
    strcpy(newNode->employee, "Not Assigned");

    newNode->next = NULL;

    if (*head == NULL)
    {
        *head = newNode;
    }
    else
    {
        temp = *head;

        while (temp->next != NULL)
            temp = temp->next;

        temp->next = newNode;
    }

    printf("\nComplaint registered successfully!\n");
    printf("Status: %s\n", newNode->status);
    printf("Department: %s\n", newNode->department);
    saveComplaints(*head);
}


/* Display all complaints */
void displayComplaints(ComplaintNode* head)
{
    ComplaintNode* current = head;

    if (head == NULL)
    {
        printf("\nNo complaints found.\n");
        return;
    }

    while (current != NULL)
    {
        printf("\n-----------------------------\n");
        printf("Complaint ID: %d\n", current->complaintID);
        printf("Citizen Name: %s\n", current->citizenName);
        printf("Category: %s\n", current->category);
        printf("Description: %s\n", current->description);
        printf("Location: %s\n", current->location);
        printf("Priority: %s\n", current->priority);
        printf("Status: %s\n", current->status);
        printf("Department: %s\n", current->department);
        printf("Employee: %s\n", current->employee);

        current = current->next;
    }
}


/* Search complaint by ID */
void searchComplaint(ComplaintNode* head, int id)
{
    ComplaintNode* current = head;

    while (current != NULL)
    {
        if (current->complaintID == id)
        {
            printf("\n========== COMPLAINT FOUND ==========\n");
            printf("Complaint ID: %d\n", current->complaintID);
            printf("Citizen Name: %s\n", current->citizenName);
            printf("Category: %s\n", current->category);
            printf("Description: %s\n", current->description);
            printf("Location: %s\n", current->location);
            printf("Priority: %s\n", current->priority);
            printf("Status: %s\n", current->status);
            printf("Department: %s\n", current->department);
            printf("Employee: %s\n", current->employee);
            return;
        }

        current = current->next;
    }

    printf("\nComplaint not found.\n");
}


/* Update complaint status */

void updateStatus(ComplaintNode* head, int id, const char newStatus[])
{
    ComplaintNode* current = head;

    while (current != NULL)
    {
        if (current->complaintID == id)
        {
            if (strcmp(current->status, newStatus) == 0)
            {
                printf("\nComplaint already has this status.\n");
                return;
            }

            char oldStatus[20];

            snprintf(oldStatus, sizeof(oldStatus), "%s",
                     current->status);

            saveStatusHistory(current->complaintID, oldStatus, newStatus);

            snprintf(current->status, sizeof(current->status), "%s", newStatus);

            printf("\nComplaint status updated successfully!\n");
            printf("Old Status: %s\n", oldStatus);
            printf("New Status: %s\n", current->status);

            saveComplaints(head);
            return;
        }

        current = current->next;
    }

    printf("\nComplaint not found.\n");
}



/* Assign department */
void assignDepartment(ComplaintNode* head, int id,
                      const char newDepartment[])
{
    ComplaintNode* current = head;

    while (current != NULL)
    {
        if (current->complaintID == id)
        {
            snprintf(current->department, sizeof(current->department),
                     "%s", newDepartment);

            printf("\nDepartment assigned successfully!\n");
            printf("Department: %s\n", current->department);
            saveComplaints(head);
            return;
        }

        current = current->next;
    }

    printf("\nComplaint not found.\n");
}


/* Assign employee */
void assignEmployee(ComplaintNode* head, int id,
                    const char newEmployee[])
{
    ComplaintNode* current = head;

    while (current != NULL)
    {
        if (current->complaintID == id)
        {
            snprintf(current->employee, sizeof(current->employee),
                     "%s", newEmployee);

            printf("\nEmployee assigned successfully!\n");
            printf("Employee: %s\n", current->employee);
            saveComplaints(head);
            return;
        }

        current = current->next;
    }

    printf("\nComplaint not found.\n");
}


/* Insert complaint into priority queue */
void insertPriorityQueue(PriorityNode** front, ComplaintNode* complaint)
{
    PriorityNode* newNode;
    PriorityNode* current;

    newNode = (PriorityNode*)malloc(sizeof(PriorityNode));

    if (newNode == NULL)
    {
        printf("Memory allocation failed.\n");
        return;
    }

    newNode->complaint = complaint;
    newNode->next = NULL;

    if (*front == NULL ||
        complaint->priorityValue >
        (*front)->complaint->priorityValue)
    {
        newNode->next = *front;
        *front = newNode;
        return;
    }

    current = *front;

    while (current->next != NULL &&
           current->next->complaint->priorityValue >=
           complaint->priorityValue)
    {
        current = current->next;
    }

    newNode->next = current->next;
    current->next = newNode;
}


/* Display priority queue */
void displayPriorityQueue(PriorityNode* front)
{
    PriorityNode* current = front;

    if (front == NULL)
    {
        printf("\nPriority Queue is empty.\n");
        return;
    }

    printf("\n========== PRIORITY QUEUE ==========\n");

    while (current != NULL)
    {
        printf("\nComplaint ID: %d\n",
               current->complaint->complaintID);
        printf("Priority: %s\n", current->complaint->priority);
        printf("Description: %s\n", current->complaint->description);
        printf("Status: %s\n", current->complaint->status);

        current = current->next;
    }
}


/* Sort complaints by priority */
void sortComplaintsByPriority(ComplaintNode* head)
{
    ComplaintNode* current;
    ComplaintNode* compare;

    if (head == NULL)
    {
        printf("\nNo complaints to sort.\n");
        return;
    }

    current = head;

    while (current != NULL)
    {
        compare = current->next;

        while (compare != NULL)
        {
            if (current->priorityValue < compare->priorityValue)
            {
                int tempValue = current->priorityValue;
                char tempPriority[sizeof(current->priority)];

                current->priorityValue = compare->priorityValue;
                compare->priorityValue = tempValue;

                strcpy(tempPriority, current->priority);
                strcpy(current->priority, compare->priority);
                strcpy(compare->priority, tempPriority);
            }

            compare = compare->next;
        }

        current = current->next;
    }

    printf("\nComplaints sorted by priority successfully!\n");
}


/* Automatically assign complaint priority */

void assignPriority(ComplaintNode* complaint)
{
    char lowerDescription[200];
    char lowerCategory[50];

    makeLowercase(complaint->description, lowerDescription,
                  sizeof(lowerDescription));

    makeLowercase(complaint->category, lowerCategory,
                  sizeof(lowerCategory));

    /* HIGH priority */
    if (containsKeyword(lowerDescription, highKeywords, highCount))
    {
        strcpy(complaint->priority, "High");
        complaint->priorityValue = 3;
    }
    /* MEDIUM priority */
    else if (containsKeyword(lowerDescription,
                             mediumKeywords, mediumCount))
    {
        strcpy(complaint->priority, "Medium");
        complaint->priorityValue = 2;
    }
    /* LOW priority */
    else if (containsKeyword(lowerDescription, lowKeywords, lowCount))
    {
        strcpy(complaint->priority, "Low");
        complaint->priorityValue = 1;
    }
    /* Category fallback */
    else if (strcmp(lowerCategory, "road") == 0 ||
             strcmp(lowerCategory, "water") == 0 ||
             strcmp(lowerCategory, "garbage") == 0)
    {
        strcpy(complaint->priority, "Medium");
        complaint->priorityValue = 2;
    }
    else
    {
        strcpy(complaint->priority, "Low");
        complaint->priorityValue = 1;
    }
}

void displayComplaintStatistics(ComplaintNode* head)
{
    int total = 0;
    int pending = 0;
    int inProgress = 0;
    int resolved = 0;

    ComplaintNode* current = head;

    while (current != NULL)
    {
        total++;

        if (strcmp(current->status, "Pending") == 0)
        {
            pending++;
        }
        else if (strcmp(current->status, "In Progress") == 0)
        {
            inProgress++;
        }
        else if (strcmp(current->status, "Resolved") == 0)
        {
            resolved++;
        }

        current = current->next;
    }

    printf("\n===== ADMIN DASHBOARD =====\n");
    printf("Total Complaints : %d\n", total);
    printf("Pending          : %d\n", pending);
    printf("In Progress      : %d\n", inProgress);
    printf("Resolved         : %d\n", resolved);
    printf("===========================\n");
}

void saveStatusHistory(int complaintID, const char oldStatus[],
                       const char newStatus[])
{
    FILE* file = fopen("status_history.txt", "a");
    time_t now;
    struct tm* localTime;
    char dateTime[30];

    if (file == NULL)
    {
        printf("Error saving status history!\n");
        return;
    }

    now = time(NULL);
    localTime = localtime(&now);

    if (localTime != NULL)
    {
        strftime(dateTime, sizeof(dateTime),
                 "%Y-%m-%d %H:%M:%S", localTime);

        fprintf(file, "ID: %d | Old: %s | New: %s | Date: %s\n",
                complaintID, oldStatus, newStatus, dateTime);
    }

    fclose(file);
}

void filterComplaintsByStatus(ComplaintNode* head, const char status[])
{
    ComplaintNode* current = head;
    int found = 0;

    printf("\n===== COMPLAINTS WITH STATUS: %s =====\n", status);

    while (current != NULL)
    {
        if (strcmp(current->status, status) == 0)
        {
            printf("\nComplaint ID : %d\n", current->complaintID);
            printf("Name         : %s\n", current->citizenName);
            printf("Category     : %s\n", current->category);
            printf("Description  : %s\n", current->description);
            printf("Location     : %s\n", current->location);
            printf("Status       : %s\n", current->status);
            printf("------------------------------\n");

            found = 1;
        }

        current = current->next;
    }

    if (!found)
    {
        printf("No complaints found with this status.\n");
    }
}

void filterComplaintsByCategory(ComplaintNode* head, const char category[])
{
    ComplaintNode* current = head;
    int found = 0;

    printf("\n===== COMPLAINTS IN CATEGORY: %s =====\n", category);

    while (current != NULL)
    {
        if (strcmp(current->category, category) == 0)
        {
            printf("\nComplaint ID : %d\n", current->complaintID);
            printf("Name         : %s\n", current->citizenName);
            printf("Category     : %s\n", current->category);
            printf("Description  : %s\n", current->description);
            printf("Location     : %s\n", current->location);
            printf("Status       : %s\n", current->status);
            printf("------------------------------\n");

            found = 1;
        }

        current = current->next;
    }

    if (!found)
    {
        printf("No complaints found in this category.\n");
    }
}


