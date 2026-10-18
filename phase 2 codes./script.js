
const STORAGE_KEY = "spca_complaints_v2";
const NEXT_ID_KEY = "spca_next_complaint_id_v2";

function getComplaints() {
    try {
        const data = JSON.parse(localStorage.getItem(STORAGE_KEY) || "[]");
        return Array.isArray(data) ? data : [];
    } catch (error) {
        return [];
    }
}

function saveComplaints(complaints) {
    localStorage.setItem(STORAGE_KEY, JSON.stringify(complaints));
}

function getNextId() {
    let nextId = Number(localStorage.getItem(NEXT_ID_KEY));

    if (!Number.isInteger(nextId) || nextId < 1001) {
        const complaints = getComplaints();
        nextId = 1001;

        complaints.forEach(function (complaint) {
            const id = Number(complaint.id);
            if (Number.isInteger(id) && id >= nextId) {
                nextId = id + 1;
            }
        });
    }

    localStorage.setItem(NEXT_ID_KEY, String(nextId + 1));
    return nextId;
}

function calculatePriority(category, description) {
    const text = (category + " " + description).toLowerCase();

    const highWords = [
        "emergency", "danger", "accident", "severe",
        "overflow", "fire", "exposed wire", "collapsed"
    ];

    const mediumWords = [
        "leak", "broken", "damaged", "blocked",
        "not working", "overflowing"
    ];

    if (highWords.some(word => text.includes(word))) {
        return "High";
    }

    if (mediumWords.some(word => text.includes(word))) {
        return "Medium";
    }

    return "Low";
}

function findDuplicate(category, location, description, complaints) {
    const normalLocation = location.trim().toLowerCase();
    const normalDescription = description.trim().toLowerCase();

    return complaints.find(function (complaint) {
        return complaint.category.toLowerCase() === category.toLowerCase()
            && complaint.location.trim().toLowerCase() === normalLocation
            && complaint.description.trim().toLowerCase() === normalDescription;
    });
}

function showMessage(elementId, message, isError) {
    const element = document.getElementById(elementId);

    if (!element) return;

    element.textContent = message;
    element.className = isError ? "message error" : "message success";
}

// Register a complaint
const complaintForm = document.getElementById("complaintForm");

if (complaintForm) {
    complaintForm.addEventListener("submit", function (event) {
        event.preventDefault();

        const citizenId = document.getElementById("citizenId").value.trim();
        const name = document.getElementById("name").value.trim();
        const category = document.getElementById("category").value;
        const location = document.getElementById("location").value.trim();
        const description = document.getElementById("description").value.trim();

        if (!citizenId || !name || !category || !location || !description) {
            showMessage("registerMessage", "Please fill in every field.", true);
            return;
        }

        const complaints = getComplaints();
        const duplicate = findDuplicate(category, location, description, complaints);

        if (duplicate) {
            showMessage(
                "registerMessage",
                "A very similar complaint already exists. Its ID is " +
                duplicate.id + ". Please check it before submitting again.",
                true
            );
            return;
        }

        const complaint = {
            id: getNextId(),
            citizenId: citizenId,
            name: name,
            category: category,
            location: location,
            description: description,
            priority: calculatePriority(category, description),
            department: "Not Assigned",
            status: "Pending",
            date: new Date().toLocaleDateString()
        };

        complaints.push(complaint);
        saveComplaints(complaints);

        showMessage(
            "registerMessage",
            "Complaint registered successfully! Your numeric complaint ID is " +
            complaint.id + ". Priority: " + complaint.priority +
            ". Please save your ID.",
            false
        );

        complaintForm.reset();
    });
}

// Display a complaint's details safely
function displayDetails(container, complaint) {
    container.replaceChildren();

    const heading = document.createElement("h3");
    heading.textContent = "Complaint Details";
    container.appendChild(heading);

    const fields = [
        ["Complaint ID", complaint.id],
        ["Citizen ID", complaint.citizenId],
        ["Name", complaint.name],
        ["Category", complaint.category],
        ["Location", complaint.location],
        ["Description", complaint.description],
        ["Priority", complaint.priority],
        ["Department", complaint.department],
        ["Status", complaint.status],
        ["Date", complaint.date]
    ];

    fields.forEach(function (field) {
        const paragraph = document.createElement("p");
        const label = document.createElement("strong");
        label.textContent = field[0] + ": ";
        paragraph.appendChild(label);
        paragraph.appendChild(document.createTextNode(String(field[1])));
        container.appendChild(paragraph);
    });
}

// Track a complaint
const trackForm = document.getElementById("trackForm");

