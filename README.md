# Smart Complaint Analyzer

An enterprise-grade hybrid C and C++ system designed for automated public complaint management and optimized field operations.

## 📋 Overview

This project implements a hybrid architecture combining low-level, performance-critical data structures (written in C99) with robust object-oriented controllers, file persistence, and an interactive CLI (written in C++17). The system manages public grievances through role-based workflows, automated text-based severity analysis, priority queue sorting, and optimal route generation for field workers.

## 🎯 System Architecture Pipeline

User Input & Authentication → Flat-File Persistence → NLP Severity Calculator → Max-Heap Priority Queue → Dijkstra Route Optimizer → Field Resolution & Dashboard

## 🗄️ Data Storage & Registry

* **Storage Type:** Flat-file database system (`data/` directory)
* **Key Files:**
  * `data/users.txt`: Dynamically stores registered citizen accounts, credentials, and default residential GPS coordinates.
  * `data/admins_and_deps.txt`: Stores pre-configured accounts for administrators, department officers, and field workers.

## 🔧 Core Components Deep Dive

* **Automated Severity Engine:** Scans natural language problem descriptions for emergency keywords (e.g., *fire*, *spark*, *pothole*) to automatically assign an urgency score from $1$ to $10$.
* **Max-Heap Priority Queue:** Automatically sorts and surfaces critical public hazards so response teams tackle high-priority safety emergencies first.
* **Dijkstra’s Route Optimizer:** Computes the shortest and most efficient traversal path for field workers navigating across city zones and nodes.
* **Role-Based Access Control (RBAC):** Multi-tier security and menu segregation supporting Citizens, Administrators, Department Officers, and Field Workers.

## 🚀 Getting Started

### Prerequisites
* **Compiler:** GCC with C++17 and C99 support (`g++`)
* **Environment:** Windows PowerShell (or compatible Linux/macOS terminal)

### Compilation
Compile the hybrid source code by linking the C engine and C++ UI modules together:

```powershell
g++ -std=c++17 -Iinclude main.cpp src/c_engine/*.c src/cpp_ui/models/*.cpp src/cpp_ui/controllers/*.cpp src/cpp_ui/persistence_ui/*.cpp -o SmartComplaintAnalyzer.exe
