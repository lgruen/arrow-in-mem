#pragma once

#include <arrow/status.h>

namespace seqr {

// Call this function once at startup time to register the Arrow compute
// function "string_list_contains_any".
arrow::Status RegisterStringListContainsAny();

}  // namespace seqr