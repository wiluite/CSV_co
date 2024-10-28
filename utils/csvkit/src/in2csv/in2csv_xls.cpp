#include "../../include/in2csv/in2csv_xls.h"
#include <cli.h>
#include "../../external/libxls/include/xls.h"
#include "../../external/date/date.h"
#include <iostream>

using namespace ::csvkit::cli;

//TODO: do type-aware printing
//TODO: RAII

namespace in2csv::detail::xls {

static char  stringSeparator = '\"';
static char const *fieldSeparator = ",";

static void OutputString(const char *string) {
    const char *str;

    printf("%c", stringSeparator);
    for (str = string; *str; str++) {
        if (*str == stringSeparator) {
            printf("%c%c", stringSeparator, stringSeparator);
        } else if (*str == '\\') {
            printf("\\\\");
        } else {
            printf("%c", *str);
        }
    }
    printf("%c", stringSeparator);
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

// Output a CSV Number
static void OutputNumber(const double number) {
#if 1
    printf("%.15g", number);
#else
    //TODO: if is1904 and "date" in header
    using date::operator<<;
    std::cout << to_chrono_time_point(number);
#endif
}

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
        if (a.sheet.empty())
            a.sheet = pWB->sheets.sheet[0].name;

        auto sheet_index = -1;
        for (auto i = 0u; i < pWB->sheets.count; i++) {
            if (!pWB->sheets.sheet[i].name)
                continue;
            if (strcmp(a.sheet.c_str(), (char *)pWB->sheets.sheet[i].name) == 0) {
                sheet_index = i;
                break;
            }
        }
        if (sheet_index == -1)
            throw std::runtime_error(std::string("No sheet named ") + "'" + a.sheet + "'");

        // open and parse the sheet
        auto const pWS = xls_getWorkSheet(pWB, sheet_index);
        if (xls_parseWorkSheet(pWS) != LIBXLS_OK)
            throw std::runtime_error("Error parsing the document.");

        for (auto j = 0u; j <= (unsigned int)pWS->rows.lastrow; ++j) {
            WORD cellRow = (WORD)j;
            if (j) printf("\n");

            WORD cellCol;
            for (cellCol = 0; cellCol <= pWS->rows.lastcol; cellCol++) {
                xlsCell *cell = xls_cell(pWS, cellRow, cellCol);
                if (!cell || cell->isHidden)
                    continue;

                if (cellCol)
                    printf(fieldSeparator);
                // display the colspan as only one cell, but reject rowspans (they can't be converted to CSV)
                if (cell->rowspan > 1) {
#if 0
                    fprintf(stderr, "Warning: %d rows spanned at col=%d row=%d: output will not match the Excel file.\n", cell->rowspan, cellCol+1, cellRow+1);
#endif
                }
                // display the value of the cell (either numeric or string)
                if (cell->id == XLS_RECORD_RK || cell->id == XLS_RECORD_MULRK || cell->id == XLS_RECORD_NUMBER) {
                    OutputNumber(cell->d);
                } else if (cell->id == XLS_RECORD_FORMULA || cell->id == XLS_RECORD_FORMULA_ALT) {
                    // formula
                    if (cell->l == 0)
                        OutputNumber(cell->d);
                    else if (cell->str) {
                        if (!strcmp((char *)cell->str, "bool")) // its boolean, and test cell->d
                        {
                            OutputString((int) cell->d ? "true" : "false");
                        } else if (!strcmp((char *)cell->str, "error")) // formula is in error
                        {
                            OutputString("*error*");
                        } else // ... cell->str is valid as the result of a string formula.
                        {
                            OutputString((char *)cell->str);
                        }
                    }
                } else if (cell->str) {
                    OutputString((char *)cell->str);
                } else {
                    OutputString("");
                }
            }
        }

        xls_close(pWB);
    }
}
