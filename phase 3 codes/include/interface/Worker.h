/*
 * Worker.h
 * Field worker role — views tasks, gets routes, marks tasks complete.
 */

#ifndef WORKER_H
#define WORKER_H

#include "Person.h"

class Worker : public Person {
public:
    Worker(const UserRecord& record);
    
    void showDashboard() override;
    void showMenu() override;
    
    void viewAssignedTasks();
    void getOptimizedRoute();
    void markTaskComplete();
    void updateLocation();
    void toggleAvailability();   /* available <-> busy (no new tasks) */
};

#endif /* WORKER_H */

