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
        bool &date_lib_parser = flag("date-lib-parser", "Use date library as Dates and DateTimes parser backend instead compiler-supported");

        void welcome() final {
            std::cout << "\nGenerate SQL statements for one or more CSV files, or execute those statements directly on a database, and execute one or more SQL queries.\n\n";
        }
    };

    template <typename ... r_types>
    struct readers_manager {
        auto & get_readers() {
            static std::deque<r_types...> readers;
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
        ~readers_manager() {
            reset_readers();
        }
    private:
        auto reset_readers() {
            get_readers().clear();
        }
    };

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
                    stream << (index != types.size() ? ",\n\t" : "\n");
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
                    stream << (index != types.size() ? ",\n\t" : "\n");
                }
            }
            static std::string table() {
                return stream.str();
            }

            struct encloser {
                encloser(auto const & args, std::vector<std::string> const & table_names) {
                    stream.str({});
                    stream.clear();
                    stream << "CREATE TABLE ";
                    if (table_names.size() > file_no)
                        stream << table_names[file_no];
                    else
                        stream << std::filesystem::path{args.files[file_no]}.stem();
                    stream << " (\n\t";
                }
                ~encloser() {
                    stream << ");\n";
                }
            } encloser_;
            print_director(auto const & args, std::vector<std::string> const & table_names)
                : encloser_(args, table_names) {}
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
    public:
        create_table_composer(auto & reader, auto const & args, std::vector<std::string> const & table_names) {

            auto const info = typify(reader, args, !args.no_constraints ? typify_option::typify_with_precisions : typify_option::typify_without_precisions);
            skip_lines(reader, args);
            auto const header = obtain_header_and_<skip_header>(reader, args);

            print_director print_director_(args, table_names);
            std::visit([&](auto & arg) {
                print_director_.direct(reader, arg, dialect(args), header);
            }, info);

            ++file_no;
        }

        [[nodiscard]] static std::string table() {
            return print_director::table();
        }

    };

    struct table_inserter {
        table_inserter(soci::session & sql, create_table_composer & composer, auto & reader) {
            sql.begin();
            sql << create_table_composer::table();
            sql << "insert into iris(sepal_length,sepal_width,petal_length,petal_width,species) values(5.1,3.5,1.4,0.2,\"Iris-setosa\")";
            sql.commit();
        }
    };

    struct query {
        query(soci::session & sql, std::string q) {
            soci::row r;
            sql << q, into(r);
            std::cout << r.size() << std::endl;
        }
    };

    void reset_environment() {
        create_table_composer::file_no = 0;
    }

} ///detail

static
struct soci_backend_dependancy {
    soci_backend_dependancy() {
    #if !defined(__unix__)
      #if !defined(BOOST_UT_DISABLE_MODULE)
        std::string path = getenv("PATH");
        path = "PATH=" + path + ";..\\..\\external_deps";
        putenv(path.c_str());
        #else
        std::string path = getenv("PATH");
        path = "PATH=" + path + ";..\\..\\..\\external_deps";
        putenv(path.c_str());
      #endif
    #endif
    }
} sbd;

namespace csvsql {
    template <typename ReaderType>
    void sql(auto & args) {

        using namespace csv_co;
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

        reset_environment();
        readers_manager<ReaderType> r_man;
        r_man.template set_readers<ReaderType>(args);

        if (args.db.empty() and args.query.empty()) {
            for (auto & r : r_man.get_readers()) {
                create_table_composer composer (r, args, table_names);
                std::cout << composer.table();
            }
            return;
        }
        if (args.db.empty()) {
            args.db = "sqlite3://db=:memory:";
        }

        soci::session session(args.db);
        for (auto & reader : r_man.get_readers()) {
            create_table_composer composer (reader, args, table_names);
            table_inserter ti (session, composer, reader);
        }

        query q(session, args.query);

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
    catch (std::exception const & e) {
        std::cout << e.what() << std::endl;
    }
    catch (...) {
        std::cout << "Unknown exception.\n";
    }
}

#endif
