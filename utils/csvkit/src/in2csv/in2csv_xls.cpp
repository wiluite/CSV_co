#include "../../include/in2csv/in2csv_xls.h"
#include <cli.h>
#include "../../external/libxls/include/xls.h"
#include "../../external/date/date.h"
#include <iostream>
#include <sstream>

using namespace ::csvkit::cli;

//TODO: do type-aware printing
//TODO: RAII

namespace in2csv::detail::xls {

static char stringSeparator = '\"';
static char const *fieldSeparator = ",";

static void OutputString(auto & oss, const char *string) {
    const char *str;

    oss << stringSeparator;
    for (str = string; *str; str++) {
        if (*str == stringSeparator) {
            oss << stringSeparator << stringSeparator;
        } else if (*str == '\\') {
            oss << "\\\\";
        } else {
            oss << *str;
        }
    }
    oss << stringSeparator;
}

static std::chrono::system_clock::time_point to_chrono_time_point(double d) {
    using namespace std::chrono;
    using namespace date;
    using ddays = duration<double, date::days::period>;
#if 0
    return date::sys_days{date::December/31/1899} + round<system_clock::duration>(ddays{d});
#endif
    return date::sys_days{date::January/01/1904} + round<system_clock::duration>(ddays{d});
}

static bool is1904_and_datetime_header;
// Output a CSV Number
static void OutputNumber(auto & oss, const double number) {
#if 1
    oss << number;
#else
    //TODO: if is1904 and "date" in header
    using date::operator<<;
    std::cout << to_chrono_time_point(number);
#endif
}

    std::vector<std::string> generate_header(unsigned length) {
        std::vector<std::string> letter_names (length);
        unsigned i = 0;
        std::generate(letter_names.begin(), letter_names.end(), [&i] {
            return letter_name(i++);
        });

        return letter_names;
    }

    void impl::convert() {
        using namespace ::xls;
        xls_error_t error = LIBXLS_OK;

        struct pwb_holder {
            std::shared_ptr<xlsWorkBook> ptr;
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
//        printf("   mode: 0x%x\n", pWB->is5ver);
//        printf("   mode: 0x%x\n", pWB->is1904);
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

        struct pws_holder {
            std::shared_ptr<xlsWorkSheet> ptr;
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

        std::ostringstream oss;
        if (a.no_header) {
            auto header = generate_header(pws->rows.lastcol + 1);
            a.no_header = false;
            for (auto & e : header)
                oss << (std::addressof(e) == std::addressof(header.front()) ? e : "," + e);
            oss << '\n';
        }
        tune_format(oss, "%.15g");
        for (auto j = a.skip_lines; j <= (unsigned int)pws->rows.lastrow; ++j) {
            WORD cellRow = (WORD)j;
            if (j != a.skip_lines)
                oss << '\n';

            WORD cellCol;
            for (cellCol = 0; cellCol <= pws->rows.lastcol; cellCol++) {
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
                    OutputNumber(oss, cell->d);
                else if (cell->id == XLS_RECORD_FORMULA || cell->id == XLS_RECORD_FORMULA_ALT) {
                    // formula
                    if (cell->l == 0)
                        OutputNumber(oss, cell->d);
                    else if (cell->str) {
                        if (!strcmp((char *)cell->str, "bool")) // its boolean, and test cell->d
                            OutputString(oss, (int) cell->d ? "true" : "false");
                        else
                        if (!strcmp((char *)cell->str, "error")) // formula is in error
                            OutputString(oss, "*error*");
                        else // ... cell->str is valid as the result of a string formula.
                            OutputString(oss, (char *)cell->str);
                    }
                } else if (cell->str)
                    OutputString(oss, (char *)cell->str);
                else
                    OutputString(oss, "");
            }
        }

        a.skip_lines = 0;
        std::variant<std::monostate, notrimming_reader_type, skipinitspace_reader_type> variants;

        if (!a.skip_init_space)
            variants = notrimming_reader_type(oss.str());
        else
            variants = skipinitspace_reader_type(oss.str());

        std::visit([&](auto & arg) {
            if constexpr(!std::is_same_v<std::decay_t<decltype(arg)>, std::monostate>) {
                auto [types, blanks] = std::get<1>(typify(arg, a, typify_option::typify_without_precisions));
                for (auto e : types) {
                    std::cout << static_cast<int>(e) << std::endl;
                }

                arg.run_rows(
                    [&](auto rowspan) {
                    }
                    ,[&](auto rowspan) {
                    }
                );
            }
        }, variants);

    }
}
