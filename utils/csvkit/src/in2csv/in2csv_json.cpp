#include "../../include/in2csv/in2csv_json.h"
#include <cli.h>
#include <iostream>
#include <jsoncons/json.hpp>
#include <jsoncons_ext/csv/csv.hpp>

using namespace ::csvkit::cli;

namespace in2csv::detail::json {
    void impl::convert() {
        using namespace jsoncons;

        class ojson_holder {
            std::shared_ptr<ojson> ptr;
        public:
            ojson_holder(impl_args const & a) : ptr (
                [&a, this] {
                    if (a.file.empty() or a.file == "_") {
                        std::string foo;
#ifndef __linux__
                        _setmode(_fileno(stdin), _O_BINARY);
#endif
                        for (;;) {
                            if (auto r = std::cin.get(); r != std::char_traits<char>::eof())
                                foo += static_cast<char>(r);
                            else
                                break;
                        }

                        return new ojson(ojson::parse(foo));
                    } else {
                        return new ojson;
                    }
                }(),
                [&](ojson* descriptor) {
                    delete descriptor;  
                }
            ) {
                if (!(a.file.empty() or a.file == "_")) {
                    std::ifstream is(a.file.string());
                    is >> *ptr;
                }
            }
            operator ojson& () {
                return *ptr;
            }
            void dump_csv(std::ostream & os) {
                csv::csv_stream_encoder encoder(os);
                ptr->dump(encoder);
            }
        } doc(a);

        doc.dump_csv(std::cout);
    }
}
