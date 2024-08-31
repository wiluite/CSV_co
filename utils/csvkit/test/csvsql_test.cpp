///
/// \file   test/csvsql_test.cpp
/// \author wiluite
/// \brief  Tests for the csvsql utility.

#define BOOST_UT_DISABLE_MODULE
#include "ut.hpp"

#include "../utils/csvkit/csvsql.cpp"
#include "strm_redir.h"
#include "common_args.h"
#include "cin_subst.h"

#define CALL_TEST_AND_REDIRECT_TO_COUT(call)    \
    std::stringstream cout_buffer;              \
    {                                           \
        redirect(cout)                          \
        redirect_cout cr(cout_buffer.rdbuf());  \
        call;                                   \
    }

int main() {
    using namespace boost::ut;
    namespace tf = csvkit::test_facilities;

#if defined (WIN32)
    cfg < override > = {.colors={.none="", .pass="", .fail=""}};
#endif

    "create table"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, tf::csvsql_specific_args {
            Args() {
                files = std::vector<std::string>{"testfixed_converted.csv"};
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
        struct Args : tf::common_args, tf::type_aware_args, tf::csvsql_specific_args {
            Args() {
                files = std::vector<std::string>{"blanks.csv"};
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
        struct Args : tf::common_args, tf::type_aware_args, tf::csvsql_specific_args {
            Args() {
                files = std::vector<std::string>{"blanks.csv"};
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
        struct Args : tf::common_args, tf::type_aware_args, tf::csvsql_specific_args {
            Args() {
                files = std::vector<std::string>{"testfixed_converted.csv"};
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
        struct Args : tf::common_args, tf::type_aware_args, tf::csvsql_specific_args {
            Args() {
                files = std::vector<std::string>{"no_header_row.csv"};
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
        struct Args : tf::common_args, tf::type_aware_args, tf::csvsql_specific_args {
            Args() {
                files = std::vector<std::string>{"dummy.csv"};
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
        struct Args : tf::common_args, tf::type_aware_args, tf::csvsql_specific_args {
            Args() {
                tables = "foo";
            }
        } args;

        std::istringstream iss("a,b,c\n4,2,3\n");
        cin_subst new_cin(iss);

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

    "stdin and filename"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, tf::csvsql_specific_args {
            Args() {
                files = std::vector<std::string>{"_", "dummy.csv"};               
            }
        } args;

        std::istringstream iss("a,b,c\n1,2,3\n");
        cin_subst new_cin(iss);

        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvsql::sql<notrimming_reader_type>(args)
        )

        expect(cout_buffer.str().find(R"(CREATE TABLE "stdin")") != std::string::npos);
        expect(cout_buffer.str().find(R"(CREATE TABLE "dummy")") != std::string::npos);
    };

    "query"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, tf::csvsql_specific_args {
            Args() {
                files = std::vector<std::string>{"iris.csv", "irismeta.csv"};
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
        struct Args : tf::common_args, tf::type_aware_args, tf::csvsql_specific_args {
            Args() {
                files = std::vector<std::string>{};
                query = "SELECT 1";
            }
        } args;

        std::istringstream iss(" ");
        cin_subst new_cin(iss);

        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvsql::sql<notrimming_reader_type>(args)
        )

        expect(cout_buffer.str() == "1\n1\n");
    };

    "query text"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, tf::csvsql_specific_args {
            Args() {
                files = std::vector<std::string>{"testfixed_converted.csv"};
                query = "SELECT text FROM testfixed_converted WHERE text LIKE \"Chicago%\"";
            }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvsql::sql<notrimming_reader_type>(args)
        )

        expect(cout_buffer.str() == "text\nChicago Reader\nChicago Sun-Times\nChicago Tribune\n");
    };

    "query file"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, tf::csvsql_specific_args {
            Args() {
                files = std::vector<std::string>{"testfixed_converted.csv"};
                query = "test_query.sql"; //TODO: see multiple queries in the file. Do the same.
            }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvsql::sql<notrimming_reader_type>(args)
        )

//      question,text
//      36,©
        expect(cout_buffer.str() == "question,text\n36,©\n");
    };

    "query update"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, tf::csvsql_specific_args {
            Args() {
                files = std::vector<std::string>{"dummy.csv"};
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
        db_file() : file("foo.db") {}
        ~db_file() {
            if (std::filesystem::exists(std::filesystem::path(file)))
                std::remove(file.c_str());
        }
        std::string operator()() {
            return file;
        }
    } ;

    "before and after insert"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, tf::csvsql_specific_args {
            db_file dbfile;
            Args() {
                files = std::vector<std::string>{"dummy.csv"};
                db = "sqlite3://db=" + dbfile();
                insert = true;
                before_insert = "CREATE TABLE foobar (date DATE)"; //TODO: see multiple queries in the file. Do the same.
                after_insert = "INSERT INTO dummy VALUES (0, 5, 6)";
            }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvsql::sql<notrimming_reader_type>(args)
        )
    };

    "no prefix unique constraint"_test = [] {
        struct Args : tf::common_args, tf::type_aware_args, tf::csvsql_specific_args {
            db_file dbfile;
            Args() {
                files = std::vector<std::string>{"dummy.csv"};
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

        bool catched = false;
        try {
            CALL_TEST_AND_REDIRECT_TO_COUT( csvsql::sql<notrimming_reader_type>(args) )
        } catch(soci::soci_error const & e) {
            expect(std::string(e.what()).find("UNIQUE constraint failed: dummy.a") != std::string::npos);
            catched = true;
        }
        expect(catched);
    };

}
