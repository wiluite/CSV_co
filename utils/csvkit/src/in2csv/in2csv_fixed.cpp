#include "../../include/in2csv/in2csv_fixed.h"
#include <cli.h>

using namespace ::csvkit::cli;

namespace in2csv::detail::fixed {

    void conv(auto & reader, auto const & args) {
        static_assert(std::is_same_v<std::decay_t<decltype(reader)>, notrimming_reader_type> or
                      std::is_same_v<std::decay_t<decltype(reader)>, skipinitspace_reader_type>);
    }

    void impl::convert() {
        std::variant<std::monostate
            , std::reference_wrapper<notrimming_reader_type>
            , std::reference_wrapper<skipinitspace_reader_type>> variants;

        if (!a.skip_init_space) {
            notrimming_reader_type r (std::filesystem::path{a.schema});
            recode_source(r, a);
            variants = std::ref(r);
        } else {
            skipinitspace_reader_type r (std::filesystem::path{a.schema});
            recode_source(r, a);
            variants = std::ref(r);
        }
        std::visit([&](auto & reader) {
            if constexpr(!std::is_same_v<std::decay_t<decltype(reader)>, std::monostate>) {
                //TODO: why can't we use: recode_source(reader.get(), a) - UB now.
                conv(reader.get(), a);
            }
        }, variants);
    }
}
