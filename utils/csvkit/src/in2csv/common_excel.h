#pragma once

namespace {
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

