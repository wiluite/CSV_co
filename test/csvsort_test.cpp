///
/// \file   test/csvsort_test.cpp
/// \author wiluite
/// \brief  Tests for the csvsort utility.

#define BOOST_UT_DISABLE_MODULE
#include "ut.hpp"

#include "../utils/csvkit/csvsort.cpp"
#include "strm_redir.h"
#include "common_args.h"
#include "test_runner_macros.h"

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

    "runs"_test = [] {
        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::type_aware_args, tf::spread_args, tf::csvsort_specific_args {
            Args() { file = "test_utf8.csv"; }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(csvsort::sort)

//      foo,bar,baz
//      1,2,3
//      4,5,ʤ

        expect(cout_buffer.str() == "foo,bar,baz\n1,2,3\n4,5,ʤ\n");
    };

    "encoding"_test = [] {
        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::type_aware_args, tf::spread_args, tf::csvsort_specific_args {
            Args() { file = "test_latin1.csv"; }
        } args;

        {
            CALL_TEST_AND_REDIRECT_TO_COUT(csvsort::sort)
            expect (cout_buffer.str().find("Your file is not \"UTF-8\" encoded") != std::string::npos);
        }
        {
            args.encoding = "latin1";
            CALL_TEST_AND_REDIRECT_TO_COUT(csvsort::sort)

//          a,b,c
//          1,2,3
//          4,5,©

            expect(cout_buffer.str() == "a,b,c\n1,2,3\n4,5,©\n");
        }
    };

    "sort string reverse"_test = [] {
        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::type_aware_args, tf::spread_args, tf::csvsort_specific_args {
            Args() { file = "testxls_converted.csv"; columns = "1"; r = true; }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(csvsort::sort)

        notrimming_reader_type new_reader (cout_buffer.str());
        std::vector<std::string> test_order ({"text", "Unicode! Σ", "This row has blanks", "Chicago Tribune", "Chicago Sun-Times", "Chicago Reader"});
        std::vector<std::string> new_order;
        new_reader.run_rows([&](auto row_span) {
            new_order.push_back(row_span[0]);
        });

        expect(test_order == new_order);
    };

    "sort date"_test = [] {
        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::type_aware_args, tf::spread_args, tf::csvsort_specific_args {
            Args() { file = "testxls_converted.csv"; columns = "2"; date_fmt = "%Y-%m-%d";}
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(csvsort::sort)

        notrimming_reader_type new_reader (cout_buffer.str());
        std::vector<std::string> test_order ({"text", "Chicago Tribune", "Chicago Sun-Times", "Chicago Reader", "This row has blanks", "Unicode! Σ" });
        std::vector<std::string> new_order;
        new_reader.run_rows([&](auto row_span) {
            new_order.push_back(row_span[0]);
        });

        expect(test_order == new_order);
    };

    "no blanks"_test = [] {
        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::type_aware_args, tf::spread_args, tf::csvsort_specific_args {
            Args() { file = "blanks.csv"; }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(csvsort::sort)

        expect(cout_buffer.str() == "a,b,c,d,e,f\n,,,,,\n");
    };

    "blanks"_test = [] {
        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::type_aware_args, tf::spread_args, tf::csvsort_specific_args {
            Args() { file = "blanks.csv"; blanks = true; }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(csvsort::sort)

        expect(cout_buffer.str() == "a,b,c,d,e,f\n,NA,N/A,NONE,NULL,.\n");
    };

    "no header row"_test = [] {
        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::type_aware_args, tf::spread_args, tf::csvsort_specific_args {
            Args() { file = "no_header_row.csv"; no_inference = true; no_header = true; }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(csvsort::sort)

        expect(cout_buffer.str() == "a,b,c\n1,2,3\n");
    };

    "no inference"_test = [] {
        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::type_aware_args, tf::spread_args, tf::csvsort_specific_args {
            Args() { file = "test_literal_order.csv"; no_inference = true; columns = "1"; }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(csvsort::sort)

        notrimming_reader_type new_reader (cout_buffer.str());
        std::vector<std::string> test_order ({ "a", "192", "27", "3" });
        std::vector<std::string> new_order;
        new_reader.run_rows([&](auto row_span) {
            new_order.push_back(row_span[0]);
        });

        expect(test_order == new_order);
    };

    "sort_t_and_nulls"_test = [] {
        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::type_aware_args, tf::spread_args, tf::csvsort_specific_args {
            Args() { file = "sort_ints_nulls.csv"; columns = "2"; }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(csvsort::sort)
 
//      a,b,c
//      5,1,7
//      1,2,3
//      4,,6
        
        expect(cout_buffer.str() == "a,b,c\n5,1,7\n1,2,3\n4,,6\n");

        notrimming_reader_type new_reader (cout_buffer.str());
        std::vector<std::string> test_order ({ "b", "1", "2", "" });
        std::vector<std::string> new_order;
        new_reader.run_rows([&](auto row_span) {
            new_order.push_back(row_span[1]);
        });

        expect(test_order == new_order);
    };

    "sort timedelta"_test = [] {
        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::type_aware_args, tf::spread_args, tf::csvsort_specific_args {
            Args() { file = "timedelta.csv"; columns = "2"; }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT(csvsort::sort)

//      a,b
//      one,0:00:01.123450
//      two,0:00:01.123457
//      three,0:00:01.123457

        expect(cout_buffer.str() == "a,b\none,0:00:01.123450\ntwo,0:00:01.123457\nthree,0:00:01.123457\n");

    };

}
