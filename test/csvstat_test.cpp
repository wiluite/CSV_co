///
/// \file   test/csvstat_test.cpp
/// \author wiluite
/// \brief  Tests for the csvstat utility.

#define BOOST_UT_DISABLE_MODULE
#include "ut.hpp"

#include "../utils/csvkit/csvstat.cpp"
#include "strm_redir.h"
#include "common_args.h"
#include <regex>
#include "rapidjson/document.h"

#define TEST_NO_THROW  std::stringstream cout_buffer;                                                                   \
                       std::variant<std::monostate, csvstat::nt_unquoting_tuple> variants                               \
                           = std::make_tuple(csvstat::nt_unquoting_cell_type{}, std::ref(r));                           \
                       {                                                                                                \
                           redirect(cout)                                                                               \
                           redirect_cout cr(cout_buffer.rdbuf());                                                       \
                           expect(nothrow ([&]{std::visit([&](auto & arg) { csvstat::stat(arg, args);}, variants);}));  \
                       }

#define TEST_NO_THROW2 std::stringstream cout_buffer;                                                                   \
                       std::variant<std::monostate, csvstat::sis_unquoting_tuple> variants                              \
                           = std::make_tuple(csvstat::sis_unquoting_cell_type{}, std::ref(r));                          \
                       {                                                                                                \
                           redirect(cout)                                                                               \
                           redirect_cout cr(cout_buffer.rdbuf());                                                       \
                           expect(nothrow ([&]{std::visit([&](auto & arg) { csvstat::stat(arg, args);}, variants);}));  \
                       }

#define TEST_NO_THROW_EN_US_LOCALE  std::stringstream cout_buffer;                                                      \
                       std::variant<std::monostate, csvstat::nt_unquoting_tuple> variants                               \
                           = std::make_tuple(csvstat::nt_unquoting_cell_type{}, std::ref(r));                           \
                       {                                                                                                \
                           redirect(cout)                                                                               \
                           redirect_cout cr(cout_buffer.rdbuf());                                                       \
                           expect(nothrow ([&]{std::visit([&](auto & arg) {                                             \
                               csvstat::stat(arg, args, "en_US");}, variants);                                          \
                           }));                                                                                         \
                       }

