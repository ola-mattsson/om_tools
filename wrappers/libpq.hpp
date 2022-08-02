#pragma once
/*
 * No nonsense C++ wrapper around PostgreSQL libpq api that works even in C++98
 *
 * All text transfer for simplicity. For binary transfer, you need a bigger boat, there
 * are plenty around but most require newer C++.
 * Values are translated to and from required types.
 *
 * Call is done with connection().exec() that returns a PGresult pointer. Catch it with the
 * scoped_result class to validate the result and to retrieve field values with
 * function template get_value<>(int row, int column) where the type parameter is the
 * required type.
 * Multiple row results are easiest iterated with scoped_result::row_iterator that serves
 * column value with get<>(int column);
 *
 * Parameter validation is rudimentary, use scoped_result::row_iterator to iterate result
 * rows safely. requesting row/column out of bounds will return an empty value, "" or 0 and so on.
 * So, good calls gives good response, bad call gets empty response. keep it simple
 *
 * scoped_result handles memory management, if it is passed a new pointer, then the
 * old will be freed, just like when going out of scope as expected
 */

#include "libpq-fe.h"
#include <boost/utility/string_view.hpp>
#include <boost/core/noncopyable.hpp>
#include <boost/optional.hpp>

#if defined __VMS
// some older compiler can't compile boost::lexical_cast<>, but there
// is an old version that may work. at least that's the case for VSI C++
// it is not as fast, barely measurably slower than std::stringstream,
// but it is a nice interface
// #include <boost/lexical_cast/lexical_cast_old.hpp>
#else

#include <boost/lexical_cast.hpp>

#endif

#include <iostream>
#include <vector>

// stop linters complaining about NULL for pre C++11 standards
#if __cplusplus < 201103L
#define EXPLICIT
#define NOEXCEPT throw()
#if !defined nullptr
#define nullptr NULL
#define defined_nullptr
#endif
#else
#define EXPLICIT explicit
#define NOEXCEPT noexcept
#endif


