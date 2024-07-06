///
/// \file   benchmark/castbench.cpp
/// \author wiluite
/// \brief  Measures casts to "cast_to_type" type for a certain column.

#include <csv_co/reader.hpp>
#include <iostream>

using cast_to_type = double;

int main(int argc, char ** argv) {

    if (argc != 3) {
        std::cout << "Usage: ./castbench <csv_file> <zero_based_column_number>\n";
        return EXIT_FAILURE;
    }

    using namespace csv_co;

    char * p_end;
    std::size_t col =  strtol(argv[2], &p_end, 10);
    if (argv[2] == p_end) {
        std::cout << "zero_based_column_number isn't numeric" << std::endl;
        exit(-13);
    }

    try {
        reader r (std::filesystem::path {argv[1]});
        if (r.validate().cols() <= col) {
            std::cout << "zero_based_column_number isn't found in " << argv[1] << std::endl;
            exit(-14);
        }

        for (;;) {
            auto const begin_wo = std::chrono::high_resolution_clock::now();
            r.run_rows([](auto){}, [&](auto row) {cell_string s = row[col].operator cell_string();});
            auto const end_wo = std::chrono::high_resolution_clock::now();
            std::cout << "Execution Time Without Casts:  " << std::chrono::duration_cast<std::chrono::milliseconds>(end_wo-begin_wo).count() << " ms" << '\n';

            auto const begin_w = std::chrono::high_resolution_clock::now();
            r.run_rows([](auto){}, [&](auto row) {  row[col].template as<cast_to_type>();});
            auto const end_w = std::chrono::high_resolution_clock::now();
            std::cout << "Execution Time With Casts:     " << std::chrono::duration_cast<std::chrono::milliseconds>(end_w-begin_w).count() << " ms" << '\n';

            if (std::chrono::duration_cast<std::chrono::microseconds>((end_w-begin_w)-(end_wo-begin_wo)).count() < 0) {
                std::cout << "Intervention of external effects. Repeat." << '\n';
                continue;
            }
            std::cout << "Execution Time Of All Casts:   " << std::chrono::duration_cast<std::chrono::microseconds>((end_w-begin_w) - (end_wo-begin_wo)).count() << " mks" << std::endl;

            std::cout << "Mean Execution Time Of 1 Cast: " << std::chrono::duration_cast<std::chrono::nanoseconds>((end_w-begin_w) - (end_wo-begin_wo)).count() / (r.rows() - 1) << " ns" << std::endl;
            break;
        }

    } catch (reader<>::exception const & e) {
        std::cout << e.what() << std::endl;
    }

}
