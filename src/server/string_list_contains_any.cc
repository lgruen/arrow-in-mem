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
      static_cast<const StringListContainsAnyState*>(ctx->state());
  const auto& value_set = state->value_set;

  // To understand the layout of an array of list of strings, see the following
  // sections and particularly the List<List<Int8>> example (where strings would
  // use char instead of Int8).
  // https://arrow.apache.org/docs/format/Columnar.html#variable-size-list-layout
  // https://arrow.apache.org/docs/format/Columnar.html#variable-size-binary-layout

  /*
    arrow::ArrayData* const output = out->mutable_array();
    arrow::internal::FirstTimeBitmapWriter writer{
        output->buffers[1]->mutable_data(), output->offset, output->length};

    const arrow::ListArray list{batch[0].array()};
    const auto string_array =
        std::static_pointer_cast<arrow::StringArray>(list.values());
    const std::string* const string_values = string_array->raw_values();

    const ArrayData& input = *batch[0].array();
    ArrayData* out_arr = out->mutable_array();
    if (input.length > 0) {
      transform(reinterpret_cast<const offset_type*>(input.buffers[1]->data()) +
                    input.offset,
                input.buffers[2]->data(), input.length, out_arr->offset,
                out_arr->buffers[1]->mutable_data());
    }

    const offset_type* offsets =
        reinterpret_cast<const offset_type*>(raw_offsets);
    FirstTimeBitmapWriter bitmap_writer(output, output_offset, length);
    for (int64_t i = 0; i < length; ++i) {
      const char* current_data = reinterpret_cast<const char*>(data +
  offsets[i]); int64_t current_length = offsets[i + 1] - offsets[i]; if
  (matcher->Match(util::string_view(current_data, current_length))) {
        bitmap_writer.Set();
      }
      bitmap_writer.Next();
    }
    bitmap_writer.Finish();
  },
          out);

  std::static_pointer_cast<arrow::Int64Array>(array);
  const auto* const offsets = list.raw_value_offsets();
  arrow::internal::VisitBitBlocksVoid(
      list.data()->buffers[0], list.offset(), list.length(),
      [&value_set, &writer, offsets](const int64_t position) {
        const auto begin = offsets[position];
        const auto end = offsets[position + 1];
        for (auto i = offsets[position], end = offsets[position + 1]; i < end;
             ++i) {
          if (value_set.contains(string_values[i])) {
            writer.Set();
            writer.Next();
            return;
          }
        }
        writer.Clear();
        writer.Next();
      },
      [&writer]() {
        writer.Clear();
        writer.Next();
      });

  writer.Finish();

  typename TypeTraits<Type>::ArrayType list(batch[0].array());
  ArrayData* const out_arr = out->mutable_array();
  auto out_values = out_arr->GetMutableValues<offset_type>(1);
  const offset_type* offsets = list.raw_value_offsets();
  ::arrow::internal::VisitBitBlocksVoid(
      list.data()->buffers[0], list.offset(), list.length(),
      [&](int64_t position) {
        *out_values++ = offsets[position + 1] - offsets[position];
      },
      [&]() { *out_values++ = 0; });

  template <typename ValidFunc, typename NullFunc>
  static void VisitVoid(const ArrayData& arr, ValidFunc&& valid_func,
                        NullFunc&& null_func) {
    using c_type = typename T::c_type;
    const c_type* data = arr.GetValues<c_type>(1);
    auto visit_valid = [&](int64_t i) { valid_func(data[i]); };
    VisitBitBlocksVoid(arr.buffers[0], arr.offset, arr.length,
                       std::move(visit_valid),
                       std::forward<NullFunc>(null_func));
  */

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
