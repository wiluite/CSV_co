#include "../../include/in2csv/in2csv_dbf.h"
#include <cli.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#include "../../external/dbf_lib/include/bool.h"
#include "../../external/dbf_lib/include/dbf.h"

using namespace ::csvkit::cli;
using namespace ::csvkit::cli::encoding;

namespace in2csv::detail::dbf {

    void impl::convert() {
        DBF_HANDLE handle = dbf_open(a.file.string().c_str(), nullptr);
        BOOL ok = (handle != nullptr);

        if (ok) {
            size_t count = dbf_getrecordcount(handle);
            size_t fields = dbf_getfieldcount(handle);
            size_t i, j;
            DBF_FIELD_INFO info;
     
            std::string header; 
            for (j = 0; j < fields; j++) {
                dbf_getfield_info (handle, j, &info);
                header += (j ? "," : "") + recode_source(std::string(info.name), a);  
            }
            std::cout << header << '\n';

            dbf_close(&handle);
        } 
        else
            throw std::runtime_error(std::string("Unable to open file: ") + "'" + a.file.string() + "'");
    }
    
}
