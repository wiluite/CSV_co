//
// Created by wiluite on 7/30/24.
//
#include <soci/soci.h>
#if defined (SOCI_HAVE_SQLITE3)
#include <soci/sqlite3/soci-sqlite3.h>
#endif

#if defined (SOCI_HAVE_POSTGRESQL)
#include <soci/postgresql/soci-postgresql.h>
#endif

#if defined(SOCI_HAVE_MYSQL)
#include <soci/mysql/soci-mysql.h>
#endif

#if defined(SOCI_HAVE_FIREBIRD)
#include <soci/firebird/soci-firebird.h>
#endif

#include <iostream>
#include <cli.h>
#include <rowset-query-impl.h>
#include <iosfwd>
#include <vector>
#include <unordered_map>

// TODO:
//  2. get rid of blanks calculations in typify() for csvsql  (--no-constraints mode)

using namespace ::csvkit::cli;

namespace csvsql::detail {
    struct Args : ARGS_positional_files {
        std::string & num_locale = kwarg("L,locale","Specify the locale (\"C\") of any formatted numbers.").set_default(std::string("C"));
        bool &blanks = flag("blanks", R"(Do not convert "", "na", "n/a", "none", "null", "." to NULL.)");
        std::vector<std::string> &null_value = kwarg("null-value","Convert these values to NULL.").multi_argument().set_default(std::vector<std::string>{});
        std::string & date_fmt = kwarg("date-format","Specify an strptime date format string like \"%m/%d/%Y\".").set_default(R"(%m/%d/%Y)");
        std::string & datetime_fmt = kwarg("datetime-format","Specify an strptime datetime format string like \"%m/%d/%Y %I:%M %p\".").set_default(R"(%m/%d/%Y %I:%M %p)");

        std::string & dialect = kwarg("i,dialect","Dialect of SQL {mysql,postgresql,sqlite,firebird} to generate. Cannot be used with --db or --query. ").set_default(std::string(""));
        std::string & db = kwarg("db","If present, a 'soci' connection string to use to directly execute generated SQL on a database.").set_default(std::string(""));
        std::string & query = kwarg("query","Execute one or more SQL queries delimited by \";\" and output the result of the last query as CSV. QUERY may be a filename. --query may be specified multiple times.").set_default(std::string(""));
        bool &insert = flag("insert", "Insert the data into the table. Requires --db.");
        std::string & prefix = kwarg("prefix","Add an expression following the INSERT keyword, like OR IGNORE or OR REPLACE.").set_default(std::string(""));
        std::string & before_insert = kwarg("before-insert","Execute SQL before the INSERT command. Requires --insert.").set_default(std::string(""));
        std::string & after_insert = kwarg("after-insert","Execute SQL after the INSERT command. Requires --insert.").set_default(std::string(""));
        std::string & tables = kwarg("tables","A comma-separated list of names of tables to be created. By default, the tables will be named after the filenames without extensions or \"stdin\".").set_default(std::string(""));
        bool &no_constraints = flag("no-constraints", "Generate a schema without length limits or null checks. Useful when sampling big tables.");
        std::string & unique_constraint = kwarg("unique-constraint","A comma-separated list of names of columns to include in a UNIQUE constraint").set_default(std::string(""));

        bool &no_create = flag("no-create", "Skip creating the table. Requires --insert.");
        bool &create_if_not_exists = flag("create-if-not-exists", "Create the table if it does not exist, otherwise keep going. Requires --insert.");
        bool &overwrite = flag("overwrite", "Drop the table if it already exists. Requires --insert. Cannot be used with --no-create.");
        std::string & db_schema = kwarg("db-schema","Optional name of database schema to create table(s) in.").set_default(std::string(""));
        bool &no_inference = flag("I,no-inference", "Disable type inference when parsing the input.");
        unsigned & chunk_size = kwarg("chunk-size","Chunk size for batch insert into the table. Requires --insert.").set_default(0);
        bool &date_lib_parser = flag("date-lib-parser", "Use date library as Dates and DateTimes parser backend instead compiler-supported").set_default(true);

        void welcome() final {
            std::cout << "\nGenerate SQL statements for one or more CSV files, or execute those statements directly on a database, and execute one or more SQL queries.\n\n";
        }
    };

    auto sql_split = [](std::stringstream && strm, char by = ';') {
        std::vector<std::string> result;
        for (std::string phrase; std::getline(strm, phrase, by);)
            result.push_back(phrase);
        return result;
    };

