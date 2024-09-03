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

}
