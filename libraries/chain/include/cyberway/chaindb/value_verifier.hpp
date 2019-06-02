#pragma once

#include <memory>

namespace cyberway { namespace chaindb {

    class driver_interface;

    struct value_verifier_impl;
    struct table_info;
    struct object_value;

    class value_verifier final {
    public:
        value_verifier(driver_interface&);
        ~value_verifier();

        void verify(const table_info&, const object_value&) const;

    private:
        std::unique_ptr<value_verifier_impl> impl_;
    }; // struct value_verifier;

}} // namespace cyberway::chaindb