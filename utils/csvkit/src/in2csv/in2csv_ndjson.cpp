#include "../../include/in2csv/in2csv_ndjson.h"
#include <cli.h>
#include <iostream>
#include <jsoncons/json.hpp>
#include <jsoncons_ext/csv/csv.hpp>
#include "header_printer.h"

using namespace ::csvkit::cli;

namespace in2csv::detail::ndjson {

    void impl::convert() {
        using namespace jsoncons;

        class holder {
            json_decoder<ojson> decoder_;
            std::shared_ptr<json_stream_reader> reader_ptr;
        public:
            holder(impl_args const & a) : reader_ptr (
                [&a, this] {
                    if (a.file.empty() or a.file == "_") {
                        static std::stringstream ss;
#ifndef __linux__
                        _setmode(_fileno(stdin), _O_BINARY);
#endif
                        for (;;) {
                            if (auto r = std::cin.get(); r != std::char_traits<char>::eof())
                                ss << static_cast<char>(r);
                            else
                                break;
                        }
                        if (ss.str().back() != '\n')
                            ss << '\n';
                        return new json_stream_reader(ss, decoder_);
                    } else {
                        static std::ifstream is(a.file.string());
                        return new json_stream_reader(is, decoder_);
                    }
                }(),
                [&](json_stream_reader* descriptor) {
                    delete descriptor;  
                }
            ) {}
            json_decoder<ojson> & decoder() {
                return decoder_;
            }
            json_stream_reader & reader() {
                return *reader_ptr;
            }
        } doc(a);

        std::vector<std::string> result_header;
        auto update_result_header = [&result_header](auto const & new_header) {
            for (auto & e : new_header)
                if (std::find(result_header.cbegin(), result_header.cend(), e) == result_header.cend())
                    result_header.push_back(e);
        };

        while (!doc.reader().eof())
        {
            doc.reader().read_next();

            if (!doc.reader().eof())
            {
                ojson _ = doc.decoder().get_result();
                std::ostringstream oss;
                oss << '[' << print(_) << ']';
                ojson next_csv = ojson::parse(oss.str());
                oss.str({});
                csv::csv_stream_encoder encoder(oss);
                next_csv.dump(encoder);

                a.skip_lines = 0;
                a.no_header = false;
                std::variant<std::monostate, notrimming_reader_type, skipinitspace_reader_type> variants;
                if (!a.skip_init_space)
                    variants = notrimming_reader_type(recode_source(oss.str(), a));
                else
                    variants = skipinitspace_reader_type(recode_source(oss.str(), a));

                std::visit([&](auto & reader) {
                    if constexpr(!std::is_same_v<std::decay_t<decltype(reader)>, std::monostate>) {
                        // no skip_lines() needed
                        auto types_and_blanks = std::get<1>(typify(reader, a, typify_option::typify_without_precisions));
                        // no skip_lines() needed again
                        auto header = string_header(obtain_header_and_<skip_header>(reader, a));
                        update_result_header(header);
#if 0
                        reader.run_rows([&](auto span) {
                            auto col = 0u;
                            using elem_type = typename std::decay_t<decltype(span.back())>::reader_type::template typed_span<csv_co::unquoted>;
                            for (auto & e : span) {
                                fill_func(elem_type{e}, header[col++], types_and_blanks, a)
                            }
                        });
#endif
                    }
                }, variants);
            }
        }

        for (auto & e : result_header) {
            std::cout << e << ' ';
        }
    }
}
