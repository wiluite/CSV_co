///
/// \file   test/csvsql_test.cpp
/// \author wiluite
/// \brief  Tests for the sql2csv utility.

#define BOOST_UT_DISABLE_MODULE
#include "ut.hpp"

#include "../utils/csvkit/sql2csv.cpp"
#include "strm_redir.h"
#include "common_args.h"


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

}