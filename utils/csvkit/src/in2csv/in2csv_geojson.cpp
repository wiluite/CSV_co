#include "../../include/in2csv/in2csv_geojson.h"
#include <cli.h>
#include "../../external/lib_geojson/geojson.hh"
#include <iostream>
#include <sstream>

using namespace ::csvkit::cli;

namespace in2csv::detail::geojson {

    void impl::convert() {
#if 0
        try {
            convert_impl(a);
        }  catch (ColumnIdentifierError const& e) {
            std::cout << e.what() << '\n';
        }
#endif
    }
}
