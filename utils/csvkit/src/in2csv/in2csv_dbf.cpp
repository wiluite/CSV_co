#include "../../include/in2csv/in2csv_dbf.h"
#include <cli.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#include "../../external/dbf_lib/include/bool.h"
#include "../../external/dbf_lib/include/dbf.h"

#include <functional>

using namespace ::csvkit::cli;
using namespace ::csvkit::cli::encoding;

//TODO: do type-aware printing
//TODO: RAII
namespace in2csv::detail::dbf {

    void impl::convert() {
        DBF_HANDLE handle = dbf_open(a.file.string().c_str(), nullptr);
        BOOL ok = (handle != nullptr);
        if (!ok)
            throw std::runtime_error(std::string("Unable to open file: ") + "'" + a.file.string() + "'");

        size_t rows = dbf_getrecordcount(handle);
        size_t cols = dbf_getfieldcount(handle);
        DBF_FIELD_INFO info;
     
        std::string header;
        using field_type = int;
        std::vector<field_type> types;

        if (a.linenumbers)
            std::cout << "line_number,";

        for (auto j = 0u; j < cols; j++) {
            dbf_getfield_info (handle, j, &info);
            header += (j ? "," : "") + recode_source(std::string(info.name), a);
            types.push_back(static_cast<field_type>(info.type));
        }
        std::cout << header << '\n';

        std::array<std::function<std::string(std::string const &)>, 9> type2func = {{
            [&](std::string const & value) { // CHAR
                return value;
            },
            [&](std::string const & value) { // INTEGER
                return value;
            },
            [&](std::string const & value) { // FLOAT
                return value;
            },
            [&](std::string const & value) { // DATE
                return value;
            },
            [&](std::string const & value) { // TIME
                return value;
            },
            [&](std::string const & value) { // DATETIME
                return value;
            },
            [&](std::string const & value) { // MEMO
                return value;
            },
            [&](std::string const & value) { // BOOLEAN
                return value;
            },
            [&](std::string const & value) { // ENUMCOUNT
                return value;
            }
        }};

        for (auto i = 0u; i < rows; i++) {
            if (a.linenumbers) {
                static std::size_t linenumber = 0; 
                std::cout << ++linenumber << ',';
            }
            std::string line;
            dbf_setposition(handle, i);
            for (auto j = 0u; j < cols; j++) {
                char temp[1024] = "";
                dbf_getfield(handle, dbf_getfieldptr(handle, j), temp, sizeof(temp), DBF_DATA_TYPE_ANY);
                auto const t = types[i];
                line += (j ? "," : "") + (t >= 0 ? type2func[types[i]](recode_source(std::string(temp), a)) : "");
            }
            std::cout << line << '\n';
        }

        dbf_close(&handle);
    }    
}
