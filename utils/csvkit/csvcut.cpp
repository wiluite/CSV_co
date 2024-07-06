///
/// \file   utils/csvkit/csvcut.cpp
/// \author wiluite
/// \brief  Filter and truncate a CSV source.

#include <cli.h>
#include <type_traits>
#include <iostream>
#include <deque>

using namespace ::csvkit::cli;

namespace csvcut {
    struct Args : ARGS_positional_1 {

        bool & names = flag ("n,names","Display column names and indices from the input CSV and exit.");
        std::string & columns = kwarg("c,columns","A comma-separated list of column indices, names or ranges to be extracted, e.g. \"1,id,3-5\".").set_default("all columns");
        std::string & not_columns = kwarg("C,not-columns","A comma-separated list of column indices, names or ranges to be excluded, e.g. \"1,id,3-5\".").set_default("no columns");
        bool & x_ = flag("x,delete-empty-rows", "After cutting delete rows which are completely empty.");

        void welcome() final {
            std::cout << "\nFilter and truncate CSV files. Like the Unix \"cut\" command, but for tabular data.\n\n";
        }
    };

    void cut(std::monostate &, auto const &) {}

    void cut(auto & reader_reference, auto const & args) {
        using namespace csv_co;

        throw_if_names_and_no_header(args);
        auto & reader = reader_reference.get();
        skip_lines(reader, args);
        quick_check(reader, args);
        auto const header = obtain_header_and_<no_skip_header>(reader, args);

        if (args.names) {
            check_max_size(reader, args, header_to_strings<unquoted>(header), init_row{1});
            print_header(std::cout, header, args);
            return;
        }

        args.columns = args.columns == "all columns" ? "" : args.columns;
        args.not_columns = args.not_columns == "no columns" ? "" : args.not_columns;

        static char delim = std::decay_t<decltype(reader)>::delimiter_type::value;

        try {
            auto ids = parse_column_identifiers(columns{args.columns}, header, get_column_offset(args), excludes(args.not_columns));
            std::ostringstream oss;
            auto print_container = [&] (auto const & deq, auto const & args) {
                if (args.linenumbers) {
                    static auto line = 0ul;
                    if (line)
                        oss << line << delim;
                    line++;
                }
                std::deque<std::string>::difference_type const sz = deq.size() - 1;
                std::for_each(deq.cbegin(), deq.cbegin() + sz, [&] (auto & e) {
                    oss << e << delim;
                });
                oss << deq.back() << '\n';
            };
           
            struct say_ln {
                explicit say_ln(std::decay_t<decltype(args)> const & args, std::ostringstream & oss) {
                    if (args.linenumbers)
                        oss << "line_number" << delim;
                }
            };           
            
            if (args.no_header) {
                check_max_size(reader, args, header_to_strings<unquoted>(header), init_row{1});
                std::deque<std::string> deq;

                for (auto e : ids)
                    deq.emplace_back(optional_quote(header[e]));

                static say_ln ln (args, oss);
                print_container(deq, args);
            }

            reader.run_rows([&] (auto & row_span) {

                if (!args.no_header)
                    static say_ln ln (args, oss);

                check_max_size<establish_new_checker>(reader, args, header_to_strings<quoted>(row_span), init_row{1});
                static std::deque<std::string> deq;

                bool empty = true;
                for (auto e : ids) {
                    auto & el = row_span[e];
                    deq.emplace_back(optional_quote(el));
                    empty = empty && args.x_ && el.operator unquoted_cell_string().empty();
                }

                if ((args.x_ && !empty) || !args.x_)
                    print_container(deq, args);

                deq.clear();
            });

            std::cout << oss.str(); 

        } catch (ColumnIdentifierError const& e) {
            std::cout << e.what() << '\n';
        }
    }

} ///namespace 

#if !defined(BOOST_UT_DISABLE_MODULE)

int main(int argc, char * argv[])
{
    using namespace csvcut;

    auto const args = argparse::parse<Args>(argc, argv);
    if (args.verbose)
        args.print();

    OutputCodePage65001 ocp65001;
    basic_reader_configurator_and_runner(read_standard_input, cut)

    return 0;
}

#endif
