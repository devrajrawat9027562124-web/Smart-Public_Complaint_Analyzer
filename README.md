Smart Complaint Analyzer
An enterprise-grade hybrid C and C++ system designed for automated public complaint management and optimized field operations, bridging low-level algorithmic performance with a robust object-oriented controller framework.

Architecture & Tech Stack
Languages: C99 for high-performance core algorithms and C++17 for UI, models, and system controllers.

Storage: File-backed flat database system (data/users.txt and data/admins_and_deps.txt).

Design Pattern: Modular separation of concerns governed by a centralized SystemController.

Key Features
Role-Based Access Control: Secure multi-role authentication for Citizens, Administrators, Department Officers, and Field Workers.

Automated Severity Engine: Instantly parses natural language problem descriptions to assign data-driven priority scores from 1 to 10.

Max-Heap Priority Queue: Automatically sorts and surfaces critical public hazards so response teams tackle emergencies first.

Dijkstra’s Route Optimizer: Computes the most efficient path for field workers navigating across city zones and nodes.

Interactive Terminal CLI: Drives a multi-layered menu system alongside a live public metrics dashboard tracking operational status.
