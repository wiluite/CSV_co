///
/// \file   test/sql2csv_test.cpp
/// \author wiluite
/// \brief  Tests for the sql2csv utility.

#define BOOST_UT_DISABLE_MODULE
#include "ut.hpp"

#include "../utils/csvkit/sql2csv.cpp"
//#include "../utils/csvkit/csvsql.cpp"
#include "strm_redir.h"
#include "stdin_subst.h"

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

    "encoding"_test = [] {
        struct Args : sql2csv_specific_args {
            Args() {
                encoding = "latin1";
                query_file = "test.sql";
            }
        } args;

        expect(nothrow([&] {CALL_TEST_AND_REDIRECT_TO_COUT(sql2csv::sql_to_csv<notrimming_reader_type>(args))}));
    };

    "query"_test = [] {
        struct Args : sql2csv_specific_args {
            Args() {
                query = "select 6*9 as question";
            }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(sql2csv::sql_to_csv<notrimming_reader_type>(args))

        expect(cout_buffer.str().find("question") != std::string::npos);
        expect(cout_buffer.str().find("54") != std::string::npos);
    };

    "file"_test = [] {
        struct Args : sql2csv_specific_args {
            Args() {
                query_file = "test.sql";
            }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(sql2csv::sql_to_csv<notrimming_reader_type>(args))

        expect(cout_buffer.str().find("question") != std::string::npos);
        expect(cout_buffer.str().find("36") != std::string::npos);
    };

    "file with query"_test = [] {
        struct Args : sql2csv_specific_args {
            Args() {
                query_file = "test.sql";
                query = "select 6*9 as question";
            }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(sql2csv::sql_to_csv<notrimming_reader_type>(args))

        expect(cout_buffer.str().find("question") != std::string::npos);
        expect(cout_buffer.str().find("54") != std::string::npos);
    };

    "stdin"_test = [] {
        struct Args : sql2csv_specific_args {
            Args() {
                //NOTE: we do not use any option, only piped text: "select cast(3.1415 * 13.37 as integer) as answer"
            }
        } args;

        stdin_redir sr("stdin_select");

        expect(nothrow([&]{
            CALL_TEST_AND_REDIRECT_TO_COUT(sql2csv::sql_to_csv<notrimming_reader_type>(args))
            expect(cout_buffer.str().find("answer") != std::string::npos);
            expect(cout_buffer.str().find("42") != std::string::npos);
        }));
    };

    "stdin with query"_test = [] {
        struct Args : sql2csv_specific_args {
            Args() {
                query = "select 6*9 as question";
            }
        } args;

        stdin_redir sr("stdin_select");

        expect(nothrow([&]{
            CALL_TEST_AND_REDIRECT_TO_COUT(sql2csv::sql_to_csv<notrimming_reader_type>(args))
            expect(cout_buffer.str().find("question") != std::string::npos);
            expect(cout_buffer.str().find("54") != std::string::npos);
        }));
    };

    "stdin with file"_test = [] {
        struct Args : sql2csv_specific_args {
            Args() {
                query_file = "test.sql";
            }
        } args;

        stdin_redir sr("stdin_select");

        expect(nothrow([&]{
            CALL_TEST_AND_REDIRECT_TO_COUT(sql2csv::sql_to_csv<notrimming_reader_type>(args))
            expect(cout_buffer.str().find("question") != std::string::npos);
            expect(cout_buffer.str().find("36") != std::string::npos);
        }));
    };

    "stdin with file and query"_test = [] {
        struct Args : sql2csv_specific_args {
            Args() {
                query_file = "test.sql";
                query = "select 6*9 as question";
            }
        } args;

        stdin_redir sr("stdin_select");

        expect(nothrow([&]{
            CALL_TEST_AND_REDIRECT_TO_COUT(sql2csv::sql_to_csv<notrimming_reader_type>(args))
            expect(cout_buffer.str().find("question") != std::string::npos);
            expect(cout_buffer.str().find("54") != std::string::npos);
        }));
    };

}
