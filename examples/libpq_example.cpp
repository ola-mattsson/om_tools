/**
 * examples for using simple_libpq_helper, wrapper around libpq, the postgres C api.
 *
*/

#include <simple_libpq_helper.h>
#include <boost/lexical_cast.hpp>

namespace ot = om_tools::simple_libpq_helper;

/**
 * connect to default db on localhost, current user, default port...
 *  make sure you have postgres running and there is a user with with the same name as $USER
 */
ot::connection& default_connection() {
    static ot::connection conn;
    if (!conn) {
        std::cout << "No connection\n";
    }

    ot::scoped_result result(conn.exec("SELECT 1"));
    if (!result) {
        std::cout << "No connection\n";
    }
    return conn;
}

void create_a_table() {
    ot::connection &conn = default_connection();

    boost::string_view stmt = "create table if not exists  a (id int not null unique, c char(10), i int)";
    ot::scoped_result result;
    result = conn.exec(stmt);
    if (!result) {
        std::cout << "Failed to create table\n";
        return;
    }
}

void insert_this(int32_t i, const char* s, int32_t ii) {
    std::string stmt("insert into a values($1, $2, $3);");

    ot::params params;
    params.add(i).add(s).add(ii);

    ot::scoped_result result;
    result = default_connection().exec_params(stmt, params);

    if (!result) {
        std::cout << "insert failed\n";
    }
}

void create_data() {
    create_a_table();
    insert_this(1, "1", 11);
    insert_this(2, "2", 22);
    insert_this(3, "3", 33);
    insert_this(4, "4", 33);
    insert_this(5, "5", 33);
    insert_this(6, "6", 33);
    insert_this(7, "7", 33);
}

void pass_params() {
    std::string stmt = "select i from a where id = $1;";
    ot::params params;
    params.add(2);
    ot::scoped_result result(default_connection().exec_params(stmt, params));

    if (result) {
        // get what ever type from the value
        int32_t i = result.get_value<int32_t>(0, 2);
        std::string s = result.get_value<std::string>(0, 2);
        std::cout << s << '|' << i << "\n";
    }
}

void print_all() {
    std::string stmt = "select c, i from a";
    ot::scoped_result result(default_connection().exec(stmt));
    if (result) {
        ot::scoped_result::row_iterator iter = result.begin();
        for (; iter < result.end(); ++iter) {
            std::cout << iter.get<std::string>(0) << '|' << iter.get<int>(1) << '\n';
        }
    }
}

void count() {
    std::string stmt = "select count(*), max(art) from iwaaa";
    ot::scoped_result result(default_connection().exec(stmt));
    if (result) {
        ot::scoped_result::row_iterator iter = result.begin();
        std::cout << iter.get<int>(0) << '|' << iter.get<int>(1) <<  '\n';
    }
}

int main() {
    pass_params();
    print_all();
    count();
}
