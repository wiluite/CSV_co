///
/// \file   test/csvcut_test.cpp
/// \author wiluite
/// \brief  Tests for the csvcut utility.

#define BOOST_UT_DISABLE_MODULE
#include "ut.hpp"

#include "../utils/csvkit/csvcut.cpp"
#include "strm_redir.h"
#include "common_args.h"
#include "test_reader_macros.h"
#include "test_max_field_size_macros.h"

#define CALL_TEST_AND_REDIRECT_TO_COUT std::stringstream cout_buffer;                        \
                                       {                                                     \
                                           redirect(cout)                                    \
                                           redirect_cout cr(cout_buffer.rdbuf());            \
                                           csvcut::cut(ref, args);                           \
                                       }


int main() {
    using namespace boost::ut;
    namespace tf = csvkit::test_facilities;

#if defined (WIN32)
    cfg < override > = {.colors={.none="", .pass="", .fail=""}};
#endif
    "skip lines"_test = [] {
        struct Args : tf::single_file_arg, tf::common_args, tf::spread_args {
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
        struct Args : tf::single_file_arg, tf::common_args, tf::spread_args {
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
        struct Args : tf::single_file_arg, tf::common_args, tf::spread_args {
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
        struct Args : tf::single_file_arg, tf::common_args, tf::spread_args {
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
        struct Args : tf::single_file_arg, tf::common_args, tf::spread_args {
            Args() { file = "dummy.csv.gz"; columns = "1,3"; }
            bool x_ {false};
        } args;

        notrimming_reader_type r (args.file);
        std::reference_wrapper<notrimming_reader_type> ref = std::ref(r);

        CALL_TEST_AND_REDIRECT_TO_COUT

        expect(cout_buffer.str() == "a,c\n1,3\n");
    };

    "exclude"_test = [] {
        struct Args : tf::single_file_arg, tf::common_args, tf::spread_args {
            Args() { file = "dummy.csv"; not_columns = "1,3"; } //TODO: does not work with spaces between commas and numbers
            bool x_ {false};
        } args;

        notrimming_reader_type r (args.file);
        std::reference_wrapper<notrimming_reader_type> ref = std::ref(r);

        CALL_TEST_AND_REDIRECT_TO_COUT

        expect(cout_buffer.str() == "b\n2\n");
    };

    "exclude and exclude"_test = [] {
        struct Args : tf::single_file_arg, tf::common_args, tf::spread_args {
            Args() { file = "dummy.csv"; columns = "1,3"; not_columns = "3"; } 
            bool x_ {false};
        } args;

        notrimming_reader_type r (args.file);
        std::reference_wrapper<notrimming_reader_type> ref = std::ref(r);

        CALL_TEST_AND_REDIRECT_TO_COUT

        expect(cout_buffer.str() == "a\n1\n");
    };

    "no header row"_test = [] {
        struct Args : tf::single_file_arg, tf::common_args, tf::spread_args {
            Args() { file = "no_header_row.csv"; columns = "2"; no_header = true; } 
            bool x_ {false};
        } args;

        notrimming_reader_type r (args.file);
        std::reference_wrapper<notrimming_reader_type> ref = std::ref(r);

        CALL_TEST_AND_REDIRECT_TO_COUT

        expect(cout_buffer.str() == "b\n2\n");
    };

    "names with skip lines"_test = [] {
        struct Args : tf::single_file_arg, tf::common_args, tf::spread_args {
            Args() { file = "test_skip_lines.csv"; skip_lines = 3; names=true;  } 
            bool x_ {false};
        } args;

        notrimming_reader_type r (args.file);
        std::reference_wrapper<notrimming_reader_type> ref = std::ref(r);

        CALL_TEST_AND_REDIRECT_TO_COUT

        expect(cout_buffer.str() == "  1: a\n  2: b\n  3: c\n");
    };


    "max field size"_test = [] {
        struct Args : tf::single_file_arg, tf::common_args, tf::spread_args {
            Args() { file = "test_field_size_limit.csv"; maxfieldsize = 100; }
            bool x_ {false};
        } args;

        csv_co::reader<> r (args.file);
        std::reference_wrapper<csv_co::reader<>> ref = std::ref(r);
        expect(nothrow([&]{CALL_TEST_AND_REDIRECT_TO_COUT}));

        using namespace z_test;
        Z_CHECK(csvcut::cut, test_reader_r1, skip_lines::skip_lines_0, header::has_header, 12, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 12 characters on line 1.)")
        Z_CHECK(csvcut::cut, test_reader_r2, skip_lines::skip_lines_0, header::no_header, 12, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 12 characters on line 1.)")

        Z_CHECK(csvcut::cut, test_reader_r3, skip_lines::skip_lines_0, header::has_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 2.)")
        Z_CHECK(csvcut::cut, test_reader_r4, skip_lines::skip_lines_0, header::no_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 2.)")

        Z_CHECK(csvcut::cut, test_reader_r5, skip_lines::skip_lines_1, header::has_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 1.)")
        Z_CHECK(csvcut::cut, test_reader_r6, skip_lines::skip_lines_1, header::no_header, 13, R"(FieldSizeLimitError: CSV contains a field longer than the maximum length of 13 characters on line 1.)")

    };

}
