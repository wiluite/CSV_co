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
                date_lib_parser = true;
            }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(
            csvsql::sql<notrimming_reader_type>(args)
        )

        auto no_tabs_rep = cout_buffer.str();
        std::replace(no_tabs_rep.begin(), no_tabs_rep.end(), '\t', ' ');
        expect(no_tabs_rep == R"(CREATE TABLE "testfixed_converted" (
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
}
