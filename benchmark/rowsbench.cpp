///
/// \file   benchmark/rowsbench.cpp
/// \author wiluite
/// \brief  Rows iteration mode benchmark.

#include <csv_co/reader.hpp>
#include <iostream>

int main(int argc, char ** argv) {

    if (argc != 2) {
        std::cout << "Usage: ./rowsbench <csv_file>\n";
        return EXIT_FAILURE;
    }

    using namespace csv_co;

    std::size_t num_exp = 5, accum_times = 0;

    try {
        auto h_rows  {0u};
        auto v_rows  {0u};
        auto h_cells {0u};
        auto v_cells {0u};

        auto const save_num_exp = num_exp;

        while (num_exp--) {
            auto const begin = std::chrono::high_resolution_clock::now();
            h_rows = v_rows = h_cells = v_cells = 0;

            reader r (std::filesystem::path{argv[1]});
            r.run_rows([&](auto &rs) {
                ++h_rows;
                for (auto & e : rs) {(void)e; ++h_cells;}
            },[&](auto &rs) {
                ++v_rows;
                for (auto & e : rs) {(void)e; ++v_cells;}
            });

            auto const end = std::chrono::high_resolution_clock::now();
            accum_times += std::chrono::duration_cast<std::chrono::milliseconds>(end-begin).count();
        }

        std::cout << "Header Rows    : " << h_rows  << '\n' <<
                     "Value Rows     : " << v_rows  << '\n' <<
                     "Header Cells   : " << h_cells << '\n' <<
                     "Value Cells    : " << v_cells << '\n';

        std::cout << "Execution Time : " << accum_times/save_num_exp << " ms" << '\n';
    }
    catch (reader<>::exception const & e) {
        std::cout << e.what() << std::endl;
    }

}
