///
/// \file   example/csv_2_matrix.cpp
/// \author wiluite
/// \brief  Copy a CSV file to an MxN memory table.

#include <csv_co/reader.hpp>
#include <iostream>

using namespace csv_co;

namespace {
    template <class R>
    class tableMxN {
    public:
        using element_type = typename R::cell_span;
    public:
        std::vector<std::vector<element_type>> impl;
    public:
        explicit tableMxN(R & reader) : impl { reader.rows() } {
            auto const cols = reader.cols();
            for (auto & elem : impl)
                elem.resize(cols);
        }
        decltype (auto) operator[](size_t r) {
            return impl[r];
        }
        [[nodiscard]] auto begin() const { return impl.cbegin(); }
        [[nodiscard]] auto end() const { return impl.cend(); }
        auto begin() { return impl.begin(); }
        auto end() { return impl.end(); }
    };
}

int main() {
    try {
        reader<trim_policy::alltrim> r (std::filesystem::path ("smallpop.csv"));

        tableMxN table(r);
        auto c_row {-1}; // will be incremented automatically
        auto c_col {0u};

        // ignore header fields, obtain value fields, and trace rows:
        r.run_spans(
                [](auto &) {},
                [&](auto & s) { table[c_row][c_col++] = s; },
                [&] {
                    c_row++;
                    c_col = 0;
                }
        );

        // population of Southborough,MA:
        std::cout << table[0][0].operator unquoted_cell_string() << ',' << table[0][1].operator unquoted_cell_string() << ':' << table[0][3].operator unquoted_cell_string() << '\n';

#if 0
        // Print all table

        for (auto const & row : table) {
            for (auto const & elem : row)
                std::cout << elem << ' ';
            std::cout << '\n';
        }
#endif

    } catch (std::exception const & e) {
        std::cout << e.what() << std::endl;
    }

}

