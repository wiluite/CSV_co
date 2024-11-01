#pragma once

#include "converter_client.h"
#include <memory>
#include <filesystem>

namespace in2csv::detail {

    namespace xls {
        struct impl_args {
            unsigned maxfieldsize;
            std::string encoding; // always UTF-8
            bool skip_init_space;
            bool no_header;
            unsigned skip_lines;
            bool linenumbers;
#if 0
            bool zero;
#endif
            bool check_integrity;
            bool date_lib_parser;
            bool asap;
            bool names; // "Display sheet names from the input Excel file."
            std::string sheet; // "The name of the Excel sheet to operate on."
            std::string write_sheets; // "The names of the Excel sheets to write to files, or \"-\" to write all sheets.
            bool use_sheet_names; // "Use the sheet names as file names when --write-sheets is set."
            std::string encoding_xls;
            std::string d_xls;
            std::string dt_xls;
            std::filesystem::path file;
        };

        struct impl {
            impl(impl_args a) : a(std::move(a)) {}
            void convert();
        private:
            impl_args a;
        };
        static std::shared_ptr<impl> pimpl;
    }

    template <class Args2>
    struct xls_client final : converter_client {
        explicit xls_client(Args2 & args) {
            xls::pimpl = std::make_shared<xls::impl> (xls::impl_args(
                args.maxfieldsize
                , args.encoding
                , args.skip_init_space
                , args.no_header
                , args.skip_lines
                , args.linenumbers
                , args.check_integrity
                , args.date_lib_parser
                , args.asap
                , args.names
                , args.sheet
                , args.write_sheets
                , args.use_sheet_names
                , args.encoding_xls
                , args.d_xls
                , args.dt_xls
                , args.file));
        }
        void convert() override {
            xls::pimpl->convert();
        }
        static std::shared_ptr<converter_client> create(Args2 & args) {
            return std::make_shared<xls_client>(args);
        }
    };

}