    template <typename ... r_types>
    struct readers_manager {
        auto & get_readers() {
            return readers;
        }
        template<typename ReaderType>
        auto set_readers(auto & args) {
            if (args.files.empty())
                args.files = std::vector<std::string>{"_"};
            for (auto & elem : args.files) {
                auto reader {elem != "_" ? ReaderType{std::filesystem::path{elem}} : (elem = "stdin", ReaderType{read_standard_input(args)})};
                recode_source(reader, args);
                skip_lines(reader, args);
                quick_check(reader, args);
                get_readers().push_back(std::move(reader));
            }
        }
    private:
        std::deque<r_types...> readers;
    };

    auto create_table_phrase(auto const & args) {
        return !args.create_if_not_exists ? std::string_view("CREATE TABLE ") : std::string_view("CREATE TABLE IF NOT EXISTS ");
    }

    class create_table_composer {
        friend void reset_environment();
        static inline std::size_t file_no;
        class print_director {
            static inline std::stringstream stream;
            struct bool_column_tag {};
            struct number_column_tag {};
            struct datetime_column_tag {};
            struct date_column_tag {};
            struct timedelta_column_tag {};
            struct text_column_tag {};

            class printer {
            public:
                virtual void print_name(std::string const & name) = 0;
                virtual void print(bool_column_tag) = 0;
                virtual void print(bool_column_tag, bool blanks, unsigned prec) = 0;
                virtual void print(number_column_tag) = 0;
                virtual void print(number_column_tag, bool blanks, unsigned prec) = 0;
                virtual void print(datetime_column_tag) = 0;
                virtual void print(datetime_column_tag, bool blanks, unsigned prec) = 0;
                virtual void print(date_column_tag) = 0;
                virtual void print(date_column_tag, bool blanks, unsigned prec) = 0;
                virtual void print(timedelta_column_tag) = 0;
                virtual void print(timedelta_column_tag, bool blanks, unsigned prec) = 0;
                virtual void print(text_column_tag) = 0;
                virtual void print(text_column_tag, bool blanks, unsigned prec) = 0;
                virtual ~printer() = default;
            protected:
                static void print_name_or_quoted_name(std::string const & name, char qch) {
                    if (std::all_of(name.cbegin(), name.cend(), [&](auto & ch) {
                        return ((std::isalpha(ch) and std::islower(ch))
                                or  (std::isdigit(ch) and std::addressof(ch) != std::addressof(name.front()))
                                or  ch == '_');
                    }))
                        stream << name;
                    else
                        stream << std::quoted(name, qch, qch);
                    stream << " ";
                }
            }; // printer

            struct varchar_precision {};
            struct generic_printer : printer {
                void print_name(std::string const & name) override { print_name_or_quoted_name(name, '"'); }
                void print(bool_column_tag) override { to_stream(stream, "BOOLEAN"); }
                void print(bool_column_tag, bool blanks, unsigned) override { to_stream(stream, "BOOLEAN", (blanks ? "" : " NOT NULL")); }
                void print(number_column_tag) override { to_stream(stream, "DECIMAL"); }
                void print(number_column_tag, bool blanks, unsigned) override { to_stream(stream, "DECIMAL", (blanks ? "" : " NOT NULL")); }
                void print(datetime_column_tag) override { to_stream(stream, "TIMESTAMP"); }
                void print(datetime_column_tag, bool blanks, unsigned) override { to_stream(stream, "TIMESTAMP"); }
                void print(date_column_tag) override { to_stream(stream, "DATE"); }
                void print(date_column_tag, bool blanks, unsigned) override { to_stream(stream, "DATE", (blanks ? "" : " NOT NULL")); }
                void print(timedelta_column_tag) override { to_stream(stream, "DATETIME"); }
                void print(timedelta_column_tag, bool blanks, unsigned) override { to_stream(stream, "DATETIME", (blanks ? "" : " NOT NULL")); }
                void print(text_column_tag) override { to_stream(stream, "VARCHAR"); }
                void print(text_column_tag, bool blanks, unsigned) override { to_stream(stream, "VARCHAR", (blanks ? "" : " NOT NULL")); }
            };

