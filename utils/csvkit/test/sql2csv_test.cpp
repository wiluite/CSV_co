///
/// \file   test/sql2csv_test.cpp
/// \author wiluite
/// \brief  Tests for the sql2csv utility.

#define BOOST_UT_DISABLE_MODULE
#include "ut.hpp"

#include "../utils/csvkit/sql2csv.cpp"
#include "../utils/csvkit/csvsql.cpp"
#include "strm_redir.h"

#define CALL_TEST_AND_REDIRECT_TO_COUT(call)    \
    std::stringstream cout_buffer;              \
    {                                           \
        redirect(cout)                          \
        redirect_cout cr(cout_buffer.rdbuf());  \
        call;                                   \
    }

int main() {
    using namespace boost::ut;
    
#if defined (WIN32)
    cfg < override > = {.colors={.none="", .pass="", .fail=""}};
#endif

    struct sql2csv_specific_args {
        std::filesystem::path query_file;
        std::string db;
        std::string query;
        bool linenumbers {false};
        std::string encoding {"UTF-8"};
        bool no_header {false};
    };

    "no query file"_test = [] {
        struct Args : sql2csv_specific_args {
            Args() {
                query_file = "query";
            }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(sql2csv::sql_to_csv<notrimming_reader_type>(args))
        expect(true);
    };

}
