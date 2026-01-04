#include <bits/stdc++.h>
#include <mysql/mysql.h>
using namespace std;

// ---------------- DATA STRUCTURES ----------------
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
            "localhost",     // host
            "root",          // user
            "password",      // password
            "bank_db",       // database
            0, NULL, 0)) {

        cerr << "MySQL connection failed\n";
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
        int u = stoi(row[0]);
        int v = stoi(row[1]);
        double amt = stod(row[2]);

        if (u <= n && v <= n) {
            adj[u].push_back({v, amt});
        }
    }
    mysql_free_result(res);
}

// ---------------- CYCLE DETECTION ----------------
bool dfsCycle(int u, vector<bool>& visited, vector<bool>& stack) {
    visited[u] = true;
    stack[u] = true;

    for (auto &e : adj[u]) {
        int v = e.to;
        if (!visited[v] && dfsCycle(v, visited, stack))
            return true;
        else if (stack[v])
            return true;
    }

    stack[u] = false;
    return false;
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

    // Fetch number of accounts
    mysql_query(conn, "SELECT COUNT(*) FROM accounts");
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

    vector<bool> visited(n + 1, false), recStack(n + 1, false);
    vector<bool> inCycle(n + 1, false);

    for (int i = 1; i <= n; i++) {
        if (!visited[i] && dfsCycle(i, visited, recStack)) {
            inCycle[i] = true;
        }
    }

    const int TXN_THRESHOLD = 3;
    const double AMOUNT_THRESHOLD = 100000.0;

    cout << "---- FRAUD ANALYSIS REPORT ----\n";

    for (int i = 1; i <= n; i++) {
        int riskScore = 0;
        string reason = "";

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
