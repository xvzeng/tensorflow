load("//tensorflow:tensorflow.bzl", "if_google")

package(
    # copybara:uncomment default_applicable_licenses = ["//tensorflow:license"],
    default_visibility = [":friends"],
    licenses = ["notice"],
)

package_group(
    name = "friends",
    packages = [
        # Authorized users go here.
        # copybara:uncomment "//learning/brain/experimental/tfrt/...",
        # copybara:uncomment "//learning/brain/tfrt/...",
        # copybara:uncomment "//learning/infra/mira/...",
        # copybara:uncomment "//learning/serving/...",
        "//tensorflow/core/runtime_fallback/...",
        "//tensorflow/core/tfrt/mlrt/application/tensorflow/tests/...",
        "//tensorflow/core/tfrt/...",
        "//tensorflow_serving/...",
        "//tensorflow/core/tfrt/saved_model/python/...",
        # copybara:uncomment "//platforms/xla/tests/saved_models/...",
        # copybara:uncomment "//quality/webanswers/servo2/...",
        "//tensorflow/core/tfrt/saved_model/python",  #python wrapping
        "//tensorflow/core/tfrt/saved_model/utils/...",
    ],
)

cc_library(
    name = "saved_model_aot_compile",
    srcs = [
        "saved_model_aot_compile.cc",
    ],
    hdrs = ["saved_model_aot_compile.h"],
    deps = [
        "//tensorflow/cc/saved_model:constants",
        "//tensorflow/compiler/mlir/tensorflow:translate_lib",
        "//tensorflow/compiler/xla/service:compiler",
        "//tensorflow/core:core_cpu_base",
        "//tensorflow/core:lib",
        "//tensorflow/core/ops",
        "//tensorflow/core/platform:enable_tf2_utils",
        "//tensorflow/core/platform:path",
        "//tensorflow/core/platform:status",
        "//tensorflow/core/runtime_fallback/kernel:kernel_fallback_execute_compat",
        "//tensorflow/core/tfrt/graph_executor:graph_execution_options",
        "//tensorflow/core/tfrt/runtime",
        "//tensorflow/tsl/platform:env",
        "//tensorflow/tsl/platform:status",
        "@com_google_absl//absl/status",
        "@tf_runtime//:core_runtime_alwayslink",
    ],
)

cc_library(
    name = "saved_model_lib",
    srcs = [
        "saved_model.cc",
        "saved_model.h",
        "saved_model_import_input.cc",
        "saved_model_import_input.h",
    ],
    visibility = ["//visibility:private"],
    deps = [
        "//tensorflow/cc/saved_model:reader",
        "//tensorflow/compiler/mlir/tensorflow",
        "//tensorflow/compiler/mlir/tensorflow:import_model",
        "//tensorflow/compiler/mlir/tensorflow:translate_lib",
        "//tensorflow/compiler/mlir/tensorflow:upgrade_graph",
        "//tensorflow/compiler/mlir/tfrt:import_model",
        "//tensorflow/compiler/mlir/tfrt:saved_model",
        "//tensorflow/compiler/mlir/tfrt:tfrt_compile_options",
        "//tensorflow/compiler/mlir/tfrt/transforms/mlrt:import_model",
        "//tensorflow/compiler/xla:status_macros",
        "//tensorflow/core:all_kernels",
        "//tensorflow/core:core_cpu_base",
        "//tensorflow/core:framework",
        "//tensorflow/core:lib",
        "//tensorflow/core/framework:graph_proto_cc",
        "//tensorflow/core/ops",
        "//tensorflow/core/platform:enable_tf2_utils",
        "//tensorflow/core/platform:errors",
        "//tensorflow/core/platform:path",
        "//tensorflow/core/profiler/lib:traceme",
        "//tensorflow/core/protobuf:for_core_protos_cc",
        "//tensorflow/core/runtime_fallback/kernel:kernel_fallback_compat_request_state",
        "//tensorflow/core/runtime_fallback/kernel:kernel_fallback_execute_compat",
        "//tensorflow/core/tfrt/fallback:fallback_state",
        "//tensorflow/core/tfrt/graph_executor",
        "//tensorflow/core/tfrt/graph_executor:export_mlir",
        "//tensorflow/core/tfrt/graph_executor:graph_execution_options",
        "//tensorflow/core/tfrt/mlrt/bytecode",
        "//tensorflow/core/tfrt/mlrt/kernel",
        "//tensorflow/core/tfrt/mlrt/kernel:batch_kernel",
        "//tensorflow/core/tfrt/runtime",
        "//tensorflow/core/tfrt/runtime:work_queue_interface",
        "//tensorflow/core/tfrt/utils",
        "//tensorflow/core/tfrt/utils:error_util",
        "//tensorflow/core/tfrt/utils:fallback_tensor",
        "//tensorflow/core/tfrt/utils:tfrt_graph_execution_state",
        "@com_google_absl//absl/cleanup",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/log",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/synchronization",
        "@com_google_absl//absl/time",
        "@com_google_absl//absl/types:span",
        "@llvm-project//mlir:FuncExtensions",
        "@llvm-project//mlir:IR",
        "@tf_runtime//:bef",
        "@tf_runtime//:befexecutor",
        "@tf_runtime//:core_runtime_alwayslink",
        "@tf_runtime//:hostcontext",
        "@tf_runtime//:metrics",
        "@tf_runtime//:support",
    ] + if_google([
        "//third_party/tf_runtime_google:streamz_metrics_registry_alwayslink",
    ]),
)

