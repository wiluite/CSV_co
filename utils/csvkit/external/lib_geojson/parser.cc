#include <iostream>
#include <stdio.h>
#include "geojson.hh"
#include <algorithm>

/////////////////////////////////////////////////////////////////////////////////////////////////////
//main
/////////////////////////////////////////////////////////////////////////////////////////////////////

std::vector<std::string> build_header(geojson_t & geojson) {
    std::vector<std::string> header{{"id"}};
    for (auto & el : geojson.m_feature) {
        for (auto & prop : el.props) {
            if (std::find(header.begin(), header.end(), prop.name) == header.end())
                header.push_back(prop.name);
        }
    }
    header.insert(header.end(), {"geojson", "type", "longitude", "latitude"});
    return header;
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

  std::cout << std::endl;
  auto h = build_header(geojson);
  for (auto e : h) {
    std::cout << e << std::endl;
  }
  return 0;

  ///////////////////////////////////////////////////////////////////////////////////////
  //render geojson
  ///////////////////////////////////////////////////////////////////////////////////////

  size_t size_features = geojson.m_feature.size();
  for (size_t idx_fet = 0; idx_fet < size_features; idx_fet++)
  {
    feature_t feature = geojson.m_feature.at(idx_fet);

    size_t size_geometry = feature.m_geometry.size();
    for (size_t idx_geo = 0; idx_geo < size_geometry; idx_geo++)
    {
      geometry_t geometry = feature.m_geometry.at(idx_geo);
      size_t size_pol = geometry.m_polygons.size();

      for (size_t idx_pol = 0; idx_pol < size_pol; idx_pol++)
      {
        polygon_t polygon = geometry.m_polygons[idx_pol];
        size_t size_crd = polygon.m_coord.size();

        if (size_crd == 0)
        {
          continue;
        }

        std::vector<double> lat;
        std::vector<double> lon;

        for (size_t idx_crd = 0; idx_crd < size_crd; idx_crd++)
        {
          lat.push_back(polygon.m_coord[idx_crd].y);
          lon.push_back(polygon.m_coord[idx_crd].x);
        }

        ///////////////////////////////////////////////////////////////////////////////////////
        //render each polygon as a vector of vertices 
        ///////////////////////////////////////////////////////////////////////////////////////

        if (geometry.m_type.compare("Point") == 0)
        {

        }
        else if (geometry.m_type.compare("Polygon") == 0 ||
          geometry.m_type.compare("MultiPolygon") == 0)
        {

        }

      }  //idx_pol
    } //idx_geo
  } //idx_fet


  return 0;
}
