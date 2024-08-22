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
//  3. calculate max length for VARCHAR cols (csvsql)

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
        auto set_readers(auto const & args) {
            for (auto const & elem : args.files) {
                auto reader {elem != "_" ? ReaderType{std::filesystem::path{elem}} : ReaderType{read_standard_input(args)}};
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
        static inline std::size_t file_no;
    public:
        struct column_tag {};
        struct bool_column_tag {};
        struct number_column_tag {};
        struct datetime_column_tag {};
        struct date_column_tag {};
        struct timedelta_column_tag {};
        struct text_column_tag {};

        struct printer {
            virtual void print_name(std::string const & name) {}
            virtual void print(bool_column_tag) {}
            virtual void print(bool_column_tag, bool blanks, unsigned prec) {}
            virtual void print(number_column_tag) {}
            virtual void print(number_column_tag, bool blanks, unsigned prec) {}
            virtual void print(datetime_column_tag) {}
            virtual void print(datetime_column_tag, bool blanks, unsigned prec) {}
            virtual void print(date_column_tag) {}
            virtual void print(date_column_tag, bool blanks, unsigned prec) {}
            virtual void print(timedelta_column_tag) {}
            virtual void print(timedelta_column_tag, bool blanks, unsigned prec) {}
            virtual void print(text_column_tag) {}
            virtual void print(text_column_tag, bool blanks, unsigned prec) {}
            virtual ~printer() {}
        protected:
            void print_name_or_quoted_name(std::string const & name, char qch) {
                if (std::all_of(name.cbegin(), name.cend(), [&](auto & ch) {
                    return ((std::isalpha(ch) and std::islower(ch))
                            or  (std::isdigit(ch) and std::addressof(ch) != std::addressof(name.front()))
                            or  ch == '_');
                }))
                    std::cout << name;
                else
                    std::cout << std::quoted(name, qch, qch);
                std::cout << " ";
            }
        };

        struct varchar_precision {};
        struct mysql_printer : printer, varchar_precision {
            void print_name(std::string const & name) override {
                print_name_or_quoted_name(name, '`');
            }
            void print(bool_column_tag) override {
                to_stream(std::cout, "BOOL");
            }
            void print(bool_column_tag, bool blanks, unsigned) override {
                to_stream(std::cout, "BOOL", (blanks ? "" : " NOT NULL"));
            }
            void print(number_column_tag) override {
                to_stream(std::cout, "DECIMAL");
            }
            void print(number_column_tag, bool blanks, unsigned prec) override {
                to_stream(std::cout, "DECIMAL(38, ", static_cast<int>(prec), ')', (blanks ? "" : " NOT NULL"));
            }
            void print(datetime_column_tag) override {
                to_stream(std::cout, "TIMESTAMP");
            }
            void print(datetime_column_tag, bool blanks, unsigned) override {
                to_stream(std::cout, "TIMESTAMP");
            }
            void print(date_column_tag) override {
                to_stream(std::cout, "DATE");
            }
            void print(date_column_tag, bool blanks, unsigned) override {
                to_stream(std::cout, "DATE", (blanks ? "" : " NOT NULL"));
            }
            void print(timedelta_column_tag) override {
                to_stream(std::cout, "DATETIME");
            }
            void print(timedelta_column_tag, bool blanks, unsigned) override {
                to_stream(std::cout, "DATETIME", (blanks ? "" : " NOT NULL"));
            }
            void print(text_column_tag) override {
                throw std::runtime_error("VARCHAR requires a length on dialect mysql");
            }
            void print(text_column_tag, bool blanks, unsigned prec) override {
                to_stream(std::cout, "VARCHAR(", static_cast<int>(prec), ')', (blanks ? "" : " NOT NULL"));
            }
        };
        struct postgresql_printer : printer {
            void print_name(std::string const & name) override {
                print_name_or_quoted_name(name, '"');
            }
            void print(bool_column_tag) override {
                to_stream(std::cout, "BOOLEAN");
            }
            void print(bool_column_tag, bool blanks, unsigned) override {
                to_stream(std::cout, "BOOLEAN", (blanks ? "" : " NOT NULL"));
            }
            void print(number_column_tag) override {
                to_stream(std::cout, "DECIMAL");
            }
            void print(number_column_tag, bool blanks, unsigned) override {
                to_stream(std::cout, "DECIMAL", (blanks ? "" : " NOT NULL"));
            }
            void print(datetime_column_tag) override {
                to_stream(std::cout, "TIMESTAMP WITHOUT TIME ZONE");
            }
            void print(datetime_column_tag, bool blanks, unsigned) override {
                to_stream(std::cout, "TIMESTAMP WITHOUT TIME ZONE");
            }
            void print(date_column_tag) override {
                to_stream(std::cout, "DATE");
            }
            void print(date_column_tag, bool blanks, unsigned) override {
                to_stream(std::cout, "DATE", (blanks ? "" : " NOT NULL"));
            }
            void print(timedelta_column_tag) override {
                to_stream(std::cout, "INTERVAL");
            }
            void print(timedelta_column_tag, bool blanks, unsigned) override {
                to_stream(std::cout, "INTERVAL", (blanks ? "" : " NOT NULL"));
            }
            void print(text_column_tag) override {
                to_stream(std::cout, "VARCHAR");
            }
            void print(text_column_tag, bool blanks, unsigned) override {
                to_stream(std::cout, "VARCHAR", (blanks ? "" : " NOT NULL"));
            }
        };
        struct sqlite_printer : printer {
            void print_name(std::string const & name) override {
                print_name_or_quoted_name(name, '"');
            }
            void print(bool_column_tag) override {
                to_stream(std::cout, "BOOLEAN");
            }
            void print(bool_column_tag, bool blanks, unsigned) override {
                to_stream(std::cout, "BOOLEAN", (blanks ? "" : " NOT NULL"));
            }
            void print(number_column_tag) override {
                to_stream(std::cout, "FLOAT");
            }
            void print(number_column_tag, bool blanks, unsigned) override {
                to_stream(std::cout, "FLOAT", (blanks ? "" : " NOT NULL"));
            }
            void print(datetime_column_tag) override {
                to_stream(std::cout, "TIMESTAMP");
            }
            void print(datetime_column_tag, bool blanks, unsigned) override {
                to_stream(std::cout, "TIMESTAMP");
            }
            void print(date_column_tag) override {
                to_stream(std::cout, "DATE");
            }
            void print(date_column_tag, bool blanks, unsigned) override {
                to_stream(std::cout, "DATE", (blanks ? "" : " NOT NULL"));
            }
            void print(timedelta_column_tag) override {
                to_stream(std::cout, "DATETIME");
            }
            void print(timedelta_column_tag, bool blanks, unsigned) override {
                to_stream(std::cout, "DATETIME", (blanks ? "" : " NOT NULL"));
            }
            void print(text_column_tag) override {
                to_stream(std::cout, "VARCHAR");
            }
            void print(text_column_tag, bool blanks, unsigned) override {
                to_stream(std::cout, "VARCHAR", (blanks ? "" : " NOT NULL"));
            }
        };
        struct firebird_printer : printer {
            void print_name(std::string const & name) override {
                print_name_or_quoted_name(name, '"');
            }
            void print(bool_column_tag) override {
                to_stream(std::cout, "BOOLEAN");
            }
            void print(bool_column_tag, bool blanks, unsigned) override {
                to_stream(std::cout, "BOOLEAN", (blanks ? "" : " NOT NULL"));
            }
            void print(number_column_tag) override {
                to_stream(std::cout, "DECIMAL");
            }
            void print(number_column_tag, bool blanks, unsigned) override {
                to_stream(std::cout, "DECIMAL", (blanks ? "" : " NOT NULL"));
            }
            void print(datetime_column_tag) override {
                to_stream(std::cout, "TIMESTAMP");
            }
            void print(datetime_column_tag, bool blanks, unsigned) override {
                to_stream(std::cout, "TIMESTAMP");
            }
            void print(date_column_tag) override {
                to_stream(std::cout, "DATE");
            }
            void print(date_column_tag, bool blanks, unsigned) override {
                to_stream(std::cout, "DATE", (blanks ? "" : " NOT NULL"));
            }
            void print(timedelta_column_tag) override {
                to_stream(std::cout, "TIMESTAMP");
            }
            void print(timedelta_column_tag, bool blanks, unsigned) override {
                to_stream(std::cout, "TIMESTAMP", (blanks ? "" : " NOT NULL"));
            }
            void print(text_column_tag) override {
                throw std::runtime_error("VARCHAR requires a length on dialect firebird");
            }
            void print(text_column_tag, bool blanks, unsigned) override {
                throw std::runtime_error("VARCHAR requires a length on dialect firebird");
            }
        };

        using printer_dialect = std::string;
        static inline std::unordered_map<printer_dialect, std::shared_ptr<printer>> printer_map {
                  {"mysql", std::make_shared<mysql_printer>()}
                , {"postgresql", std::make_shared<postgresql_printer>()}
                , {"sqlite", std::make_shared<sqlite_printer>()}
                , {"firebird", std::make_shared<firebird_printer>()}
        };

        static inline std::unordered_map<column_type
                , std::variant<bool_column_tag, number_column_tag, datetime_column_tag, date_column_tag, timedelta_column_tag, text_column_tag>>
                tag_map {
                  {column_type::bool_t, bool_column_tag{}}
                , {column_type::bool_t, bool_column_tag()}
                , {column_type::number_t, number_column_tag()}
                , {column_type::datetime_t, datetime_column_tag()}
                , {column_type::date_t, date_column_tag()}
                , {column_type::timedelta_t, timedelta_column_tag()}
                , {column_type::text_t, text_column_tag()}
        };

        struct print_director {
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
                    std::cout << (index != types.size() ? ",\n\t" : "\n");
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
                    std::cout << (index != types.size() ? ",\n\t" : "\n");
                }
            }
        };

        create_table_composer(auto & reader, auto const & args, std::vector<std::string> const & table_names) {
            auto const info = typify(reader, args, !args.no_constraints ? typify_option::typify_with_precisions : typify_option::typify_without_precisions);
            skip_lines(reader, args);
            auto const header = obtain_header_and_<skip_header>(reader, args);

            if (args.dialect.empty())
                args.dialect = "postgresql";

            using args_type = decltype(args);
            struct encloser {
                encloser(args_type & args, std::vector<std::string> const & table_names) {
                    std::cout << "CREATE TABLE ";
                    if (table_names.size() > file_no)
                        std::cout << table_names[file_no];
                    else
                        std::cout << args.files[file_no];
                    std::cout << " (\n\t";
                }
                ~encloser() {
                    std::cout << ");\n";
                }
            } eo(args, table_names);

            std::visit([&](auto & arg) {
                print_director().direct(reader, arg, args.dialect, header);
            }, info);
            ++file_no;
        }
    };

} ///detail

namespace csvsql {
    template <typename ReaderType>
    void sql(auto const & args) {
#if !defined(__unix__)
        std::string path = getenv("PATH");
        path = "PATH=" + path + ";..\\..\\external_deps";
        putenv(path.c_str());
#endif

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


        readers_manager<ReaderType> r_man;
        r_man.template set_readers<ReaderType>(args);

        if (args.db.empty() and args.query.empty())  // the simplest "create table" mode
            for (auto & r : r_man.get_readers()) {
                create_table_composer(r, args, table_names);
            }
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
