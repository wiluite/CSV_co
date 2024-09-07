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
#include <unistd.h>

using namespace ::csvkit::cli;

namespace sql2csv::detail {
    struct Args : argparse::Args {
        std::filesystem::path & query_file = arg("The FILE to use as SQL query. If it and --query are omitted, the query is piped data via STDIN.").set_default("");
        bool & verbose = flag("v,verbose", "A flag to toggle verbose.");
        bool & linenumbers = flag("l,linenumbers", "Insert a column of line numbers at the front of the output. Useful when piping to grep or as a simple primary key.");
        std::string & db = kwarg("db","If present, a 'soci' connection string to use to directly execute generated SQL on a database.").set_default(std::string(""));
        std::string & query = kwarg("query","The SQL query to execute. Overrides FILE and STDIN.").set_default(std::string(""));
        std::string & encoding = kwarg("e,encoding","Specify the encoding of the input query file.").set_default("UTF-8");
        bool & no_header = flag("H,no-header-row","Do not output column names.");

        void welcome() final {
            std::cout << "\nExecute an SQL query on a database and output the result to a CSV file.\n\n";
        }
    };

    std::string & query_stdin() {
        static std::string query_stdin_;
        return query_stdin_;
    }

    auto sql_split = [](std::stringstream && strm, char by = ';') {
        bool only_one = false;
        std::string result_phrase;
        for (std::string phrase; std::getline(strm, phrase, by);) {
            if (!only_one) {
                only_one = true;
                result_phrase = std::move(phrase);
            } else
                throw std::runtime_error("Warning: You can only execute one statement at a time.");
        }
        return result_phrase;
    };

    class query {
        static auto query_impl(auto const & args) {
            std::stringstream queries;
            if (!(args.query.empty())) {
                queries << args.query;
            } else
            if (!args.query_file.empty()) {
                if (std::filesystem::exists(std::filesystem::path{args.query_file})) {
                    std::filesystem::path path{args.query_file};
                    auto const length = std::filesystem::file_size(path);
                    if (length == 0)
                        throw std::runtime_error("Query file '" + args.query_file.string() +"' exists, but it is empty.");
                    std::ifstream f(args.query_file);
                    if (!(f.is_open()))
                        throw std::runtime_error("Error opening the query file: '" + args.query_file.string() + "'.");
                    std::string queries_s;
                    for (std::string line; std::getline(f, line, '\n');)
                        queries_s += line;
                    queries << recode_source(std::move(queries_s), args);
                } else
                    throw std::runtime_error("No such file or directory: '" + args.query_file.string() + "'.");
            } else {
                assert(!query_stdin().empty());
                queries << query_stdin();
            }
            return sql_split(std::move(queries));
        }
    public:
        query(auto const & args, soci::session && sql) {
            using namespace ::csvkit::cli::sql;
            rowset_query(sql, args, query_impl(args));
        }
    };

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


} /// detail

namespace sql2csv {
    void recode_FILE(auto const & args) {
    }

    template<typename ReaderType>
    void sql_to_csv(auto &args) {
        using namespace detail;
        if (args.query.empty() and args.query_file.empty() and isatty(STDIN_FILENO))
            throw std::runtime_error("sql2csv: error: You must provide an input file or piped data.");

        if ((args.query.empty() and args.query_file.empty() and !isatty(STDIN_FILENO)) or (args.query.empty() and args.query_file == "_")) {
            query_stdin() = read_standard_input(args);
            if (args.query_file == "_")
                args.query_file.clear();
        }

        if (args.db.empty())
            args.db = "sqlite3://db=:memory:";

        query{args, soci::session{args.db}};
    }
}

#if !defined(BOOST_UT_DISABLE_MODULE)

int main(int argc, char * argv[]) {
    auto args = argparse::parse<sql2csv::detail::Args>(argc, argv);

    if (args.verbose)
        args.print();

    OutputCodePage65001 ocp65001;

    try {
        sql2csv::sql_to_csv<notrimming_reader_type>(args);
    }
    catch (soci::soci_error const & e) {
        std::cout << e.what() << std::endl;
    }
    catch (std::exception const & e) {
        std::cout << e.what() << std::endl;
    }
    catch (...) {
        std::cout << "Unknown exception.\n";
    }
}

#endif
