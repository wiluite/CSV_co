///
/// \file   benchmark/spanbench.cpp
/// \author wiluite
/// \brief  Span iteration mode benchmark.

#include <csv_co/reader.hpp>
#include <iostream>

int main(int argc, char ** argv) {

    if (argc != 2) {
        std::cout << "Usage: ./spanbench <csv_file>\n";
        return EXIT_FAILURE;
    }

    using namespace csv_co;

    std::size_t num_exp = 5;
    std::size_t accum_times = 0;

    try
    {

        auto h_cells{0u};
        auto v_cells{0u};
        auto rows {0u};

        auto const save_num_exp = num_exp;

        while (num_exp--) {
            auto const begin = std::chrono::high_resolution_clock::now();
            h_cells = v_cells = rows = 0;

            reader r (std::filesystem::path {argv[1]});
            r.run_spans([&h_cells](auto ) { ++h_cells; },
                        [&v_cells](auto ) { ++v_cells; },
                        [&rows] { ++rows; });

            auto const end = std::chrono::high_resolution_clock::now();
            accum_times += std::chrono::duration_cast<std::chrono::milliseconds>(end-begin).count();
        }

        std::cout << "Total Rows     : " << rows << '\n' <<
                     "Header Cells   : " << h_cells << '\n' <<
                     "Value Cells    : " << v_cells << '\n';

        std::cout << "Execution Time : " << accum_times/save_num_exp << " ms" << '\n';

    } catch (reader<>::exception const & e) {
        std::cout << e.what() << std::endl;
    }

}
