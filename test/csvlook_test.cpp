///
/// \file   test/csvlook_test.cpp
/// \author wiluite
/// \brief  Tests for the csvlook utility.

#define BOOST_UT_DISABLE_MODULE
#include "ut.hpp"

#include "../utils/csvkit/csvlook.cpp"
#include "strm_redir.h"
#include "common_args.h"
#include "test_runner_macros.h"

#define CALL_TEST_AND_REDIRECT_TO_COUT_1 std::stringstream cout_buffer;                        \
                                         {                                                     \
                                             redirect(cout)                                    \
                                             redirect_cout cr(cout_buffer.rdbuf());            \
                                             csvlook::look(ref, args);                         \
                                         }

#define CALL_TEST_AND_REDIRECT_TO_COUT_2 std::stringstream cout_buffer;                               \
                                         {                                                            \
                                             redirect(cout)                                           \
                                             redirect_cout cr(cout_buffer.rdbuf());                   \
                                             test_reader_configurator_and_runner(args, csvlook::look) \
                                         }


int main() {
    using namespace boost::ut;

#if defined (WIN32)
    cfg < override > = {.colors={.none="", .pass="", .fail=""}};
#endif
    "runs"_test = [] {
        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::type_aware_args, tf::csvlook_specific_args {
            Args() { file = "test_utf8.csv"; }
        } args;

        notrimming_reader_type r (args.file);
        std::reference_wrapper<notrimming_reader_type> ref = std::ref(r);

        CALL_TEST_AND_REDIRECT_TO_COUT_1

//      | foo | bar | baz |
//      | --- | --- | --- |
//      |   1 |   2 | 3   |
//      |   4 |   5 | ʤ   |

        expect(cout_buffer.str() == "| foo | bar | baz | \n| --- | --- | --- | \n|   1 |   2 | 3   | \n|   4 |   5 | ʤ   | \n");
    };

    "simple"_test = [] {
        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::type_aware_args, tf::csvlook_specific_args {
            Args() { file = "dummy3.csv"; }
        } args;

        notrimming_reader_type r (args.file);
        std::reference_wrapper<notrimming_reader_type> ref = std::ref(r);

        CALL_TEST_AND_REDIRECT_TO_COUT_1

//      |    a | b | c |
//      | ---- | - | - |
//      | True | 2 | 3 |
//      | True | 4 | 5 |

        expect(cout_buffer.str() == "|    a | b | c | \n| ---- | - | - | \n| True | 2 | 3 | \n| True | 4 | 5 | \n");
    };

    "encoding"_test = [] {
        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::type_aware_args, tf::csvlook_specific_args {
            Args() { file = "test_latin1.csv"; encoding = "latin1"; }
        } args;

        CALL_TEST_AND_REDIRECT_TO_COUT_2

//      | a | b | c |
//      | - | - | - |
//      | 1 | 2 | 3 |
//      | 4 | 5 | © |
        expect(cout_buffer.str() == "| a | b | c | \n| - | - | - | \n| 1 | 2 | 3 | \n| 4 | 5 | © | \n");
    };

    "no blanks"_test = [] {
        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::type_aware_args, tf::csvlook_specific_args {
            Args() { file = "blanks.csv"; }
        } args;

        notrimming_reader_type r (args.file);
        std::reference_wrapper<notrimming_reader_type> ref = std::ref(r);

        CALL_TEST_AND_REDIRECT_TO_COUT_1

//      | a | b | c | d | e | f |
//      | - | - | - | - | - | - |
//      |   |   |   |   |   |   |

        std::string result = "| a | b | c | d | e | f | \n| - | - | - | - | - | - | \n|   |   |   |   |   |   | \n";
        expect(cout_buffer.str() == result);
    };

    "blanks"_test = [] {
        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::type_aware_args, tf::csvlook_specific_args {
            Args() { file = "blanks.csv"; blanks = true;}
        } args;

        notrimming_reader_type r (args.file);
        std::reference_wrapper<notrimming_reader_type> ref = std::ref(r);

        CALL_TEST_AND_REDIRECT_TO_COUT_1

//      | a | b  | c   | d    | e    | f |
//      | - | -- | --- | ---- | ---- | - |
//      |   | NA | N/A | NONE | NULL | . |

        expect(cout_buffer.str() == "| a | b  | c   | d    | e    | f | \n| - | -- | --- | ---- | ---- | - | \n|   | NA | N/A | NONE | NULL | . | \n");
    };

    "no header row"_test = [] {
        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::type_aware_args, tf::csvlook_specific_args {
            Args() { file = "no_header_row3.csv"; no_header = true; }
        } args;

        notrimming_reader_type r (args.file);
        std::reference_wrapper<notrimming_reader_type> ref = std::ref(r);

        CALL_TEST_AND_REDIRECT_TO_COUT_1

//      | a | b | c |
//      | - | - | - |
//      | 1 | 2 | 3 |
//      | 4 | 5 | 6 |

        expect(cout_buffer.str() == "| a | b | c | \n| - | - | - | \n| 1 | 2 | 3 | \n| 4 | 5 | 6 | \n");
    };

    // TODO: add just unicode test
    "unicode bom"_test = [] {
        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::type_aware_args, tf::csvlook_specific_args {
            Args() { file = "test_utf8_bom.csv"; }
        } args;

        notrimming_reader_type r (args.file);
        std::reference_wrapper<notrimming_reader_type> ref = std::ref(r);

        CALL_TEST_AND_REDIRECT_TO_COUT_1

//      | foo | bar | baz |
//      | --- | --- | --- |
//      |   1 |   2 | 3   |
//      |   4 |   5 | ʤ   |

        expect(cout_buffer.str() == "| foo | bar | baz | \n| --- | --- | --- | \n|   1 |   2 | 3   | \n|   4 |   5 | ʤ   | \n");
    };

    "linenumbers"_test = [] {
        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::type_aware_args, tf::csvlook_specific_args {
            Args() { file = "dummy3.csv"; linenumbers = true;}
        } args;

        notrimming_reader_type r (args.file);
        std::reference_wrapper<notrimming_reader_type> ref = std::ref(r);

        CALL_TEST_AND_REDIRECT_TO_COUT_1

//      | line_numbers |    a | b | c |
//      | ------------ | ---- | - | - |
//      |            1 | True | 2 | 3 |
//      |            2 | True | 4 | 5 |

        expect(cout_buffer.str() == "| line_numbers |    a | b | c | \n| ------------ | ---- | - | - | \n|            1 | True | 2 | 3 | \n|            2 | True | 4 | 5 | \n");
    };

    "no_inference"_test = [] {
        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::type_aware_args, tf::csvlook_specific_args {
            Args() { file = "dummy3.csv"; no_inference = true; }
        } args;

        notrimming_reader_type r (args.file);
        std::reference_wrapper<notrimming_reader_type> ref = std::ref(r);

        CALL_TEST_AND_REDIRECT_TO_COUT_1

//      | a | b | c |
//      | - | - | - |
//      | 1 | 2 | 3 |
//      | 1 | 4 | 5 |

        expect(cout_buffer.str() == "| a | b | c | \n| - | - | - | \n| 1 | 2 | 3 | \n| 1 | 4 | 5 | \n");
    };

    "max rows"_test = [] {
        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::type_aware_args, tf::csvlook_specific_args {
            Args() { file = "dummy.csv"; max_rows = 0; }
        } args;

        notrimming_reader_type r (args.file);
        std::reference_wrapper<notrimming_reader_type> ref = std::ref(r);

        CALL_TEST_AND_REDIRECT_TO_COUT_1

//      | a | b | c |
//      | - | - | - |
//      | . | . | . |

        expect(cout_buffer.str() == "| a | b | c | \n| - | - | - | \n| . | . | . | \n");
    };

    "max columns"_test = [] {
        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::type_aware_args, tf::csvlook_specific_args {
            Args() { file = "dummy.csv"; max_columns = 1; }
        } args;

        notrimming_reader_type r (args.file);
        std::reference_wrapper<notrimming_reader_type> ref = std::ref(r);

        CALL_TEST_AND_REDIRECT_TO_COUT_1

//      |    a | ... |
//      | ---- | --- |
//      | True | ... |

        expect(cout_buffer.str() == "|    a | ... |\n| ---- | --- |\n| True | ... |\n");
    };

    "max columns width"_test = [] {
        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::type_aware_args, tf::csvlook_specific_args {
            Args() { file = "dummy4.csv"; max_column_width = 4; }
        } args;

        notrimming_reader_type r (args.file);
        std::reference_wrapper<notrimming_reader_type> ref = std::ref(r);

        CALL_TEST_AND_REDIRECT_TO_COUT_1

//      |    a | b | c |
//      | ---- | - | - |
//      | F... | 2 | 3 |

        expect(cout_buffer.str() == "|    a | b | c | \n| ---- | - | - | \n| F... | 2 | 3 | \n");
    };

    "max precision"_test = [] {
        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::type_aware_args, tf::csvlook_specific_args {
            Args() { file = "test_precision.csv"; max_precision = 0; }
        } args;

        notrimming_reader_type r (args.file);
        std::reference_wrapper<notrimming_reader_type> ref = std::ref(r);

        CALL_TEST_AND_REDIRECT_TO_COUT_1

//      |  a |
//      | -- |
//      | 1… |

        expect(cout_buffer.str() == "|  a | \n| -- | \n| 1… | \n");
    };

    "no number ellipsis"_test = [] {
        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::type_aware_args, tf::csvlook_specific_args {
            Args() { file = "test_precision.csv"; no_number_ellipsis = true;}
        } args;

        notrimming_reader_type r (args.file);
        std::reference_wrapper<notrimming_reader_type> ref = std::ref(r);

        CALL_TEST_AND_REDIRECT_TO_COUT_1

//      |     a |
//      | ----- |
//      | 1.235 |

        expect(cout_buffer.str() == "|     a | \n| ----- | \n| 1.235 | \n");
    };

    "max precision no number ellipsis"_test = [] {
        namespace tf = csvkit::test_facilities;
        struct Args : tf::common_args, tf::type_aware_args, tf::csvlook_specific_args {
            Args() { file = "test_precision.csv"; max_precision = 0; no_number_ellipsis = true; }
        } args;

        notrimming_reader_type r (args.file);
        std::reference_wrapper<notrimming_reader_type> ref = std::ref(r);

        CALL_TEST_AND_REDIRECT_TO_COUT_1

//      | a |
//      | - |
//      | 1 |

        expect(cout_buffer.str() == "| a | \n| - | \n| 1 | \n");
    };
}