if (trackForm) {
    trackForm.addEventListener("submit", function (event) {
        event.preventDefault();

        const id = document.getElementById("complaintId").value.trim();
        const result = document.getElementById("trackResult");
        const complaint = getComplaints().find(item => String(item.id) === id);

        result.replaceChildren();

        if (!complaint) {
            showMessage("trackMessage", "Complaint not found. Check the ID.", true);
            return;
        }

        showMessage("trackMessage", "Complaint found.", false);
        displayDetails(result, complaint);
    });
}

// Admin page
const complaintList = document.getElementById("complaintList");

function displayComplaints() {
    if (!complaintList) return;

    complaintList.replaceChildren();

    const searchInput = document.getElementById("adminSearch");
    const sortInput = document.getElementById("sortPriority");
    const search = searchInput ? searchInput.value.trim().toLowerCase() : "";
    const sort = sortInput ? sortInput.value : "newest";

    let complaints = getComplaints();

    complaints = complaints.filter(function (complaint) {
        const searchable = [
            complaint.id, complaint.citizenId, complaint.name,
            complaint.category, complaint.location, complaint.description
        ].join(" ").toLowerCase();

        return searchable.includes(search);
    });

    if (sort === "priority") {
        const priorityRank = { High: 3, Medium: 2, Low: 1 };
        complaints.sort((a, b) =>
            (priorityRank[b.priority] || 0) - (priorityRank[a.priority] || 0)
        );
    } else {
        complaints.sort((a, b) => Number(b.id) - Number(a.id));
    }

    if (complaints.length === 0) {
        const message = document.createElement("p");
        message.textContent = "No complaints found.";
        complaintList.appendChild(message);
        return;
    }

    complaints.forEach(function (complaint) {
        const card = document.createElement("section");
        card.className = "card complaint-card";

        const heading = document.createElement("h3");
        heading.textContent = "Complaint ID: " + complaint.id;
        card.appendChild(heading);

        const details = [
            "Citizen ID: " + complaint.citizenId,
            "Name: " + complaint.name,
            "Category: " + complaint.category,
            "Location: " + complaint.location,
            "Description: " + complaint.description,
            "Priority: " + complaint.priority,
            "Date: " + complaint.date
        ];

        details.forEach(function (detail) {
            const p = document.createElement("p");
            p.textContent = detail;
            card.appendChild(p);
        });

        const departmentLabel = document.createElement("label");
        departmentLabel.textContent = "Department";

        const departmentSelect = document.createElement("select");

        [
            "Not Assigned", "Road Department", "Water Department",
            "Electricity Department", "Sanitation Department",
            "Municipal Department", "Other"
        ].forEach(function (department) {
            const option = document.createElement("option");
            option.value = department;
            option.textContent = department;
            option.selected = complaint.department === department;
            departmentSelect.appendChild(option);
        });

        departmentLabel.appendChild(departmentSelect);
        card.appendChild(departmentLabel);

        const statusLabel = document.createElement("label");
        statusLabel.textContent = "Status";

        const statusSelect = document.createElement("select");

        ["Pending", "In Progress", "Resolved"].forEach(function (status) {
            const option = document.createElement("option");
            option.value = status;
            option.textContent = status;
            option.selected = complaint.status === status;
            statusSelect.appendChild(option);
        });

        statusLabel.appendChild(statusSelect);
        card.appendChild(statusLabel);

        const updateButton = document.createElement("button");
        updateButton.textContent = "Save Changes";

        updateButton.addEventListener("click", function () {
            const allComplaints = getComplaints();
            const index = allComplaints.findIndex(
                item => String(item.id) === String(complaint.id)
            );

            if (index === -1) return;

            allComplaints[index].department = departmentSelect.value;
            allComplaints[index].status = statusSelect.value;
            saveComplaints(allComplaints);

            showMessage("adminMessage", "Complaint " + complaint.id + " updated.", false);
            displayComplaints();
        });

        card.appendChild(updateButton);

        const deleteButton = document.createElement("button");
        deleteButton.textContent = "Delete Complaint";
        deleteButton.className = "danger-button";

        deleteButton.addEventListener("click", function () {
            if (!confirm("Delete complaint " + complaint.id + "?")) return;

            const remaining = getComplaints().filter(
                item => String(item.id) !== String(complaint.id)
            );

            saveComplaints(remaining);
            showMessage("adminMessage", "Complaint deleted.", false);
            displayComplaints();
        });

        card.appendChild(deleteButton);
        complaintList.appendChild(card);
    });
}

if (complaintList) {
    document.getElementById("adminSearch").addEventListener("input", displayComplaints);
    document.getElementById("sortPriority").addEventListener("change", displayComplaints);
    displayComplaints();
}
