#include <iostream>
#include <stdio.h>
#include "geojson.hh"
#include <algorithm>
#include <iomanip>

/////////////////////////////////////////////////////////////////////////////////////////////////////
//main
/////////////////////////////////////////////////////////////////////////////////////////////////////

std::vector<std::string> build_header(geojson_t & geojson) {
    std::vector<std::string> header;
    header.push_back("id");
    for (auto & el : geojson.m_feature) {
        for (auto prop : el.props) {
            if (std::find(header.begin(), header.end(), prop.name) == header.end()) {
                header.push_back(prop.name);
            }
        }
    }
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
        }
        else
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

int main(int argc, char *argv[])
{
  if (argc < 2)
  {
    std::cout << "usage : ./parser <GEOJSON file>" << std::endl;
    return 1;
  }

  geojson_t geojson;
  if (geojson.convert(argv[1]) < 0)
  {
    return 1;
  }

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
      auto h_count = header.size();
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

  return 0;

}
