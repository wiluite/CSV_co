///
/// \file   example/csv_sum.cpp
/// \author wiluite
/// \brief  Population sum.

#include <csv_co/reader.hpp>
#include <iostream>

int main() {
    using namespace csv_co;

    try {

        constexpr unsigned population_col = 3;
        auto sum = 0u;

        reader (std::filesystem::path("smallpop.csv"))
                .run_spans([](auto){}, // skip header line
                           [&](auto s) {
                               static auto col{0u};
                               if (col++ == population_col) {
                                   try {sum += s.template as<unsigned>();}
                                   catch(reader<>::exception const & e) { std::cout << e.what() << '\n'; }
                                   col = 0;
                               }
                           });

        std::cout << "Total population is: " << sum << std::endl;
    } catch (reader<>::exception const & e) {
        std::cout << e.what() << '\n';
    }

}
