#pragma once

namespace csvkit::cli {
#if 0
    void tune_format(std::ostream & os, char const * fmt);
    std::string letter_name(std::size_t column);
#endif
}

namespace {
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

    enum use_date_datetime_excel {
        yes,
        no
    };

    static std::vector<unsigned> dates_ids;
    static std::vector<unsigned> datetimes_ids;

    auto get_date_and_datetime_columns(auto && args, auto && header, use_date_datetime_excel use_d_dt) {
        using namespace ::csvkit::cli;
        if (use_d_dt == use_date_datetime_excel::no)
            return;

        if (args.d_excel != "none") {
            std::string not_columns;
            dates_ids = parse_column_identifiers(columns{args.d_excel}, header, get_column_offset(args), excludes{not_columns});
        }

        if (args.dt_excel != "none") {
            std::string not_columns;
            datetimes_ids = parse_column_identifiers(columns{args.dt_excel}, header, get_column_offset(args), excludes{not_columns});
        }
    };

    auto is_number = [](std::string const & name) {
        return std::all_of(name.begin(), name.end(), [](auto c) {return std::isdigit(c);});
    };

    std::vector<std::string> header;
    unsigned header_cell_index = 0;
    std::vector<unsigned> can_be_number;

    static void OutputHeaderNumber(std::ostringstream & oss, const double number, unsigned) {
        // now we have native header, and so "1" does not influence on the nature of this column
        can_be_number.push_back(1);

        std::ostringstream header_cell;
        csvkit::cli::tune_format(header_cell, "%.16g");

        header_cell << number;
        header.push_back(header_cell.str());
        oss << header.back();
        ++header_cell_index;
    }

    inline bool is_date_column(unsigned column) {
        return can_be_number[column] and std::find(dates_ids.begin(), dates_ids.end(), column) != std::end(dates_ids);
    }

    inline bool is_datetime_column(unsigned column) {
        return can_be_number[column] and std::find(datetimes_ids.begin(), datetimes_ids.end(), column) != std::end(datetimes_ids);
    }

    std::vector<std::string> generate_header(unsigned length) {
        std::vector<std::string> letter_names (length);
        unsigned i = 0;
        std::generate(letter_names.begin(), letter_names.end(), [&i] {
            return csvkit::cli::letter_name(i++);
        });

        return letter_names;
    }

    void generate_and_print_header(std::ostream & oss, auto const & args, unsigned column_count, use_date_datetime_excel use_d_dt) {
        if (args.no_header) {
            header = generate_header(column_count);
            for (auto & e : header)
                oss << (std::addressof(e) == std::addressof(header.front()) ? e : "," + e);
            oss << '\n';
            get_date_and_datetime_columns(args, header, use_d_dt);
        }
    }

}
