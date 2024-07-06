///
/// \file   test/csvgrep_test.cpp
/// \author wiluite
/// \brief  Tests for the csvgrep utility.

#define BOOST_UT_DISABLE_MODULE
#include "ut.hpp"

#include "../utils/csvkit/csvgrep.cpp"
#include "strm_redir.h"
#include "common_args.h"
#include "test_runner_macros.h"

//TODO: add all "stdin" tests
int main() {
    using namespace boost::ut;

#if defined (WIN32)
    cfg < override > = {.colors={.none="", .pass="", .fail=""}};
#endif
#define CALL_TEST_AND_REDIRECT_TO_COUT(name) std::stringstream cout_buffer;                        \
                                             {                                                     \
                                                 redirect(cout)                                    \
                                                 redirect_cout cr(cout_buffer.rdbuf());            \
                                                 test_reader_configurator_and_runner(args, name)   \
                                             }

    "skip lines"_test = [] {

        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::spread_args, tf::csvgrep_specific_args {
            Args() { file = "test_skip_lines.csv"; skip_lines = 3; columns = "1"; match = "1"; }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(csvgrep::grep)

//      a,b,c
//      1,2,3

        expect("a,b,c\n1,2,3\n" == cout_buffer.str());
    };

    "match"_test = [] {

        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::spread_args, tf::csvgrep_specific_args {
            Args() { file = "dummy.csv"; columns = "1"; match = "1"; }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(csvgrep::grep)

//      a,b,c
//      1,2,3

        expect("a,b,c\n1,2,3\n" == cout_buffer.str());
    };

    "any match"_test = [] {

        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::spread_args, tf::csvgrep_specific_args {
            Args() { file = "dummy.csv"; columns = "1,2,3"; match = "1"; any = true; }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(csvgrep::grep)

//      a,b,c
//      1,2,3

        expect("a,b,c\n1,2,3\n" == cout_buffer.str());
    };

    "match utf8"_test = [] {

        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::spread_args, tf::csvgrep_specific_args {
            Args() { file = "test_utf8.csv"; columns = "3"; match = "ʤ"; any = true; }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(csvgrep::grep)

//      foo,bar,baz
//      4,5,ʤ

        expect("foo,bar,baz\n4,5,ʤ\n" == cout_buffer.str());
    };

    "match utf8_bom"_test = [] {

        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::spread_args, tf::csvgrep_specific_args {
            Args() { file = "test_utf8_bom.csv"; columns = "3"; match = "ʤ"; any = true; }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(csvgrep::grep)

//      foo,bar,baz
//      4,5,ʤ

        expect("foo,bar,baz\n4,5,ʤ\n" == cout_buffer.str());
    };

    "no match"_test = [] {

        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::spread_args, tf::csvgrep_specific_args {
            Args() { file = "dummy.csv"; columns = "1"; match = "NO MATCH"; }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(csvgrep::grep)

//      a,b,c

        expect("a,b,c\n" == cout_buffer.str());
    };

    "invert match"_test = [] {

        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::spread_args, tf::csvgrep_specific_args {
            Args() { file = "dummy.csv"; columns = "1"; match = "NO MATCH"; invert = true; }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(csvgrep::grep)

//      a,b,c
//      1,2,3

        expect("a,b,c\n1,2,3\n" == cout_buffer.str());
    };

    "re match"_test = [] {

        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::spread_args, tf::csvgrep_specific_args {
            Args() { file = "dummy.csv"; columns = "3"; regex = "^(3|9)$"; }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(csvgrep::grep)

//      a,b,c
//      1,2,3

        expect("a,b,c\n1,2,3\n" == cout_buffer.str());
    };

    "re match utf8"_test = [] {

        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::spread_args, tf::csvgrep_specific_args {
            Args() { file = "test_utf8.csv"; columns = "3"; regex = "ʤ"; }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(csvgrep::grep)

//      foo,bar,baz
//      4,5,ʤ

        expect("foo,bar,baz\n4,5,ʤ\n" == cout_buffer.str());
    };

    "string match"_test = [] {

        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::spread_args, tf::csvgrep_specific_args {
            Args() { file = "FY09_EDU_Recipients_by_State.csv"; columns = "1"; match = "ILLINOIS"; }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(csvgrep::grep)

//      State Name,State Abbreviate,Code,Montgomery GI Bill-Active Duty,Montgomery GI Bill- Selective Reserve,Dependents' Educational Assistance,Reserve Educational Assistance Program,Post-Vietnam Era Veteran's Educational Assistance Program,TOTAL,
//      ILLINOIS,IL,17,"15,659","2,491","2,025","1,770",19,"21,964",

        expect("State Name,State Abbreviate,Code,Montgomery GI Bill-Active Duty,Montgomery GI Bill- Selective Reserve,Dependents' Educational Assistance"
               ",Reserve Educational Assistance Program,Post-Vietnam Era Veteran's Educational Assistance Program,TOTAL,\n"
               "ILLINOIS,IL,17,\"15,659\",\"2,491\",\"2,025\",\"1,770\",19,\"21,964\",\n" == cout_buffer.str());
    };

    "string match with line numbers"_test = [] {

        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::spread_args, tf::csvgrep_specific_args {
            Args() { file = "FY09_EDU_Recipients_by_State.csv"; columns = "1"; match = "ILLINOIS"; linenumbers = true;}
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(csvgrep::grep)

//      line_number,State Name,State Abbreviate,Code,Montgomery GI Bill-Active Duty,Montgomery GI Bill- Selective Reserve,Dependents' Educational Assistance,Reserve Educational Assistance Program,Post-Vietnam Era Veteran's Educational Assistance Program,TOTAL,
//      14,ILLINOIS,IL,17,"15,659","2,491","2,025","1,770",19,"21,964",

        expect("line_number,State Name,State Abbreviate,Code,Montgomery GI Bill-Active Duty,Montgomery GI Bill- Selective Reserve,Dependents' Educational Assistance"
               ",Reserve Educational Assistance Program,Post-Vietnam Era Veteran's Educational Assistance Program,TOTAL,\n"
               "14,ILLINOIS,IL,17,\"15,659\",\"2,491\",\"2,025\",\"1,770\",19,\"21,964\",\n" == cout_buffer.str());
    };

    "match with linenumbers"_test = [] {

        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::spread_args, tf::csvgrep_specific_args {
            Args() { file = "dummy.csv"; columns = "1"; match = "1"; linenumbers = true; }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(csvgrep::grep)

//      line_number,a,b,c
//      1,1,2,3

        expect("line_number,a,b,c\n1,1,2,3\n" == cout_buffer.str());
    };

}
