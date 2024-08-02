//
// Created by wiluite on 7/30/24.
//
#include <soci/soci.h>
#if defined (SOCI_HAVE_SQLITE3)
 #include <soci/sqlite3/soci-sqlite3.h>
#endif

#if defined (SOCI_HAVE_EMPTY)
#include <soci/empty/soci-empty.h>
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

namespace csvsql {
    struct Args : ARGS_positional_files {
        std::string & num_locale = kwarg("L LOCALE,locale LOCALE","Specify the locale (\"C\") of any formatted numbers.").set_default(std::string("C"));
        bool &blanks = flag("blanks", R"(Do not convert "", "na", "n/a", "none", "null", "." to NULL.)");
        std::vector<std::string> &null_value = kwarg("null-value NULL_VALUES...","Convert these values to NULL.").multi_argument().set_default(std::vector<std::string>{});
        std::string & date_fmt = kwarg("date-format DATE_FORMAT","Specify an strptime date format string like \"%m/%d/%Y\".").set_default(R"(%m/%d/%Y)");
        std::string & datetime_fmt = kwarg("datetime-format DATETIME_FORMAT","Specify an strptime datetime format string like \"%m/%d/%Y %I:%M %p\".").set_default(R"(%m/%d/%Y %I:%M %p)");

        std::string & dialect = kwarg("i DIALECT,dialect DIALECT","Dialect of SQL {mysql,postgresql,sqlite,firebird} to generate. Cannot be used with --db.").set_default(std::string("sqlite"));
        std::string & db = kwarg("db CONNECTION_STRING","If present, a 'soci' connection string to use to directly execute generated SQL on a database.").set_default(std::string(""));
        std::string & query = kwarg("query QUERIES","Execute one or more SQL queries delimited by \";\" and output the result of the last query as CSV. QUERY may be a filename. --query may be specified multiple times.").set_default(std::string(""));
        bool &insert = flag("insert", "Insert the data into the table. Requires --db.");
        std::string & prefix = kwarg("prefix PREFIX","Add an expression following the INSERT keyword, like OR IGNORE or OR REPLACE.").set_default(std::string(""));
        std::string & before_insert = kwarg("before-insert BEFORE_INSERT","Execute SQL before the INSERT command. Requires --insert.").set_default(std::string(""));
        std::string & after_insert = kwarg("after-insert AFTER_INSERT","Execute SQL after the INSERT command. Requires --insert.").set_default(std::string(""));
        std::string & tables = kwarg("tables TABLE_NAMES","A comma-separated list of names of tables to be created. By default, the tables will be named after the filenames without extensions or \"stdin\".").set_default(std::string(""));
        bool &no_constraints = flag("no-constraints", "Generate a schema without length limits or null checks. Useful when sampling big tables.");
        std::string & unique_constraint = kwarg("unique-constraint UNIQUE_CONSTRAINT","A comma-separated list of names of columns to include in a UNIQUE constraint").set_default(std::string(""));

        bool &no_create = flag("no-create", "Skip creating the table. Requires --insert.");
        bool &create_if_not_exists = flag("create-if-not-exists", "Create the table if it does not exist, otherwise keep going. Requires --insert.");
        bool &overwrite = flag("overwrite", "Drop the table if it already exists. Requires --insert. Cannot be used with --no-create.");
        std::string & db_schema = kwarg("db-schema DB_SCHEMA","Optional name of database schema to create table(s) in.").set_default(std::string(""));
        bool &no_inference = flag("I,no-inference", "Disable type inference when parsing the input.");
        unsigned & chunk_size = kwarg("chunk-size CHUNK_SIZE","Chunk size for batch insert into the table. Requires --insert.").set_default(4096);
        bool &date_lib_parser = flag("date-lib-parser", "Use date library as Dates and DateTimes parser backend instead compiler-supported");

        void welcome() final {
            std::cout << "\nGenerate SQL statements for one or more CSV files, or execute those statements directly on a database, and execute one or more SQL queries.\n\n";
        }

    };
}

int main(int argc, char * argv[]) {
    using namespace csvsql;

    auto args = argparse::parse<Args>(argc, argv);

    if (args.verbose)
        args.print();

    try {
        using namespace soci;
        //session sql(sqlite3, "db.sqlite3");               // need linkage to soci_sqlite3 for its link target
        session sql("sqlite3://db=db.sqlite3"); // no need linkage to soci_sqlite3 for its link target
        sql << "create table test1 (id integer)";

    } catch (soci::soci_error const &err) {
        std::cout << "ERROR!\n";
    }
}
