///
/// \file   test/csvjoin_test.cpp
/// \author wiluite
/// \brief  Tests for the csvjoin utility.

#define BOOST_UT_DISABLE_MODULE
#include "ut.hpp"

#include "../utils/csvkit/csvjoin.cpp"
#include "strm_redir.h"
#include "common_args.h"

int main() {
    using namespace boost::ut;

#if defined (WIN32)
    cfg < override > = {.colors={.none="", .pass="", .fail=""}};
#endif

#define CALL_TEST_AND_REDIRECT_TO_COUT(call) std::stringstream cout_buffer;                        \
                                             {                                                     \
                                                 redirect(cout)                                    \
                                                 redirect_cout cr(cout_buffer.rdbuf());            \
                                                 call;                                             \
                                             }

    "runs"_test = [] {
        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::type_aware_args, tf::spread_args, tf::csvjoin_specific_args {
            Args() {
                columns.clear();
            }
        } args;

        "sequential"_test = [&] {
            auto args_copy = args;
            args_copy.files = std::vector<std::string>{"join_a.csv", "join_b.csv"};
            CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy))

            notrimming_reader_type new_reader (cout_buffer.str());
            expect(4 == new_reader.rows());
        };

        "inner"_test = [&] {
            auto args_copy = args;
            args_copy.columns = "a";
            args_copy.files = std::vector<std::string>{"join_a.csv", "join_b.csv"};
            CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy))

            notrimming_reader_type new_reader (cout_buffer.str());
            expect(3 == new_reader.rows());
        };

        "left"_test = [&] {
            auto args_copy = args;
            args_copy.columns = "a";
            args_copy.left_join = true;  
            args_copy.files = std::vector<std::string>{"join_a.csv", "join_b.csv"};
            CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy))

            notrimming_reader_type new_reader (cout_buffer.str());
            expect(5 == new_reader.rows());
        };

        "right"_test = [&] {
            auto args_copy = args;
            args_copy.columns = "a";
            args_copy.right_join = true;  
            args_copy.files = std::vector<std::string>{"join_a.csv", "join_b.csv"};
            CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy))

            notrimming_reader_type new_reader (cout_buffer.str());
            expect(4 == new_reader.rows());
        };

        "right indices"_test = [&] {
            auto args_copy = args;
            args_copy.columns = "1,4";
            args_copy.right_join = true;  
            args_copy.files = std::vector<std::string>{"join_a.csv", "blanks.csv"};
            CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy))

            notrimming_reader_type new_reader (cout_buffer.str());
            expect(2 == new_reader.rows());
        };

#if !defined(_MSC_VER)
        "outer"_test = [&] {
            auto args_copy = args;
            args_copy.columns = "a";
            args_copy.outer_join = true;  
            args_copy.files = std::vector<std::string>{"join_a.csv", "join_b.csv"};
            CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy))

            notrimming_reader_type new_reader (cout_buffer.str());
            expect(6 == new_reader.rows());
        };
#endif

        "single"_test = [&] {
            auto args_copy = args;            
            args_copy.no_inference = true;  
            args_copy.files = std::vector<std::string>{"dummy.csv"};
            CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy))

//          a,b,c
//          1,2,3

            expect("a,b,c\n1,2,3\n" == cout_buffer.str());
        };

        "no blanks"_test = [&] {
            auto args_copy = args;            
            args_copy.files = std::vector<std::string>{"blanks.csv", "blanks.csv"};
            CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy))

//          a,b,c,d,e,f,a_2,b_2,c_2,d_2,e_2,f_2
//          ,,,,,,,,,,,

            expect("a,b,c,d,e,f,a_2,b_2,c_2,d_2,e_2,f_2\n,,,,,,,,,,,\n" == cout_buffer.str());
        };

        "blanks"_test = [&] {
            auto args_copy = args;
            args_copy.blanks = true;            
            args_copy.files = std::vector<std::string>{"blanks.csv", "blanks.csv"};
            CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy))

//          a,b,c,d,e,f,a_2,b_2,c_2,d_2,e_2,f_2
//          ,NA,N/A,NONE,NULL,.,,NA,N/A,NONE,NULL,.

            expect("a,b,c,d,e,f,a_2,b_2,c_2,d_2,e_2,f_2\n,NA,N/A,NONE,NULL,.,,NA,N/A,NONE,NULL,.\n" == cout_buffer.str());
        };

        "no header row"_test = [&] {
            auto args_copy = args;
            args_copy.columns = "1";
            args_copy.no_header = true;            
            args_copy.files = std::vector<std::string>{"join_a.csv", "join_no_header_row.csv"};
            CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy))

            notrimming_reader_type new_reader (cout_buffer.str());
            expect(3 == new_reader.rows());
        };

        "sniff limit"_test = [&] {
            auto args_copy = args;
            args_copy.files = std::vector<std::string>{"join_a.csv", "sniff_limit.csv"};
            CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy))

//          -- FROM PYTHON CSVKIT --
//          ['a', 'b', 'c', 'a;b;c'],
//          ['1', 'b', 'c', '1;2;3'],
//          ['2', 'b', 'c', ''],
//          ['3', 'b', 'c', ''],

            expect("a,b,c,a;b;c\n1,b,c,1;2;3\n2,b,c,\n3,b,c,\n" == cout_buffer.str());
        };

        "both left and right"_test = [&] {
            auto args_copy = args;
            args_copy.right_join = args_copy.left_join = true;
            args_copy.columns = "1";
            expect(throws([&] { csvjoin::join_wrapper(args_copy); }));
        };

        "no join column names"_test = [&] {
            auto args_copy = args;
            args_copy.right_join = true;
            expect(throws([&] { csvjoin::join_wrapper(args_copy); }));
        };

        "join column names must match the number of files"_test = [&] {
            auto args_copy = args;
            args_copy.right_join = true;
            args_copy.columns = "1,1";
            args_copy.files = std::vector<std::string>{"file","file2","file3"};
            expect(throws([&] { csvjoin::join_wrapper(args_copy); }));
        };

        "csvjoin union strings"_test = [&] {
            auto args_copy = args;
            args_copy.files = std::vector<std::string>{"h1\nabc","h2\nabc\ndef","h3\n\nghi"};
            CALL_TEST_AND_REDIRECT_TO_COUT(csvjoin::join_wrapper(args_copy, csvjoin_source_option::csvjoin_string_source))
            expect(cout_buffer.str() == "h1,h2,3\r\nabc,abc,\r\n,def,ghi\r\n" || cout_buffer.str() == "h1,h2,h3\nabc,abc,\n,def,ghi\n");
        };
    };
}
