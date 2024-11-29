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

    struct in2csv_args : tf::single_file_arg, tf::common_args, tf::type_aware_args, tf::output_args, in2csv_specific_args {};

    "exceptions"_test = [] {
        struct Args : in2csv_args {
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

    auto get_source = [](std::string const & pattern_filename) {
        std::ifstream ifs(pattern_filename);
        std::string result {std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>()};
        return result;
    };

    bool locale_support = detect_locale_support();
    if (locale_support) {
        "locale"_test = [&] {
            struct Args : in2csv_args {
                Args() { file = "test_locale.csv"; num_locale = "de_DE"; }
            } args;
            expect(nothrow([&] {
                CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
                expect(cout_buffer.str() == get_source("test_locale_converted.csv"));
            }));
        };
    }

    "no blanks"_test = [&] {
        struct Args : in2csv_args {
            Args() { file = "blanks.csv"; }
        } args;
        expect(nothrow([&] {
            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            expect(cout_buffer.str() == get_source("blanks_converted.csv"));
        }));
    };

    "blanks"_test = [&] {
        struct Args : in2csv_args {
            Args() { file = "blanks.csv"; blanks = true; }
        } args;
        expect(nothrow([&] {
            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            expect(cout_buffer.str() == get_source("blanks.csv"));
        }));
    };

    "null value"_test = [&] {
        struct Args : in2csv_args {
            // TODO : fixme with argument "\N"
            // TODO : check against clang sanitizer
            Args() { file = "_"; format = "csv"; null_value = {R"(N+)"}; }
        } args;

        std::istringstream iss("a,b\nn/a,N+");
        stdin_subst new_cin(iss);

        CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
        expect(cout_buffer.str() == "a,b\n,\n");
    };
#if 1
    "null value blanks"_test = [&] {
        struct Args : in2csv_args {
            // TODO : fixme with argument "\N"
            // TODO : check against clang sanitizer
            Args() { file = "_"; format = "csv"; null_value = {R"(N-)"}; blanks = true;}
        } args;

        std::istringstream iss("a,b\nn/a,N-\n");
        stdin_subst new_cin(iss);

        CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
        expect(cout_buffer.str() == "a,b\nn/a,\n");
    };
#endif

    "no leading zeroes"_test = [&] {
        struct Args : in2csv_args {
            Args() { file = "test_no_leading_zeroes.csv"; no_leading_zeroes = true;}
        } args;
        expect(nothrow([&] {
            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            expect(cout_buffer.str() == get_source("test_no_leading_zeroes.csv"));
        }));
    };

    "date format default"_test = [&] {
        struct Args : in2csv_args {
            Args() { file = "test_date_format.csv";}
        } args;
        expect(nothrow([&] {
            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            expect(cout_buffer.str() == get_source("test_date_format.csv"));
        }));
    };

    "numeric date format default"_test = [&] {
        struct Args : in2csv_args {
            Args() { file = "test_numeric_date_format.csv";}
        } args;
        expect(nothrow([&] {
            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            expect(cout_buffer.str() == get_source("test_numeric_date_format.csv"));
        }));
    };

    "date like number"_test = [&] {
        struct Args : in2csv_args {
            Args() { file = "date_like_number.csv";}
        } args;
        expect(nothrow([&] {
            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            expect(cout_buffer.str() == get_source("date_like_number.csv"));
        }));
    };

    "convert csv"_test = [&] {
        struct Args : in2csv_args {
            Args() { file = "testfixed_converted.csv";}
        } args;
        expect(nothrow([&] {
            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            expect(cout_buffer.str() == get_source("testfixed_converted.csv"));
        }));
    };

    "convert csv with skip lines"_test = [&] {
        struct Args : in2csv_args {
            Args() { file = "test_skip_lines.csv"; skip_lines = 3; no_inference = true; }
        } args;
        expect(nothrow([&] {
            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            expect(cout_buffer.str() == get_source("dummy.csv"));
        }));
    };

    "convert dbf"_test = [&] {
        struct Args : in2csv_args {
            Args() { file = "testdbf.dbf";}
        } args;
        expect(nothrow([&] {
            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            expect(cout_buffer.str() == get_source("testdbf_converted.csv"));
        }));
    };

    "convert json"_test = [&] {
        struct Args : in2csv_args {
            Args() { file = "testjson.json";}
        } args;
        expect(nothrow([&] {
            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            expect(cout_buffer.str() == get_source("testjson_converted.csv"));
        }));
    };

    //TODO: Adjust test_geojson.csv to canonical version.
    "convert geojson"_test = [&] {
        struct Args : in2csv_args {
            Args() { file = "test_geojson.json"; format = "geojson"; }
        } args;
        expect(nothrow([&] {
            CALL_TEST_AND_REDIRECT_TO_COUT(in2csv::in2csv(args))
            expect(cout_buffer.str() == get_source("test_geojson.csv"));
        }));
    };

}
