cc_library(
    name = "batcher",
    hdrs = ["batcher.h"],
    deps = [
        "graph",
        "block",
    ],
)
cc_library(
    name = "block",
    hdrs = ["block.h"],
    deps = [
        "graph",
        "integer_log2",
    ],
)

cc_library(
    name = "bucket",
    hdrs = ["buckets.h"],
    deps = [
        "block",
    ],
)

cc_library(
    name = "dynamic_hashed_blocks",
    hdrs = ["dynamic_hashed_blocks.h"],
    deps = [
        "batcher",
        "block",
        "bucket",
        "graph",
    ],
)

cc_library(
    name = "graph",
    hdrs = ["graph.h"],
)

cc_library(
    name = "integer_log2",
    hdrs = ["integer_log2.h"],
)



package(
    default_visibility = ["//visibility:public"],
)
