///
/// \file   test/csvsql_test.cpp
/// \author wiluite
/// \brief  Tests for the csvsql utility.

#define BOOST_UT_DISABLE_MODULE
#include "ut.hpp"

#include "../utils/csvkit/csvsql.cpp"
#include "strm_redir.h"
#include "common_args.h"

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
        auto cin_buf = std::cin.rdbuf();
        std::cin.rdbuf(iss.rdbuf());
        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvsql::sql<notrimming_reader_type>(args)
        )
        std::cin.rdbuf(cin_buf);

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
        auto cin_buf = std::cin.rdbuf();
        std::cin.rdbuf(iss.rdbuf());
        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvsql::sql<notrimming_reader_type>(args)
        )
        std::cin.rdbuf(cin_buf);

        expect(cout_buffer.str().find(R"(CREATE TABLE "stdin")") != std::string::npos);
        expect(cout_buffer.str().find(R"(CREATE TABLE "dummy")") != std::string::npos);
    };

#if 0
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
        std::cout << cout_buffer.str() << std::endl; 
    };
#endif

}