///
/// \file   test/csvsql_test.cpp
/// \author wiluite
/// \brief  Tests for the csvsql utility.

#define BOOST_UT_DISABLE_MODULE
#include "ut.hpp"

#include "../utils/csvkit/csvsql.cpp"
#include "../utils/csvkit/sql2csv.cpp"
#include "strm_redir.h"
#include "common_args.h"
#include "stdin_subst.h"

#define CALL_TEST_AND_REDIRECT_TO_COUT(call)    \
    std::stringstream cout_buffer;              \
    {                                           \
        redirect(cout)                          \
        redirect_cout cr(cout_buffer.rdbuf());  \
        call;                                   \
    }


using namespace boost::ut;
namespace tf = csvkit::test_facilities;

class SQL2CSV {
public:
    SQL2CSV(std::string const & db, std::string const & query) {
        sql2csvargs.db = db;
        sql2csvargs.query = query;
    }
    SQL2CSV& call() {
        expect(nothrow([&]{
            CALL_TEST_AND_REDIRECT_TO_COUT(sql2csv::sql_to_csv<notrimming_reader_type>(sql2csvargs))
            output = cout_buffer.str();
        }));
        return *this;
    }
    [[nodiscard]] std::string cout_buffer() const {
        return output;
    }
private:
    struct sql2csv_specific_args {
        std::filesystem::path query_file;
        std::string db;
        std::string query;
        bool linenumbers {false};
        std::string encoding {"UTF-8"};
        bool no_header {false};
    } sql2csvargs;

    std::string output;
};

struct table_dropper {
    table_dropper(std::string const & db, char const * const table_name) {
        soci::session sql (db);
        sql << "DROP TABLE " << table_name;
    }
};

std::string get_db_conn(char const * const env_var_name) {
    std::string db_conn;
    if (char const * env_p = getenv(env_var_name)) {
        db_conn = env_p;
        if (!db_conn.empty()) {
            // Well, further: we must unquote the quoted env var, because the SOCI framework imply DB connection
            // string can not be quoted (otherwise, SOCI does not evaluate the backend to load, properly).
            csv_co::string_functions::unquote(db_conn, '"');
            // So, why an ENV VAR can be quoted? This is because there can be complicated things like:
            // "oracle://service=//127.0.0.1:1521/xepdb1 user=usr password=pswd", and getenv can't extract it as is
            // from the environment.
        }
    }
    return db_conn;
}

