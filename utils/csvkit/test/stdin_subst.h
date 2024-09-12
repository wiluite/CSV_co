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
#ifndef _MSC_VER
        strm = freopen(fn, "r", stdin);
        assert(strm != nullptr);
#else
        auto err = freopen_s(&strm, fn, "r", stdin);
        assert(err == 0);
#endif
    }
    ~stdin_redir() {
        fclose(strm);
        fclose(stdin);
    }
private:
    FILE *strm = nullptr;
};
