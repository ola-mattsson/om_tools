#pragma once

#include "libpq-fe.h"
#include <boost/utility/string_view.hpp>
#include <boost/core/noncopyable.hpp>
#include <boost/optional.hpp>
#include <iostream>
#include <vector>
#include <sstream>
#include <arpa/inet.h>

class scoped_PGresult : public boost::noncopyable {
    PGresult* result;
    mutable std::string last_error;

public:
    scoped_PGresult()
            :result(NULL) { }

    explicit scoped_PGresult(PGresult* res)
            :result(res) { }

    ~scoped_PGresult() { if (result) PQclear(result); }

    operator PGresult*() const { return result; }

    operator bool() const {
        if (result) {
            ExecStatusType status = PQresultStatus(result);
            return status == PGRES_COMMAND_OK || status == PGRES_TUPLES_OK;
        }
        return false;
    }

    scoped_PGresult& operator=(PGresult* res) {
        if (result) {
            PQclear(result);
        }
        result = res;
        return *this;
    }

    void clear() {
        if (result) {
            PQclear(result);
            result = NULL;
        }
    }

    std::string& get_error() const {
        if (result) {
            last_error = PQresultErrorMessage(result);
        } else {
            last_error.clear();
        }
        return last_error;
    }

    std::string& get_last_error() const {
        return last_error;
    }

    int get_rows() const {
        return PQntuples(result);
    }

    int get_columns() const {
        return PQnfields(result);
    }

    const char* get_value(int row, int column) const {
        return PQgetvalue(result, row, column);
    }

    size_t get_length(int row, int column) {
        return PQgetlength(result, row, column);
    }

    void list_tuples() const {
        for (int i(0); i < get_rows(); ++i) {
            for (int j(0); j < get_columns(); ++j) {
                const char* text = get_value(i, j);
                if (0 != j) std::cout << ',';
                std::cout << text;
            }
            std::cout << '\n';
        }
    }

    void print() {
        if (result) {
            list_tuples();
        }
        else {
            std::cout << get_error() << "\n";
        }
    }
};

class simple_libpq_helper {
    PGconn* conn;
    mutable std::string last_error;

    simple_libpq_helper& operator=(const simple_libpq_helper*) { return *this; }

    simple_libpq_helper& operator=(const simple_libpq_helper& other) {
        if (this != &other) return *this;
        if (conn) PQfinish(conn);
        conn = other.conn;
        last_error.clear();
        return *this;
    }

public:
    explicit simple_libpq_helper(const boost::string_view& connect_info)
            :conn(NULL) {
        conn = PQconnectdb(&connect_info[0]);

        /* Check to see that the backend connection was successfully made */
        if (PQstatus(conn) != CONNECTION_OK) {
            std::cerr << "Connection to database failed: " << PQerrorMessage(conn);
            conn = NULL;
        }
    }

    ~simple_libpq_helper() {
        PQfinish(conn);
    }

    operator PGconn*() const { return conn; }

    bool get_status() { return conn != NULL && PQstatus(conn) == CONNECTION_OK; }

    std::string get_host() const { return PQhost(conn); }
    std::string get_port() const { return PQport(conn); }
    std::string get_db() const { return PQdb(conn); }

    int get_socket() const { return PQsocket(conn); }

    std::string& get_error() const {
        last_error = PQerrorMessage(conn);
        return last_error;
    }

    std::string& get_last_error() const {
        return last_error;
    }

    PGresult* exec(const boost::string_view& query, unsigned int retries = 3, unsigned int retry_sleep_time = 10) const {
        unsigned int retry = 0;
        while (PQstatus(conn) == CONNECTION_BAD && retry < retries) {
            ++retry;
            std::cerr << "Detected bad postgres connection. Status is CONNECTION_BAD. Attempting to reconnect (attempt "
                      << retry << ")." << std::endl;
            PQreset(conn);
            if (PQstatus(conn) == CONNECTION_BAD) {
                sleep(retry_sleep_time * retry);
            }
        }
        return PQexec(conn, &query[0]);
    }

    void exec(const std::string& query, scoped_PGresult& result, unsigned int retries = 3, unsigned int retry_sleep_time = 10) const {
        result = exec(query, retries, retry_sleep_time);
    }
};
