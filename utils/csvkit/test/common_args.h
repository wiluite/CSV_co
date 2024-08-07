///
/// \file   test/common_args.h
/// \author wiluite
/// \brief  Arguments fake classes useful for most small csv kit's tests.

#pragma once

namespace csvkit::test_facilities {
    struct single_file_arg {
        std::filesystem::path file;
    };
    struct common_args {
        bool no_header {false};
        bool zero {false};
        mutable bool linenumbers {false};
        unsigned skip_lines {0};
        mutable unsigned maxfieldsize = 1000;
        bool skip_init_space {false};
        std::string encoding {"UTF-8"};
    };
    struct type_aware_args {
        mutable std::vector<std::string> null_value {};
        std::string num_locale {"C"};
        bool blanks {false};
        std::string date_fmt {R"(%m/%d/%Y)"};
        std::string datetime_fmt {R"(%m/%d/%Y %I:%M %p)"};
        bool no_inference {false};
        std::string decimal_format {"%.3f"};
        bool no_grouping_sep {false};
        bool date_lib_parser {true};
    };
    struct csvlook_specific_args {
        mutable unsigned max_columns {max_unsigned_limit};
        unsigned max_column_width {max_unsigned_limit};
        unsigned long max_rows {max_size_t_limit};
        unsigned max_precision {3u};
        bool no_number_ellipsis {false};
        std::string glob_locale {"C"};
        mutable unsigned precision_locally{};
    };
    struct spread_args {
        bool names {false}; // Display column names and indices from the input CSV and exit.
        mutable std::string columns {"all columns"};
        mutable std::string not_columns {"no columns"};
    };
    struct csvstat_specific_args {
        bool csv {false};
        bool json {false};
        int indent {min_int_limit};
        bool type {false};
        bool nulls {false};
        bool non_nulls {false};
        bool unique {false};
        bool min {false};
        bool max {false};
        bool sum {false};
        bool mean {false};
        bool median {false};
        bool stdev {false};
        bool len {false};
        bool max_precision {false};
        bool freq {false};
        unsigned long freq_count {5ul};
        bool count {false};
        bool no_mdp {false};
    };
    struct csvsort_specific_args {
        bool r {false};
    };
    struct csvjoin_specific_args {
        std::vector<std::string> files;
        bool right_join {false};
        bool left_join {false};
        bool outer_join {false};
    };
    struct csvstack_specific_args {
        std::vector<std::string> files;
        std::string groups;
        std::string group_name;
        bool filenames {false};
    };
    struct csvjson_specific_args {
        int indent {min_int_limit};
        std::string key;
        std::string lat;
        std::string lon;
        std::string type;
        std::string geometry;
        std::string crs;
        bool no_bbox {false};
        bool stream {false};
    };
    struct csvgrep_specific_args {
        std::string match;
        std::string regex;
        bool r_icase {false};
        std::string f;
        bool invert {false};
        bool any {};
    };

}