namespace om_tools::libpq_helper {
inline namespace v1_0_0 {

class Timestamp {
    std::string m_time;
    double m_time_d{};
    bool numeric() {
        return m_time.at(2) == '-';
    }
public:
    Timestamp() : m_time_d(0) {}

    explicit Timestamp(time_t time) : m_time_d(time) {}

    explicit Timestamp(double time) : m_time_d(time) {}

    explicit Timestamp(boost::string_view t) : m_time(t) {}

    // boost lexical_cast needs this for derived types
    friend std::istream& operator>> (std::istream& is, Timestamp& t) {
        is >> t.m_time;
        return is;
    }

    friend std::ostream &operator<<(std::ostream &os, const Timestamp &timestamp) {
        os << timestamp.m_time;
        return os;
    }

#if __cplusplus >= 201103L
    [[nodiscard]]
#endif
    time_t seconds() const {
//                assert(m_time_d < std::numeric_limits<time_t>::max());
        return static_cast<time_t>(m_time_d);
    }

    std::string seconds_string() {
        std::string result;
        if (numeric()) {
            result = boost::lexical_cast<std::string>(seconds());
        } else {
            result = to_string();
            result = std::string(m_time.data(), m_time.find('.'));
        }
        return result;
    }

    time_t to_time_t() {
        if (numeric()) {
            return boost::lexical_cast<time_t>(seconds());
        } else {
            return seconds(); // boost::lexical_cast<time_t>()
        }
    }

    time_t to_timestamp_string() {
        if (numeric()) {
            return boost::lexical_cast<time_t>(seconds());
        } else {
            return seconds(); // boost::lexical_cast<time_t>()
        }
    }

    std::string to_string() const {
        char buffer[25] = {};
        tm time = {};
        time_t t = seconds();
        ::localtime_r(&t, &time);
        // todo add fractions/millis whatever
        ::strftime(buffer, sizeof buffer, "%F %T", &time);
        return buffer;
    }
};

class Scoped_result : public boost::noncopyable {
    PGresult *result;
    mutable std::string last_error;

public:
    Scoped_result() NOEXCEPT
        : result(nullptr) {}

    explicit Scoped_result(PGresult *res) NOEXCEPT
        : result(res) {}

    ~Scoped_result() NOEXCEPT { if (result) PQclear(result); }

    EXPLICIT operator PGresult *() const { return result; }

    EXPLICIT operator bool() const {
        if (result) {
            ExecStatusType status = PQresultStatus(result);
            return status == PGRES_COMMAND_OK || status == PGRES_TUPLES_OK;
        }
        return false;
    }

    Scoped_result &operator=(PGresult *res) NOEXCEPT {
        clear();
        result = res;
        return *this;
    }

    void clear() NOEXCEPT {
        if (result) {
            PQclear(result);
            result = nullptr;
        }
    }

    std::string &get_error() const NOEXCEPT {
        if (result) {
            last_error = PQresultErrorMessage(result);
        } else {
            last_error.clear();
        }
        return last_error;
    }

    std::string &get_last_error() const NOEXCEPT {
        return last_error;
    }

    int32_t get_rows() const NOEXCEPT {
        return PQntuples(result);
    }

    int32_t get_columns() const NOEXCEPT {
        return PQnfields(result);
    }

    // get the converted value
    template<typename T>
    T get_value(int32_t row, int32_t column) const {
        if (0 < get_rows() && row < get_rows() && column < get_columns()) {
            return boost::lexical_cast<T>(get_string_value(row, column));
        }
        return T();
    }

    // get the char array value, use for large string values
    const char *get_string_value(int32_t row, int32_t column) const NOEXCEPT {
        return PQgetvalue(result, row, column);
    }

    // length of the field at row/column
    size_t get_length(int32_t row, int32_t column) NOEXCEPT {
        return PQgetlength(result, row, column);
    }


    // dump result data, not very useful perhaps
    void list_tuples() const NOEXCEPT;

    // the row iterator, not a true iterator but a counter to be
    // used in a familiar fashion as a stl containers iterator
    // for large datasets, declare the end object before instead
    // of testing with "iter != result.end()"
    struct row_iterator {
        const Scoped_result &res;
        int32_t row_num;

        explicit row_iterator(const Scoped_result &result) : res(result), row_num(0) {}

        row_iterator &operator++() {
            ++row_num;
            return *this;
        }

        bool operator<(const row_iterator &other) const {
            return row_num < other.row_num;
        }

        bool operator!=(const row_iterator &other) const {
            return row_num != other.row_num;
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

    row_iterator end() const {
        row_iterator end(*this);
        end.row_num = get_rows();
        return end;
    }

    static void check(PGresult *result) {
        ExecStatusType status = PQresultStatus(result);
        if (status != PGRES_COMMAND_OK && status != PGRES_TUPLES_OK) {
            char const *msg = PQresultErrorMessage(result);
            throw std::runtime_error(msg ? msg : "PG error");
        }
    }
};

void Scoped_result::list_tuples() const NOEXCEPT {
    Scoped_result::row_iterator end_ = end();
    for (Scoped_result::row_iterator iter(*this); iter != end_; ++iter) {
        for (int32_t column(0); column < get_columns(); ++column) {
            std::cout << iter.get<std::string>(column) << '|';
        }
        std::cout << '\n';
    }
}

class Params {
    std::vector<std::string> data;
    mutable const char *array[30];
    typedef std::vector<std::string>::const_iterator iterator;
public:
    Params() : array() {}

    template<class T>
    Params &add(const T &item) {
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

    Params &clear() {
        data.clear();
        return *this;
    }
};

class PGConnection {
    PGconn *conn;
    mutable std::string last_error;

    PGConnection &operator=(const PGConnection *) { return *this; }

    PGConnection &operator=(const PGConnection &other) {
        if (this != &other) return *this;
        if (conn) PQfinish(conn);
        conn = other.conn;
        last_error.clear();
        return *this;
    }

public:
    explicit PGConnection(const boost::string_view &connect_info = "")
        : conn(nullptr) {
        conn = PQconnectdb(&connect_info[0]);

        /* Check to see that the backend connection was successfully made */
        if (PQstatus(conn) != CONNECTION_OK) {
            std::cerr << "Connection to database failed: " << PQerrorMessage(conn);
            conn = nullptr;
        }
    }

    ~PGConnection() {
        PQfinish(conn);
    }

    EXPLICIT operator PGconn *() const { return conn; }

    bool check_connection(uint32_t retries = 3, uint32_t retry_delay = 30) const {
        uint32_t retry = 0;
        while (PQstatus(conn) != CONNECTION_OK && retry < retries) {
            ++retry;
            std::cerr
                << "PostgreSQL connection bad. trying to reconnect (attempt "
                << retry << ")." << std::endl;
            PQreset(conn);
            if (PQstatus(conn) != CONNECTION_OK) {
                sleep(retry_delay * retry);
            } else {
                return true; // successful reconnect
            }
        }
        return retry < retries; // true if not all retries were used
    }

    PGresult *exec(const boost::string_view &query,
                   const Params &values,
                   uint32_t retries = 0,
                   uint32_t retry_sleep_time = 0) const {
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
        Scoped_result::check(result);
        return result;
    }

    PGresult *exec(const boost::string_view &query,
                   uint32_t retries = 3,
                   uint32_t retry_sleep_time = 10) const {
        if (check_connection(retries, retry_sleep_time)) {
            PGresult *result = PQexec(conn, &query[0]);
            Scoped_result::check(result);
            return result;
        }
        return nullptr;
    }

    /**
     * connection pool support, allows a pool to TEST the connection that may
     * have been idle in the pool for a period, typically longer than
     * server timout or any reason for the server to hang up.
     * Calling this every time a connection is requested is ofc a bad idea, you decide.
     * @return true if the connection looks fine
     */
    [[nodiscard]]
    bool test_connection() const noexcept {
        if (check_connection()) {
            try {
                PGresult * result = exec("SELECT 1");
                Scoped_result::check(result);
                return true;
            } catch (const std::exception&) { /*failed*/ }
        }
        return false;
    }

    /**
     * connection pool support, allows a pool to decide if the connection is worth keeping
     * @return true if the connection looks fine
     */
    [[nodiscard]]
    bool good_connection() const noexcept {
        return check_connection();
    }

    /**
     * Do nothing, this class doesn't hold state
     * other than the connection itself
     */
    void reset() noexcept {}

};

// basic error handling, to be continued, if necessary
class Error {
    enum STATE {
        PG_STATE_OK,
        PG_STATE_WARNING,
        PG_STATE_NO_DATA_WARNING,
        PG_STATE_NO_CONNECTION_ERROR,
        PG_STATE_ERROR
    };
    STATE state;
    std::string error_message;


    explicit Error(PGresult *result) : state(PG_STATE_OK) {
        ExecStatusType exec_state = PQresultStatus(result);
        if (exec_state == PGRES_COMMAND_OK || exec_state == PGRES_TUPLES_OK) {
            return;
        }
        const char *error_field = PQresultErrorField(result, PG_DIAG_SQLSTATE);
        if (nullptr == error_field) {
            // no error, works for me
        }
        boost::string_view category(error_field, 2);
        if (category == "00") {}
        else if (category == "01") {
            state = PG_STATE_WARNING;
        } else if (category == "02") {
            state = PG_STATE_NO_DATA_WARNING;
            // no data warning
        } else if (category == "03") {
            state = PG_STATE_ERROR;
        } else if (category == "08") {
            state = PG_STATE_NO_CONNECTION_ERROR;
        } else if (category == "09") {
            state = PG_STATE_ERROR;
        } else if (category == "0A") {
            state = PG_STATE_ERROR;
        } else if (category == "0B") {
            state = PG_STATE_ERROR;
        } else if (category == "0F") {
            state = PG_STATE_ERROR;
        } else if (category == "0L") {
            state = PG_STATE_ERROR;
        } else if (category == "0P") {
            state = PG_STATE_ERROR;
        } else if (category == "0Z") {
            state = PG_STATE_ERROR;
        } else if (category == "20") {
            state = PG_STATE_ERROR;
        } else if (category == "21") {
            state = PG_STATE_ERROR;
        } else if (category == "22") {
            state = PG_STATE_ERROR;
        } else if (category == "23") {
            state = PG_STATE_ERROR;
        } else if (category == "24") {
            state = PG_STATE_ERROR;
        } else if (category == "25") {
            state = PG_STATE_ERROR;
        } else if (category == "26") {
            state = PG_STATE_ERROR;
        } else if (category == "27") {
            state = PG_STATE_ERROR;
        } else if (category == "28") {
            state = PG_STATE_ERROR;
        } else if (category == "2B") {
            state = PG_STATE_ERROR;
        } else if (category == "2D") {
            state = PG_STATE_ERROR;
        } else if (category == "2F") {
            state = PG_STATE_ERROR;
        } else if (category == "34") {
            state = PG_STATE_ERROR;
        } else if (category == "38") {
            state = PG_STATE_ERROR;
        } else if (category == "39") {
            state = PG_STATE_ERROR;
        } else if (category == "3B") {
            state = PG_STATE_ERROR;
        } else if (category == "3D") {
            state = PG_STATE_ERROR;
        } else if (category == "3F") {
            state = PG_STATE_ERROR;
        } else if (category == "40") {
            state = PG_STATE_ERROR;
        } else if (category == "42") {
            state = PG_STATE_ERROR;
        } else if (category == "44") {
            state = PG_STATE_ERROR;
        } else if (category == "53") {
            state = PG_STATE_ERROR;
        } else if (category == "54") {
            state = PG_STATE_ERROR;
        } else if (category == "55") {
            state = PG_STATE_ERROR;
        } else if (category == "57") {
            state = PG_STATE_ERROR;
        } else if (category == "58") {
            state = PG_STATE_ERROR;
        } else if (category == "72") {
            state = PG_STATE_ERROR;
        } else if (category == "F0") {
            state = PG_STATE_ERROR;
        } else if (category == "HV") {
            state = PG_STATE_ERROR;
        }
        if (category != "00") {
            const char *msg = PQresultErrorMessage(result);
            if (msg) {
                error_message = msg;
            }
        }
    }


};
}
}


#if __cplusplus < 201103L && defined defined_nullptr
#undef nullptr
#endif