int main() {
#if defined (WIN32)
    cfg < override > = {.colors={.none="", .pass="", .fail=""}};
#endif

    OutputCodePage65001 ocp65001;

    struct csvsql_specific_args {
        std::vector<std::string> files;
        std::string dialect;
        std::string db;
        std::string query;
        bool insert {false};
        std::string prefix;
        std::string before_insert;
        std::string after_insert;
        std::string tables;
        bool no_constraints {false};
        std::string unique_constraint;
        bool no_create {false};
        bool create_if_not_exists {false};
        bool overwrite {false};
        std::string schema {false};
        unsigned chunk_size {0};
    };

    "create table"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            Args() {
                files = {"testfixed_converted.csv"};
                tables = "foo";
                date_lib_parser = true;
            }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvsql::sql<notrimming_reader_type>(args)
        )

        auto no_tabs_rep = cout_buffer.str();
        std::replace(no_tabs_rep.begin(), no_tabs_rep.end(), '\t', ' ');
        expect(no_tabs_rep == R"(CREATE TABLE foo (
 text VARCHAR NOT NULL,
 date DATE,
 integer DECIMAL,
 boolean BOOLEAN,
 float DECIMAL,
 time DATETIME,
 datetime TIMESTAMP,
 empty_column BOOLEAN
);
)");
    };

    "no blanks"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            Args() {
                files = {"blanks.csv"};
                tables = "foo";
            }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvsql::sql<notrimming_reader_type>(args)
        )

        auto no_tabs_rep = cout_buffer.str();
        std::replace(no_tabs_rep.begin(), no_tabs_rep.end(), '\t', ' ');
        expect(no_tabs_rep == R"(CREATE TABLE foo (
 a BOOLEAN,
 b BOOLEAN,
 c BOOLEAN,
 d BOOLEAN,
 e BOOLEAN,
 f BOOLEAN
);
)");
    };

    "blanks"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            Args() {
                files = {"blanks.csv"};
                tables = "foo";
                blanks = true;
            }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvsql::sql<notrimming_reader_type>(args)
        )

        auto no_tabs_rep = cout_buffer.str();
        std::replace(no_tabs_rep.begin(), no_tabs_rep.end(), '\t', ' ');
        expect(no_tabs_rep == R"(CREATE TABLE foo (
 a VARCHAR NOT NULL,
 b VARCHAR NOT NULL,
 c VARCHAR NOT NULL,
 d VARCHAR NOT NULL,
 e VARCHAR NOT NULL,
 f VARCHAR NOT NULL
);
)");
    };

    "no inference"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            Args() {
                files = {"testfixed_converted.csv"};
                tables = "foo";
                date_lib_parser = true;
                no_inference = true;
            }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvsql::sql<notrimming_reader_type>(args)
        )

        auto no_tabs_rep = cout_buffer.str();
        std::replace(no_tabs_rep.begin(), no_tabs_rep.end(), '\t', ' ');
        expect(no_tabs_rep == R"(CREATE TABLE foo (
 text VARCHAR NOT NULL,
 date VARCHAR,
 integer VARCHAR,
 boolean VARCHAR,
 float VARCHAR,
 time VARCHAR,
 datetime VARCHAR,
 empty_column VARCHAR
);
)");
    };

    "no header row"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            Args() {
                files = {"no_header_row.csv"};
                tables = "foo";
                no_header = true;
            }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvsql::sql<notrimming_reader_type>(args)
        )

        auto no_tabs_rep = cout_buffer.str();
        std::replace(no_tabs_rep.begin(), no_tabs_rep.end(), '\t', ' ');
        expect(no_tabs_rep == R"(CREATE TABLE foo (
 a BOOLEAN NOT NULL,
 b DECIMAL NOT NULL,
 c DECIMAL NOT NULL
);
)");
    };

    "linenumbers"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            Args() {
                files = {"dummy.csv"};
                tables = "foo";
                linenumbers = true;
            }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvsql::sql<notrimming_reader_type>(args)
        )

        auto no_tabs_rep = cout_buffer.str();
        std::replace(no_tabs_rep.begin(), no_tabs_rep.end(), '\t', ' ');
        expect(no_tabs_rep == R"(CREATE TABLE foo (
 a BOOLEAN NOT NULL,
 b DECIMAL NOT NULL,
 c DECIMAL NOT NULL
);
)");
    };

    "stdin"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            Args() {
                tables = "foo";
                files = {"_"};
            }
        } args;

        std::istringstream iss("a,b,c\n4,2,3\n");
        stdin_subst new_cin(iss);

        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvsql::sql<notrimming_reader_type>(args)
        )

        auto no_tabs_rep = cout_buffer.str();
        std::replace(no_tabs_rep.begin(), no_tabs_rep.end(), '\t', ' ');
        expect(no_tabs_rep == R"(CREATE TABLE foo (
 a DECIMAL NOT NULL,
 b DECIMAL NOT NULL,
 c DECIMAL NOT NULL
);
)");
    };

    "piped stdin"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            Args() = default; // NOTE: now we do not use '_' placeholder to help. We read a csv file and pipe it with us.
        } args;

        std::istringstream iss(" ");
        stdin_redir sr("piped_stdin");

        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvsql::sql<notrimming_reader_type>(args)
        )

        auto no_tabs_rep = cout_buffer.str();
        std::replace(no_tabs_rep.begin(), no_tabs_rep.end(), '\t', ' ');
        expect(no_tabs_rep == R"(CREATE TABLE stdin (
 a DATETIME NOT NULL
);
)");
    };


    "stdin and filename"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            Args() {
                files = {"_", "dummy.csv"};
            }
        } args;

        std::istringstream iss("a,b,c\n1,2,3\n");
        stdin_subst new_cin(iss);

        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvsql::sql<notrimming_reader_type>(args)
        )

        expect(cout_buffer.str().find(R"(CREATE TABLE stdin)") != std::string::npos);
        expect(cout_buffer.str().find(R"(CREATE TABLE dummy)") != std::string::npos);
    };

    "query"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            Args() {
                files = {"iris.csv", "irismeta.csv"};
                query = "SELECT m.usda_id, avg(i.sepal_length) AS mean_sepal_length FROM iris "
                        "AS i JOIN irismeta AS m ON (i.species = m.species) GROUP BY m.species";
            }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvsql::sql<notrimming_reader_type>(args)
        )
        expect(cout_buffer.str().find("usda_id,mean_sepal_length") != std::string::npos);
        expect(cout_buffer.str().find("IRSE,5.00") != std::string::npos);
        expect(cout_buffer.str().find("IRVE2,5.936") != std::string::npos);
        expect(cout_buffer.str().find("IRVI,6.58") != std::string::npos);
    };

    "query empty"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            Args() {
                files = {"_"};
                query = "SELECT 1";
            }
        } args;

        std::istringstream iss(" ");
        stdin_subst new_cin(iss);

        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvsql::sql<notrimming_reader_type>(args)
        )

        expect(cout_buffer.str() == "1\n1\n");
    };

    "query text"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            Args() {
                files = {"testfixed_converted.csv"};
                query = "SELECT text FROM testfixed_converted WHERE text LIKE \"Chicago%\"";
            }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvsql::sql<notrimming_reader_type>(args)
        )

        expect(cout_buffer.str() == "text\nChicago Reader\nChicago Sun-Times\nChicago Tribune\n");
    };

    "query file"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            Args() {
                files = {"testfixed_converted.csv"};
                query = "test_query.sql";
            }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvsql::sql<notrimming_reader_type>(args)
        )

