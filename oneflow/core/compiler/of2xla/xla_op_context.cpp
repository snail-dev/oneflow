#include "tensorflow/compiler/xla/status.h"
#include "tensorflow/compiler/xla/shape.h"
#include "tensorflow/compiler/xla/client/xla_builder.h"
#include "oneflow/core/compiler/of2xla/xla_shape.h"
#include "oneflow/core/compiler/of2xla/xla_utility.h"
#include "oneflow/core/compiler/of2xla/xla_argument.h"
#include "oneflow/core/compiler/of2xla/xla_op_context.h"

namespace oneflow {
namespace mola {

xla::Shape GetXlaOpShape(const xla::XlaOp &handle) {
  CHECK(!handle.IsUninitialized()) << "XlaOp has not been initialized.";
  xla::StatusOr<xla::Shape> shape = handle.builder()->GetShape(handle);
  return shape.ValueOrDie();
}

XlaOprand XlaOprand::Constant(xla::Shape shape) {
  XlaOprand op;
  op.shape_ = shape;
  op.initialized_ = true;
  return op;
}

XlaOprand XlaOprand::XlaOp(xla::XlaOp handle) {
  XlaOprand op;
  op.handle_ = handle;
  op.shape_ = GetXlaOpShape(handle);
  op.initialized_ = true;
  return op;
}

xla::XlaOp XlaOprand::AsXlaOp(xla::XlaBuilder *builder) {
  CHECK(initialized_) << "XlaOprand has not been initialized.";
  if (handle_.IsUninitialized()) {
//    LiteralSlice literal;
//    HostTensorToBorrowingLiteral(constant_value_, &literal);
//    return xla::ConstantLiteral(builder, literal);
  }
  CHECK_EQ(builder, handle_.builder())
      << "Mismatched builders in XlaOprand::AsXlaOp";
  return handle_;
}

xla::XlaBuilder *XlaOpContext::Builder() const { return param_.builder; }

xla::XlaOp XlaOpContext::Input(const std::string &name) {
  return Input(ArgumentFromString(name));
}

xla::XlaOp XlaOpContext::Output(const std::string &name) {
  return Output(ArgumentFromString(name));
}

xla::XlaOp XlaOpContext::Input(const Argument &arg) {
  DCHECK_GT(param_.inputs.count(arg), 0);
  return param_.inputs[arg].AsXlaOp(Builder());
}

xla::XlaOp XlaOpContext::Output(const Argument &arg) {
  DCHECK_GT(outputs_.count(arg), 0);
  return outputs_[arg].AsXlaOp(Builder());
}

void XlaOpContext::SetOutput(const std::string &name,
                             const xla::XlaOp &handle) {
  Argument arg = ArgumentFromString(name);
  SetOutput(name, XlaOprand::XlaOp(handle));
}

void XlaOpContext::SetOutput(const std::string &name, const XlaOprand &handle) {
  Argument arg = ArgumentFromString(name);
  CHECK_EQ(OfShapeToXlaShape(arg.shape(), arg.data_type()), handle.shape_);
  outputs_[arg] = handle;
}

DataType XlaOpContext::InputType(const std::string &name) const {
  return ArgumentFromString(name).data_type();
}

DataType XlaOpContext::OutputType(const std::string &name) const {
  return ArgumentFromString(name).data_type();
}

Shape XlaOpContext::InputShape(const std::string &name) const {
  return ArgumentFromString(name).shape();
}

Shape XlaOpContext::OutputShape(const std::string &name) const {
  return ArgumentFromString(name).shape();
}

Argument XlaOpContext::ArgumentFromString(const std::string &name) const {
  return param_.arguments.at(name);
}

}  // namespace mola
}  // namespace oneflow
