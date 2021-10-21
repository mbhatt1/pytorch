/**
 * This is a handwritten file that accompanies codegenerated header
 * LazyShapeDtype.h
 *
 * The purpose of these shape/dtype inference methods are to fill gaps
 * where we do not yet have structured kernels in pytorch core.  Ops
 * for which there _are_ structured kernels can use meta::op() to infer
 * shape/dtype, and codegen makes use of this.  Ops for which there are not
 * yet structured kernels can still be used with lazy_tensor codegen, but require
 * manual intervention to implement compute_shape_{op} and compute_dtype_{op}.
 *
 * READ THIS!
 *
 * 1. Beware: Tech Debt!
 * ---------------------
 * These functions are tech debt.  We want to delete them all and use structured
 * kernels instead, but it's a lot faster to write these so we're decoupling the
 * two efforts to move fast for adding support for codegenned Lazy Tensor ops.
 *
 * Codegenned Lazy Tensor ops with handwritten shape formulae are still better than
 * fully handwritten Lazy Tensor ops (which also have handwritten shape formulae).
 *
 * 2. Structured Kernels For The Win
 * ---------------------------------
 * Long term, more and more ops should be supported as 'structured kernels'.  Consider
 * doing your part and porting an op.  As ops get ported over, the codegen will automatically
 * notice and stop generating declarations for these shape formulae, so we'll need to
 * manually clean up the unused functions in this file, or somehow automate that.
 *
 * https://dev-discuss.pytorch.org/t/slides-from-structured-kernel-presentation/179
 *
 * 3. How to figure out the shape/dtype
 * ------------------------------------
 * Unfortunatley there isn't a one-stop-shop for learning the output shape formulae for all
 * operators.  This is partly becuase some operators are not part of our 'public' API, including
 * backward operators which users don't directly invoke.
 *
 * Check our opinfo registry:
 *  https://github.com/pytorch/pytorch/blob/13b859983183ea9938deb5030ac9a0747841f0a8/torch/csrc/jit/runtime/symbolic_shape_registry.cpp
 *
 * Read the manual (for ops that are 1:1 with python frontend):
 *  https://pytorch.org/docs/stable/generated/torch.trace.html
 *
 */

#include "lazy_tensor_core/csrc/ts_backend/LazyShapeDtype.h"

