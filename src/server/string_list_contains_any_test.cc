#include "string_list_contains_any.h"

#include <arrow/testing/gtest_util.h>
#include <gtest/gtest.h>

namespace seqr {
namespace cp = arrow::compute;

TEST(TestStringListContainsAny, BasicTests) {
  // Don't clobber the global registry.
  const auto registry = cp::FunctionRegistry::Make();
  ASSERT_OK(RegisterStringListContainsAny(registry.get()));

  // TODO(@lgruen): add call
}

}  // namespace seqr