//      question,text
//      36,©
        expect(cout_buffer.str() == "question,text\n36,©\n");
    };

    "no UTF-8 query file"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            Args() {
                files = {"testfixed_converted.csv"};
                query = "test_query_1252.sql";
            }
        } args;

        expect(throws([&]{ CALL_TEST_AND_REDIRECT_TO_COUT( csvsql::sql<notrimming_reader_type>(args) ) }));
    };

    "CP1252 query file"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            Args() {
                files = {"testfixed_converted.csv"};
                query = "test_query_1252.sql";
                encoding = "CP1252";
            }
        } args;

        expect(nothrow([&]{
            CALL_TEST_AND_REDIRECT_TO_COUT( csvsql::sql<notrimming_reader_type>(args) )

//          question,text
//          36,©
            expect(cout_buffer.str() == "question,text\n36,©\n");

        }));
    };

    "empty query file"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            Args() {
                files = {"testfixed_converted.csv"};
                query = "test_query_empty.sql";
            }
        } args;

        try {
            CALL_TEST_AND_REDIRECT_TO_COUT( csvsql::sql<notrimming_reader_type>(args) )
        } catch (std::exception const & e) {
            expect(std::string(e.what()).find("Query file 'test_query_empty.sql' exists, but it is empty.") != std::string::npos);
        }
    };

    "query update"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            Args() {
                files = {"dummy.csv"};
                no_inference = true;
                query = "UPDATE dummy SET a=10 WHERE a=1";
            }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvsql::sql<notrimming_reader_type>(args)
        )

        expect(cout_buffer.str().empty());
    };

    class db_file {
        std::string file;
    public:
        explicit db_file(char const * const name = "foo.db") : file(name) {}
        ~db_file() {
            if (std::filesystem::exists(std::filesystem::path(file)))
                std::remove(file.c_str());
        }
        std::string operator()() {
            return file;
        }
        std::string operator()() const {
            return file;
        }
    };

    "before and after insert"_test = [] {
        // Longer test
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            db_file dbfile;
            Args() {
                files = {"dummy.csv"};
                db = "sqlite3://db=" + dbfile();
                insert = true;
                before_insert = "SELECT 1; CREATE TABLE foobar (date DATE)";
                after_insert = "INSERT INTO dummy VALUES (0, 5, 6.1)";
            }
            std::string str1;
        } args;

        {
            CALL_TEST_AND_REDIRECT_TO_COUT(
                csvsql::sql<notrimming_reader_type>(args)
            )
        }

        expect(SQL2CSV{"sqlite3://db=" + args.dbfile(), "SELECT * FROM foobar"}.call().cout_buffer() == "date\n");
        expect(SQL2CSV{"sqlite3://db=" + args.dbfile(), "SELECT * from dummy"}.call().cout_buffer() == "a,b,c\n1,2,3\n0,5,6.1\n");
    };

    "no prefix unique constraint"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            db_file dbfile;
            Args() {
                files = {"dummy.csv"};
                db = "sqlite3://db=" + dbfile();
                insert = true;
                unique_constraint = "a";
           }
        } args;

        {
            CALL_TEST_AND_REDIRECT_TO_COUT(
                csvsql::sql<notrimming_reader_type>(args)
            )
        }
        {   // now "unique constraint" exception
            args.no_create = true;
            bool catched = false;
            try {
                CALL_TEST_AND_REDIRECT_TO_COUT( csvsql::sql<notrimming_reader_type>(args) )
            } catch(soci::soci_error const & e) {
                expect(std::string(e.what()).find("UNIQUE constraint failed: dummy.a") != std::string::npos);
                catched = true;
            }
            expect(catched);
        }
    };

    "prefix unique constraint"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            db_file dbfile;
            Args() {
                files = {"dummy.csv"};
                db = "sqlite3://db=" + dbfile();
                insert = true;
                unique_constraint = "a";
           }
        } args;

        {
            CALL_TEST_AND_REDIRECT_TO_COUT(
                csvsql::sql<notrimming_reader_type>(args)
            )
        }

        args.no_create = true;
        args.prefix = "OR IGNORE";

        expect(nothrow([&]{CALL_TEST_AND_REDIRECT_TO_COUT( csvsql::sql<notrimming_reader_type>(args)) }));
    };

    "no create-if-not-exists"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            db_file dbfile;
            Args() {
                files = {"foo1.csv"};
                db = "sqlite3://db=" + dbfile();
                insert = true;
                tables = "foo";
            }
        } args;

        {
            CALL_TEST_AND_REDIRECT_TO_COUT(
                csvsql::sql<notrimming_reader_type>(args)
            )
        }

        args.files = {"foo2.csv"};
        {
            bool catched = false;
            try {
                CALL_TEST_AND_REDIRECT_TO_COUT( csvsql::sql<notrimming_reader_type>(args) )
            } catch(soci::soci_error const & e) {
                expect(std::string(e.what()).find("table foo already exists") != std::string::npos);
                catched = true;
            }
            expect(catched);
        }
    };

    "create-if-not-exists"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            db_file dbfile;
            Args() {
                files = {"foo1.csv"};
                db = "sqlite3://db=" + dbfile();
                insert = true;
                tables = "foo";
            }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(csvsql::sql<notrimming_reader_type>(args))

        args.files = {"foo2.csv"};
        args.create_if_not_exists = true;
        expect(nothrow([&]{CALL_TEST_AND_REDIRECT_TO_COUT( csvsql::sql<notrimming_reader_type>(args)) }));
    };

    "batch_bulk_inserter"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            Args() {
                files = {"_"};
                insert = true;
                query = "SELECT * FROM stdin";
                chunk_size = 3;
            }
        } args;
        std::istringstream iss("a,b\n1,1972-04-14\n3,1973-05-15\n5,1974-06-16\n7, \n9,04/18/1976\n");
        stdin_subst new_cin(iss);
        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvsql::sql<notrimming_reader_type>(args)
        )
        expect(cout_buffer.str() == "a,b\n1,1972-04-14\n3,1973-05-15\n5,1974-06-16\n7,\n9,1976-04-18\n");
    };

