#include "../../include/in2csv/in2csv_xls.h"
#include <cli.h>
#include "../../external/libxls/include/xls.h"
#include <iostream>
#include "common_datetime_excel.h"
#include "common_excel.h"
#include "common_xls.h"

using namespace ::csvkit::cli;

namespace in2csv::detail::xls {

    static char stringSeparator = '\"';

    static void OutputHeaderString(std::ostringstream & oss, const char *string) {
        // now we have native header, and so "1" does not influence on the nature of this column
        can_be_number.push_back(1);
#if 0
        std::cerr << "OutputHeaderString\n";
#endif
        std::ostringstream header_cell;
        tune_format(header_cell, "%.16g");
#if 0
        header_cell << stringSeparator;
#endif
        for (const char * str = string; *str; str++) {
            if (*str == stringSeparator)
                header_cell << stringSeparator << stringSeparator;
            else
            if (*str == '\\')
                header_cell << "\\\\";
            else
                header_cell << *str;
        }
#if 0
        header_cell << stringSeparator;
#endif
        if (header_cell.str().empty()) {
            std::cerr << "UnnamedColumnWarning: Column " << header_cell_index << " has no name. Using " << '"' << letter_name(header_cell_index) << "\".\n"; 
            header.push_back(letter_name(header_cell_index));
        }
        else
            header.push_back(header_cell.str());
        oss << header.back();
        ++header_cell_index;
    }

    inline static void OutputString(std::ostringstream & oss, const char *string) {
        // now we have first line of the body, and so "0" really influence on the nature of this column
        if (can_be_number.size() < header.size())
            can_be_number.push_back(0);

        oss << stringSeparator;
        for (const char *str = string; *str; str++) {
            if (*str == stringSeparator)
                oss << stringSeparator << stringSeparator;
            else
            if (*str == '\\')
                oss << "\\\\";
            else
                oss << *str;
        }
        oss << stringSeparator;
    }

    inline static void OutputNumber(std::ostringstream & oss, const double number, unsigned column) {
        // now we have first line of the body, and so "1" really influence on the nature of this column
        if (can_be_number.size() < header.size())
            can_be_number.push_back(1);

        if (number == 1.0) {
            oss << "1.0";
            return;
        }

        if (is_date_column(column)) {
            using date::operator<<;
            std::ostringstream local_oss;
            local_oss << to_chrono_time_point(number);
            auto str = local_oss.str();
            oss << std::string{str.begin(), str.begin() + 10};
        } else
        if (is_datetime_column(column)) {
            using date::operator<<;
            std::ostringstream local_oss;
            oss << to_chrono_time_point(number);
        } else
            oss << number;
    }

