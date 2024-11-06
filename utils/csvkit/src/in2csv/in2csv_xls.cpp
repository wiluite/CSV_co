#include "../../include/in2csv/in2csv_xls.h"
#include <cli.h>
#include "../../external/libxls/include/xls.h"
#include "../../external/date/date.h"
#include <iostream>
#include <sstream>

using namespace ::csvkit::cli;

namespace in2csv::detail::xls {

    static char stringSeparator = '\"';
    static char const *fieldSeparator = ",";

    std::vector<std::string> header;
    static unsigned header_cell_index = 0;
    // TODO: do not forget to implement
    std::vector<unsigned> can_be_number;

    static void OutputHeaderString(std::ostringstream & oss, const char *string) {
#if 0
        std::cerr << "OutputHeaderString\n";
#endif
        std::ostringstream header_cell;
        tune_format(header_cell, "%.15g");
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
        if (header_cell.str() == R"()") {
            std::cerr << "UnnamedColumnWarning: Column " << header_cell_index << " has no name. Using " << '"' << letter_name(header_cell_index) << "\".\n"; 
            header.push_back(letter_name(header_cell_index));
        }
        else
            header.push_back(header_cell.str());
        oss << header.back();
        ++header_cell_index;
        can_be_number.push_back(0);
    }

    static void OutputHeaderNumber(std::ostringstream & oss, const double number, unsigned) {
        std::ostringstream header_cell;
        tune_format(header_cell, "%.15g");

        header_cell << number;
        header.push_back(header_cell.str());
        oss << header.back();
        ++header_cell_index;
        can_be_number.push_back(1);
    }

    std::vector<unsigned> dates_ids;
    std::vector<unsigned> datetimes_ids;

