/* Copyright 2023 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "tensorflow/compiler/xla/stream_executor/cuda/cuda_graph.h"

#include <atomic>

#include "absl/strings/str_format.h"
#include "third_party/gpus/cuda/include/cuda_runtime_api.h"
#include "tensorflow/compiler/xla/stream_executor/gpu/gpu_stream.h"
#include "tensorflow/tsl/platform/env.h"
#include "tensorflow/tsl/platform/path.h"

namespace stream_executor {
namespace gpu {

//===----------------------------------------------------------------------===//
// RAII helpers for CUDA graph types.
//===----------------------------------------------------------------------===//

std::atomic<size_t> CudaGraphSupport::allocated_cuda_graph_execs_;
std::atomic<size_t> CudaGraphSupport::alive_cuda_graph_execs_;

/*static*/ size_t CudaGraphSupport::NotifyGraphExecCreated() {
  alive_cuda_graph_execs_.fetch_add(1, std::memory_order_relaxed);
  return allocated_cuda_graph_execs_.fetch_add(1, std::memory_order_relaxed);
}

/*static*/ size_t CudaGraphSupport::NotifyGraphExecDestroyed() {
  return alive_cuda_graph_execs_.fetch_sub(1, std::memory_order_relaxed) - 1;
}

/*static*/ size_t CudaGraphSupport::allocated_cuda_graph_execs() {
  return allocated_cuda_graph_execs_.load(std::memory_order_relaxed);
}

/*static*/ size_t CudaGraphSupport::alive_cuda_graph_execs() {
  return alive_cuda_graph_execs_.load(std::memory_order_relaxed);
}

void CudaGraphSupport::DestroyGraph::operator()(cudaGraph_t graph) {
  cudaError_t err = cudaGraphDestroy(graph);
  CHECK(err == cudaSuccess)
      << "Failed to destroy CUDA graph: " << cudaGetErrorString(err);
}

void CudaGraphSupport::DestroyGraphExec::operator()(cudaGraphExec_t instance) {
  cudaError_t err = cudaGraphExecDestroy(instance);
  CHECK(err == cudaSuccess)
      << "Failed to destroy CUDA graph instance: " << cudaGetErrorString(err);
}

tsl::Status OwnedCudaGraphExec::Update(OwnedCudaGraph graph) {
  VLOG(3) << "Update CUDA graph exec with a new graph after " << num_launches_
          << " launches since last update"
          << " #" << num_updates_++;

  num_launches_ = 0;

#if CUDA_VERSION >= 12000
  cudaGraphExecUpdateResultInfo updated;

  auto err = cudaGraphExecUpdate(get(), graph.get(), &updated);
  if (err != cudaSuccess || updated.result != cudaGraphExecUpdateSuccess)
    return absl::InternalError(absl::StrFormat(
        "failed to update cuda graph: %s", cudaGetErrorString(err)));

#else
  cudaGraphExecUpdateResult updated;
  cudaGraphNode_t error_node;

  auto err = cudaGraphExecUpdate(get(), graph.get(), &error_node, &updated);
  if (err != cudaSuccess || updated != cudaGraphExecUpdateSuccess)
    return absl::InternalError(absl::StrFormat("Failed to update cuda graph %s",
                                               cudaGetErrorString(err)));
#endif

  return tsl::OkStatus();
}

tsl::Status OwnedCudaGraphExec::Launch(stream_executor::Stream* stream) {
  VLOG(3) << "Launch CUDA graph " << get()
          << " on a stream: " << stream->DebugStreamPointers() << " #"
          << ++num_launches_;

  if (auto err = cudaGraphLaunch(get(), AsGpuStreamValue(stream));
      err != cudaSuccess)
    return absl::InternalError(absl::StrFormat("failed to run cuda graph: %s",
                                               cudaGetErrorString(err)));

  return tsl::OkStatus();
}

OwnedCudaGraphExec::~OwnedCudaGraphExec() {
  if (*this)  // do not log for moved-from instances
    VLOG(5) << "Destroy CUDA graph exec #" << id_
            << " (remaining alive instances: "
            << CudaGraphSupport::NotifyGraphExecDestroyed() << ")";
}

//===----------------------------------------------------------------------===//
// CUDA Graph Helpers.
//===----------------------------------------------------------------------===//

