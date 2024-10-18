///
/// \file   benchmark/validatebench.cpp
/// \author wiluite
/// \brief  A CSV source validation benchmark.

#include <csv_co/reader.hpp>
#include <iostream>

int main(int argc, char ** argv) {

    if (argc != 2) {
        std::cout << "Usage: ./validatebench <csv_file>\n";
        return EXIT_FAILURE;
    }

    using namespace csv_co;

    try {
        auto const begin = std::chrono::high_resolution_clock::now();
        reader r (std::filesystem::path{argv[1]});
        auto & _ {r.validate()};
        (void)_;
        auto const end = std::chrono::high_resolution_clock::now();
        std::cout << "Execution Time : " << std::chrono::duration_cast<std::chrono::milliseconds>(end-begin).count() << " ms \n";
    } catch (reader<>::exception const & e) {
        std::cout << e.what() << '\n';
    } catch(std::exception const & e) {
        std::cout << e.what() << '\n';
    }

}
