#pragma once

#include "converter_client.h"
#include <memory>
#include <filesystem>
#if 0
#include <vector>
#endif

namespace in2csv::detail {

    namespace geojson {
        struct impl_args {
            unsigned maxfieldsize;
            std::string encoding; // always UTF-8
            bool skip_init_space;
#if 0
            bool no_header;
            unsigned skip_lines;
            bool linenumbers;
            bool zero;
            std::string num_locale;
            bool blanks;
            mutable std::vector<std::string> null_value;
            std::string date_fmt;
            std::string datetime_fmt;
            bool no_leading_zeroes;
            bool no_inference;
            bool date_lib_parser;
            bool asap;
#endif
            std::filesystem::path file;
        };

        struct impl {
            explicit impl(impl_args a) : a(std::move(a)) {}
            void convert();
        private:
            impl_args a;
        };
        static std::shared_ptr<impl> pimpl;
    }

    template <class Args2>
    struct geojson_client final : converter_client {
        explicit geojson_client(Args2 & args) {
            geojson::pimpl = std::make_shared<geojson::impl> (geojson::impl_args(
                args.maxfieldsize
                , args.encoding
                , args.skip_init_space
#if 0
                , args.no_header
                , args.skip_lines
                , args.linenumbers
                , args.zero
                , args.num_locale
                , args.blanks
                , args.null_value
                , args.date_fmt
                , args.datetime_fmt
                , args.no_leading_zeroes
                , args.no_inference
                , args.date_lib_parser
                , args.asap
#endif
                , args.file));
        }
        void convert() override {
            geojson::pimpl->convert();
        }
        static std::shared_ptr<converter_client> create(Args2 & args) {
            return std::make_shared<geojson_client>(args);
        }
    };

}
