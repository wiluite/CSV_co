///
/// \file   test/test_runner_macros.h
/// \author wiluite
/// \brief  Tests for the csvstat utility.

#pragma once

#define test_reader_configurator_and_runner(arguments, function)                   \
        std::variant<std::monostate                                                \
                   , std::reference_wrapper<notrimming_reader_type>                \
                   , std::reference_wrapper<skipinitspace_reader_type>> variants;  \
        notrimming_reader_type r(std::filesystem::path{arguments.file});           \
        recode_source(r, arguments);                                               \
        variants = std::ref(r);                                                    \
        std::visit([&](auto & arg) { function(arg, arguments);}, variants);
