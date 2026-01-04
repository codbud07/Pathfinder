# SQL-Backed Fraud Network Detection Engine

A backend fraud detection system that models banking transactions as a graph and detects suspicious activity using deterministic algorithms.

## Tech Stack
- C++
- MySQL
- Graph Algorithms (Cycle Detection, Traversal)
- SQL Transactions & Constraints

## System Design
- Accounts are modeled as graph nodes
- Transactions are directed weighted edges
- Fraud is detected via:
  - Circular transaction flows
  - High transaction frequency
  - High transaction value
- Fraud decisions are persisted with audit logs
