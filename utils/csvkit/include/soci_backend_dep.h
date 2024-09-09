//
// Created by wiluite on 09.09.2024.
//

#ifndef CSV_CO_SOCI_BACKEND_DEP_H
#define CSV_CO_SOCI_BACKEND_DEP_H
#include <string>
#if !defined(__unix__)

struct soci_backend_dependency {
    soci_backend_dependency() {
        std::string path = getenv("PATH");
#if !defined(BOOST_UT_DISABLE_MODULE)
        path = "PATH=" + path + R"(;..\..\external_deps)";
#else
        path = "PATH=" + path + R"(;..\..\..\external_deps)";
#endif
#if !defined(_MSC_VER)
        putenv(path.c_str());
#else
        _putenv(path.c_str());
#endif
    }
};
#endif

#endif //CSV_CO_SOCI_BACKEND_DEP_H
