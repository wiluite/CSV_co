//
// Created by wiluite on 10/6/24.
//
#include <cli.h>

using namespace ::csvkit::cli;

namespace in2csv::detail {

    struct Args : ARGS_positional_1 {
        std::string & num_locale = kwarg("L,locale","Specify the locale (\"C\") of any formatted numbers.").set_default(std::string("C"));
        bool & blanks = flag("blanks",R"(Do not convert "", "na", "n/a", "none", "null", "." to NULL.)");
        std::vector<std::string> & null_value = kwarg("null-value", "Convert this value to NULL. --null-value can be specified multiple times.").multi_argument().set_default(std::vector<std::string>{});
        std::string & date_fmt = kwarg("date-format","Specify an strptime date format string like \"%m/%d/%Y\".").set_default(R"(%m/%d/%Y)");
        std::string & datetime_fmt = kwarg("datetime-format","Specify an strptime datetime format string like \"%m/%d/%Y %I:%M %p\".").set_default(R"(%m/%d/%Y %I:%M %p)");
        std::string & format = kwarg("f,format","The format {csv,dbf,fixed,geojson,json,ndjson,xls,xlsx} of the input file. If not specified will be inferred from the file type.").set_default(std::string{});
        std::string & schema = kwarg("s,schema","Specify a CSV-formatted schema file for converting fixed-width files. See in2csv_test as example.").set_default(std::string{});

        void welcome() final {
            std::cout << "\nConvert common, but less awesome, tabular data formats to CSV.\n\n";
        }
    };
    struct empty_file_and_no_piping_now : std::runtime_error {
        empty_file_and_no_piping_now() : std::runtime_error("sql2csv: error: You must provide an input file or piped data.") {}
    };
    struct no_format_specified_on_piping : std::runtime_error {
        no_format_specified_on_piping() : std::runtime_error("in2csv: error: You must specify a format when providing input as piped data via STDIN.") {}
    };
    struct no_schema_when_no_extension : std::runtime_error {
        no_schema_when_no_extension() : std::runtime_error("ValueError: schema must not be null when format is \"fixed\".") {}
    };
    struct unable_to_automatically_determine_format : std::runtime_error {
        unable_to_automatically_determine_format() : std::runtime_error("in2csv: error: Unable to automatically determine the format of the input file. Try specifying a format with --format.") {}
    };
    struct invalid_input_format : std::runtime_error {
        explicit invalid_input_format(std::string const & f)
        : std::runtime_error(std::string("in2csv: error: argument -f/--format: invalid choice: ") + '\''
            + f + '\'' + " (choose from 'csv', 'dbf', 'fixed', 'geojson', 'json', 'ndjson', 'xls', 'xlsx')") {}
    };
    struct schema_file_not_found : std::runtime_error {
        explicit schema_file_not_found(std::string const & f) : std::runtime_error(std::string("File not found error: No such file or directory: ") + '\'' + f +'\'') {}
    };
    struct converter_client
    {
        virtual ~converter_client() = default;
        virtual void convert() = 0;
    };

    template <class Args2>
    struct fixed_client : converter_client {
        explicit fixed_client(Args2 & args) : args(args) {
        }
        void convert() override {
        }
        static std::shared_ptr<converter_client> create(Args2 & args) {
            return std::make_shared<fixed_client>(args);
        }
    private:
        Args2 & args;
    };

    template <class Args2>
    struct xlsx_client : converter_client {
        explicit xlsx_client(Args2 & args) : args(args) {
        }
        void convert() override {
        }
        static std::shared_ptr<converter_client> create(Args2 & args) {
            return std::make_shared<xlsx_client>(args);
        }
    private:
        Args2 & args;
    };

    template <class Args2>
    class dbms_client_factory
    {
    public:
        using CreateCallback = std::function<std::shared_ptr<converter_client>(Args2 &)>;

        static void register_client(const std::string &type, CreateCallback cb) {
            clients[type] = cb;
        }
        static void unregister_client(const std::string &type) {
            clients.erase(type);
        }
        static std::shared_ptr<converter_client> create_client(const std::string &type, Args2 & args) {
            auto it = clients.find(type);
            if (it != clients.end())
                return (it->second)(args);
            return nullptr;
        }

    private:
        using CallbackMap = std::map<std::string, CreateCallback>;
        static inline CallbackMap clients;
    };

}
namespace in2csv {
    void in2csv(auto &args) {
        using namespace detail;
        namespace fs = std::filesystem;

        std::array<std::string, 8> extensions {"csv", "dbf", "fixed", "geojson", "json", "ndjson", "xls", "xlsx" };

        if (!args.format.empty()) {
            if (std::find(extensions.cbegin(), extensions.cend(), args.format) == extensions.cend())
                throw invalid_input_format(args.format);
        }

        if (args.format.empty() and !args.schema.empty()) {
            if (!(std::filesystem::exists(fs::path{args.schema})))
                throw schema_file_not_found(args.schema);
        }

        // no file, no piped data
        if (args.file.empty() and isatty(STDIN_FILENO))
            throw empty_file_and_no_piping_now();

        // piped data, but no format specified
        if (args.file.empty() and args.format.empty()) {
            assert(!isatty(STDIN_FILENO));
            throw no_format_specified_on_piping();
        }
        auto extension = fs::path(args.file).extension().string();
        // there is a file with no extension(i.e. fixed-wide format) and no schema file provided
        if (extension.empty() and args.schema.empty())
            throw no_schema_when_no_extension();

        extension = extension.empty() ? "fixed" : extension;
        if (!args.file.empty() and args.format.empty()) {
            if (std::find(extensions.cbegin(), extensions.cend(), extension) == extensions.cend())
                throw unable_to_automatically_determine_format();
        }

        using args_type = std::decay_t<decltype(args)>;
        using dbms_client_factory_type = dbms_client_factory<args_type>;
        dbms_client_factory_type::register_client("fixed", fixed_client<args_type>::create);
        dbms_client_factory_type::register_client("xlsx", xlsx_client<args_type>::create);

        auto client = dbms_client_factory_type::create_client(args.format.empty() ? extension : args.format, args);
        client->convert();
    }
}

#if !defined(BOOST_UT_DISABLE_MODULE)

int main(int argc, char * argv[]) {
    auto args = argparse::parse<in2csv::detail::Args>(argc, argv);

    if (args.verbose)
        args.print();

    OutputCodePage65001 ocp65001;

    try {
        in2csv::in2csv(args);
    }
    catch (std::exception const & e) {
        std::cout << e.what() << std::endl;
    }
    catch (...) {
        std::cout << "Unknown exception.\n";
    }
}

#endif
