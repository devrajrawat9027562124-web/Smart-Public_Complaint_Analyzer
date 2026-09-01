🚀 Smart Complaint Analyzer
An enterprise-grade hybrid C and C++ complaint management and field operations system. This project combines low-level data structures and algorithmic efficiency (written in C) with robust object-oriented controllers, file-based persistence, and a role-based interactive CLI (written in C++).
✨ Key Features
Role-Based Authentication & Persistence: Secure login and user management for Citizens, Administrators, Department Officers, and Field Workers, backed by flat-file storage (users.txt, admins_and_deps.txt). Citizens can seamlessly sign up with custom profile information and default location tracking.
Automated Severity Engine: Instantly parses natural language problem descriptions using a keyword-scanning engine to automatically assign an urgency score from 1 to 10 (e.g., emergency fire/sparks vs. routine road potholes).
Max-Heap Priority Queue: Automatically sorts and surfaces the most critical public complaints so response teams tackle high-priority hazards first.
Dijkstra’s Route Optimizer: Computes the shortest and most efficient path for field workers navigating across city zones and nodes.
Interactive Terminal CLI & Live Dashboard: Features a multi-layered menu system driven by a central SystemController alongside a public metrics dashboard tracking total filed, resolved, and pending issues.
🛠️ Tech Stack & Architecture
Language: C++17 (UI, Models, Controllers, Persistence) & C99 (Core Algorithms & Engine)
Design Pattern: Modular separation of concerns (UI Layer $\rightarrow$ Controllers $\rightarrow$ C-Engine Core)
Storage: File-backed flat database system
only 350 words
