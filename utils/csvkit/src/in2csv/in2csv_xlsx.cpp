#include "../../include/in2csv/in2csv_xlsx.h"
#include <cli.h>
#include <OpenXLSX.hpp>
#include <iostream>
#include "common_functions.h"

using namespace ::csvkit::cli;

namespace in2csv::detail::xlsx {

    static char stringSeparator = '\"';

    static void OutputHeaderString(std::ostringstream & oss, std::string const & str) {
        // now we have native header, and so "1" does not influence on the nature of this column
        can_be_number.push_back(1);
        std::ostringstream header_cell;
        tune_format(header_cell, "%.16g");
        for (auto e : str) {
            if (e == stringSeparator)
                header_cell << stringSeparator << stringSeparator;
            else
            if (e == '\\')
                header_cell << "\\\\";
            else
                header_cell << e;
        }
        if (header_cell.str().empty()) {
            std::cerr << "UnnamedColumnWarning: Column " << header_cell_index << " has no name. Using " << '"' << letter_name(header_cell_index) << "\".\n"; 
            header.push_back(letter_name(header_cell_index));
        }
        else
            header.push_back(header_cell.str());
        oss << header.back();
        ++header_cell_index;
    }

    inline static void OutputString(std::ostringstream & oss, std::string const & str) {
        // now we have first line of the body, and so "0" really influence on the nature of this column
        if (can_be_number.size() < header.size())
            can_be_number.push_back(0);

        oss << stringSeparator;
        for (auto e : str) {
            if (e == stringSeparator)
                oss << stringSeparator << stringSeparator;
            else
            if (e == '\\')
                oss << "\\\\";
            else
                oss << e;
        }
     
        oss << stringSeparator;
    }

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

        // note: all indices in OpenXLSX is 1-based, this function is for --write-sheets option
        auto sheet_name_by_zero_based_index = [&doc](std::string const & index) { 
            unsigned idx = std::atoi(index.c_str()); 
            return static_cast<XLDocument&>(doc).workbook().worksheet(idx + 1).name();
        };

        if (a.sheet.empty())
            a.sheet = sheet_name_by_zero_based_index("0");

        auto sheet_index_by_name = [&doc](std::string const & name) {
            return static_cast<XLDocument&>(doc).workbook().indexOfSheet(name);
        };

        auto sheet_index = sheet_index_by_name(a.sheet);

        auto print_sheet = [&doc](int sheet_idx, std::ostream & os, impl_args arguments, use_date_datetime_xls use_d_dt_xls) {
            auto args (std::move(arguments));
            header.clear();
            header_cell_index = 0;
            can_be_number.clear();
            dates_ids.clear();
            datetimes_ids.clear();
        };

#if 0
        std::vector<XLCellValue> readValues;
        auto wks = static_cast<XLDocument&>(doc).workbook().worksheet("data");
        auto const col_cnt = wks.columnCount();
        tune_format(std::cout, "%.16g");
        for (auto& row : wks.rows()) {
            readValues = row.values();
            if (readValues.size() != col_cnt)
                readValues.resize(col_cnt); 

            std::cout << readValues.front();
            std::for_each (readValues.begin() + 1, readValues.end(), [](auto & elem) {
                //std::cout << ',' << elem.getString();
                std::cout << ',' << elem;
            });
                
            std::cout << '\n'; 
        }
#endif
    }

    void impl::convert() {
        try {
            convert_impl(a);
        }  catch (ColumnIdentifierError const& e) {
            std::cout << e.what() << '\n';
        }
    }
}
