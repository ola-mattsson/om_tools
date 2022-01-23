/**
 * examples for using simple_libpq_helper, wrapper around libpq, the postgres C api.
 *
*/

#include <libpq_wrapper.h>
#include <boost/lexical_cast.hpp>

namespace ot = om_tools::libpq_helper;

/**
 * connect to default db on localhost, current user, default port...
 *  make sure you have postgres running and there is a user with with the same name as $USER
 *  This is only one db global connection. That's not efficient for example for a webserver
 *  accepting multiple clients simultaneously.
 *  Do one of 2 things, protect the global single connection from being used
 *  more than once at the time OR make a connection pool so every client has it's own
 *  connection but they are reused which makes for better performance
 */
ot::connection& default_connection(bool verify = false) {
    // add params to suite your environment
    static ot::connection conn("host=localhost");
    if (!conn) {
        std::cout << "No connection\n";
    }

    if (verify) {
        ot::scoped_result result(conn.exec("SELECT 1"));
        if (!result) {
            std::cout << "No connection\n";
        }
    }
    return conn;
}

void create_a_table() {
    ot::connection &conn = default_connection();

    boost::string_view stmt = "create table a ("
                              "id integer not null unique,"
                              "c char(8),"
                              "i integer,"
                              "cc char(20))";
    ot::scoped_result result;
    result = conn.exec(stmt);
    if (!result) {
        std::cout << "Failed to create table\n";
        return;
    }
}

void insert_this(int32_t id, const char* s, int32_t i, const char* ss) {
    std::string stmt("insert into a values($1, $2, $3, $4);");

    ot::params params;
    params.add(id).add(s).add(i).add(ss);

    ot::scoped_result result;
    result = default_connection().exec(stmt, params);

    if (!result) {
        std::cout << "insert failed\n";
    }
}

void create_data() {
    create_a_table();
    insert_this(1, "1", 11, "can't cast 11");
    insert_this(2, "2", 22, "can't cast 22");
    insert_this(3, "3", 33, "can't cast 33");
    insert_this(4, "4", 33, "can't cast 44");
    insert_this(5, "5", 33, "can't cast 55");
    insert_this(6, "6", 33, "can't cast 66");
    insert_this(7, "7", 33, "can't cast 77");
}

void pass_params() {
    std::string stmt = "select i, c, id, cc from a where id = $1;";
    ot::params params;
    params.add(2);
    ot::scoped_result result(default_connection().exec(stmt, params));

    if (result) {
        // get what ever type from the value
        // you can get a string from int columns but ints from
        // text columns may result in failed conversion.
        int32_t i = result.get_value<int32_t>(0, 2);
        std::string s = result.get_value<std::string>(0, 2);

        std::string i_s = result.get_value<std::string>(0, 0);
        std::string c = result.get_value<std::string>(0, 1);
        std::string cc = result.get_value<std::string>(0, 3);

        std::cout << s << '|' << i << ' ' << i_s << ' ' << c << ' ' << cc << '\n';
    }

    stmt = "select id, cc from a where c = $1";
    params.clear().add("3");
    result = default_connection().exec(stmt, params);
    if (result) {
        std::cout << result.get_value<int>(0,0) << ' ' << result.get_value<std::string>(0,1)<< '\n';
    }
}

void print_all() {
    std::string stmt = "select * from a";
    ot::scoped_result result(default_connection().exec(stmt));
    result.list_tuples();
}

void count() {
    std::string stmt = "select count(*), max(art) from iwaaa";
    ot::scoped_result result(default_connection().exec(stmt));
    if (result) {
        ot::scoped_result::row_iterator iter = result.begin();
        std::cout << iter.get<int>(0) << '|' << iter.get<int>(1) <<  '\n';
    }
}

bool has_data() {
    const char stmt[] = "SELECT EXISTS ("
                        "   SELECT FROM information_schema.tables"
                        "   WHERE table_name = 'a')";
    ot::scoped_result result(default_connection().exec(stmt));

    if (!result) {
        return false;
    } else {
        char exists = result.get_value<char>(0, 0);
        return exists == 't';
    }
}
void varchars() {
    ot::scoped_result result(default_connection().exec("select * from b"));
    if (result) {
        std::string v = result.get_value<std::string>(0, 0);
        std::string c = result.get_value<std::string>(0, 1);
        std::cout << v << ' ' << c << "\n";
    }
    std::cout << "\n";
}
int main() {
    try {
        varchars();
        if (!has_data()) {
            create_data();
        }
        pass_params();
        print_all();
        count();
    } catch (const std::exception& e) {
        std::clog << "Upps " << e.what() << '\n';
    }
}
