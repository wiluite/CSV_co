/// \file   test/csvclean_optional_quote_characters_test.cpp
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
    "optional quote characters"_test = [] () mutable {
        struct Args : csvkit::test_facilities::single_file_arg, csvkit::test_facilities::common_args {
            Args() { file = "optional_quote_characters.csv"; maxfieldsize = max_unsigned_limit; }
            bool dry_run {false};
        } args;

        notrimming_reader_type r (args.file);
        std::reference_wrapper<notrimming_reader_type> ref = std::ref(r);
        csvclean::clean(ref, args);
        expect(nothrow([&](){
            csvkit::test_facilities::assertCleaned ("optional_quote_characters",{"a,b,c","1,2,3"},{});
        }));
    };

}