    void print_func (auto && elem, std::size_t col, auto && types_n_blanks, auto const & args, std::ostream & os) {
        using elem_type = std::decay_t<decltype(elem)>;
        auto & [types, blanks] = types_n_blanks;
        bool const is_null = elem.is_null();
        if (types[col] == column_type::text_t or (!args.blanks and is_null)) {
            auto compose_text = [&](auto const & e) -> std::string {
                typename elem_type::template rebind<csv_co::unquoted>::other const & another_rep = e;
                if (another_rep.raw_string_view().find(',') != std::string_view::npos)
                    return another_rep;
                else
                    return another_rep.str();
            };
            os << (!args.blanks && is_null ? "" : compose_text(elem));
            return;
        }
        assert(!is_null && (!args.blanks || (args.blanks && !blanks[col])) && !args.no_inference);

        using func_type = std::function<std::string(elem_type const &, std::any const &)>;

        // TODO: FIXME. Clang sanitizers complains for this in unittests.
        //  Although it seems neither false positives nor bloat code here.
#if !defined(BOOST_UT_DISABLE_MODULE)
        static
#endif
        std::array<func_type, static_cast<std::size_t>(column_type::sz)> type2func {
                compose_bool<elem_type>
                , [&](elem_type const & e, std::any const & info) {
                    assert(!e.is_null());

                    static std::ostringstream ss;
                    ss.str({});

                    // Surprisingly, csvkit represents a number from file without distortion:
                    // knowing, that it is a valid number in any locale, it simply removes
                    // the thousands separators and replaces the decimal point with its
                    // C-locale equivalent. Thus, the number actually written to the file
                    // is output. and we have to do some tricks.
                    typename elem_type::template rebind<csv_co::unquoted>::other const & another_rep = e;
                    auto const value = another_rep.num();

                    if (std::isnan(value))
                        ss << "NaN";
                    else if (std::isinf(value))
                        ss << (value > 0 ? "Infinity" : "-Infinity");
                    else {
                        if (args.num_locale != "C") {
                            std::string s = another_rep.str();
                            another_rep.to_C_locale(s);
                            ss << s;
                        } else
                            ss << another_rep.str();
                    }
                    return ss.str();
                }
                , compose_datetime<elem_type>
                , compose_date<elem_type>
                , [](elem_type const & e, std::any const &) {
                    auto str = std::get<1>(e.timedelta_tuple());
                    return str.find(',') != std::string::npos ? R"(")" + str + '"' : str;
                }
        };
        auto const type_index = static_cast<std::size_t>(types[col]) - 1;
        os << type2func[type_index](elem, std::any{});
    }

    void convert_impl(impl_args & a) {
        using namespace ::xls;
        xls_error_t error = LIBXLS_OK;

        class pwb_holder {
            std::shared_ptr<xlsWorkBook> ptr;
        public:
            pwb_holder(impl_args const & a, xls_error_t & err) : ptr (
                [&a, &err] {
                    if (a.file.empty() or a.file == "_") {
                        static std::string WB;
#ifndef __linux__
                        _setmode(_fileno(stdin), _O_BINARY);
#endif
                        for (;;) {
                            if (auto r = std::cin.get(); r != std::char_traits<char>::eof())
                                WB += static_cast<char>(r);
                            else
                                break;
                        }
                        return xls_open_buffer(reinterpret_cast<unsigned char const *>(&*WB.cbegin()), WB.length(), a.encoding_xls.c_str(), &err);
                    } else
                        return xls_open_file(a.file.string().c_str(), a.encoding_xls.c_str(), &err);
                }(),
                [&](xlsWorkBook* descriptor) { xls_close_WB(descriptor); }
            ) {}
            operator xlsWorkBook* () {
                return ptr.get();
            }
            xlsWorkBook* operator->() {
                return ptr.get();
            }
        } pwb(a, error);

        if (xls_parseWorkBook(pwb) != 0)
            throw std::runtime_error("Error parsing the XLS workbook source.");
        is1904 = pwb->is1904;

        if (!pwb)
            throw std::runtime_error(std::string(xls_getError(error)));

        if (a.names) {
            for (auto i = 0u; i < pwb->sheets.count; i++)
                printf("%s\n", pwb->sheets.sheet[i].name ? pwb->sheets.sheet[i].name : "");
            return;
        }
        if (a.sheet.empty())
            a.sheet = pwb->sheets.sheet[0].name;

        auto sheet_index_by_name = [&pwb](std::string const & name) {
            for (auto i = 0u; i < pwb->sheets.count; i++) {
                if (!pwb->sheets.sheet[i].name)
                    continue;
                if (strcmp(name.c_str(), (char *)pwb->sheets.sheet[i].name) == 0)
                    return static_cast<int>(i);
            }
            throw std::runtime_error(std::string("No sheet named ") + "'" + name + "'");
        };

        auto sheet_index = sheet_index_by_name(a.sheet);

        auto print_sheet = [&pwb](int sheet_idx, std::ostream & os, impl_args arguments, use_date_datetime_excel use_d_dt) {
            auto args (std::move(arguments));
            header.clear();
            header_cell_index = 0;
            can_be_number.clear();
            dates_ids.clear();
            datetimes_ids.clear();
            class pws_holder {
                std::shared_ptr<xlsWorkSheet> ptr;
            public:
                pws_holder(pwb_holder & pwb, int sheet_index) : ptr (
                        [&pwb, &sheet_index] {
                            return xls_getWorkSheet(pwb, sheet_index);
                        }(),
                        [&](xlsWorkSheet* descriptor) { xls_close_WS(descriptor); }
                ) {}
                operator xlsWorkSheet* () {
                    return ptr.get();
                }
                xlsWorkSheet* operator->() {
                    return ptr.get();
                }
            } pws(pwb, sheet_idx);

            // open and parse the sheet
            if (xls_parseWorkSheet(pws) != LIBXLS_OK)
                throw std::runtime_error("Error parsing the sheet. Index: " + std::to_string(sheet_idx));

            std::ostringstream oss;

            generate_and_print_header(oss, args, pws->rows.lastcol + 1, use_d_dt);

            tune_format(oss, "%.16g");

            for (auto j = args.skip_lines; j <= (unsigned int)pws->rows.lastrow; ++j) {
                auto cellRow = (unsigned int)j;
                if (j != args.skip_lines)
                    oss << '\n';

                if (j == args.skip_lines + 1 and !args.no_header) // now we have really the native header
                    get_date_and_datetime_columns(args, header, use_d_dt);

                static void (*output_string_func)(std::ostringstream &, const char *) = OutputString;
                static void (*output_number_func)(std::ostringstream &, const double, unsigned) = OutputNumber;

                output_string_func = (!args.no_header and j == args.skip_lines) ? OutputHeaderString : OutputString;
                output_number_func = (!args.no_header and j == args.skip_lines) ? OutputHeaderNumber : OutputNumber;

                for (WORD cellCol = 0; cellCol <= pws->rows.lastcol; cellCol++) {
                    xlsCell *cell = xls_cell(pws, cellRow, cellCol);
                    if (!cell || cell->isHidden)
                        continue;

                    if (cellCol)
                        oss << ',';

                    // display the value of the cell (either numeric or string)
                    if (cell->id == XLS_RECORD_RK || cell->id == XLS_RECORD_MULRK || cell->id == XLS_RECORD_NUMBER)
                        output_number_func(oss, cell->d, cellCol);
                    else if (cell->id == XLS_RECORD_FORMULA || cell->id == XLS_RECORD_FORMULA_ALT) {
                        // formula
                        if (cell->l == 0)
                            output_number_func(oss, cell->d, cellCol);
                        else if (cell->str) {
                            if (!strcmp((char *)cell->str, "bool")) // its boolean, and test cell->d
                                output_string_func(oss, (int) cell->d ? "True" : "False");
                            else
                            if (!strcmp((char *)cell->str, "error")) // formula is in error
                                output_string_func(oss, "*error*");
                            else // ... cell->str is valid as the result of a string formula.
                                output_string_func(oss, (char *)cell->str);
                        }
                    } else if (cell->str)
                        output_string_func(oss, (char *)cell->str);
                    else
                        output_string_func(oss, "");
                }
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

        auto sheet_name_by_index = [&pwb](std::string const & index) {
            unsigned idx = std::atoi(index.c_str());
            if (idx >= pwb->sheets.count)
                throw std::runtime_error("List index out of range");
            return pwb->sheets.sheet[idx].name;
        };

        if (!a.write_sheets.empty()) {
            std::vector<std::string> sheet_names = [&] {
                std::vector<std::string> result;
                if (a.write_sheets != "-") {
                    std::istringstream stream(a.write_sheets);
                    for (std::string word; std::getline(stream, word, ',');) {
                        if (is_number(word)) {
                            result.push_back(sheet_name_by_index(word));
                        } else {
                            sheet_index_by_name(word);
                            result.push_back(word);
                        }
                    }
                } else {
                    for (auto i = 0u; i < pwb->sheets.count; i++) {
                        if (!pwb->sheets.sheet[i].name)
                            continue;
                        result.push_back(pwb->sheets.sheet[i].name);
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
                    print_sheet(sheet_index_by_name(e), ofs, a, use_date_datetime_excel::no);
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