#if defined(SOCI_HAVE_SQLITE3)
    "Sqlite3 date, datetime, timedelta"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            Args() {
                files = {"_"};
                insert = true;
                query = "SELECT * FROM stdin";
                chunk_size = 2;
            }
        } args;
        std::string db_conn = get_db_conn("SOCI_DB_SQLITE3");
        if (!db_conn.empty()) {
            args.db = db_conn;
            std::istringstream iss("a,b,c\n1971-01-01,1971-01-01T04:14:00,2 days 01:14:47.123\n");
            stdin_subst new_cin(iss);
            CALL_TEST_AND_REDIRECT_TO_COUT(
                csvsql::sql<notrimming_reader_type>(args)
            )

//          a,b,c
//          1971-01-01,1971-01-01 04:14:00.000000,1970-01-03 01:14:47.123000
            expect(cout_buffer.str() == "a,b,c\n1971-01-01,1971-01-01 04:14:00.000000,1970-01-03 01:14:47.123000\n");
            table_dropper{db_conn, "stdin"};
        }
    };
#endif

#if defined(SOCI_HAVE_MYSQL)
    "MySQL date, datetime, timedelta"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            Args() {
                files = {"_"};
                insert = true;
                query = "SELECT * FROM stdin";
            }
        } args;
        std::string db_conn = get_db_conn("SOCI_DB_MYSQL");
        if (!db_conn.empty()) {
            args.db = db_conn;
            std::istringstream iss("a,b,c\n1971-01-01,1971-01-01T04:14:00,2 days 01:14:47.123\n");
            stdin_subst new_cin(iss);
            CALL_TEST_AND_REDIRECT_TO_COUT(
                csvsql::sql<notrimming_reader_type>(args)
            )

//          a,b,c
//          1971-01-01,1971-01-01 04:14:00.000000,1970-01-03 01:14:47.000000
            expect(cout_buffer.str() == "a,b,c\n1971-01-01,1971-01-01 04:14:00.000000,1970-01-03 01:14:47.000000\n");
            table_dropper{db_conn, "stdin"};
        }
    };