            struct mysql_printer : generic_printer, varchar_precision {
                void print_name(std::string const & name) override {
                    print_name_or_quoted_name(name, '`');
                }
                void print(bool_column_tag) override { to_stream(stream, "BOOL"); }
                void print(bool_column_tag, bool blanks, unsigned) override { to_stream(stream, "BOOL", (blanks ? "" : " NOT NULL")); }
                void print(number_column_tag, bool blanks, unsigned prec) override { to_stream(stream, "DECIMAL(38, ", static_cast<int>(prec), ')', (blanks ? "" : " NOT NULL")); }
                void print(text_column_tag) override { throw std::runtime_error("VARCHAR requires a length on dialect mysql"); }
                void print(text_column_tag, bool blanks, unsigned prec) override { to_stream(stream, "VARCHAR(", static_cast<int>(prec), ')', (blanks ? "" : " NOT NULL")); }
            };

            struct postgresql_printer : generic_printer {
                void print(datetime_column_tag) override { to_stream(stream, "TIMESTAMP WITHOUT TIME ZONE"); }
                void print(datetime_column_tag, bool blanks, unsigned) override { to_stream(stream, "TIMESTAMP WITHOUT TIME ZONE"); }
                void print(timedelta_column_tag) override { to_stream(stream, "INTERVAL"); }
                void print(timedelta_column_tag, bool blanks, unsigned) override { to_stream(stream, "INTERVAL", (blanks ? "" : " NOT NULL")); }
            };

            struct sqlite_printer : generic_printer {
                void print(number_column_tag) override { to_stream(stream, "FLOAT"); }
                void print(number_column_tag, bool blanks, unsigned) override { to_stream(stream, "FLOAT", (blanks ? "" : " NOT NULL")); }
            };

            struct firebird_printer : generic_printer {
                void print(timedelta_column_tag) override { to_stream(stream, "TIMESTAMP");}
                void print(timedelta_column_tag, bool blanks, unsigned) override { to_stream(stream, "TIMESTAMP", (blanks ? "" : " NOT NULL"));}
                void print(text_column_tag) override { throw std::runtime_error("VARCHAR requires a length on dialect firebird");}
                void print(text_column_tag, bool blanks, unsigned) override { throw std::runtime_error("VARCHAR requires a length on dialect firebird");}
            };

            using printer_dialect = std::string;
            static inline std::unordered_map<printer_dialect, std::shared_ptr<printer>> printer_map {
                  {"mysql", std::make_shared<mysql_printer>()}
                , {"postgresql", std::make_shared<postgresql_printer>()}
                , {"sqlite", std::make_shared<sqlite_printer>()}
                , {"firebird", std::make_shared<firebird_printer>()}
                , {"generic", std::make_shared<generic_printer>()}
            };

            static inline std::unordered_map<column_type
                    , std::variant<bool_column_tag, number_column_tag, datetime_column_tag, date_column_tag
                    , timedelta_column_tag, text_column_tag>> tag_map {
                  {column_type::bool_t, bool_column_tag{}}
                , {column_type::bool_t, bool_column_tag()}
                , {column_type::number_t, number_column_tag()}
                , {column_type::datetime_t, datetime_column_tag()}
                , {column_type::date_t, date_column_tag()}
                , {column_type::timedelta_t, timedelta_column_tag()}
                , {column_type::text_t, text_column_tag()}
            };

        public:
            void direct(auto & reader, typify_with_precisions_result const & value, std::string const & dialect, auto const & header) {
                auto [types, blanks, precisions] = value;
                auto index = 0ull;

                // re-fill precisions with varchar lengths if needed

                auto * has = dynamic_cast<varchar_precision*>(printer_map[dialect].get());
                if (has) {
                    using reader_type = std::decay_t<decltype(reader)>;
                    using elem_type = typename reader_type::template typed_span<csv_co::unquoted>;
                    reader.run_rows([&](auto & row_span) {
                        auto col = 0ull;
                        for (auto & elem : row_span) {
                            if (types[col] == column_type::text_t) {
                                auto candidate_size = elem_type{elem}.str_size_in_symbols();
                                if (candidate_size > precisions[col])
                                    precisions[col] = candidate_size;
                            }
                            col++;
                        }
                    });
                }
                for (auto e : types) {
                    printer_map[dialect]->print_name(header[index].operator csv_co::cell_string());
                    std::visit([&](auto & arg) {
                        printer_map[dialect]->print(arg, blanks[index], precisions[index]);
                    }, tag_map[e]);
                    ++index;
                    stream << (index != types.size() ? ",\n\t" :(unique_constraint.empty() ? "\n" : (",\n\tUNIQUE (" + unique_constraint + ")\n")));
                }
            }

