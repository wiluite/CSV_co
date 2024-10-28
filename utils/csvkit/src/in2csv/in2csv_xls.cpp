#include "../../include/in2csv/in2csv_xls.h"
#include <cli.h>
#include "../../external/libxls/include/xls.h"

using namespace ::csvkit::cli;
//using namespace ::csvkit::cli::encoding;

//TODO: do type-aware printing
//TODO: RAII
namespace in2csv::detail::xls {

    void impl::convert() {
        using namespace ::xls;
        xls_error_t error = LIBXLS_OK;
        auto const pWB = xls_open_file(a.file.string().c_str(), a.encoding_xls.c_str(), &error);
        if (!pWB)
            std::runtime_error(std::string(xls_getError(error)));
        if (a.names) {
            for (auto i = 0u; i < pWB->sheets.count; i++)
                printf("%s\n", pWB->sheets.sheet[i].name ? pWB->sheets.sheet[i].name : "");
            return;
        } 
        xls_close(pWB);
    }
}
