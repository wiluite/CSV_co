/// \file   test/csvclean_changes_character_encoding_test.cpp
/// \author wiluite
/// \brief  One of the tests for the csvclean utility.

#define BOOST_UT_DISABLE_MODULE
#include "ut.hpp"

#include "../utils/csvkit/csvclean.cpp"
#include "csvclean_test_funcs.h"
#include "common_args.h"

int main() {

    using namespace boost::ut;

#if defined (WIN32)
    cfg < override > = {.colors={.none="", .pass="", .fail=""}};
#endif
    "changes character encoding"_test = [] () mutable {
        struct Args : csvkit::test_facilities::single_file_arg, csvkit::test_facilities::common_args {
            Args() { file = "test_latin1.csv"; maxfieldsize = max_unsigned_limit;} // TODO: Why does it requires max_unsigned_limit for test to pass?
            bool dry_run {false};
        } args;

        notrimming_reader_type r (args.file);
        std::reference_wrapper<notrimming_reader_type> ref = std::ref(r);
        // TODO: It should be done inside assertCleaned. -e latin1 is needed simply to parse and also to use transformation
        csvclean::clean(ref, args);
        expect(nothrow([&](){
            using namespace csvkit::cli;
            std::u8string u8str = u8"4,5,©";
            csvkit::test_facilities::assertCleaned ("test_latin1", {"a,b,c","1,2,3", encoding::iconv("latin1", "utf-8", std::string(u8str.begin(), u8str.end()))}, {});
        }));
    };

}
