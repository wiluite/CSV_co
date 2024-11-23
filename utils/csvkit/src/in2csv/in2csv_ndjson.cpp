#include "../../include/in2csv/in2csv_ndjson.h"
#include <cli.h>
#include <iostream>
#include <jsoncons/json.hpp>
#include <jsoncons_ext/csv/csv.hpp>

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

        while (!doc.reader().eof())
        {
            doc.reader().read_next();

            if (!doc.reader().eof())
            {
                ojson j = doc.decoder().get_result();
                std::ostringstream oss;
                oss << '[' << print(j) << ']';
                ojson jj = ojson::parse(oss.str());
                csv::csv_stream_encoder encoder(std::cout);
                jj.dump(encoder);
            }
        }
    }
}
