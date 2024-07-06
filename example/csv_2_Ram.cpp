///
/// \file   example/csv_2_Ram.cpp
/// \author wiluite
/// \brief  Copy a CSV file to a vector.

#include <csv_co/reader.hpp>
#include <filesystem>
#include <iostream>
#include <vector>

int main() {
    using namespace csv_co;
    using reader_type = reader<trim_policy::alltrim>;

    try {
        reader_type r(std::filesystem::path("smallpop.csv"));
        std::vector<decltype(r)::cell_span> ram;
        ram.reserve(r.cols() * r.rows());
        r.validate().run_spans( // check validity and run
                [](auto) {
                    // ignore header fields
                }, [&ram](auto s) {
                    // save value fields
                    ram.emplace_back(s);
                });

        // population of Southborough,MA:
        std::cout << ram[0].operator unquoted_cell_string() << ',' << ram[1].operator unquoted_cell_string() << ':' << ram[3].operator unquoted_cell_string() << '\n';
    } catch (reader_type::exception const & e) {
        std::cout << e.what() << '\n';
    }
}

