//
// Created by wiluite on 21.09.2024.
//
namespace ocilib_client_ns {
    class table_creator {
    public:
        explicit table_creator (auto const & args, Connection & con) {
            if (!args.no_create) {
                std::string s = create_table_composer::table();
                Statement st(con);
                st.Execute(std::string{s.cbegin(), s.cend() - 2});
            }
        }
    };

    class table_inserter {
        template<template<class...> class F, class L> struct mp_transform_impl;
        template<template<class...> class F, class L>
        using mp_transform = typename mp_transform_impl<F, L>::type;
        template<template<class...> class F, template<class...> class L, class... T>
        struct mp_transform_impl<F, L<T...>> {
            using type = L<F<T>...>;
        };

        inline static unsigned value_index;
        void static reset_value_index() {
            value_index = 0;
        }

        using generic_bool = int;

        static inline auto fill_date_time = [](date::sys_time<std::chrono::seconds> tp, Timestamp & tstamp) {
            using namespace date;
            auto day_point = floor<days>(tp);
            year_month_day ymd = day_point;
            hh_mm_ss tod {tp - day_point};

            tstamp.SetDateTime(int(ymd.year()) - 1900, static_cast<int>(static_cast<unsigned>(ymd.month())) - 1
                , static_cast<int>(static_cast<unsigned>(ymd.day())), static_cast<int>(tod.hours().count())
                , static_cast<int>(tod.minutes().count()), static_cast<int>(tod.seconds().count()), 0);
        };
        static inline auto fill_date = [](date::sys_time<std::chrono::seconds> tp, Date & date) {
            using namespace date;
            year_month_day ymd = floor<days>(tp);
            date.SetDate(int(ymd.year()) - 1900, static_cast<int>(static_cast<unsigned>(ymd.month())) - 1
                , static_cast<int>(static_cast<unsigned>(ymd.day())));
        };
        static inline auto fill_interval = [](long double secs, Interval & interval) {
            using namespace date;
            date::sys_time<std::chrono::seconds> tp(std::chrono::seconds(static_cast<int>(secs)));
            auto day_point = floor<days>(tp);
            year_month_day ymd = day_point;
            hh_mm_ss tod {tp - day_point};
            double int_part;
            interval.SetDaySecond(static_cast<int>(static_cast<unsigned>(ymd.day())), static_cast<int>(tod.hours().count())
                , static_cast<int>(tod.minutes().count()), static_cast<int>(tod.seconds().count())
                , static_cast<int>(std::modf(static_cast<double>(secs), &int_part) * 1000000));
        };

        class simple_inserter {
            using db_types=std::variant<double, std::string, Date, Timestamp, Interval, generic_bool>;
            using db_types_ptr = mp_transform<std::add_pointer_t, db_types>;

            std::vector<db_types> data_holder;

            using ct = column_type;
            std::unordered_map<ct, db_types> type2value = {{ct::bool_t, generic_bool{}}, {ct::text_t, std::string{}}
                , {ct::number_t, double{}}, {ct::datetime_t, Timestamp{Timestamp::NoTimeZone}}, {ct::date_t, Date{}}, {ct::timedelta_t, Interval{Interval::DaySecond}}
            };

            table_inserter & parent_;
            Connection & con;
            create_table_composer & composer_;

