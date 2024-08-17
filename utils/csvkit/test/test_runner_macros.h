///
/// \file   test/test_runner_macros.h
/// \author wiluite
/// \brief  Tests for the csvstat utility.

#pragma once

#define test_runner_impl(reader_type_, args_, reader_arg_, function_)                                                   \
    reader_type_ r(reader_arg_);                                                                                        \
    recode_source(r, args_);                                                                                            \
    variants = std::ref(r);                                                                                             \
    std::visit([&](auto & arg) { function_(arg, args_);}, variants);

#define test_reader_configurator_and_runner(arguments, function)                                                        \
    try {                                                                                                               \
        std::variant<std::monostate                                                                                     \
                   , std::reference_wrapper<notrimming_reader_type>                                                     \
                   , std::reference_wrapper<skipinitspace_reader_type>> variants;                                       \
            test_runner_impl(notrimming_reader_type, arguments, std::filesystem::path{arguments.file}, function)        \
                                                                                                                        \
    } catch (notrimming_reader_type::exception const & e) {                                                             \
        std::cout << e.what() << std::endl;                                                                             \
    }                                                                                                                   \
    catch (skipinitspace_reader_type::exception const & e) {                                                            \
        std::cout << e.what() << std::endl;                                                                             \
    }                                                                                                                   \
    catch (std::exception const & e) {                                                                                  \
        std::cout << e.what() << std::endl;                                                                             \
    }                                                                                                                   \
    catch (...) {                                                                                                       \
        std::cout << "Unknown exception.\n";                                                                            \
    }

#define test_reader_configurator_and_runner2(arguments, function)                  \
        std::variant<std::monostate                                                \
                   , std::reference_wrapper<notrimming_reader_type>                \
                   , std::reference_wrapper<skipinitspace_reader_type>> variants;  \
        notrimming_reader_type r(std::filesystem::path{arguments.file});           \
        recode_source(r, arguments);                                               \
        variants = std::ref(r);                                                    \
        std::visit([&](auto & arg) { function(arg, arguments);}, variants);
