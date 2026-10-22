#ifndef COMPLAINT_H
#define COMPLAINT_H

#include <string>

class Complaint
{
private:
    // Encapsulation: data members are private
    int complaintID;
    std::string citizenName;
    std::string category;
    std::string description;
    std::string location;
    std::string priority;
    std::string status;

public:
    // Constructor: initializes a Complaint object
    Complaint(int id,
              const std::string& name,
              const std::string& cat,
              const std::string& desc,
              const std::string& loc,
              const std::string& pr,
              const std::string& st)
        : complaintID(id),
          citizenName(name),
          category(cat),
          description(desc),
          location(loc),
          priority(pr),
          status(st)
    {
    }

    // Getter functions
    int getComplaintID() const
    {
        return complaintID;
    }

    const std::string& getCitizenName() const
    {
        return citizenName;
    }

    const std::string& getCategory() const
    {
        return category;
    }

    const std::string& getDescription() const
    {
        return description;
    }

    const std::string& getLocation() const
    {
        return location;
    }

    const std::string& getPriority() const
    {
        return priority;
    }

    const std::string& getStatus() const
    {
        return status;
    }

    // Setter function
    void setStatus(const std::string& newStatus)
    {
        status = newStatus;
    }
};

#endif