#include "torch/csrc/api/include/torch/enum.h"
namespace torch_lazy_tensors{
namespace ir {
namespace ops {


std::vector<std::vector<int64_t>> compute_shape_bitwise_and(const at::Tensor& self, const at::Scalar& other) {
  return {self.sizes().vec()};
}

std::vector<c10::ScalarType> compute_dtype_bitwise_and(const at::Tensor& self, const at::Scalar& other) {
  return {self.scalar_type()};
}

std::vector<std::vector<int64_t>> compute_shape_mean(const at::Tensor& self, c10::optional<at::ScalarType> dtype) {
  return {{}};
}

std::vector<c10::ScalarType> compute_dtype_mean(const at::Tensor& self, c10::optional<at::ScalarType> dtype) {
  if (dtype.has_value()) {
    return {dtype.value()};
  }
  return {self.scalar_type()};
}

std::vector<std::vector<int64_t>> compute_shape_mv(const at::Tensor& self, const at::Tensor& vec) {
  return {{self.size(0)}};
}

std::vector<c10::ScalarType> compute_dtype_mv(const at::Tensor& self, const at::Tensor& vec) {
  return {self.scalar_type()};
}

std::vector<std::vector<int64_t>> compute_shape_native_batch_norm(const at::Tensor & input, const c10::optional<at::Tensor> & weight, const c10::optional<at::Tensor> & bias, const c10::optional<at::Tensor> & running_mean, const c10::optional<at::Tensor> & running_var, bool training, double momentum, double eps) {
  if (running_mean.has_value() && running_var.has_value()) {
    return {input.sizes().vec(), running_mean.value().sizes().vec(), running_var.value().sizes().vec()};
  } else if (running_mean.has_value() || running_var.has_value()) {
    LTC_ERROR() << "Unexpected case, running_mean or running_var but not both";
  } else {
    // input shape is assumed [N, C, H, W] and Batch Norm is defined as operating over C,
    // so mean, var have shape of [C]
    return {input.sizes().vec(), {input.sizes().vec()[1]}, {input.sizes().vec()[1]}};
  }
}

std::vector<c10::ScalarType> compute_dtype_native_batch_norm(const at::Tensor & input, const c10::optional<at::Tensor> & weight, const c10::optional<at::Tensor> & bias, const c10::optional<at::Tensor> & running_mean, const c10::optional<at::Tensor> & running_var, bool training, double momentum, double eps) {
  if (running_mean.has_value() && running_var.has_value()) {
    return {input.scalar_type(), running_mean.value().scalar_type(), running_var.value().scalar_type()};
  } else if (running_mean.has_value() || running_var.has_value()) {
    LTC_ERROR() << "Unexpected case, running_mean or running_var but not both";
  } else {
    return {input.scalar_type(), input.scalar_type(), input.scalar_type()};
  }
}

std::vector<std::vector<int64_t>> compute_shape_native_batch_norm_backward(const at::Tensor & grad_out, const at::Tensor & input, const c10::optional<at::Tensor> & weight, const c10::optional<at::Tensor> & running_mean, const c10::optional<at::Tensor> & running_var, const c10::optional<at::Tensor> & save_mean, const c10::optional<at::Tensor> & save_invstd, bool train, double eps, ::std::array<bool,3> output_mask) {
  LTC_CHECK(weight.has_value()) << "Not sure what to do if weight is undefined";
  return {input.sizes().vec(), weight.value().sizes().vec(), weight.value().sizes().vec()};
}

std::vector<c10::ScalarType> compute_dtype_native_batch_norm_backward(const at::Tensor & grad_out, const at::Tensor & input, const c10::optional<at::Tensor> & weight, const c10::optional<at::Tensor> & running_mean, const c10::optional<at::Tensor> & running_var, const c10::optional<at::Tensor> & save_mean, const c10::optional<at::Tensor> & save_invstd, bool train, double eps, ::std::array<bool,3> output_mask) {
  // Autograd seems to always save a weight tensor even if it was nullopt in forward.
  // but, it may be 'undefined' - so .defined() is the real source of truth here
  if (weight.has_value() && weight.value().defined()){
    // Following implementation in aten/src/ATen/native/Normalization.cpp : batch_norm_backward_cpu_template()
    return {input.scalar_type(), weight.value().scalar_type(), input.scalar_type()};
  }

  // if weight has no value, I don't think gradient to weight matters; but we still have to provide a valid
  // scalartype or lazy tensor won't be happy
  return {input.scalar_type(), input.scalar_type(), input.scalar_type()};
}

std::vector<std::vector<int64_t>> compute_shape_native_layer_norm(const at::Tensor & input,
    at::IntArrayRef normalized_shape, const c10::optional<at::Tensor> & weight, const c10::optional<at::Tensor> & bias,
    double eps) {
  // Copied from aten/src/ATen/native/layer_norm.cpp::layer_norm_cpu_out.
  auto input_shape = input.sizes().vec();
  const size_t axis = input.dim() - normalized_shape.size();

  std::vector<int64_t> stat_shape;
  for (const auto idx : c10::irange(axis)) {
    stat_shape.emplace_back(input_shape[idx]);
  }
  for (const auto idx : c10::irange(axis, input.dim())) {
    (void)idx; // Suppress unused variable warning
    stat_shape.emplace_back(1);
  }

  return {std::move(input_shape), stat_shape, std::move(stat_shape)};
}

std::vector<c10::ScalarType> compute_dtype_native_layer_norm(const at::Tensor & input, at::IntArrayRef normalized_shape,
    const c10::optional<at::Tensor> & weight, const c10::optional<at::Tensor> & bias, double eps) {
  return {input.scalar_type(), input.scalar_type(), input.scalar_type()};
}

std::vector<std::vector<int64_t>> compute_shape_native_layer_norm_backward(const at::Tensor& grad_out,
    const at::Tensor& input, at::IntArrayRef normalized_shape, const at::Tensor& mean, const at::Tensor& rstd,
    const c10::optional<at::Tensor>& weight, const c10::optional<at::Tensor>& bias, ::std::array<bool,3> output_mask) {
  std::vector<std::vector<int64_t>> shapes;
  shapes.push_back(output_mask[0] ? input.sizes().vec() : std::vector<int64_t>{});
  shapes.push_back(output_mask[1] && weight ? weight->sizes().vec() : std::vector<int64_t>{});
  shapes.push_back(output_mask[2] && bias ? bias->sizes().vec() : std::vector<int64_t>{});
  return shapes;
}

std::vector<c10::ScalarType> compute_dtype_native_layer_norm_backward(const at::Tensor& grad_out,
    const at::Tensor& input, at::IntArrayRef normalized_shape, const at::Tensor& mean, const at::Tensor& rstd,
    const c10::optional<at::Tensor>& weight, const c10::optional<at::Tensor>& bias, ::std::array<bool,3> output_mask)
{
  std::vector<c10::ScalarType> dtypes;
  dtypes.push_back(input.scalar_type());
  dtypes.push_back(weight && weight->defined() ? weight->scalar_type() : input.scalar_type());
  dtypes.push_back(bias && weight->defined() ? bias->scalar_type() : input.scalar_type());
  return dtypes;
}

std::vector<std::vector<int64_t>> compute_shape_smooth_l1_loss(
    const at::Tensor& self, const at::Tensor& target, int64_t reduction,
    double beta) {
  // Taken from definition of 'Output' shape here:
  // https://pytorch.org/docs/stable/generated/torch.nn.SmoothL1Loss.html
  switch (reduction) {
    case at::Reduction::None:
      return {self.sizes().vec()};
    default:
      return {{}};
  }
}

std::vector<c10::ScalarType> compute_dtype_smooth_l1_loss(
    const at::Tensor& self, const at::Tensor& target, int64_t reduction,
    double beta) {
  return {self.scalar_type()};
}

std::vector<std::vector<int64_t>> compute_shape_smooth_l1_loss_backward(
    const at::Tensor& grad_output, const at::Tensor& self,
    const at::Tensor& target, int64_t reduction, double beta) {
  // The `grad_output` tensor is really the input to this kernel, and while its
  // shape may vary following the logic of the forward output, the outputs of
  // this kernel should have fixed shapes matching the inputs to the forward
  // kernel.
  return {self.sizes().vec(), target.sizes().vec()};
}

std::vector<c10::ScalarType> compute_dtype_smooth_l1_loss_backward(
    const at::Tensor& grad_output, const at::Tensor& self,
    const at::Tensor& target, int64_t reduction, double beta) {
  return {self.scalar_type(), target.scalar_type()};
}

std::vector<std::vector<int64_t>> compute_shape_sum(
    const at::Tensor& self, c10::optional<at::ScalarType> dtype) {
  return {{}};
}

std::vector<c10::ScalarType> compute_dtype_sum(
    const at::Tensor& self, c10::optional<at::ScalarType> dtype) {
  if (dtype.has_value()) {
    return {dtype.value()};
  }
  // It's undocumented, but torch::sum promotes all integral types to int64 by
  // default
  if (isIntegralType(self.scalar_type(), /*includeBool*/ true)) {
    return {c10::ScalarType::Long};
  }
  return {self.scalar_type()};
  ;
}

std::vector<std::vector<int64_t>> compute_shape_trace(const at::Tensor& self) {
  return {{}};
}

std::vector<c10::ScalarType> compute_dtype_trace(const at::Tensor& self) {
  return {self.scalar_type()};
}


} // namespace ops
} // namespace ir
} // namespace torch_lazy_tensors