            void direct(auto&, typify_without_precisions_result const & value, std::string const & dialect, auto const & header) {
                auto [types, blanks] = value;
                auto index = 0ull;
                for (auto e : types) {
                    printer_map[dialect]->print_name(header[index].operator csv_co::cell_string());
                    std::visit([&](auto & arg) {
                        printer_map[dialect]->print(arg);
                    }, tag_map[e]);
                    ++index;
                    stream << (index != types.size() ? ",\n\t" : (unique_constraint.empty() ? "\n" : (",\n\tUNIQUE (" + unique_constraint + ")\n")));
                }
            }
            static std::string table() {
                return stream.str();
            }

            struct open_close {
                open_close(auto const & args, std::vector<std::string> const & table_names) {
                    stream.str({});
                    stream.clear();
                    stream << create_table_phrase(args);
                    if (table_names.size() > file_no) {
                        auto tn = table_names[file_no];
                        csv_co::string_functions::unquote(tn, '"');
                        stream << tn;
                    }
                    else {
                        auto tn = std::filesystem::path{args.files[file_no]}.stem().string();
                        csv_co::string_functions::unquote(tn, '"');
                        stream << tn;
                    }
                    stream << " (\n\t";
                }
                ~open_close() {
                    stream << ");\n";
                }
            } wrapper_;
            std::string const & unique_constraint;
            print_director(auto const & args, std::vector<std::string> const & table_names)
                : wrapper_(args, table_names),  unique_constraint(args.unique_constraint) {}
        };

    private:
        [[nodiscard]] std::string dialect(auto const & args) const {
            if (args.db.empty() and args.dialect.empty())
                 return "generic";
            if (!args.dialect.empty())
                 return args.dialect;
            std::vector<std::string> dialects {"mysql", "postgresql", "sqlite", "firebird"};
            for (auto elem : dialects) {
                if (args.db.find(elem) != std::string::npos)
                    return elem;
            }
            return "generic";
        }
        std::vector<std::string> header_;
        std::vector<column_type> types_;
        std::vector<unsigned char> blanks_;
    public:
        create_table_composer(auto & reader, auto const & args, std::vector<std::string> const & table_names) {

            auto const info = typify(reader, args, !args.no_constraints ? typify_option::typify_with_precisions : typify_option::typify_without_precisions);
            skip_lines(reader, args);
            auto const header = obtain_header_and_<skip_header>(reader, args);
            header_ = header_to_strings<csv_co::unquoted>(header);

            print_director print_director_(args, table_names);
            std::visit([&](auto & arg) {
                types_ = std::get<0>(arg);
                blanks_ = std::get<1>(arg);
                print_director_.direct(reader, arg, dialect(args), header);
            }, info);

            ++file_no;
        }

        [[nodiscard]] static std::string table() {
            return print_director::table();
        }

        [[nodiscard]] auto const & header() const { return header_; }
        [[nodiscard]] auto const & types() const { return types_; }
        [[nodiscard]] auto const & blanks() const { return blanks_; }
    };

    class table_creator {
    public:
        explicit table_creator (auto const & args, soci::session & sql) {
            if (!args.no_create)
                sql << create_table_composer::table();
        }
    };

    class table_inserter {

        [[nodiscard]] std::string insert_expr(auto const & args) const {
            constexpr const std::array<std::string_view, 1> sql_keywords {{"UNIQUE"}};
            auto const t = create_table_composer::table();
            std::string expr = "insert" + (insert_prefix_.empty() ? "": " " + insert_prefix_) + " into ";
            auto const expr_prim_size = create_table_phrase(args).size();

            auto pos = t.find('(', expr_prim_size);
            expr += std::string(t.begin() + expr_prim_size, t.begin() + static_cast<std::ptrdiff_t>(pos) + 1);
            std::string values;
            unsigned counter = 0;
            while (pos = t.find('\t', pos + 1), pos != std::string::npos) {
                using diff_t = std::ptrdiff_t;
                std::string next = {t.begin() + static_cast<diff_t>(pos) + 1, t.begin() + static_cast<diff_t>(t.find(' ', pos + 1))};
                bool keyword = false;
                for (auto e : sql_keywords) {
                    if (e == next) {
                        keyword = true;
                        break;
                    }
                }
                if (keyword)
                    continue;
                expr += next + ",";
                values += ":v" + std::to_string(counter++) + ',';
            }
            expr.back() = ')';
            expr += " values (" + values;
            expr.back() = ')';
            return expr;
        }