            void insert_data(auto & reader, create_table_composer & composer, Statement & stmt) {
                using reader_type = std::decay_t<decltype(reader)>;
                using elem_type = typename reader_type::template typed_span<csv_co::unquoted>;
                using func_type = std::function<void(elem_type const&)>;

                auto col = 0u;
                std::array<func_type, static_cast<std::size_t>(column_type::sz)> fill_funcs {
                        [](elem_type const &) { assert(false && "this is unknown data type, logic error."); }
                        , [&](elem_type const & e) {
                            if (!e.is_null())
                                data_holder[col] = static_cast<generic_bool>(e.is_boolean(), e.unsafe_bool());
                            else
                                stmt.GetBind(col).SetDataNull(true);
                        }
                        , [&](elem_type const & e) {
                            if (!e.is_null())
                                data_holder[col] = static_cast<double>(e.num());
                            else
                                stmt.GetBind(col).SetDataNull(true);
                        }
                        , [&](elem_type const & e) {
                            if (!e.is_null()) {
                                Timestamp tstamp(Timestamp::NoTimeZone);
                                fill_date_time(std::get<1>(e.datetime()), tstamp);
                                data_holder[col] = tstamp;
                            } else
                                stmt.GetBind(col).SetDataNull(true);
                        }
                        , [&](elem_type const & e) {
                            if (!e.is_null()) {
                                Date date{};
                                fill_date(std::get<1>(e.date()), date);
                                data_holder[col] = date;
                            } else
                                stmt.GetBind(col).SetDataNull(true);
                        }
                        , [&](elem_type const & e) {
                            if (!e.is_null()) {
                                Interval interval{};
                                fill_interval(e.timedelta_seconds(), interval);
                                data_holder[col] = interval; 
                            } else 
                                stmt.GetBind(col).SetDataNull(true);                            
                        }
                        , [&](elem_type const & e) {
                            if (!e.is_null())
                                data_holder[col] = e.str();
                            else
                                stmt.GetBind(col).SetDataNull(true);
                        }
                };

                reader.run_rows([&] (auto & row_span) {
                    col = 0u;
                    for (auto & elem : row_span) {
                        fill_funcs[static_cast<std::size_t>(composer.types()[col])](elem_type{elem});
                        col++;
                    }
                    stmt.ExecutePrepared();
                });

            }

            void prepare_statement_object(auto const & args, Statement & st) {
                st.Prepare(insert_expr(parent_.insert_prefix_, args));
                reset_value_index();

                auto prepare_next_arg = [&](auto arg) -> db_types_ptr& {
                    static db_types_ptr each_next;
                    data_holder[value_index] = type2value[arg];
                    std::visit([&](auto & arg) {
                        each_next = &arg;
                    }, data_holder[value_index]);
                    return each_next;
                };

                for(auto e : composer_.types()) {
                    std::visit([&](auto & arg) {
                        std::string name = ":v" + std::to_string(value_index);
                        if constexpr(std::is_same_v<std::decay_t<decltype(*arg)>, generic_bool>) {
                            st.Bind(name, *arg, BindInfo::In);
                        } 
                        else if constexpr(std::is_same_v<std::decay_t<decltype(*arg)>, std::string>) {
                            st.Bind(name, *arg, 200, BindInfo::In);
                        }  
                        else if constexpr(std::is_same_v<std::decay_t<decltype(*arg)>, double>) {
                            st.Bind(name, *arg, BindInfo::In);
                        }
                        else if constexpr(std::is_same_v<std::decay_t<decltype(*arg)>, Timestamp>) {
                            st.Bind(name, *arg, BindInfo::In);
                        }
                        else if constexpr(std::is_same_v<std::decay_t<decltype(*arg)>, Date>) {
                            st.Bind(name, *arg, BindInfo::In);
                        } else {
                            static_assert(std::is_same_v<std::decay_t<decltype(*arg)>, Interval>); 
                            st.Bind(name, *arg, BindInfo::In);
                        } 
                    }, prepare_next_arg(e));
                    value_index++;
                }
            }

        public:
            simple_inserter(table_inserter & parent, Connection & con, create_table_composer & composer) : parent_(parent), con(con), composer_(composer) {}
            void insert(auto const &args, auto & reader) {
                data_holder.resize(composer_.types().size());
                Statement stmt(con);
                prepare_statement_object(args, stmt);
                insert_data(reader, composer_, stmt);
            }

        };

        Connection & con;
        create_table_composer & composer_;
        std::string const & after_insert_;
        std::string const & insert_prefix_;

    public:
        table_inserter(auto const &args, Connection & con, create_table_composer & composer)
                : con(con), composer_(composer), after_insert_(args.after_insert), insert_prefix_(args.prefix) {

        }
        void insert(auto const &args, auto & reader) {
            if (args.chunk_size <= 1)
                simple_inserter(*this, con, composer_).insert(args, reader);
            else {
                //batch_bulk_inserter(*this, con, composer_).insert(args, reader);
            }
        }

    };

}
