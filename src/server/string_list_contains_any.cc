#include "string_list_contains_any.h"

#include <absl/container/flat_hash_set.h>
#include <arrow/compute/api_scalar.h>
#include <arrow/compute/exec.h>
#include <arrow/compute/function.h>
#include <arrow/compute/kernel.h>
#include <arrow/compute/registry.h>
#include <arrow/type.h>
#include <arrow/util/bitmap_writer.h>
#include <arrow/util/string_view.h>
#include <arrow/visitor_inline.h>

namespace seqr {
namespace cp = arrow::compute;
namespace {

struct StringListContainsAnyState : public cp::KernelState {
  arrow::Datum values;  // Keep a reference for value_set string_views.
  absl::flat_hash_set<arrow::util::string_view> value_set;
};

// Returns a StringListContainsAnyState initialized from SetLookupOptions.
arrow::Result<std::unique_ptr<cp::KernelState>> InitStringListContainsAny(
    cp::KernelContext* ctx, const cp::KernelInitArgs& args) {
  const auto* options = static_cast<const cp::SetLookupOptions*>(args.options);
  if (options->value_set.kind() != arrow::Datum::ARRAY) {
    return arrow::Status::Invalid(
        "SetLookupOptions value_set needs to be an array");
  }

  auto result = std::make_unique<StringListContainsAnyState>();
  result->values = options->value_set;

  arrow::VisitArrayDataInline<arrow::StringType>(
      *(result->values.array()),
      [&value_set = result->value_set](const arrow::util::string_view sv) {
        value_set.insert(sv);
      },
      [] {});

  return result;
}

arrow::Status ExecStringListContainsAny(cp::KernelContext* const ctx,
                                        const cp::ExecBatch& batch,
                                        arrow::Datum* const out) {
  const auto state =
      static_cast<const StringListContainsAnyState&>(*ctx->state());
  const auto& value_set = state.value_set;  // Based on SetLookupOptions.

  // The boolean output array has already been preallocated.
  // See IsIn (scalar_set_lookup.cc).
  arrow::ArrayData* const output = out->mutable_array();
  arrow::internal::FirstTimeBitmapWriter writer{
      output->buffers[1]->mutable_data(), output->offset, output->length};

  // To understand the layout of an array of list of strings, see the following
  // sections and particularly the List<List<Int8>> example (where strings would
  // use char instead of Int8).
  // https://arrow.apache.org/docs/format/Columnar.html#variable-size-list-layout
  // https://arrow.apache.org/docs/format/Columnar.html#variable-size-binary-layout
  arrow::ListArray lists(batch[0].array());
  const auto& strings = static_cast<const arrow::StringArray&>(*lists.values());
  const auto* const list_offsets = lists.raw_value_offsets();
  // See ListValueLength (scalar_nested.cc).
  arrow::internal::VisitBitBlocksVoid(
      lists.data()->buffers[0], lists.offset(), lists.length(),
      [&](const int64_t i) {
        // See BinaryJoin (scalar_string.cc).
        const auto end = list_offsets[i + 1];
        for (auto j = list_offsets[i]; j < end; ++j) {
          if (!strings.IsNull(j) && value_set.contains(strings.GetView(j))) {
            writer.Set();
            writer.Next();
            return;
          }
        }
        writer.Clear();
        writer.Next();
      },
      [&]() {
        writer.Clear();
        writer.Next();
      });

  writer.Finish();

  return arrow::Status::OK();
}

}  // namespace

arrow::Status RegisterStringListContainsAny() {
  // See Arrow's scalar_set_lookup.cc's IsIn for reference.
  cp::ScalarKernel kernel;
  kernel.init = InitStringListContainsAny;
  kernel.exec = ExecStringListContainsAny;
  kernel.null_handling = cp::NullHandling::OUTPUT_NOT_NULL;
  kernel.signature = cp::KernelSignature::Make(
      {arrow::list(arrow::utf8()), arrow::utf8()}, arrow::boolean());
  auto string_list_contains_any = std::make_shared<cp::ScalarFunction>(
      "string_list_contains_any", cp::Arity::Unary(), nullptr);
  if (const auto status = string_list_contains_any->AddKernel(kernel);
      !status.ok()) {
    return status;
  }
  return cp::GetFunctionRegistry()->AddFunction(
      std::move(string_list_contains_any));
}

}  // namespace seqr