        template<template<class...> class F, class L> struct mp_transform_impl;
        template<template<class...> class F, class L>
        using mp_transform = typename mp_transform_impl<F, L>::type;
        template<template<class...> class F, template<class...> class L, class... T>
        struct mp_transform_impl<F, L<T...>> {
            using type = L<F<T>...>;
        };

        using generic_bool = int32_t;
        using db_types=std::variant<double, std::string, std::tm, generic_bool>;
        using db_types_ptr = mp_transform<std::add_pointer_t, db_types>;

        std::vector<db_types> data_holder;
        std::vector<soci::indicator> indicators;
        using ct = column_type;
        std::map<ct, db_types> type2value = {{ct::bool_t, generic_bool{}}, {ct::text_t, std::string{}}, {ct::number_t, double{}}
            , {ct::datetime_t, std::tm{}}, {ct::date_t, std::tm{}}, {ct::timedelta_t, std::tm{}}
        };

        inline static unsigned value_index;
        void static reset_value_index() {
            value_index = 0;
        }

        db_types_ptr& prepare_next_arg(auto arg) {
            static db_types_ptr each_next;
            data_holder[value_index] = type2value[arg];
            std::visit([&](auto & arg) {
                each_next = &arg;
            }, data_holder[value_index]);
            return each_next;
        };

        void insert_data(auto & reader, create_table_composer & composer, soci::statement & stmt) {
            using reader_type = std::decay_t<decltype(reader)>;
            using elem_type = typename reader_type::template typed_span<csv_co::unquoted>;
            using func_type = std::function<void(elem_type const&)>;

            auto col = 0u;
            std::array<func_type, static_cast<std::size_t>(column_type::sz)> fill_funcs {
                [](elem_type const &) { assert(false && "this is unknown data type, logic error."); }
                , [&](elem_type const & e) {
                    if (!e.is_null()) {
                        data_holder[col] = static_cast<generic_bool>(e.is_boolean(), e.unsafe_bool());
                        indicators[col] = soci::i_ok;
                    } else {
                        data_holder[col] = static_cast<generic_bool>(false);
                        indicators[col] = soci::i_null;
                    }
                }
                , [&](elem_type const & e) {
                    if (!e.is_null()) {
                        data_holder[col] = static_cast<double>(e.num());
                        indicators[col] = soci::i_ok;
                    } else {
                        data_holder[col] = static_cast<double>(0);
                        indicators[col] = soci::i_null;
                    }
                }
                , [&](elem_type const & e) {
                    if (!e.is_null()) {
                        using namespace date;

                        date::sys_time<std::chrono::seconds> tp = std::get<1>(e.datetime());
                        auto daypoint = floor<days>(tp);
                        year_month_day ymd = daypoint;
                        std::tm tm_{};
                        tm_.tm_year = int(ymd.year()) - 1900;
                        tm_.tm_mon = unsigned(ymd.month()) - 1;
                        tm_.tm_mday = unsigned(ymd.day());
                        hh_mm_ss tod{tp - daypoint};
                        tm_.tm_hour = tod.hours().count();
                        tm_.tm_min = tod.minutes().count();
                        tm_.tm_sec = tod.seconds().count();
                        data_holder[col] = tm_;
                        indicators[col] = soci::i_ok;
                    } else {
                        data_holder[col] = std::tm{};
                        indicators[col] = soci::i_null;
                    }
                }
                , [&](elem_type const & e) {
                    if (!e.is_null()) {
                        using namespace date;

                        date::sys_time<std::chrono::seconds> tp = std::get<1>(e.date());
                        auto daypoint = floor<days>(tp);
                        year_month_day ymd = daypoint;
                        std::tm tm_{};
                        tm_.tm_year = int(ymd.year()) - 1900;
                        tm_.tm_mon = unsigned(ymd.month()) - 1;
                        tm_.tm_mday = unsigned(ymd.day());
                        tm_.tm_hour = 0;
                        tm_.tm_min = 0;
                        tm_.tm_sec = 0;
                        data_holder[col] = tm_;
                        indicators[col] = soci::i_ok;
                    } else {
                        data_holder[col] = std::tm{};
                        indicators[col] = soci::i_null;
                    }
                }
                , [&](elem_type const & e) {
                    if (!e.is_null()) {
                        using namespace date;

                        long double secs = e.timedelta_seconds();
                        date::sys_time<std::chrono::seconds> tp(std::chrono::seconds(static_cast<int>(secs)));
                        auto daypoint = floor<days>(tp);
                        year_month_day ymd = daypoint;
                        std::tm tm_{};
                        tm_.tm_year = int(ymd.year()) - 1900;
                        tm_.tm_mon = unsigned(ymd.month()) - 1;
                        tm_.tm_mday = unsigned(ymd.day());
                        hh_mm_ss tod{tp - daypoint};
                        tm_.tm_hour = tod.hours().count();
                        tm_.tm_min = tod.minutes().count();
                        tm_.tm_sec = tod.seconds().count();
                        double int_part;
                        tm_.tm_isdst = std::modf(secs, &int_part) * 1000000;
                        data_holder[col] = tm_;
                        indicators[col] = soci::i_ok;
                    } else {
                        data_holder[col] = std::tm{};
                        indicators[col] = soci::i_null;
                    }
                }
                , [&](elem_type const & e) { data_holder[col] = e.str(); }
            };

            reader.run_rows([&] (auto & row_span) {
                col = 0u;
                for (auto & elem : row_span) {
                    fill_funcs[static_cast<std::size_t>(composer.types()[col])](elem_type{elem});
                    col++;
                }
                stmt.execute(true);
            });
        }
        soci::session & sql_;
        create_table_composer & composer_;
        std::string const & after_insert_;
        std::string const & insert_prefix_;
    public:
        table_inserter(auto const &args, soci::session & sql, create_table_composer & composer)
        :sql_(sql), composer_(composer), after_insert_(args.after_insert), insert_prefix_(args.prefix) {
            if (!args.before_insert.empty()) {
                auto const statements = sql_split(std::stringstream(args.before_insert));
                sql.begin();
                for (auto & elem : statements)
                    sql << elem;
                sql.commit();
            }
        }
        ~table_inserter() {
            if (!after_insert_.empty()) {
                auto const statements = sql_split(std::stringstream(after_insert_));
                sql_.begin();
                for (auto & elem : statements)
                    sql_ << elem;
                sql_.commit();
            }
        }

