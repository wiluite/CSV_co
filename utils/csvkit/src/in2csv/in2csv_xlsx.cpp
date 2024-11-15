#include "../../include/in2csv/in2csv_xlsx.h"
#include <cli.h>
#include "../../external/openXLSX/openXLSX.hpp"
#include <iostream>
#include "common_functions.h"

using namespace ::csvkit::cli;

namespace in2csv::detail::xlsx {

    void convert_impl(impl_args & a) {
    }

    void impl::convert() {
        try {
            convert_impl(a);
        }  catch (ColumnIdentifierError const& e) {
            std::cout << e.what() << '\n';
        }
    }
}
