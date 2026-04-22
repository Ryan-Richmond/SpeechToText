#pragma once

#include "ggml.h"

#ifdef  __cplusplus
extern "C" {
#endif

typedef struct w_ggml_backend_buffer_type * w_ggml_backend_buffer_type_t;
typedef struct w_ggml_backend_buffer * w_ggml_backend_buffer_t;
typedef struct w_ggml_backend * w_ggml_backend_t;

// Tensor allocator
struct w_ggml_tallocr {
    w_ggml_backend_buffer_t buffer;
    void * base;
    size_t alignment;
    size_t offset;
};

W_GGML_API struct w_ggml_tallocr w_ggml_tallocr_new(w_ggml_backend_buffer_t buffer);
W_GGML_API void                w_ggml_tallocr_alloc(struct w_ggml_tallocr * talloc, struct w_ggml_tensor * tensor);

// Graph allocator
/*
  Example usage:
    w_ggml_gallocr_t galloc = w_ggml_gallocr_new(w_ggml_bacckend_cpu_buffer_type());

    // optional: create a worst-case graph and reserve the buffers to avoid reallocations
    w_ggml_gallocr_reserve(galloc, build_graph(max_batch));

    // allocate the graph
    struct w_ggml_cgraph * graph = build_graph(batch);
    w_ggml_gallocr_alloc_graph(galloc, graph);

    printf("compute buffer size: %zu bytes\n", w_ggml_gallocr_get_buffer_size(galloc, 0));

    // evaluate the graph
    w_ggml_backend_graph_compute(backend, graph);
*/

// special tensor flags for use with the graph allocator:
//   w_ggml_set_input(): all input tensors are allocated at the beginning of the graph in non-overlapping addresses
//   w_ggml_set_output(): output tensors are never freed and never overwritten

typedef struct w_ggml_gallocr * w_ggml_gallocr_t;

W_GGML_API w_ggml_gallocr_t w_ggml_gallocr_new(w_ggml_backend_buffer_type_t buft);
W_GGML_API w_ggml_gallocr_t w_ggml_gallocr_new_n(w_ggml_backend_buffer_type_t * bufts, int n_bufs);
W_GGML_API void           w_ggml_gallocr_free(w_ggml_gallocr_t galloc);

// pre-allocate buffers from a measure graph - does not allocate or modify the graph
// call with a worst-case graph to avoid buffer reallocations
// not strictly required for single buffer usage: w_ggml_gallocr_alloc_graph will reallocate the buffers automatically if needed
// returns false if the buffer allocation failed
W_GGML_API bool w_ggml_gallocr_reserve(w_ggml_gallocr_t galloc, struct w_ggml_cgraph * graph);
W_GGML_API bool w_ggml_gallocr_reserve_n(
    w_ggml_gallocr_t galloc,
    struct w_ggml_cgraph * graph,
    const int * node_buffer_ids,
    const int * leaf_buffer_ids);

// automatic reallocation if the topology changes when using a single buffer
// returns false if using multiple buffers and a re-allocation is needed (call w_ggml_gallocr_reserve_n first to set the node buffers)
W_GGML_API bool w_ggml_gallocr_alloc_graph(w_ggml_gallocr_t galloc, struct w_ggml_cgraph * graph);

W_GGML_API size_t w_ggml_gallocr_get_buffer_size(w_ggml_gallocr_t galloc, int buffer_id);

// Utils
// Create a buffer and allocate all the tensors in a w_ggml_context
W_GGML_API struct w_ggml_backend_buffer * w_ggml_backend_alloc_ctx_tensors_from_buft(struct w_ggml_context * ctx, w_ggml_backend_buffer_type_t buft);
W_GGML_API struct w_ggml_backend_buffer * w_ggml_backend_alloc_ctx_tensors(struct w_ggml_context * ctx, w_ggml_backend_t backend);

#ifdef  __cplusplus
}
#endif
