#include "../../include/in2csv/in2csv_csv.h"
#include <cli.h>
#include <iostream>
#include "common_functions.h"

using namespace ::csvkit::cli;

namespace in2csv::detail::csv {
    namespace detail {
        inline bool is_date_column(unsigned column) {
            return std::find(dates_ids.begin(), dates_ids.end(), column) != std::end(dates_ids);
        }

        inline bool is_datetime_column(unsigned column) {
            return std::find(datetimes_ids.begin(), datetimes_ids.end(), column) != std::end(datetimes_ids);
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
                            } else {
                                auto c = std::any_cast<std::size_t const>(info); 
                                if (is_date_column(c)) {
                                    using date::operator<<;
                                    std::ostringstream local_oss;
                                    local_oss << to_chrono_time_point(value);
                                    auto str = local_oss.str();
                                    ss << std::string{str.begin(), str.begin() + 10};
                                } else
                                if (is_datetime_column(c)) {
                                    using date::operator<<;
                                    std::ostringstream local_oss;
                                    ss << to_chrono_time_point(value);
                                } else
                                ss << another_rep.str();
                            }
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
            os << type2func[type_index](elem, col);
        }

        void convert_impl(auto & reader, impl_args & args) {
            skip_lines(reader, args);
            quick_check(reader, args);

            auto types_and_blanks = std::get<1>(typify(reader, args, typify_option::typify_without_precisions));

            skip_lines(reader, args);
            auto const header = obtain_header_and_<skip_header>(reader, args);

            std::vector<std::string> string_header(header.size());
            std::transform(header.cbegin(), header.cend(), string_header.begin(), [&](auto & elem) {
                return optional_quote(elem);
            });

            get_date_and_datetime_columns(args, header, use_date_datetime_xls::yes);

            std::ostream & os = std::cout;

            auto write_header = [&string_header, &args] {
                if (args.linenumbers)
                    os << "line_number,";

                std::for_each(string_header.begin(), string_header.end() - 1, [&](auto const & elem) {
                    os << elem << ',';
                });
                os << string_header.back() << '\n';
            };

            write_header();

            std::size_t line_nums = 0;
            reader.run_rows(
                    [&](auto rowspan) {
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
    }
    void impl::convert() {
        try {
            impl_args args = a;
            basic_reader_configurator_and_runner(read_standard_input, detail::convert_impl)
        }  catch (ColumnIdentifierError const& e) {
            std::cout << e.what() << '\n';
        }
    }
}