        void insert(auto const &args, auto & reader) {
            data_holder.resize(composer_.types().size());
            indicators.resize(composer_.types().size(), soci::i_ok);

            sql_.begin();
            auto prep = sql_.prepare.operator<<(insert_expr(args));
            reset_value_index();
            for(auto e : composer_.types()) {
                std::visit([&](auto & arg) {
                    prep = std::move(prep.operator,(soci::use(*arg, indicators[value_index])));
                }, prepare_next_arg(e));
                value_index++;
            }
            soci::statement stmt = prep;
            insert_data(reader, composer_, stmt);
            sql_.commit();
        }
    };

    class query {
        static auto queries(auto const & args) {
            std::stringstream queries;
            if (std::filesystem::exists(std::filesystem::path{args.query})) {
                std::filesystem::path path{args.query};
                auto const length = std::filesystem::file_size(path);
                if (length == 0)
                    throw std::runtime_error("Query file '" + args.query +"' exists, but it is empty.");
                std::ifstream f(args.query);
                if (!(f.is_open()))
                    throw std::runtime_error("Error opening the query file: '" + args.query + "'.");
                std::string queries_s;
                for (std::string line; std::getline(f, line, '\n');)
                    queries_s += line;
                queries << recode_source(std::move(queries_s), args);
            } else
                queries << args.query;
            return sql_split(std::move(queries));
        }
    public:
        query(auto const & args, soci::session & sql) {
            if (!args.query.empty()) {
                auto q_array = queries(args);
                std::for_each(q_array.begin(), q_array.end() - 1, [&](auto & elem){
                    sql.begin();
                    sql << elem;
                    sql.commit();
                });

                using namespace ::csvkit::cli::sql;
                rowset_query(sql, args, q_array.back());
            }
        }
    };

