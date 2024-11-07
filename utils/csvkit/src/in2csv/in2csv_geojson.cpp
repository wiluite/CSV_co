#include "../../include/in2csv/in2csv_geojson.h"
#include <cli.h>
#include "../../external/lib_geojson/geojson.hh"
#include <iostream>
#include <sstream>

using namespace ::csvkit::cli;

namespace in2csv::detail::geojson {
    namespace detail {
        std::vector<std::string> build_header(geojson_t & geojson) {
            std::vector<std::string> header;
            header.emplace_back("id");
            for (auto & el : geojson.m_feature)
                for (auto & prop : el.props)
                    if (std::find(header.begin(), header.end(), prop.name) == header.end())
                        header.push_back(prop.name);
            header.insert(header.end(), {"geojson", "type", "longitude", "latitude"});
            return header;
        }

        std::string json_value_2_csv(std::string const& s) {
            bool paired_quote = true;
            bool has_comma = false;
            unsigned quotes = 0;
            for (auto ch : s) {
                if (ch == '"') {
                    paired_quote = !paired_quote;
                    quotes++;
                } else
                if (ch == ',') {
                    if (paired_quote) {
                        std::ostringstream oss;
                        oss << std::quoted(s, '"', '"');
                        return oss.str();
                    } else {
                        if (!has_comma)
                            has_comma = true;
                    }
                }
            }

            if (quotes == 2 and !has_comma and s.front() == '"' and s.back() == '"')
                return {s.cbegin() + 1, s.cend() - 1};
            if (quotes == 0 and !has_comma)
                return s;
            std::ostringstream oss;
            oss << std::quoted(s, '"', '"');
            assert(oss.str() != s);
            return oss.str();
        }
    } // namespace detail

    void impl::convert() {
        using namespace detail;
        geojson_t geojson;
        if (a.file.empty() or a.file == "_") {
            std::string stream;
            _setmode(_fileno(stdin), _O_BINARY);
            for (;;) {
                if (auto r = std::cin.get(); r != std::char_traits<char>::eof())
                    stream += r;
                else
                    break;
            }
            geojson.convert_stream(stream);
        } else
            geojson.convert_file(a.file.string().c_str());

        auto header = build_header(geojson);
        auto h_count = header.size();
        for (auto & e : header) {
            std::cout << e;
            if (--h_count)
                std::cout << ',';
        }
        std::cout << '\n';

        auto geojson_it = std::find(std::cbegin(header), std::cend(header), "geojson");
        assert(geojson_it != header.cend());
        for (auto & feature : geojson.m_feature) {
            //print properties
            //auto h_count = header.size();
            std::for_each(header.cbegin(), geojson_it, [&](auto & field) {
                for (auto & prop : feature.props) {
                    if (field == prop.name) {
                        std::cout << json_value_2_csv(prop.value);
                        break;
                    }
                }
                std::cout << ',';
            });
            // print geojson
            std::cout << std::quoted(feature.gjson, '"','"') << ',';
            // print rest
            auto & geom = feature.m_geometry[feature.m_geometry.size()-1];
            std::cout << geom.m_type << ',';
            if (geom.m_type == "Point") {
                auto & polyg = geom.m_polygons[geom.m_polygons.size()-1];
                std::cout << polyg.m_coord[polyg.m_coord.size()-1].x << ','
                    << polyg.m_coord[polyg.m_coord.size()-1].y;
            } else
                std::cout << ',';
            std::cout << '\n';
        }
    }
}
