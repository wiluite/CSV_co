#pragma once
#include <iostream>

struct stdin_subst {
    explicit stdin_subst(std::istream & to) {
        cin_buf = std::cin.rdbuf();
        std::cin.rdbuf(to.rdbuf());
    }
    ~stdin_subst() {
        std::cin.rdbuf(cin_buf);
    }
private:
    std::streambuf* cin_buf;
};

struct stdin_redir {
    explicit stdin_redir(char const * const fn) {
        auto res = freopen(fn, "r", stdin);
        assert(res!=nullptr);
    }
    ~stdin_redir() {
        fclose(stdin);
        //fdopen(0,"r");
    }
};
