#pragma once

#include "converter_client.h"
#include <memory>
#include <filesystem>

namespace in2csv::detail {

    namespace dbf {
        struct impl_args {
            bool linenumbers;
            std::string encoding;
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
    struct dbf_client final : converter_client {
        explicit dbf_client(Args2 & args) {
            dbf::pimpl = std::make_shared<dbf::impl>
                (dbf::impl_args(args.linenumbers, args.encoding, args.file));
        }
        void convert() override {
            dbf::pimpl->convert();
        }
        static std::shared_ptr<converter_client> create(Args2 & args) {
            return std::make_shared<dbf_client>(args);
        }
    };

}
