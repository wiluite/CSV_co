///
/// \file   test/csvcut_test.cpp
/// \author wiluite
/// \brief  Tests for the csvcut utility.

#define BOOST_UT_DISABLE_MODULE
#include "ut.hpp"

#include "../utils/csvkit/csvcut.cpp"
#include "strm_redir.h"
#include "common_args.h"

#define CALL_TEST_AND_REDIRECT_TO_COUT std::stringstream cout_buffer;                        \
                                       {                                                     \
                                           redirect(cout)                                    \
                                           redirect_cout cr(cout_buffer.rdbuf());            \
                                           csvcut::cut(ref, args);                           \
                                       }


int main() {
    using namespace boost::ut;

#if defined (WIN32)
    cfg < override > = {.colors={.none="", .pass="", .fail=""}};
#endif
    "skip lines"_test = [] {
        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::spread_args {
            Args() { file = "test_skip_lines.csv"; columns = "1,3"; skip_lines = 3; }
            bool x_ {false};
        } args;

        notrimming_reader_type r (args.file);
        std::reference_wrapper<notrimming_reader_type> ref = std::ref(r);

        CALL_TEST_AND_REDIRECT_TO_COUT

//      a,c
//      1,3

        expect(cout_buffer.str() == "a,c\n1,3\n");
    };

    "simple"_test = [] {
        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::spread_args {
            Args() { file = "dummy.csv"; columns = "1,3"; }
            bool x_ {false};
        } args;

        notrimming_reader_type r (args.file);
        std::reference_wrapper<notrimming_reader_type> ref = std::ref(r);

        CALL_TEST_AND_REDIRECT_TO_COUT

//      a,c
//      1,3

        expect(cout_buffer.str() == "a,c\n1,3\n");
    };

    "linenumbers"_test = [] {
        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::spread_args {
            Args() { file = "dummy.csv"; linenumbers = true; columns = "1,3"; }
            bool x_ {false};
        } args;

        notrimming_reader_type r (args.file);
        std::reference_wrapper<notrimming_reader_type> ref = std::ref(r);

        CALL_TEST_AND_REDIRECT_TO_COUT

//      line_number,a,c
//      1,1,3

        expect(cout_buffer.str() == "line_number,a,c\n1,1,3\n");
    };

    "unicode"_test = [] {
        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::spread_args {
            Args() { file = "test_utf8.csv"; columns = "1,3"; }
            bool x_ {false};
        } args;

        notrimming_reader_type r (args.file);
        std::reference_wrapper<notrimming_reader_type> ref = std::ref(r);

        CALL_TEST_AND_REDIRECT_TO_COUT

//      foo,baz
//      1,3
//      4,ʤ

        expect(cout_buffer.str() == "foo,baz\n1,3\n4,ʤ\n");
    };

    "with gzip"_test = [] {
        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::spread_args {
            Args() { file = "dummy.csv.gz"; columns = "1,3"; }
            bool x_ {false};
        } args;

        notrimming_reader_type r (args.file);
        std::reference_wrapper<notrimming_reader_type> ref = std::ref(r);

        CALL_TEST_AND_REDIRECT_TO_COUT

        expect(cout_buffer.str() == "a,c\n1,3\n");
    };

    "exclude"_test = [] {
        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::spread_args {
            Args() { file = "dummy.csv"; not_columns = "1,3"; } //TODO: does not work with spaces between commas and numbers
            bool x_ {false};
        } args;

        notrimming_reader_type r (args.file);
        std::reference_wrapper<notrimming_reader_type> ref = std::ref(r);

        CALL_TEST_AND_REDIRECT_TO_COUT

        expect(cout_buffer.str() == "b\n2\n");
    };

    "exclude and exclude"_test = [] {
        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::spread_args {
            Args() { file = "dummy.csv"; columns = "1,3"; not_columns = "3"; } 
            bool x_ {false};
        } args;

        notrimming_reader_type r (args.file);
        std::reference_wrapper<notrimming_reader_type> ref = std::ref(r);

        CALL_TEST_AND_REDIRECT_TO_COUT

        expect(cout_buffer.str() == "a\n1\n");
    };

    "no header row"_test = [] {
        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::spread_args {
            Args() { file = "no_header_row.csv"; columns = "2"; no_header = true; } 
            bool x_ {false};
        } args;

        notrimming_reader_type r (args.file);
        std::reference_wrapper<notrimming_reader_type> ref = std::ref(r);

        CALL_TEST_AND_REDIRECT_TO_COUT

        expect(cout_buffer.str() == "b\n2\n");
    };

    "names with skip lines"_test = [] {
        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::spread_args {
            Args() { file = "test_skip_lines.csv"; skip_lines = 3; names=true;  } 
            bool x_ {false};
        } args;

        notrimming_reader_type r (args.file);
        std::reference_wrapper<notrimming_reader_type> ref = std::ref(r);

        CALL_TEST_AND_REDIRECT_TO_COUT

        expect(cout_buffer.str() == "  1: a\n  2: b\n  3: c\n");
    };

}
