#include "../../include/in2csv/in2csv_fixed.h"
#include <cli.h>

using namespace ::csvkit::cli;

namespace in2csv::detail::fixed {

    void convert_impl(auto & reader, auto const & args) {
        static_assert(std::is_same_v<std::decay_t<decltype(reader)>, notrimming_reader_type> or
                      std::is_same_v<std::decay_t<decltype(reader)>, skipinitspace_reader_type>);
        skip_lines(reader, args);
        quick_check(reader, args);
        reader.run_spans([&](auto e) {
            std::cout << " " << e.operator csv_co::unquoted_cell_string() << std::endl;
        }, [&] {
        });

    }

    void impl::convert() {
        std::variant<std::monostate
            , notrimming_reader_type
            , skipinitspace_reader_type> variants;

        if (!a.skip_init_space)
            variants = notrimming_reader_type(std::filesystem::path{a.schema});
        else
            variants = skipinitspace_reader_type(std::filesystem::path{a.schema});

        std::visit([&](auto & arg) {
            if constexpr(!std::is_same_v<std::decay_t<decltype(arg)>, std::monostate>) {
                //TODO: why can't we use: recode_source(reader.get(), a) - UB now.
                recode_source(arg, a);
                convert_impl(arg, a);
            }
        }, variants);
    }
}
