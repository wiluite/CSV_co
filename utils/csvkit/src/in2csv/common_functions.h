#pragma once

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

    enum use_date_datetime_xls {
        yes,
        no
    };

    static std::vector<unsigned> dates_ids;
    static std::vector<unsigned> datetimes_ids;

    auto get_date_and_datetime_columns(auto && args, auto && header, use_date_datetime_xls use_d_dt_xls) {
        using namespace ::csvkit::cli;
        if (use_d_dt_xls == use_date_datetime_xls::no)
            return;

        if (args.d_xls != "none") {
            std::string not_columns;
            dates_ids = parse_column_identifiers(columns{args.d_xls}, header, get_column_offset(args), excludes{not_columns});
        }

        if (args.dt_xls != "none") {
            std::string not_columns;
            datetimes_ids = parse_column_identifiers(columns{args.dt_xls}, header, get_column_offset(args), excludes{not_columns});
        }
    };
}
