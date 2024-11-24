#include "../../include/in2csv/in2csv_ndjson.h"
#include <cli.h>
#include <iostream>
#include <jsoncons/json.hpp>
#include <jsoncons_ext/csv/csv.hpp>
#include "header_printer.h"

using namespace ::csvkit::cli;

namespace in2csv::detail::ndjson {
    using column_name_type = std::string;
    using column_values_type = std::vector<std::string>;
    std::unordered_map<column_name_type, column_values_type> csv_map;

    void fill_func (auto && elem, auto const & header, unsigned col, auto && types_n_blanks, auto const & args) {
        using elem_type = std::decay_t<decltype(elem)>;
        auto & [types, blanks] = types_n_blanks;
        bool const is_null = elem.is_null();
        if (types[col] == column_type::text_t or (!args.blanks and is_null)) {
            auto compose_text = [&](auto const & e) -> std::string {
                typename elem_type::template rebind<csv_co::unquoted>::other const & another_rep = e;
                if (another_rep.raw_string_view().find(',') != std::string_view::npos)
                    return another_rep;
                else
                    return another_rep.str();
            };
            csv_map[header[col]].push_back(!args.blanks && is_null ? "" : compose_text(elem));
            //os << (!args.blanks && is_null ? "" : compose_text(elem));
            return;
        }
        assert(!is_null && (!args.blanks || (args.blanks && !blanks[col])) && !args.no_inference);

        using func_type = std::function<std::string(elem_type const &)>;

#if !defined(BOOST_UT_DISABLE_MODULE)
        static
#endif
        std::array<func_type, static_cast<std::size_t>(column_type::sz)> type2func {
                compose_bool<elem_type>
                , [&](elem_type const & e) {
                    assert(!e.is_null());

                    static std::ostringstream ss;
                    ss.str({});

                    typename elem_type::template rebind<csv_co::unquoted>::other const & another_rep = e;
                    auto const value = another_rep.num();

                    if (std::isnan(value))
                        ss << "NaN";
                    else if (std::isinf(value))
                        ss << (value > 0 ? "Infinity" : "-Infinity");
                    else {
                        if (args.num_locale != "C") {
                            std::string s = another_rep.str();
                            another_rep.to_C_locale(s);
                            ss << s;
                        } else
                            ss << another_rep.str();
                    }
                    return ss.str();
                }
                , compose_datetime<elem_type>
                , compose_date<elem_type>
                , [](elem_type const & e) {
                    auto str = std::get<1>(e.timedelta_tuple());
                    return str.find(',') != std::string::npos ? R"(")" + str + '"' : str;
                }
        };
        auto const type_index = static_cast<std::size_t>(types[col]) - 1;
        //os << type2func[type_index](elem);
        csv_map[header[col]].push_back(type2func[type_index](elem));
    }

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

        std::size_t total_rows = 0;

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
#if 1
                        reader.run_rows([&](auto span) {
                            unsigned col = 0;
                            using elem_type = typename std::decay_t<decltype(span.back())>::reader_type::template typed_span<csv_co::unquoted>;
                            for (auto & e : span)
                                fill_func(elem_type{e}, header, col++, types_and_blanks, a);
                            
                            total_rows++;
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
