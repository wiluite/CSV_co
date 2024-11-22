#include "../../include/in2csv/in2csv_xlsx.h"
#include <cli.h>
#include <OpenXLSX.hpp>
#include <iostream>
#include "common_datetime_excel.h"
#include "common_time_point.h"
#include "common_excel.h"

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
                    delete descriptor;
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
            a.sheet = sheet_name_by_zero_based_index("0"); // "ʤ"

        auto zero_based_sheet_index_by_name = [&doc](std::string const & name) {
            return static_cast<XLDocument&>(doc).workbook().indexOfSheet(name) - 1;
        };

        auto sheet_index = zero_based_sheet_index_by_name(a.sheet);

        is1904 = a.is1904;

        auto print_sheet = [&](int sheet_idx, std::ostream & os, impl_args arguments, use_date_datetime_excel use_d_dt) {
            auto args (std::move(arguments));

            auto wks = static_cast<XLDocument&>(doc).workbook().worksheet(sheet_name_by_zero_based_index(std::to_string(sheet_idx)));

            header.clear();
            header_cell_index = 0;
            can_be_number.clear();
            dates_ids.clear();
            datetimes_ids.clear();

            std::ostringstream oss;

            auto const column_count = wks.columnCount();
            if (!column_count)
                throw std::runtime_error("Column count in this particular sheet is 0.");

            generate_and_print_header(oss, args, column_count, use_d_dt);

            tune_format(oss, "%.16g");

            std::size_t j = 0;
            std::vector<XLCellValue> readValues;
            for (auto& row : wks.rows()) {
                if (j < args.skip_lines) {
                    ++j;
                    continue;
                }
                if (j != args.skip_lines)
                    oss << '\n';

                if (j == args.skip_lines + 1 and !args.no_header) // now we have really the native header
                    get_date_and_datetime_columns(args, header, use_d_dt);

                static void (*output_string_func)(std::ostringstream &, std::string const &) = OutputString;
                static void (*output_number_func)(std::ostringstream &, const double, unsigned) = OutputNumber;

                output_string_func = (!args.no_header and j == args.skip_lines) ? OutputHeaderString : OutputString;
                output_number_func = (!args.no_header and j == args.skip_lines) ? OutputHeaderNumber : OutputNumber;

                readValues = row.values();
                if (readValues.size() != column_count)
                    readValues.resize(column_count);
                auto col = 0u;
                for (auto & value : readValues) {
                   if (std::addressof(value) != std::addressof(readValues.front()))
                       oss << ',';

                   switch (value.type()) {
                       case XLValueType::Empty:
                           output_string_func(oss, "");
                           break;
                       case XLValueType::Boolean:
                           output_string_func(oss, (value.get<bool>() == true ? "True" : "False"));
                           break;
                       case XLValueType::Integer:
                           output_number_func(oss, value.get<int64_t>(), col);
                           break;
                       case XLValueType::Float:
                           output_number_func(oss, value.get<double>(), col);
                           break;
                       case XLValueType::String:
                           output_string_func(oss, value.getString());
                           break;
                       default:
                           output_string_func(oss, "*error*");
                   }
                   col++;
                }

                j++;
            }

            args.skip_lines = 0;
            args.no_header = false;
            std::variant<std::monostate, notrimming_reader_type, skipinitspace_reader_type> variants;
            if (!args.skip_init_space)
                variants = notrimming_reader_type(oss.str());
            else
                variants = skipinitspace_reader_type(oss.str());

            std::visit([&](auto & arg) {
                if constexpr(!std::is_same_v<std::decay_t<decltype(arg)>, std::monostate>) {
                    auto types_and_blanks = std::get<1>(typify(arg, args, typify_option::typify_without_precisions));
                    std::size_t line_nums = 0;
                    arg.run_rows(
                            [&](auto) {
                                if (args.linenumbers)
                                    os << "line_number,";

                                std::for_each(header.begin(), header.end() - 1, [&](auto const & elem) {
                                    os << elem << ',';
                                });

                                os << header.back() << '\n';
                            }
                            ,[&](auto rowspan) {
                                if (args.linenumbers)
                                    os << ++line_nums << ',';

                                auto col = 0u;
                                using elem_type = typename std::decay_t<decltype(rowspan.back())>::reader_type::template typed_span<csv_co::unquoted>;
                                std::for_each(rowspan.begin(), rowspan.end()-1, [&](auto const & e) {
                                    print_func(elem_type{e}, col++, types_and_blanks, args, os);
                                    os << ',';
                                });
                                print_func(elem_type{rowspan.back()}, col, types_and_blanks, args, os);
                                os << '\n';
                            }
                    );
                }
            }, variants);
        };

        print_sheet(sheet_index, std::cout, a, use_date_datetime_excel::yes);

        if (!a.write_sheets.empty()) {
            std::vector<std::string> sheet_names = [&] {
                std::vector<std::string> result;
                if (a.write_sheets != "-") {
                    std::istringstream stream(a.write_sheets);
                    for (std::string word; std::getline(stream, word, ',');) {
                        if (is_number(word))
                            result.push_back(sheet_name_by_zero_based_index(word));
                        else {
                            zero_based_sheet_index_by_name(word);
                            result.push_back(word);
                        }
                    }
                } else {
                    for (auto & name : static_cast<XLDocument&>(doc).workbook().sheetNames()) {
                        if (name.empty())
                            continue;
                        result.push_back(name);
                    }
                }
                return result;
            }();

            std::vector<std::string> sheet_filenames (sheet_names.size());
            int cursor = 0;
            for (auto const & e : sheet_names) {
                //TODO: encode filename according to activepage in Windows.
                auto const filename = "sheets_" + (a.use_sheet_names ? e : std::to_string(cursor)) + ".csv";
                std::ofstream ofs(filename);
                try {
                    print_sheet(zero_based_sheet_index_by_name(e), ofs, a, use_date_datetime_excel::no);
                } catch(std::exception const & ex) {
                    std::cerr << ex.what() << std::endl;
                }
                cursor++;
            }
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
