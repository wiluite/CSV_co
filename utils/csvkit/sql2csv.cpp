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
using namespace ::csvkit::cli;

namespace sql2csv::detail {
    struct Args : argparse::Args {
        std::filesystem::path & query_file = arg("The FILE to use as SQL query. If it and --query are omitted, the query is piped data via STDIN.").set_default("cin");
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
