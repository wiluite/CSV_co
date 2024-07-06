/// \file   test/csvclean_dry_run_test.cpp
/// \author wiluite
/// \brief  One of the tests for the csvclean utility.

#define BOOST_UT_DISABLE_MODULE
#include "ut.hpp"

#include "../utils/csvkit/csvclean.cpp"
#include "strm_redir.h"
#include "common_args.h"

int main() {

    using namespace boost::ut;

#if defined (WIN32)
    cfg < override > = {.colors={.none="", .pass="", .fail=""}};
#endif
    "dry_run"_test = [] () mutable {
        struct Args : csvkit::test_facilities::common_args {
            Args() { file = "bad.csv"; }
            bool dry_run {true};
        } args;

        notrimming_reader_type r (args.file);
        std::reference_wrapper<notrimming_reader_type> ref = std::ref(r);
       
        std::stringstream cerr_buffer;
        {
            redirect(cerr)
            redirect_cerr cr (cerr_buffer.rdbuf());
            csvclean::clean(ref, args);
        }

        expect(nothrow([&] {
              expect(!std::filesystem::exists(std::filesystem::path{"bad_err.csv"}));
              expect(!std::filesystem::exists(std::filesystem::path{"bad_out.csv"}));
              std::string temp; 
              std::getline(cerr_buffer, temp);
              expect(temp.substr(0,6) == "Line 1");
              std::getline(cerr_buffer, temp);
              expect(temp.substr(0,6) == "Line 2");
        }));
    };

}