tsl::StatusOr<OwnedCudaGraph> CaptureCudaGraph(
    stream_executor::Stream* stream, absl::AnyInvocable<tsl::Status()> capture,
    cudaStreamCaptureMode mode) {
  VLOG(3) << "Capture CUDA graph on a stream: "
          << stream->DebugStreamPointers();

  cudaGraph_t graph;

  // Get the underlying CUDA stream for passing to CUDA APIs.
  auto gpu_stream = AsGpuStreamValue(stream);

  // Capture graph constructed by the exported graph capture function.
  if (auto err = cudaStreamBeginCapture(gpu_stream, mode); err != cudaSuccess)
    return absl::InternalError(absl::StrFormat(
        "stream begin capture failed: %s", cudaGetErrorString(err)));

  // Call into graph capture function.
  auto captured = capture();

  // Always stop capturing the stream before checking `captured` result.
  if (auto err = cudaStreamEndCapture(gpu_stream, &graph); err != cudaSuccess)
    return absl::InternalError(absl::StrFormat("stream end capture failed: %s",
                                               cudaGetErrorString(err)));

  if (!captured.ok())
    return absl::InternalError(absl::StrFormat(
        "failed to capture CUDA graph: %s", captured.message()));

  VLOG(5) << "Captured CUDA graph " << graph;

#if CUDA_VERSION >= 12000
  // If verbose logging is enabled print captured CUDA graph debug information.
  if (VLOG_IS_ON(100)) {
    if (const char* path = getenv("XLA_CUDA_GRAPH_DEBUG_DIRECTORY"); path) {
      std::string file = tsl::io::JoinPath(std::string(path), "/cuda_graph-");

      if (tsl::Env::Default()->CreateUniqueFileName(&file, ".dot")) {
        VLOG(100) << "Print CUDA graph " << graph
                  << " debug dot file to: " << file;

        int flags = cudaGraphDebugDotFlagsVerbose;
        if (auto err = cudaGraphDebugDotPrint(graph, file.c_str(), flags);
            err != cudaSuccess) {
          LOG(WARNING) << "failed to print CUDA graph debug file: "
                       << cudaGetErrorString(err);

        } else if (VLOG_IS_ON(200)) {
          std::string data;
          if (tsl::ReadFileToString(tsl::Env::Default(), file, &data).ok()) {
            VLOG(200) << "CUDA graph " << graph << " debug file:\n" << data;
          } else {
            LOG(WARNING) << "failed to read CUDA graph debug file";
          }
        }

      } else {
        LOG(WARNING) << "cannot create unique filename, won't enable CUDA "
                        "graph debugging";
      }
    }
  }
#endif  // CUDA_VERSION >= 12000

  return OwnedCudaGraph(graph);
}

tsl::StatusOr<OwnedCudaGraphExec> InstantiateCudaGraph(OwnedCudaGraph graph) {
  cudaGraphExec_t exec;

#if CUDA_VERSION >= 12000
  if (auto err = cudaGraphInstantiate(&exec, &*graph);
#else
  if (auto err = cudaGraphInstantiate(&exec, &*graph, nullptr, nullptr, 0);
#endif
      err != cudaSuccess) {
    if (err == cudaErrorMemoryAllocation) {
      // OOM is a recoverable error, we evict all instantiated cuda graphs to
      // free up some space (see graph launch.cc). Clear error status.
      return absl::ResourceExhaustedError(
          absl::StrFormat("graph instantiation failed: %s",
                          cudaGetErrorString(cudaGetLastError())));
    } else {
      return absl::InternalError(absl::StrFormat(
          "graph instantiation failed: %s", cudaGetErrorString(err)));
    }
  }

  size_t id = CudaGraphSupport::NotifyGraphExecCreated();
  VLOG(5) << "Instantiated CUDA graph exec instance #" << id
          << " (alive instances: " << CudaGraphSupport::alive_cuda_graph_execs()
          << ")";
  return OwnedCudaGraphExec(id, exec);
}

tsl::StatusOr<bool> IsStreamCapturing(stream_executor::Stream* stream) {
  cudaStreamCaptureStatus capture_status;
  cudaError_t err = cudaStreamIsCapturing(
      stream_executor::gpu::AsGpuStreamValue(stream), &capture_status);
  if (err != cudaSuccess) {
    return absl::InternalError(absl::StrFormat(
        "Failed to get stream's capture status: %s", cudaGetErrorString(err)));
  }

  return capture_status == cudaStreamCaptureStatusActive;
}

}  // namespace gpu
}  // namespace stream_executor