    void reset_environment() {
        create_table_composer::file_no = 0;
    }

#if !defined(__unix__)
    static
    struct soci_backend_dependancy {
        soci_backend_dependancy() {
#if !defined(BOOST_UT_DISABLE_MODULE)
        std::string path = getenv("PATH");
        path = "PATH=" + path + ";..\\..\\external_deps";
        putenv(path.c_str());
#else
        std::string path = getenv("PATH");
        path = "PATH=" + path + ";..\\..\\..\\external_deps";
        putenv(path.c_str());
#endif
        }
    } sbd;
#endif

} ///detail

namespace csvsql {
    template <typename ReaderType>
    void sql(auto & args) {

        using namespace detail;

        std::vector<std::string> table_names;
        if (!args.tables.empty())
            table_names = [&] {
                std::istringstream stream(args.tables);
                std::vector<std::string> result;
                for (std::string word; std::getline(stream, word, ',');)
                    result.push_back(word != "_" ? word : "stdin");
                return result;
            }();

        if (!args.dialect.empty()) {
            std::vector<std::string_view> svv{"mysql", "postgresql", "sqlite", "firebird"};
            if (!std::any_of(svv.cbegin(), svv.cend(), [&](auto elem){ return elem == args.dialect;}))
                throw std::runtime_error("csvsql: error: argument -i/--dialect: invalid choice: '" + args.dialect + "' (choose from 'mysql', 'postgresql', 'sqlite', 'firebird').");
            if (!args.db.empty() or !args.query.empty())
                throw std::runtime_error("csvsql: error: The --dialect option is only valid when neither --db nor --query are specified.");
        }

        if (args.insert and args.db.empty() and args.query.empty())
            throw std::runtime_error("csvsql: error: The --insert option is only valid when either --db or --query is specified.");

        auto need_insert = [](char const * const option) {
            return std::string("csvsql: error: The ") + option + " option is only valid if --insert is also specified.";
        };

        if (!args.before_insert.empty() and !args.insert)
            throw std::runtime_error(need_insert("--before-insert"));
        if (!args.after_insert.empty() and !args.insert)
            throw std::runtime_error(need_insert("--after-insert"));
        if (args.no_create and !args.insert)
            throw std::runtime_error(need_insert("--no-create option"));
        if (args.create_if_not_exists and !args.insert)
            throw std::runtime_error(need_insert("--create-if-not-exists option"));
        if (args.overwrite and !args.insert)
            throw std::runtime_error(need_insert("--overwrite"));
        if (args.chunk_size and !args.insert)
            throw std::runtime_error(need_insert("--chunk-size"));

        // TODO: FixMe in the future.
        if (args.create_if_not_exists and args.db.find("firebird") != std::string::npos) {
            throw std::runtime_error("Sorry, we do not support the --create-if-not-exists option for Firebird DBMS client.");
        }

        reset_environment();
        readers_manager<ReaderType> r_man;
        r_man.template set_readers<ReaderType>(args);

        if (args.db.empty() and args.query.empty()) {
            for (auto & r : r_man.get_readers()) {
                create_table_composer composer (r, args, table_names);
                std::cout << create_table_composer::table();
            }
            return;
        }

        if (args.db.empty())
            args.db = "sqlite3://db=:memory:";

        soci::session session(args.db);

        for (auto & reader : r_man.get_readers()) {
            try {
                create_table_composer composer(reader, args, table_names);
                table_creator{args, session};
                if (args.insert or (args.db.find(":memory:") != std::string::npos and !args.query.empty()))
                    table_inserter(args, session, composer).insert(args, reader);
            } catch(std::exception & e) {
                if (std::string(e.what()).find("Vain to do next actions") != std::string::npos)
                    continue;
                throw;
            }
        }
        query q(args, session);
    }

}

#if !defined(BOOST_UT_DISABLE_MODULE)

int main(int argc, char * argv[]) {
    auto args = argparse::parse<csvsql::detail::Args>(argc, argv);

    if (args.verbose)
        args.print();

    OutputCodePage65001 ocp65001;

    try {
        if (!args.skip_init_space)
            csvsql::sql<notrimming_reader_type>(args);
        else
            csvsql::sql<skipinitspace_reader_type>(args);
    }
    catch (soci::soci_error const & e) {
        std::cout << e.get_error_message() << std::endl;
    }
    catch (std::exception const & e) {
        std::cout << e.what() << std::endl;
    }
    catch (...) {
        std::cout << "Unknown exception.\n";
    }
}

#endif
