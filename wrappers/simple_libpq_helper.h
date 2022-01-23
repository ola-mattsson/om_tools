#pragma once

#include "libpq-fe.h"
#include <boost/utility/string_view.hpp>
#include <boost/core/noncopyable.hpp>
#include <boost/optional.hpp>
// some older compiler can't compile boost::lexical_cast<>, but there
// is an old version that may work. at least that's the case for VSI C++
// it is not so fast, barely measurably slower than std::stringstream,
// but it is a nice interface
// #include <boost/lexical_cast/lexical_cast_old.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <vector>
#include <sstream>
#include <arpa/inet.h>

// stop linters complaining about NULL for pre C++11 standards
#if __cplusplus < 201103L && !defined nullptr
#define nullptr NULL
#endif
namespace om_tools {
    namespace simple_libpq_helper {

        class scoped_result : public boost::noncopyable {
            PGresult *result;
            mutable std::string last_error;

        public:
            scoped_result()
                : result(nullptr) {}

            explicit scoped_result(PGresult *res)
                : result(res) {}

            ~scoped_result() { if (result) PQclear(result); }

            operator PGresult *() const { return result; }

            operator bool() const {
                if (result) {
                    ExecStatusType status = PQresultStatus(result);
                    return status == PGRES_COMMAND_OK || status == PGRES_TUPLES_OK;
                }
                return false;
            }

            scoped_result &operator=(PGresult *res) {
                if (result) {
                    PQclear(result);
                }
                result = res;
                return *this;
            }

            void clear() {
                if (result) {
                    PQclear(result);
                    result = nullptr;
                }
            }

            std::string &get_error() const {
                if (result) {
                    last_error = PQresultErrorMessage(result);
                } else {
                    last_error.clear();
                }
                return last_error;
            }

            std::string &get_last_error() const {
                return last_error;
            }

            int32_t get_rows() const {
                return PQntuples(result);
            }

            int32_t get_columns() const {
                return PQnfields(result);
            }

            // get the converted value
            template<typename T>
            T get_value(int32_t row, int32_t column) const {
                if (0 < get_rows() && row < get_rows() && column < get_columns()) {
                    return boost::lexical_cast<T>(std::string(get_string_value(row, column)));
                }
                return T();
            }

            // get the char array value, use for large string values
            const char *get_string_value(int32_t row, int32_t column) const {
                return PQgetvalue(result, row, column);
            }

            size_t get_length(int32_t row, int32_t column) {
                return PQgetlength(result, row, column);
            }


            // dump result data, not very useful perhaps
            void list_tuples() const;

            void print();

            struct row_iterator {
                const scoped_result &res;
                int32_t row_num;

                explicit row_iterator(const scoped_result &result) : res(result), row_num(0) {}

                row_iterator &operator++() {
                    ++row_num;
                    return *this;
                }

                bool operator<(const row_iterator &other) const {
                    return row_num < other.row_num;
                }

                template<class T>
                T get(int32_t column) {
                    if (column < res.get_columns()) {
                        return boost::lexical_cast<T>(res.get_string_value(row_num, column));
                    }
                    return T();
                }
            };

            row_iterator begin() {
                return row_iterator(*this);
            }

            row_iterator end() {
                row_iterator _this(*this);
                _this.row_num = get_rows();
                return _this;
            }

        };

        void scoped_result::list_tuples() const {
            for (int32_t i(0); i < get_rows(); ++i) {
                for (int32_t j(0); j < get_columns(); ++j) {
                    const char *text = get_string_value(i, j);
                    if (0 != j) std::cout << ',';
                    std::cout << text;
                }
                std::cout << '\n';
            }
        }

        void scoped_result::print() {
            if (result) {
                list_tuples();
            } else {
                std::cout << get_error() << "\n";
            }
        }

        class params {
            std::vector<std::string> data;
            mutable const char *array[30];
            typedef std::vector<std::string>::const_iterator iterator;
        public:
            params() : array() {}

            template<class T>
            params &add(const T &item) {
                data.push_back(boost::lexical_cast<std::string>(item));
                return *this;
            }

            // postgres PGexecParams wants an int for the size, fair enough,
            // 2147483647 parameters should suffice :P
            int32_t size() const { return static_cast<int32_t>(data.size()); }

            const char **get() const {
                int32_t counter(0);
                for (iterator iter = data.begin(); iter != data.end(); ++iter) {
                    array[counter++] = iter->c_str();
                }
                return array;
            }
        };

        class connection {
            PGconn *conn;
            mutable std::string last_error;

            connection &operator=(const connection *) { return *this; }

            connection &operator=(const connection &other) {
                if (this != &other) return *this;
                if (conn) PQfinish(conn);
                conn = other.conn;
                last_error.clear();
                return *this;
            }

        public:
            explicit connection(const boost::string_view &connect_info = "")
                : conn(nullptr) {
                conn = PQconnectdb(&connect_info[0]);

                /* Check to see that the backend connection was successfully made */
                if (PQstatus(conn) != CONNECTION_OK) {
                    std::cerr << "Connection to database failed: " << PQerrorMessage(conn);
                    conn = nullptr;
                }
            }

            ~connection() {
                PQfinish(conn);
            }

            operator PGconn *() const { return conn; }

            bool get_status() { return conn != nullptr && PQstatus(conn) == CONNECTION_OK; }

            std::string get_host() const { return PQhost(conn); }

            std::string get_port() const { return PQport(conn); }

            std::string get_db() const { return PQdb(conn); }

            int32_t get_socket() const { return PQsocket(conn); }

            std::string &get_error() const {
                last_error = PQerrorMessage(conn);
                return last_error;
            }

            std::string &get_last_error() const {
                return last_error;
            }

            bool check_connection(uint32_t retries = 3, uint32_t retry_delay = 30) const {
                uint32_t retry = 0;
                while (PQstatus(conn) == CONNECTION_BAD && retry < retries) {
                    ++retry;
                    std::cerr
                        << "Detected bad postgres connection. Status is CONNECTION_BAD. Attempting to reconnect (attempt "
                        << retry << ")." << std::endl;
                    PQreset(conn);
                    if (PQstatus(conn) == CONNECTION_BAD) {
                        sleep(retry_delay * retry);
                    } else {
                        return true;
                    }
                }
                return false;
            }

            PGresult *exec_params(const boost::string_view &query,
                                  const params &values,
                                  uint32_t retries = 3,
                                  uint32_t retry_sleep_time = 10) const {
                if (check_connection(retries, retry_sleep_time)) {
                    return nullptr;
                }
                PGresult *result = PQexecParams(conn,
                                                &query[0],
                                                values.size(),
                                                nullptr,
                                                values.get(),
                                                nullptr,
                                                nullptr,
                                                0
                );
                return result;
            }

            PGresult *exec(const boost::string_view &query,
                           uint32_t retries = 3,
                           uint32_t retry_sleep_time = 10) const {
                check_connection(retries, retry_sleep_time);
                return PQexec(conn, &query[0]);
            }

            void exec(const std::string &query, scoped_result &result, uint32_t retries = 3,
                      uint32_t retry_sleep_time = 10) const {
                result = exec(query, retries, retry_sleep_time);
            }
        };
    }
}

#if __cplusplus < 201103L && !defined nullptr
#undef nullptr
#endif
