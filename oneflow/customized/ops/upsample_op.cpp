#include "oneflow/core/framework/framework.h"

namespace oneflow {

REGISTER_USER_OP("upsample")
    .Input("x")
    .Output("y")
    .Attr("height_scale", UserOpAttrType::kAtFloat)
    .Attr("width_scale", UserOpAttrType::kAtFloat)
    .Attr("data_format", UserOpAttrType::kAtString)
    .Attr("interpolation", UserOpAttrType::kAtString)
    .SetTensorDescInferFn([](user_op::InferContext* ctx) -> Maybe<void> {
      const user_op::TensorDesc* x_desc = ctx->TensorDesc4ArgNameAndIndex("x", 0);
      user_op::TensorDesc* y_desc = ctx->TensorDesc4ArgNameAndIndex("y", 0);
      const float height_scale = ctx->Attr<float>("height_scale");
      const float width_scale = ctx->Attr<float>("width_scale");
      if (ctx->Attr<std::string>("data_format") != "channels_first"
          || x_desc->shape().NumAxes() != 4) {
        LOG(FATAL) << "upsample only supports NCHW";
      }
      *y_desc->mut_shape() = Shape({x_desc->shape().At(0), x_desc->shape().At(1),
                                    static_cast<int32_t>(height_scale) * x_desc->shape().At(2),
                                    static_cast<int32_t>(width_scale) * x_desc->shape().At(3)});
      return Maybe<void>::Ok();
    })
    .SetBatchAxisInferFn([](user_op::BatchAxisContext* ctx) -> Maybe<void> {
      *ctx->BatchAxis4ArgNameAndIndex("y", 0) = *ctx->BatchAxis4ArgNameAndIndex("x", 0);
      return Maybe<void>::Ok();
    })
    .SetGetSbpFn([](user_op::SbpContext* ctx) -> Maybe<void> {
      ctx->NewBuilder().Split(user_op::OpArg("x", 0), 0).Split(user_op::OpArg("y", 0), 0).Build();
      return Maybe<void>::Ok();
    });

REGISTER_USER_OP("upsample_grad")
    .Input("dy")
    .Output("dx")
    .Attr("height_scale", UserOpAttrType::kAtFloat)
    .Attr("width_scale", UserOpAttrType::kAtFloat)
    .Attr("data_format", UserOpAttrType::kAtString)
    .Attr("interpolation", UserOpAttrType::kAtString)
    .SetTensorDescInferFn([](user_op::InferContext* ctx) -> Maybe<void> {
      const Shape* dy_shape = ctx->Shape4ArgNameAndIndex("dy", 0);
      Shape* dx_shape = ctx->Shape4ArgNameAndIndex("dx", 0);
      const float height_scale = ctx->Attr<float>("height_scale");
      const float width_scale = ctx->Attr<float>("width_scale");
      if (ctx->Attr<std::string>("data_format") != "channels_first" || dy_shape->NumAxes() != 4) {
        LOG(FATAL) << "upsample_nearest only supports NCHW";
      }
      *dx_shape = Shape({dy_shape->At(0), dy_shape->At(1),
                         dy_shape->At(2) / static_cast<int32_t>(height_scale),
                         dy_shape->At(3) / static_cast<int32_t>(width_scale)});
      return Maybe<void>::Ok();
    })
    .SetGetSbpFn([](user_op::SbpContext* ctx) -> Maybe<void> {
      ctx->NewBuilder().Split(user_op::OpArg("dy", 0), 0).Split(user_op::OpArg("dx", 0), 0).Build();
      return Maybe<void>::Ok();
    });

REGISTER_USER_OP_GRAD("upsample")
    .SetGenBackwardOpConfFn([](const user_op::UserOpWrapper& op, user_op::AddOpFn AddOp) {
      if (op.NeedGenGradTensor4OpInput("x", 0)) {
        user_op::UserOpConfWrapperBuilder builder(op.op_name() + "_grad");
        user_op::UserOpConfWrapper grad_op =
            builder.Op("upsample_grad")
                .Input("dy", op.GetGradTensorWithOpOutput("y", 0))
                .Output("dx")
                .Attr("height_scale", op.attr<float>("height_scale"))
                .Attr("width_scale", op.attr<float>("width_scale"))
                .Attr("data_format", op.attr<std::string>("data_format"))
                .Attr("interpolation", op.attr<std::string>("interpolation"))
                .Build();
        op.BindGradTensorWithOpInput(grad_op.output("dx", 0), "x", 0);
        AddOp(grad_op);
      }
    });

}  // namespace oneflow