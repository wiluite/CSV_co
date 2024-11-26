///
/// \file   test/in2csv_test.cpp
/// \author wiluite
/// \brief  Tests for the in2csv utility.

#define BOOST_UT_DISABLE_MODULE
#include "ut.hpp"

#include "../utils/csvkit/in2csv.cpp"
#include "common_args.h"
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
    namespace tf = csvkit::test_facilities;

#if defined (WIN32)
    cfg < override > = {.colors={.none="", .pass="", .fail=""}};
#endif

    struct in2csv_specific_args {
        std::string format;
        std::string schema;
        std::string key;
        bool names = false;
        std::string sheet;
        std::string write_sheets;
        bool use_sheet_names = false;
        std::string encoding_xls = "UTF-8";
        std::string d_excel = "none";
        std::string dt_excel = "none";
        bool is1904;
    };

    "exceptions"_test = [] {
        struct Args : tf::single_file_arg, tf::common_args, tf::type_aware_args, tf::output_args, in2csv_specific_args {
            Args() = default;
        } args;

        // Neither file name specified nor piping data is coming.
        expect(throws<in2csv::detail::empty_file_and_no_piping_now>([&] {CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))}));
        {
            // now emulate pipe data pull
            stdin_redir sr("stdin_select");
            expect(throws<in2csv::detail::no_format_specified_on_piping>([&] {CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))}));
        }

        args.file = "blah-blah";
        args.format.clear();
        expect(throws<in2csv::detail::no_schema_when_no_extension>([&] {CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))}));

        args.file = "blah-blah.unknown";
        expect(throws<in2csv::detail::unable_to_automatically_determine_format>([&] {CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))}));

        args.format = "dbf"; // file blah-blah.unknown not found
        expect(throws<in2csv::detail::file_not_found>([&] {CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))}));

        args.format = "unknown";
        expect(throws<in2csv::detail::invalid_input_format>([&] {CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))}));

        args.format = "fixed";
        args.schema = "schema.csv";
        expect(throws<in2csv::detail::schema_file_not_found>([&] {CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))}));

        args.schema = "foo2.csv";
        expect(throws<in2csv::detail::file_not_found>([&] {CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))}));

        args.file = "stdin_select"; // all is fine (format, schema and file) - no exception
        args.schema = "testfixed_schema.csv";
        expect(nothrow([&] {CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))}));

        args.file = "";
        args.format = "dbf";
        expect(throws<in2csv::detail::dbf_cannot_be_converted_from_stdin>([&] {CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))}));

    };

    auto assert_converted = [](std::string const & source, std::string const & pattern_filename) {
        std::ifstream ifs(pattern_filename);
        std::string result {std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>()};
        expect(result == source);
    };

    bool locale_support = detect_locale_support();
    if (locale_support) {
        "locale"_test = [&] {
            struct Args : tf::single_file_arg, tf::common_args, tf::type_aware_args, tf::output_args, in2csv_specific_args {
                Args() { file = "test_locale.csv"; num_locale = "de_DE"; }
            } args;
            expect(nothrow([&] {
                CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
                assert_converted(cout_buffer.str(), "test_locale_converted.csv");
            }));
        };
    }

    "no blanks"_test = [&] {
        struct Args : tf::single_file_arg, tf::common_args, tf::type_aware_args, tf::output_args, in2csv_specific_args {
            Args() { file = "blanks.csv"; }
        } args;
        expect(nothrow([&] {
            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            assert_converted(cout_buffer.str(), "blanks_converted.csv");
        }));
    };

    "blanks"_test = [&] {
        struct Args : tf::single_file_arg, tf::common_args, tf::type_aware_args, tf::output_args, in2csv_specific_args {
            Args() { file = "blanks.csv"; blanks = true; }
        } args;
        expect(nothrow([&] {
            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            assert_converted(cout_buffer.str(), "blanks.csv");
        }));
    };

    "null value"_test = [&] {
        struct Args : tf::single_file_arg, tf::common_args, tf::type_aware_args, tf::output_args, in2csv_specific_args {
            Args() { file = "_"; format = "csv"; null_value = {"\\N"}; }
        } args;

        std::istringstream iss("a,b\nn/a,\\N");
        stdin_subst new_cin(iss);

        CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
        expect(cout_buffer.str() == "a,b\n,\n");
    };
    //TODO fixme. blanks influences to standard blanks but not to --null-value;
    #if 0
    "null value blanks"_test = [&] {
        struct Args : tf::single_file_arg, tf::common_args, tf::type_aware_args, tf::output_args, in2csv_specific_args {
            Args() { file = "_"; format = "csv"; null_value = {"N"}; blanks = true;}
        } args;

        std::istringstream iss("a,b\nn/a,N");
        stdin_subst new_cin(iss);

        CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
        expect(cout_buffer.str() == "a,b\n,n/a,\n");
        std::cout << cout_buffer.str() << std::endl;
    };
    #endif

}
