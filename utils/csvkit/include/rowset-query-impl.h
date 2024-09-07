#pragma once

namespace csvkit::cli::sql {

    void rowset_query(soci::session & sql, auto const & args, std::string const & q_s) {
        using namespace soci;

        enum class yes_no_time {
            no = 0,
            yes
        };
        std::map<unsigned, yes_no_time> col2time;
        rowset<row> rs = (sql.prepare << q_s);

        bool print_header = false;
        for (auto && elem : rs) {
            row const &rr = elem;
            if (!print_header) {
                if (!args.no_header) {
                    if (args.linenumbers)
                        std::cout << "line_number" << ',';
                    std::cout << rr.get_properties(0).get_name();
                    for (std::size_t i = 1; i != rr.size(); ++i)
                        std::cout << ',' << rr.get_properties(i).get_name();
                    std::cout << '\n';
                }
                print_header = true;
            }

            std::tm d;
            char timeString_v1[std::size("yyyy-mm-dd hh:mm:ss")];
            char timeString_v2[std::size("yyyy-mm-dd")];

            auto print_data = [&](std::size_t i) {
                column_properties const & props = rr.get_properties(i);
                if (rr.get_indicator(i) == soci::i_null)
                    return;

                switch(props.get_db_type())
                {
                    case db_string:
                        std::cout << rr.get<std::string>(i);
                        break;
                    case db_double:
                        std::cout << rr.get<double>(i);
                        break;
                    case db_int32:
                        std::cout << rr.get<int32_t>(i);
                        break;
                    case db_date:
                        d = rr.get<std::tm>(i);
                        if (col2time[i] == yes_no_time::no and d.tm_hour == 0 and d.tm_min == 0 and d.tm_sec == 0) {
                            std::strftime(std::data(timeString_v2), std::size(timeString_v2),"%Y-%m-%d", &d);
                            std::cout << timeString_v2;
                        } else {
                            std::strftime(std::data(timeString_v1), std::size(timeString_v1),"%Y-%m-%d %H:%M:%S", &d);
                            std::cout << timeString_v1;
                            if (col2time[i] == yes_no_time::no)
                                col2time[i] = yes_no_time::yes;
                        }
                        break;
                    default:
                        break;
                }
            };

            if (args.linenumbers) {
                static auto line = 1ul;
                std::cout << line++ << ',';
            }
            print_data(0);
            for (std::size_t i = 1; i != rr.size(); ++i) {
                std::cout << ',';
                print_data(i);
            }
            std::cout << '\n';
        }

        if (!print_header) { // no data - just print table header
            if (!args.no_header) {
                row v;
                sql.once << q_s,into (v);
                if (!v.size())
                    return;  // we had supposed it was a "select", but it was another statement!
                if (args.linenumbers)
                    std::cout << "line_number" << ',';
                column_properties const &prop = v.get_properties(0);
                std::cout << prop.get_name();
                for (auto i = 1u; i < v.size(); i++) {
                    std::cout << ',';
                    column_properties const &prop = v.get_properties(i);
                    std::cout << prop.get_name();
                }
                std::cout << '\n';
            }
        }
    }
}
