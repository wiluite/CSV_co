#include "../../include/in2csv/in2csv_xlsx.h"
#include <cli.h>
#include <OpenXLSX.hpp>
#include <iostream>
#include "common_functions.h"

using namespace ::csvkit::cli;

namespace in2csv::detail::xlsx {

    void convert_impl(impl_args & a) {
        using namespace OpenXLSX;

        class xldocument_holder {
            std::string tmp_file = "temporary_xlsx_file.xlsx";
            std::shared_ptr<XLDocument> ptr;
        public:
            xldocument_holder(impl_args const & a) : ptr (
                [&a, this] {
                    if (a.file.empty() or a.file == "_") {
                        std::string WB;
#ifndef __linux__
                        _setmode(_fileno(stdin), _O_BINARY);
#endif
                        for (;;) {
                            if (auto r = std::cin.get(); r != std::char_traits<char>::eof())
                                WB += static_cast<char>(r);
                            else
                                break;
                        }

                        std::ofstream tmp (tmp_file, std::ios::app | std::ios::binary);
                        tmp << WB;
                        return new XLDocument(tmp_file);
                    } else
                        return new XLDocument(a.file.string());
                }(),
                [&](XLDocument* descriptor) {
                    descriptor->close();
                    if (a.file.empty() or a.file == "_")
                        std::remove(tmp_file.c_str());
                }
            ) {}
            operator XLDocument& () {
                return *ptr;
            }
        } doc(a);

        auto print_sheets = [](const XLDocument& d) {
            auto const & wb = d.workbook(); 
            for (const auto& name : wb.worksheetNames())
                std::cout << name << std::endl;
        };

        if (a.names) {
            print_sheets(doc);
            return;
        }

    }

    void impl::convert() {
        try {
            convert_impl(a);
        }  catch (ColumnIdentifierError const& e) {
            std::cout << e.what() << '\n';
        }
    }
}
