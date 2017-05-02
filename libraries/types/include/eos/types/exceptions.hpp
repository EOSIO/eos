/*
 * Copyright (c) 2017, Respective Authors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#pragma once

#include <fc/exception/exception.hpp>
#include <eos/utilities/exception_macros.hpp>

namespace eos { namespace types {

   FC_DECLARE_EXCEPTION(type_exception, 4000000, "type exception")
   FC_DECLARE_DERIVED_EXCEPTION(unknown_type_exception, type_exception,
                                4010000, "Could not find requested type")
   FC_DECLARE_DERIVED_EXCEPTION(duplicate_type_exception, type_exception,
                                4020000, "Requested type already exists")
   FC_DECLARE_DERIVED_EXCEPTION(invalid_type_name_exception, type_exception,
                                4030000, "Requested type name is invalid")
   FC_DECLARE_DERIVED_EXCEPTION(invalid_field_name_exception, type_exception,
                                4040000, "Requested field name is invalid")
   FC_DECLARE_DERIVED_EXCEPTION(invalid_schema_exception, type_exception,
                                4050000, "Schema is invalid")

} } // eos::chain
