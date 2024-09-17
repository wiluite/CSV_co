#pragma once
#include "dbms-backend-id.h"

namespace csvkit::cli::sql {

    void rowset_query(soci::session & sql, auto const & args, std::string const & q_s) {
        using namespace soci;

        enum class if_time {
            no = 0,
            yes
        };

        bool printable_can_be_value = false;
        bool printable_can_be_weird = false;
        SOCI::backend_id backend_id {SOCI::backend_id::ANOTHER};
        if (sql.get_backend_name() == "postgresql") {    //timedelta is string
            printable_can_be_value = true;
            backend_id = SOCI::backend_id::PG;
        }
        else if (sql.get_backend_name() == "oracle") {   //datetime and timedelta are strings
            backend_id = SOCI::backend_id::ORCL;
            printable_can_be_value = true;
        }
        else if (sql.get_backend_name() == "firebird") { //datetime and timedelta are weird std::tm
            backend_id = SOCI::backend_id::FB;
            printable_can_be_weird = true;
        }


        std::map<unsigned, if_time> col2time;
        rowset<row> rs = (sql.prepare << q_s);

        std::tm d{};
        char timeString_v2[std::size("yyyy-mm-dd")];
        char timeString_v11[std::size("yyyy-mm-dd hh:mm:ss.uuuuuu")];
        bool print_header = false;
        for (auto && elem : rs) {
            row const &rr = elem;
            if (!rr.size())
                return; // we had supposed it was a "select", but it was another statement!

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

            auto print_data = [&](std::size_t i) {
                column_properties const & props = rr.get_properties(i);
                if (rr.get_indicator(i) == soci::i_null) {
                    if (rr.size() == 1)
                        std::cout << R"("")";
                    return;
                }

                num_stringstream ss("C");

                switch(props.get_db_type())
                {
                    case db_string:
                        if (backend_id == SOCI::backend_id::ORCL) {
                            int days;
                            int hours;
                            int mins;
                            int secs;
                            int microsecs;
                            auto cnt = sscanf(rr.get<std::string>(i).c_str(), "+%d %02d:%02d:%02d.%06d", &days, &hours, &mins, &secs, &microsecs);
                            if (cnt == 5) {
                                char buf [64];
                                if (days)
                                    snprintf(buf, 64, "\"%d %s, %02d:%02d:%02d.%06d\"", days, (days > 1 ? "days" : "day"), hours, mins, secs, microsecs);
                                else
                                    snprintf(buf, 64, "%02d:%02d:%02d.%06d", hours, mins, secs, microsecs);
                                std::cout << buf;
                            } else {
                                std::cout << rr.get<std::string>(i);
                            }
                        } else
                            std::cout << rr.get<std::string>(i);
                        break;
                    case db_double:
                        ss << csv_mcv_prec(rr.get<double>(i));
                        std::cout << ss.str();
                        break;
                    case db_int32:
                        std::cout << rr.get<int32_t>(i);
                        break;
                    case db_date:
                        d = rr.get<std::tm>(i);
                        if (col2time[i] == if_time::no and d.tm_hour == 0 and d.tm_min == 0 and d.tm_sec == 0) {
                            std::strftime(std::data(timeString_v2), std::size(timeString_v2),"%Y-%m-%d", &d);
                            std::cout << timeString_v2;
                        } else {
                            if (printable_can_be_weird)
                                d.tm_isdst = 0;
                            snprintf(timeString_v11, std::size(timeString_v11), "%d-%02d-%02d %02d:%02d:%02d.%06d",
                                d.tm_year + 1900, d.tm_mon + 1, d.tm_mday, d.tm_hour, d.tm_min, d.tm_sec, d.tm_isdst);
                            std::cout << timeString_v11;

                            if (col2time[i] == if_time::no)
                                col2time[i] = if_time::yes;
                        }
                        break;
                    case db_int64:
                        std::cout << rr.get<int64_t>(i); // oracle non-doubles (ints)
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
                    column_properties const &p = v.get_properties(i);
                    std::cout << p.get_name();
                }
                std::cout << '\n';
            }
        }
    }
}
