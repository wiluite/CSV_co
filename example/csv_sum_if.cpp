///
/// \file   example/csv_sum_if.cpp
/// \author wiluite
/// \brief  Population sum in CA where the latitude is less than 34.

#include <csv_co/reader.hpp>
#include <iostream>

int main() {
    using namespace csv_co;
    using reader_type = reader<trim_policy::alltrim>;

    try {

        auto sum = 0ul;

        reader_type r(std::filesystem::path("uspop.csv"));

        r.validate().
        run_rows(
                [](auto){},
                [&sum](reader_type::row_span rs) {
                    if (rs["State"] == "CA" and !rs["Population"].empty() and rs["Latitude"].as<float>() < 34.0)
                        sum += rs["Population"].as<unsigned long long>();
                }
        );

        // Alternatively:

//        r.run_rows([](auto){},
//                   [&sum](auto row) {
//                       if (row["State"]=="CA" and row["Population"]!="" and row["Latitude"]. template as<float>()<34.0)
//                           sum += row["Population"]. template as<unsigned long long>();
//                   });

        std::cout << "Total population by condition is: " << sum << std::endl;

    } //catch (reader<>::exception const & e)
    catch (reader_type::exception const & e) {
        std::cout << e.what() << '\n';
    }

}
