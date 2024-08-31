#pragma once
#include <iostream>

struct cin_subst {
    explicit cin_subst(std::istream & to) {
        cin_buf = std::cin.rdbuf();
        std::cin.rdbuf(to.rdbuf());
    }
    ~cin_subst() {
        std::cin.rdbuf(cin_buf);
    }
private:
    std::streambuf* cin_buf;
};