#endif

#if defined(SOCI_HAVE_POSTGRESQL)
    "PostgreSQL date, datetime, timedelta"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            Args() {
                files = {"_"};
                insert = true;
                query = "SELECT * FROM stdin";
                chunk_size = 1;
            }
        } args;
        std::string db_conn = get_db_conn("SOCI_DB_POSTGRESQL");
        if (!db_conn.empty()) {
            args.db = db_conn;
            std::istringstream iss("a,b,c\n1971-01-01,1971-01-01T04:14:00,5 days 01:14:10.737\n");
            stdin_subst new_cin(iss);
            CALL_TEST_AND_REDIRECT_TO_COUT(
                csvsql::sql<notrimming_reader_type>(args)
            )

//          a,b,c
//          1971-01-01,1971-01-01 04:14:00.000000,121:14:10.737
            expect(cout_buffer.str() == "a,b,c\n1971-01-01,1971-01-01 04:14:00.000000,121:14:10.737\n");
            table_dropper{db_conn, "stdin"};
        }
    };
#endif

#if defined(SOCI_HAVE_FIREBIRD)
    "Firebird date, datetime, timedelta"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            Args() {
                files = {"_"};
                insert = true;
                query = "SELECT * FROM stdin";
                chunk_size = 2;
            }
        } args;
        std::string db_conn = get_db_conn("SOCI_DB_FIREBIRD");
        if (!db_conn.empty()) {
            args.db = db_conn;
            std::istringstream iss("a,b,c\n1971-01-01,1971-01-01T04:14:00,2 days 01:14:47.123\n");
            stdin_subst new_cin(iss);
            CALL_TEST_AND_REDIRECT_TO_COUT(
                csvsql::sql<notrimming_reader_type>(args)
            )

//          A,B,C
//          1971-01-01,1971-01-01 04:14:00.-00001,1970-01-03 01:14:47.-00001
            expect(cout_buffer.str() == "A,B,C\n1971-01-01,1971-01-01 04:14:00.000000,1970-01-03 01:14:47.000000\n");
            table_dropper{db_conn, "stdin"};
        }
    };
#endif

#if defined(SOCI_HAVE_ORACLE)
    "Oracle date, datetime, timedelta"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, csvsql_specific_args {
            Args() {
                files = {"_"};
                insert = true;
                query = "SELECT * FROM stdin";
                chunk_size = 2;
            }
        } args;
        std::string db_conn = get_db_conn("SOCI_DB_ORACLE");
        if (!db_conn.empty()) {
            args.db = db_conn;
            std::istringstream iss("a,b,c\n1971-01-01,2973-02-01T04:14:01, 2 d 01:14:47.123\n1972-01-01,1972-01-01 04:14:00,01:14:47.123\n");
            stdin_subst new_cin(iss);
            CALL_TEST_AND_REDIRECT_TO_COUT(
                csvsql::sql<notrimming_reader_type>(args)
            )


//          A,B,C
//          1971-01-01,01-JAN-71 04.14.01.000000 AM,"2 days, 01:14:47.123000"
//          1972-01-01,01-JAN-72 04.14.00.000000 AM,01:14:47.123000
            expect(cout_buffer.str() == "A,B,C\n1971-01-01,01-FEB-73 04.14.01.000000 AM,\"2 days, 01:14:47.123000\"\n1972-01-01,01-JAN-72 04.14.00.000000 AM,01:14:47.123000\n");
            table_dropper{db_conn, "stdin"};
        }
    };
#endif

}