int main() {
    using namespace boost::ut;

#if defined (WIN32)
    cfg < override > = {.colors={.none="", .pass="", .fail=""}};
#endif

    "runs"_test = [] {
        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::type_aware_args, tf::spread_args, tf::csvstat_specific_args {
            Args() { file = "test_utf8.csv"; }
        } args;
        {
            notrimming_reader_type r(args.file);
            TEST_NO_THROW
        }
        {
            notrimming_reader_type r("aa\n");
            std::variant<std::monostate, csvstat::nt_unquoting_tuple> variants = std::make_tuple(csvstat::nt_unquoting_cell_type{}, std::ref(r));
            expect(throws([&] { std::visit([&](auto &arg) { csvstat::stat(arg, args); }, variants); }));
        }
        {
            notrimming_reader_type r("aa\n1\n");
            TEST_NO_THROW
        }
    };

    "encoding"_test = [] {
        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::type_aware_args, tf::spread_args, tf::csvstat_specific_args {
            Args() { file = "test_latin1.csv"; }
        } args;

        {
            notrimming_reader_type r(args.file);
            std::variant<std::monostate, csvstat::nt_unquoting_tuple> variants = std::make_tuple(csvstat::nt_unquoting_cell_type{}, std::ref(r));
            expect(throws([&] { std::visit([&](auto &arg) { csvstat::stat(arg, args); }, variants); }));
        }
        {
            args.encoding = "bad_encoding";
            notrimming_reader_type r(args.file);
            std::variant<std::monostate, csvstat::nt_unquoting_tuple> variants = std::make_tuple(csvstat::nt_unquoting_cell_type{}, std::ref(r));
            expect(throws([&] { std::visit([&](auto &arg) { csvstat::stat(arg, args); }, variants); }));
        }
        {
            args.encoding = "latin1";
            notrimming_reader_type r(args.file);
            TEST_NO_THROW
        }
    };

    "columns"_test = [] {
        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::type_aware_args, tf::spread_args, tf::csvstat_specific_args {
            Args() {
                file = "testxls_converted.csv";
                columns = "2";
            }
        } args;

        notrimming_reader_type r(args.file);

        TEST_NO_THROW

        expect(cout_buffer.str().find(R"(1. "text")") == std::string::npos);
        expect(cout_buffer.str().find(R"(2. "date")") != std::string::npos);
    };

    "linenumber"_test = [] {
        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::type_aware_args, tf::spread_args, tf::csvstat_specific_args {
            Args() {
                file = "dummy.csv";
                columns = "2";
            }
        } args;

        notrimming_reader_type r(args.file);

        TEST_NO_THROW

        expect(cout_buffer.str().find(R"(1. "a")") == std::string::npos);
        expect(cout_buffer.str().find(R"(2. "b")") != std::string::npos);
    };

    "no header row"_test = [] {
        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::type_aware_args, tf::spread_args, tf::csvstat_specific_args {
            Args() {
                file = "no_header_row.csv";
                columns = "2";
                no_header = true;
            }
        } args;

        notrimming_reader_type r(args.file);

        TEST_NO_THROW

        expect(cout_buffer.str().find(R"(1. "a")") == std::string::npos);
        expect(cout_buffer.str().find(R"(2. "b")") != std::string::npos);
    };

    "count only"_test = [] {
        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::type_aware_args, tf::spread_args, tf::csvstat_specific_args {
            Args() {
                file = "ks_1033_data.csv";
                count = true;
            }
        } args;

        notrimming_reader_type r(args.file);

        TEST_NO_THROW

        expect(cout_buffer.str() == "1575\n");
    };

    "unique"_test = [] {
        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::type_aware_args, tf::spread_args, tf::csvstat_specific_args {
            Args() {
                file = "ks_1033_data.csv";
                columns = "county";
            }
        } args;

        notrimming_reader_type r(args.file);

        TEST_NO_THROW

        const std::regex txt_regex(R"([\s|\S]*Unique values:\s+73[\s|\S]*)");
        expect(std::regex_match(cout_buffer.str(), txt_regex));
    };

    "max length"_test = [] {
        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::type_aware_args, tf::spread_args, tf::csvstat_specific_args {
            Args() {
                file = "ks_1033_data.csv";
                columns = "county";
            }
        } args;

        notrimming_reader_type r(args.file);

        TEST_NO_THROW

        const std::regex txt_regex(R"([\s|\S]*Longest value:\s+12[\s|\S]*)");
        expect(std::regex_match(cout_buffer.str(), txt_regex));
    };

    "freq list"_test = [] {
        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::type_aware_args, tf::spread_args, tf::csvstat_specific_args {
            Args() { file = "ks_1033_data.csv"; }
        } args;

        notrimming_reader_type r(args.file);

        TEST_NO_THROW

        expect(cout_buffer.str().find(R"(WYANDOTTE (123x))") != std::string::npos);
        expect(cout_buffer.str().find(R"(FINNEY (103x))") != std::string::npos);
        expect(cout_buffer.str().find(R"(SALINE (59x))") != std::string::npos);
        expect(cout_buffer.str().find(R"(MIAMI (56x))") == std::string::npos);
    };

    "freq"_test = [] {
        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::type_aware_args, tf::spread_args, tf::csvstat_specific_args {
            Args() {
                file = "ks_1033_data.csv";
                freq = true;
            }
        } args;

        notrimming_reader_type r(args.file);

        TEST_NO_THROW

        expect(cout_buffer.str().find(R"(1. state: { "KS": 1575 })") != std::string::npos);
    };

    "freq count"_test = [] {
        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::type_aware_args, tf::spread_args, tf::csvstat_specific_args {
            Args() {
                file = "ks_1033_data.csv";
                freq_count = 1;
            }
        } args;

        notrimming_reader_type r(args.file);

        TEST_NO_THROW

        expect(cout_buffer.str().find(R"(WYANDOTTE (123x))") != std::string::npos);
        expect(cout_buffer.str().find(R"(FINNEY (103x))") == std::string::npos);
        expect(cout_buffer.str().find(R"(MIAMI (56x))") == std::string::npos);
    };

    "csv"_test = [] {
        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::type_aware_args, tf::spread_args, tf::csvstat_specific_args {
            Args() {
                file = "ks_1033_data.csv";
                csv = true;
            }
        } args;

        notrimming_reader_type r(args.file);

        TEST_NO_THROW

        notrimming_reader_type csv_reader(cout_buffer.str());
        int crow = 1;
        csv_reader.run_rows([](auto &header) {
                                expect(header[1] == "column_name");
                                expect(header[5] == "unique");
                            },
                            [&](auto &row) {
                                if (crow++ == 1) {
                                    using cell_type = notrimming_reader_type::typed_span<csv_co::unquoted>;

                                    //TODO: do conversion operators
                                    expect(cell_type(row[1]).str() == "state");
                                    expect(cell_type(row[2]).str() == "Text");
                                    expect(cell_type(row[6]).str().empty());
                                    expect(cell_type(row[12]).num() == 2);
                                }
                            });
    };

    "csv_columns"_test = [] {
        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::type_aware_args, tf::spread_args, tf::csvstat_specific_args {
            Args() {
                file = "ks_1033_data.csv";
                csv = true;
                columns = "4";
            }
        } args;

        notrimming_reader_type r(args.file);

        TEST_NO_THROW

        notrimming_reader_type csv_reader(cout_buffer.str());
        int crow = 1;
        csv_reader.run_rows([](auto &header) {
                                expect(header[1] == "column_name");
                                expect(header[5] == "unique");
                            },
                            [&](auto &row) {
                                if (crow++ == 1) {
                                    using cell_type = notrimming_reader_type::typed_span<csv_co::unquoted>;
                                    expect(cell_type(row[1]).str() == "nsn");
                                    expect(cell_type(row[2]).str() == "Text");
                                    expect(cell_type(row[6]).str().empty());
                                    expect(cell_type(row[12]).num() == 16);
                                }
                            });
    };

    "json"_test = [] {
        using namespace rapidjson;

        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::type_aware_args, tf::spread_args, tf::csvstat_specific_args {
            Args() {
                file = "ks_1033_data.csv";
                json = true;
            }
        } args;

        notrimming_reader_type r(args.file);

        TEST_NO_THROW

        Document document;
        document.Parse(cout_buffer.str().c_str());
        expect(!document.HasParseError());
        expect(document.IsArray());
        for (auto &e: document.GetArray()) {
            static unsigned row = 1;
            expect(e.HasMember("column_name"));
            expect(e.HasMember("unique"));
            expect(e.FindMember("column_name") - e.MemberBegin() == 1);
            expect(e.FindMember("unique") - e.MemberBegin() == 5);

            if (row++ == 1) {
                expect(std::string(e["column_name"].GetString()) == "state");
                expect(std::string(e["type"].GetString()) == "Text");
                expect(!e.HasMember("min"));
                expect(e.HasMember("len"));
                expect(e["len"].GetInt() == 2);
            }
        }
    };

    "json_columns"_test = [] {
        using namespace rapidjson;

        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::type_aware_args, tf::spread_args, tf::csvstat_specific_args {
            Args() {
                file = "ks_1033_data.csv";
                json = true;
                columns = "4";
            }
        } args;

        notrimming_reader_type r(args.file);

        TEST_NO_THROW

        Document document;
        document.Parse(cout_buffer.str().c_str());
        expect(!document.HasParseError());
        expect(document.IsArray());

        for (auto &e: document.GetArray()) {
            static unsigned row = 1;
            expect(e.HasMember("column_name"));
            expect(e.HasMember("unique"));
            expect(e.FindMember("column_name") - e.MemberBegin() == 1);
            expect(e.FindMember("unique") - e.MemberBegin() == 5);

            if (row++ == 1) {
                expect(std::string(e["column_name"].GetString()) == "nsn");
                expect(std::string(e["type"].GetString()) == "Text");
                expect(!e.HasMember("min"));
                expect(e.HasMember("len"));
                expect(e["len"].GetInt() == 16);
            }
        }
    };

    auto get_result = [&] (auto s){
        return std::string(s.begin(), s.end()-1);
    };

    "decimal_format"_test = [&] {
        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::type_aware_args, tf::spread_args, tf::csvstat_specific_args {
            // TODO:provide in csvstat the num_locale cell state many times in the session
            Args() {
                file = "FY09_EDU_Recipients_by_State.csv";
                columns = "TOTAL";
                num_locale = "en_US";
                mean = true;
            }
        } args;

        bool locale_support = detect_locale_support();

        if (locale_support) {
            {
                notrimming_reader_type r(args.file);

                TEST_NO_THROW_EN_US_LOCALE

                expect("9,748.346" == get_result(cout_buffer.str()));
            }
            {
                args.no_grouping_sep = true;
                notrimming_reader_type r(args.file);

                TEST_NO_THROW_EN_US_LOCALE

                expect("9748.346" == get_result(cout_buffer.str()));
            }
            {
                args.decimal_format = "%.2f";
                args.no_grouping_sep = false;
                notrimming_reader_type r(args.file);

                TEST_NO_THROW_EN_US_LOCALE

                expect("9,748.35" == get_result(cout_buffer.str()));
            }
        }
    };

    "max_precision_on_off"_test = [&] {
        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::type_aware_args, tf::spread_args, tf::csvstat_specific_args {
            Args() {
                columns = "1";
                max_precision = true;
            }
        } args;

        {
            assert(!args.no_mdp);
            notrimming_reader_type r("a\n1.1\n3.333\n2.22\n");
            maxprecision_policy(r, args);

            TEST_NO_THROW

            expect("3" == get_result(cout_buffer.str()));
        }
        {
            args.no_mdp = true;
            skipinitspace_reader_type r("a\n1.1\n3.333\n2.22\n");
            maxprecision_policy(r, args);

            TEST_NO_THROW2

            expect("0" == get_result(cout_buffer.str()));
        }
    };
}
