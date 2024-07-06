///
/// \file   test/csvstack_test.cpp
/// \author wiluite
/// \brief  Tests for the csvstack utility.

#define BOOST_UT_DISABLE_MODULE
#include "ut.hpp"

#include "../utils/csvkit/csvstack.cpp"
#include "strm_redir.h"
#include "common_args.h"

//TODO: add all "stdin" tests
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


    "skip lines"_test = [] {

        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::csvstack_specific_args {
            Args() {
            }
        } args;

        args.files = std::vector<std::string>{"test_skip_lines.csv", "test_skip_lines.csv"};
        args.skip_lines = 3;
        CALL_TEST_AND_REDIRECT_TO_COUT(csvstack::stack<notrimming_reader_type>(args));

//      a,b,c
//      1,2,3
//      1,2,3

        expect("a,b,c\n1,2,3\n1,2,3\n" == cout_buffer.str());
    };

    "single file stack"_test = [] {

        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::csvstack_specific_args {
            Args() {
            }
        } args;

        args.files = std::vector<std::string>{"dummy.csv"};
        CALL_TEST_AND_REDIRECT_TO_COUT(csvstack::stack<notrimming_reader_type>(args));

//      a,b,c
//      1,2,3

        expect("a,b,c\n1,2,3\n" == cout_buffer.str());
    };

    "multiple file stack col"_test = [] {

        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::csvstack_specific_args {
            Args() {
            }
        } args;

        args.files = std::vector<std::string>{"dummy.csv", "dummy_col_shuffled.csv"};
        {
        CALL_TEST_AND_REDIRECT_TO_COUT(csvstack::stack<notrimming_reader_type>(args));

//      a,b,c
//      1,2,3
//      1,2,3

        expect("a,b,c\n1,2,3\n1,2,3\n" == cout_buffer.str());
        }


        args.files = std::vector<std::string>{"dummy_col_shuffled.csv", "dummy.csv"};
        {
        CALL_TEST_AND_REDIRECT_TO_COUT(csvstack::stack<notrimming_reader_type>(args));

//      a,b,c
//      1,2,3
//      1,2,3

        expect("b,c,a\n2,3,1\n2,3,1\n" == cout_buffer.str());
        }
    };

    "multiple file stack col ragged"_test = [] {

        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::csvstack_specific_args {
            Args() {
            }
        } args;

        args.files = std::vector<std::string>{"dummy.csv", "dummy_col_shuffled_ragged.csv"};
        CALL_TEST_AND_REDIRECT_TO_COUT(csvstack::stack<notrimming_reader_type>(args));

//      a,b,c,d
//      1,2,3,
//      1,2,3,4
        
        expect("a,b,c,d\n1,2,3,\n1,2,3,4\n" == cout_buffer.str());
    };

    "explicit grouping"_test = [] {

        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::csvstack_specific_args {
            Args() {
                group_name= "foo";
                groups = "asd,sdf";
            }
        } args;

        args.files = std::vector<std::string>{"dummy.csv", "dummy2.csv"};
        CALL_TEST_AND_REDIRECT_TO_COUT(csvstack::stack<notrimming_reader_type>(args));

//      foo,a,b,c
//      asd,1,2,3
//      sdf,1,2,3
        
        expect("foo,a,b,c\nasd,1,2,3\nsdf,1,2,3\n" == cout_buffer.str());
    };

    "filenames grouping"_test = [] {

        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::csvstack_specific_args {
            Args() {
                group_name= "path";
                filenames = true;
            }
        } args;

        args.files = std::vector<std::string>{"dummy.csv", "dummy2.csv"};
        CALL_TEST_AND_REDIRECT_TO_COUT(csvstack::stack<notrimming_reader_type>(args));

//      path,a,b,c
//      dummy.csv,1,2,3
//      dummy2.csv,1,2,3
        
        expect("path,a,b,c\ndummy.csv,1,2,3\ndummy2.csv,1,2,3\n" == cout_buffer.str());
    };

    "no header row basic"_test = [] {

        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::csvstack_specific_args {
            Args() {
                no_header = true;
            }
        } args;

        args.files = std::vector<std::string>{"no_header_row.csv", "no_header_row2.csv"};
        CALL_TEST_AND_REDIRECT_TO_COUT(csvstack::stack<notrimming_reader_type>(args));

//      a,b,c
//      1,2,3
//      4,5,6
        
        expect("a,b,c\n1,2,3\n4,5,6\n" == cout_buffer.str());
    };

    "no header row basic"_test = [] {

        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::csvstack_specific_args {
            Args() {
                no_header = true;
            }
        } args;

        args.files = std::vector<std::string>{"no_header_row.csv", "no_header_row2.csv"};
        CALL_TEST_AND_REDIRECT_TO_COUT(csvstack::stack<notrimming_reader_type>(args));

//      a,b,c
//      1,2,3
//      4,5,6
        
        expect("a,b,c\n1,2,3\n4,5,6\n" == cout_buffer.str());
    };

    "grouped manual and named column"_test = [] {

        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::csvstack_specific_args {
            Args() {
                no_header = true;
                groups = "foo,bar";
                group_name = "hey"; 
            }
        } args;

        args.files = std::vector<std::string>{"dummy.csv", "dummy3.csv"};
        CALL_TEST_AND_REDIRECT_TO_COUT(csvstack::stack<notrimming_reader_type>(args));

//      hey,a,b,c
//      foo,a,b,c
//      foo,1,2,3
//      bar,a,b,c
//      bar,1,2,3
//      bar,1,4,5
        
        expect("hey,a,b,c\nfoo,a,b,c\nfoo,1,2,3\nbar,a,b,c\nbar,1,2,3\nbar,1,4,5\n" == cout_buffer.str());
    };

    "grouped filenames"_test = [] {

        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::csvstack_specific_args {
            Args() {
                no_header = true;
                filenames = true;
            }
        } args;

        args.files = std::vector<std::string>{"no_header_row.csv", "no_header_row2.csv"};
        CALL_TEST_AND_REDIRECT_TO_COUT(csvstack::stack<notrimming_reader_type>(args));

//      group,a,b,c
//      no_header_row.csv,1,2,3
//      no_header_row2.csv,4,5,6
        
        expect("group,a,b,c\nno_header_row.csv,1,2,3\nno_header_row2.csv,4,5,6\n" == cout_buffer.str());
    };

    "grouped filenames and named column"_test = [] {

        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::csvstack_specific_args {
            Args() {
                no_header = true;
                filenames = true;
                group_name = "hello";
            }
        } args;

        args.files = std::vector<std::string>{"no_header_row.csv", "no_header_row2.csv"};
        CALL_TEST_AND_REDIRECT_TO_COUT(csvstack::stack<notrimming_reader_type>(args));

//      hello,a,b,c
//      no_header_row.csv,1,2,3
//      no_header_row2.csv,4,5,6
        
        expect("hello,a,b,c\nno_header_row.csv,1,2,3\nno_header_row2.csv,4,5,6\n" == cout_buffer.str());
    };
}
