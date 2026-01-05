#include <bits/stdc++.h>
#include <mysql/mysql.h>
using namespace std;

struct Edge {
    int to;
    double amount;
};

MYSQL* conn;
vector<vector<Edge>> adj;
int n;

// ---------------- MYSQL CONNECTION ----------------
void connectDB() {
    conn = mysql_init(NULL);
    if (!mysql_real_connect(
            conn,
            "127.0.0.1",   // TCP (IMPORTANT)
            "root",
            "root123",     // your MySQL password
            "bank_db",
            3306,
            NULL,
            0)) {

        cerr << "MySQL connection failed: "
             << mysql_error(conn) << endl;
        exit(1);
    }
}

// ---------------- LOAD TRANSACTIONS ----------------
void loadTransactions() {
    string query =
        "SELECT from_account, to_account, amount "
        "FROM transactions WHERE status='SUCCESS'";

    mysql_query(conn, query.c_str());
    MYSQL_RES* res = mysql_store_result(conn);
    MYSQL_ROW row;

    while ((row = mysql_fetch_row(res))) {
        if (row[0] && row[1] && row[2]) {
    int u = stoi(row[0]);
    int v = stoi(row[1]);
    double amt = stod(row[2]);

    if (u <= n && v <= n) {
        adj[u].push_back({v, amt});
    }
}
    }
    mysql_free_result(res);
}

// ---------------- CYCLE DETECTION ----------------
bool dfsCycle(int u, vector<bool>& visited,
              vector<bool>& stack,
              vector<bool>& inCycle) {

    visited[u] = true;
    stack[u] = true;

    for (auto &e : adj[u]) {
        int v = e.to;
        if (!visited[v] && dfsCycle(v, visited, stack, inCycle))
            inCycle[u] = true;
        else if (stack[v])
            inCycle[u] = true;
    }

    stack[u] = false;
    return inCycle[u];
}

// ---------------- STORE FRAUD FLAG ----------------
void storeFraudFlag(int accountId, int riskScore, const string& reason) {
    string query =
        "INSERT INTO fraud_flags (account_id, risk_score, reason) VALUES (" +
        to_string(accountId) + "," +
        to_string(riskScore) + ",'" +
        reason + "')";

    mysql_query(conn, query.c_str());
}

// ---------------- MAIN ----------------
int main() {
    connectDB();

    mysql_query(conn, "SELECT MAX(account_id) FROM accounts");
    MYSQL_RES* res = mysql_store_result(conn);
    MYSQL_ROW row = mysql_fetch_row(res);
    n = stoi(row[0]);
    mysql_free_result(res);

    adj.resize(n + 1);

    loadTransactions();

    vector<int> txnCount(n + 1, 0);
    vector<double> totalAmount(n + 1, 0);

    for (int u = 1; u <= n; u++) {
        for (auto &e : adj[u]) {
            txnCount[u]++;
            totalAmount[u] += e.amount;
        }
    }

    vector<bool> visited(n + 1, false);
    vector<bool> recStack(n + 1, false);
    vector<bool> inCycle(n + 1, false);

    for (int i = 1; i <= n; i++) {
        if (!visited[i]) {
            dfsCycle(i, visited, recStack, inCycle);
        }
    }

    const int TXN_THRESHOLD = 3;
    const double AMOUNT_THRESHOLD = 100000.0;

    cout << "---- FRAUD ANALYSIS REPORT ----\n";

    for (int i = 1; i <= n; i++) {
        int riskScore = 0;
        string reason;

        if (txnCount[i] >= TXN_THRESHOLD) {
            riskScore++;
            reason += "High transaction frequency; ";
        }

        if (totalAmount[i] >= AMOUNT_THRESHOLD) {
            riskScore++;
            reason += "High transaction amount; ";
        }

        if (inCycle[i]) {
            riskScore++;
            reason += "Circular money flow detected; ";
        }

        if (riskScore > 0) {
            cout << "Account " << i
                 << " | Risk Score: " << riskScore << "\n";
            storeFraudFlag(i, riskScore, reason);
        }
    }

    cout << "---- END OF REPORT ----\n";

    mysql_close(conn);
    return 0;
}
