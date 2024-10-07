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
    };

    "exceptions"_test = [] {
        struct Args : tf::single_file_arg, tf::common_args, in2csv_specific_args {
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
        expect(nothrow([&] {CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))}));
    };

}