    inline static void OutputString(std::ostringstream & oss, const char *string) {
        assert(header.size() == 8);
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

    static bool is1904;

    inline static std::chrono::system_clock::time_point to_chrono_time_point(double d) {
        using namespace std::chrono;
        using namespace date;
        using ddays = duration<double, date::days::period>;
        if (is1904)
            return date::sys_days{date::January/01/1904} + round<system_clock::duration>(ddays{d});
        else if (d < 60)
            return date::sys_days{date::December/31/1899} + round<system_clock::duration>(ddays{d});
        else
            return date::sys_days{date::December/30/1899} + round<system_clock::duration>(ddays{d});
    }

    inline bool is_date_column(unsigned column) {
        return /*can_be_number[column] and */ std::find(dates_ids.begin(), dates_ids.end(), column) != std::end(dates_ids);
    }

    inline bool is_datetime_column(unsigned column) {
        return /*can_be_number[column] and */ std::find(datetimes_ids.begin(), datetimes_ids.end(), column) != std::end(datetimes_ids);
    }

    inline static void OutputNumber(std::ostringstream & oss, const double number, unsigned column) {
        assert(header.size() == 8);
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
 
    std::vector<std::string> generate_header(unsigned length) {
        std::vector<std::string> letter_names (length);
        unsigned i = 0;
        std::generate(letter_names.begin(), letter_names.end(), [&i] {
            return letter_name(i++);
        });

        return letter_names;
    }

    void print_func (auto && elem, std::size_t col, auto && types_n_blanks, auto const & args) {
        using elem_type = std::decay_t<decltype(elem)>;
        auto & [types, blanks] = types_n_blanks;
        bool const is_null = elem.is_null();
        if (types[col] == column_type::text_t or (!args.blanks and is_null)) {
            auto compose_text = [&](auto const & e) -> std::string {
                typename elem_type::template rebind<csv_co::unquoted>::other const & another_rep = e;
                std::string unquoted = another_rep.str();
                return unquoted.find(',') == std::string::npos ? unquoted : another_rep; // another_rep casts to original string by default
            };
            std::cout << (!args.blanks && is_null ? "" : compose_text(elem));
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
        };
        auto const type_index = static_cast<std::size_t>(types[col]) - 1;
        std::cout << type2func[type_index](elem, std::any{});
    }

    void convert_impl(auto & a) {
        using namespace ::xls;
        xls_error_t error = LIBXLS_OK;

        class pwb_holder {
            std::shared_ptr<xlsWorkBook> ptr;
        public:
            pwb_holder(impl_args const & a, xls_error_t & err) : ptr (
                [&a, &err] {
                    if (a.file.empty() or a.file == "_") {
                        static std::string WB;
                        _setmode(_fileno(stdin), _O_BINARY);
                        for (;;) {
                            if (auto r = std::cin.get(); r != std::char_traits<char>::eof())
                                WB += r;
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

        assert(xls_parseWorkBook(pwb) == 0);
        is1904 = pwb->is1904;

        if (!pwb)
            std::runtime_error(std::string(xls_getError(error)));

        if (a.names) {
            for (auto i = 0u; i < pwb->sheets.count; i++)
                printf("%s\n", pwb->sheets.sheet[i].name ? pwb->sheets.sheet[i].name : "");
            return;
        }
        if (a.sheet.empty())
            a.sheet = pwb->sheets.sheet[0].name;

        auto sheet_index = -1;
        for (auto i = 0u; i < pwb->sheets.count; i++) {
            if (!pwb->sheets.sheet[i].name)
                continue;
            if (strcmp(a.sheet.c_str(), (char *)pwb->sheets.sheet[i].name) == 0) {
                sheet_index = i;
                break;
            }
        }
        if (sheet_index == -1)
            throw std::runtime_error(std::string("No sheet named ") + "'" + a.sheet + "'");

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
        } pws(pwb, sheet_index);

        // open and parse the sheet
        if (xls_parseWorkSheet(pws) != LIBXLS_OK)
            throw std::runtime_error("Error parsing the sheet.");

        auto get_date_and_datetime_columns = [&] {
            if (a.d_xls != "none") {
                std::string not_columns;
                dates_ids = parse_column_identifiers(columns{a.d_xls}, header, get_column_offset(a), excludes{not_columns});
            }

            if (a.dt_xls != "none") {
                std::string not_columns;
                datetimes_ids = parse_column_identifiers(columns{a.dt_xls}, header, get_column_offset(a), excludes{not_columns});
            }
        };

        std::ostringstream oss;
        if (a.no_header) {
            header = generate_header(pws->rows.lastcol + 1);
            for (auto & e : header)
                oss << (std::addressof(e) == std::addressof(header.front()) ? e : "," + e);
            oss << '\n';
            get_date_and_datetime_columns();
        }

        tune_format(oss, "%.15g");

        for (auto j = a.skip_lines; j <= (unsigned int)pws->rows.lastrow; ++j) {
            WORD cellRow = (WORD)j;
            if (j != a.skip_lines)
                oss << '\n';

            if (j == a.skip_lines + 1 and !a.no_header) { // now we have really the native header
                get_date_and_datetime_columns();
                assert(header.size() == 8);
            }

            static void (*output_string_func)(std::ostringstream &, const char *) = OutputString;
            static void (*output_number_func)(std::ostringstream &, const double, unsigned) = OutputNumber;

            output_string_func = (!a.no_header and j == a.skip_lines) ? OutputHeaderString : OutputString;
            output_number_func = (!a.no_header and j == a.skip_lines) ? OutputHeaderNumber : OutputNumber;

            for (WORD cellCol = 0; cellCol <= pws->rows.lastcol; cellCol++) {
                xlsCell *cell = xls_cell(pws, cellRow, cellCol);
                if (!cell || cell->isHidden)
                    continue;

                if (cellCol)
                    oss << fieldSeparator;

#if 0
                // display the colspan as only one cell, but reject rowspans (they can't be converted to CSV)

                if (cell->rowspan > 1)
                    fprintf(stderr, "Warning: %d rows spanned at col=%d row=%d: output will not match the Excel file.\n", cell->rowspan, cellCol+1, cellRow+1);
#endif
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

        //std::cout << oss.str() << std::endl;
        a.skip_lines = 0;
        a.no_header = false;
        std::variant<std::monostate, notrimming_reader_type, skipinitspace_reader_type> variants;

        if (!a.skip_init_space)
            variants = notrimming_reader_type(oss.str());
        else
            variants = skipinitspace_reader_type(oss.str());

        std::visit([&](auto & arg) {
            if constexpr(!std::is_same_v<std::decay_t<decltype(arg)>, std::monostate>) {
                auto types_and_blanks = std::get<1>(typify(arg, a, typify_option::typify_without_precisions));
#if 0
                for (auto e : std::get<0>(types_and_blanks))
                    std::cout << static_cast<int>(e) << std::endl;
#endif
                arg.run_rows(
                    [&](auto) {
                        if (a.linenumbers)
                            std::cout << "line_number,";

                        std::for_each(header.begin(), header.end() - 1, [&](auto const & elem) {
                            std::cout << elem << ',';
                        });

                        std::cout << header.back() << '\n';
                    }
                    ,[&](auto rowspan) {
                        if (a.linenumbers) {
                            static std::size_t line_nums = 0;
                            std::cout << ++line_nums << ',';
                        }
                        auto col = 0u;
                        using elem_type = typename std::decay_t<decltype(rowspan.back())>::reader_type::template typed_span<csv_co::unquoted>;
                        std::for_each(rowspan.begin(), rowspan.end()-1, [&](auto const & e) {
                            print_func(elem_type{e}, col++, types_and_blanks, a);
                            std::cout << ',';
                        });
                        print_func(elem_type{rowspan.back()}, col, types_and_blanks, a);
                        std::cout << '\n';
                    }
                );
            }
        }, variants);

    }

    void impl::convert() {
        try {
            convert_impl(a);
        }  catch (ColumnIdentifierError const& e) {
            std::cout << e.what() << '\n';
        }
    }
}
