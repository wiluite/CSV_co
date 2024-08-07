/// \file   test/csvclean_no_header_row_test.cpp
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
    cfg<override> ={.colors={.none="", .pass="", .fail=""}};
#endif

    "no header row"_test = [] () mutable {
        struct Args : csvkit::test_facilities::single_file_arg, csvkit::test_facilities::common_args {
            Args() { file = "no_header_row.csv"; }
            bool dry_run {false};
        } args;

        notrimming_reader_type r (args.file);
        std::reference_wrapper<notrimming_reader_type> ref = std::ref(r);
        csvclean::clean(ref, args);
        expect(nothrow([&](){
            csvkit::test_facilities::assertCleaned ("no_header_row",{"1,2,3"},{});
        }));
    };

}
