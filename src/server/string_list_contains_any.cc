#include "string_list_contains_any.h"

#include <absl/container/flat_hash_set.h>
#include <arrow/compute/exec.h>
#include <arrow/compute/function.h>
#include <arrow/compute/kernel.h>
#include <arrow/compute/registry.h>
#include <arrow/type.h>
#include <arrow/util/string_view.h>
#include <arrow/visitor_inline.h>

namespace seqr {
namespace cp = arrow::compute;
namespace {

arrow::Status ExecStringListContainsAny(cp::KernelContext* const ctx,
                                        const cp::ExecBatch& batch,
                                        arrow::Datum* const out) {
  // First, create a hash set of all the strings in the second input array.
  absl::flat_hash_set<arrow::util::string_view> value_set;
  if (const auto status = arrow::VisitArrayDataInline<arrow::StringType>(
          *batch[1].array(),
          [&value_set](const arrow::util::string_view sv) {
            value_set.insert(sv);
            return arrow::Status::OK();
          },
          [] { return arrow::Status::OK(); });
      !status.ok()) {
    return status;
  }

  // TODO(@lgruen): implement this.

  return arrow::Status::NotImplemented(
      "ExecStringListContainsAny not implemented");
}

}  // namespace

// See Arrow's scalar_set_lookup.cc and scalar_nested.cc for reference.
arrow::Status RegisterStringListContainsAny() {
  auto string_list_contains_any = std::make_shared<cp::ScalarFunction>(
      "string_list_contains_any", cp::Arity::Binary(), nullptr);
  if (const auto status = string_list_contains_any->AddKernel(
          {arrow::list(arrow::utf8()), arrow::utf8()}, arrow::boolean(),
          ExecStringListContainsAny);
      !status.ok()) {
    return status;
  }
  return cp::GetFunctionRegistry()->AddFunction(
      std::move(string_list_contains_any));
}

}  // namespace seqr
