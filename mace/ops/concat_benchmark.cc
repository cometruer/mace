//
// Copyright (c) 2017 XiaoMi All rights reserved.
//

#include "mace/core/operator.h"
#include "mace/core/testing/test_benchmark.h"
#include "mace/ops/ops_test_util.h"

namespace mace {
template <DeviceType D, typename T>
static void ConcatHelper(int iters, int concat_dim, int dim1) {
  mace::testing::StopTiming();

  OpsTestNet net;
  OpDefBuilder("Concat", "ConcatBM")
      .Input("Input0")
      .Input("Input1")
      .AddIntArg("axis", concat_dim)
      .Output("Output")
      .Finalize(net.NewOperatorDef());

  // Add input data
  const int kDim0 = 100;
  net.AddRandomInput<DeviceType::CPU, T>("Input0", {kDim0, dim1});
  net.AddRandomInput<DeviceType::CPU, T>("Input1", {kDim0, dim1});

  // Warm-up
  for (int i = 0; i < 5; ++i) {
    net.RunOp(D);
  }
  const int64_t tot = static_cast<int64_t>(iters) * kDim0 * dim1 * 2;
  mace::testing::MaccProcessed(tot);
  testing::BytesProcessed(tot * sizeof(T));
  mace::testing::StartTiming();
  while (iters--) {
    net.RunOp(D);
  }
}

#define BM_CONCAT_CPU_MACRO(DIM0, DIM1) \
  static void BM_CONCAT_CPU_##DIM0##_##DIM1( \
      int iters) { \
    ConcatHelper<DeviceType::CPU, float>(iters, DIM0, DIM1); \
  } \
  BENCHMARK(BM_CONCAT_CPU_##DIM0##_##DIM1)

BM_CONCAT_CPU_MACRO(0, 1000);
BM_CONCAT_CPU_MACRO(0, 100000);
BM_CONCAT_CPU_MACRO(1, 1000);
BM_CONCAT_CPU_MACRO(1, 100000);

template <typename T>
static void OpenclConcatHelper(int iters,
                               const std::vector<index_t> &shape0,
                               const std::vector<index_t> &shape1,
                               int concat_dim) {
  mace::testing::StopTiming();

  OpsTestNet net;

  // Add input data
  net.AddRandomInput<DeviceType::OPENCL, float>("Input0", shape0);
  net.AddRandomInput<DeviceType::OPENCL, float>("Input1", shape1);

  BufferToImage<DeviceType::OPENCL, T>(net, "Input0", "InputImage0",
                                       kernels::BufferType::IN_OUT_CHANNEL);
  BufferToImage<DeviceType::OPENCL, T>(net, "Input1", "InputImage1",
                                       kernels::BufferType::IN_OUT_CHANNEL);
  OpDefBuilder("Concat", "ConcatBM")
      .Input("InputImage0")
      .Input("InputImage1")
      .AddIntArg("axis", concat_dim)
      .Output("OutputImage")
      .AddIntArg("T", static_cast<int>(DataTypeToEnum<T>::value))
      .Finalize(net.NewOperatorDef());

  // Warm-up
  for (int i = 0; i < 5; ++i) {
    net.RunOp(DeviceType::OPENCL);
  }

  const int64_t tot =
      static_cast<int64_t>(iters) *
      (net.GetTensor("Input0")->size() + net.GetTensor("Input1")->size());
  mace::testing::MaccProcessed(tot);
  testing::BytesProcessed(tot * sizeof(T));
  mace::testing::StartTiming();
  while (iters--) {
    net.RunOp(DeviceType::OPENCL);
  }
}


#define BM_CONCAT_OPENCL_MACRO(N, C, H, W, TYPE) \
  static void BM_CONCAT_OPENCL_##N##_##C##_##H##_##W##_##TYPE( \
      int iters) { \
    std::vector<index_t> shape = {N, H, W, C}; \
    OpenclConcatHelper<TYPE>(iters, shape, shape, 3); \
  } \
  BENCHMARK(BM_CONCAT_OPENCL_##N##_##C##_##H##_##W##_##TYPE)

BM_CONCAT_OPENCL_MACRO(3, 32, 32, 32, float);
BM_CONCAT_OPENCL_MACRO(3, 32, 32, 64, float);
BM_CONCAT_OPENCL_MACRO(3, 32, 32, 128, float);
BM_CONCAT_OPENCL_MACRO(3, 32, 32, 256, float);

BM_CONCAT_OPENCL_MACRO(3, 32, 32, 32, half);
BM_CONCAT_OPENCL_MACRO(3, 32, 32, 64, half);
BM_CONCAT_OPENCL_MACRO(3, 32, 32, 128, half);
BM_CONCAT_OPENCL_MACRO(3, 32, 32, 256, half);

}  // namespace mace