cc_library(
    name = "saved_model_cpu",
    hdrs = ["saved_model.h"],
    deps = [
        ":saved_model_lib",
        "//tensorflow/core/framework:graph_proto_cc",
        "//tensorflow/core/platform:thread_annotations",
        "//tensorflow/core/protobuf:for_core_protos_cc",
        "//tensorflow/core/tfrt/fallback:fallback_state",
        "//tensorflow/core/tfrt/graph_executor",
        "//tensorflow/core/tfrt/graph_executor:graph_execution_options",
        "//tensorflow/core/tfrt/runtime",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/types:span",
        "@tf_runtime//:hostcontext",
    ],
)

cc_library(
    name = "saved_model",
    hdrs = ["saved_model.h"],
    deps = [
        ":saved_model_lib",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/types:span",
        "//tensorflow/core/framework:graph_proto_cc",
        "//tensorflow/core/platform:thread_annotations",
        "//tensorflow/core/protobuf:for_core_protos_cc",
        # TODO(chky): Remove kernel fallback tensor deps.
        "//tensorflow/core/runtime_fallback/kernel:kernel_fallback_tensor_conversion_alwayslink",
        "//tensorflow/core/runtime_fallback/kernel:gpurt_kernels",
        "//tensorflow/core/runtime_fallback/runtime:runtime_fallback_alwayslink",
        "//tensorflow/core/tfrt/fallback:fallback_state",
        "//tensorflow/core/tfrt/graph_executor",
        "//tensorflow/core/tfrt/graph_executor:graph_execution_options",
        "//tensorflow/core/tfrt/runtime",
        "@tf_runtime//:hostcontext",
    ] + if_google([
        "//learning/brain/tfrt/support:export_mlir",
        "//learning/brain/tfrt/tpu/compiler/mlir:tf_to_tfrt_tpu",
    ]),
)

cc_library(
    name = "saved_model_testutil",
    testonly = 1,
    srcs = ["saved_model_testutil.cc"],
    hdrs = ["saved_model_testutil.h"],
    tags = ["no_oss"],
    deps = [
        ":saved_model",
        "//tensorflow/cc/saved_model:loader",
        "//tensorflow/compiler/mlir/tfrt:tfrt_compile_options",
        "//tensorflow/core:lib",
        "//tensorflow/core:protos_all_cc",
        "//tensorflow/core/framework:tensor_testutil",
        "//tensorflow/core/tfrt/runtime",
        "@tf_runtime//:hostcontext",
    ],
)

cc_library(
    name = "saved_model_import_input",
    srcs = ["saved_model_import_input.cc"],
    hdrs = ["saved_model_import_input.h"],
    deps = [
        "//tensorflow/compiler/mlir/tensorflow:import_model",
        "//tensorflow/compiler/mlir/tensorflow:upgrade_graph",
        "//tensorflow/core:core_cpu_base",
        "//tensorflow/core:framework",
        "//tensorflow/core/tfrt/fallback:fallback_state",
        "//tensorflow/core/tfrt/utils:tfrt_graph_execution_state",
        "@com_google_absl//absl/synchronization",
    ],
)

cc_library(
    name = "saved_model_mira_impl",
    srcs = ["saved_model_mira_impl.cc"],
    hdrs = ["saved_model_mira_impl.h"],
    deps = [
        ":saved_model",
    ] + if_google([
        "@com_google_absl//absl/status:statusor",
        "//learning/infra/mira/tfrt:tfrt_executable",
        "//learning/infra/mira/tfrt:tfrt_module",
        "//third_party/mira/mlarchive:status_macro",
        "//third_party/mira/mlvalue:value",
        "//third_party/mira/runtime:module",
        "//third_party/mira/runtime:vm",
        "//third_party/mira/runtime:vm_session",
        "//tensorflow/compiler/xla:status_macros",
        "//tensorflow/core/platform:status",
        "//tensorflow/core/tfrt/runtime",
    ]),
)
