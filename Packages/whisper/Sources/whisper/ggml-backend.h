#pragma once

#include "ggml.h"
#include "ggml-alloc.h"

#ifdef  __cplusplus
extern "C" {
#endif

    typedef struct w_ggml_backend_buffer_type * w_ggml_backend_buffer_type_t;
    typedef struct w_ggml_backend_buffer * w_ggml_backend_buffer_t;
    typedef struct w_ggml_backend_event * w_ggml_backend_event_t;
    typedef struct w_ggml_backend * w_ggml_backend_t;
    typedef void * w_ggml_backend_graph_plan_t;

    //
    // Backend buffer
    //

    // buffer type
    W_GGML_API           const char *          w_ggml_backend_buft_name            (w_ggml_backend_buffer_type_t buft);
    W_GGML_API W_GGML_CALL w_ggml_backend_buffer_t w_ggml_backend_buft_alloc_buffer    (w_ggml_backend_buffer_type_t buft, size_t size);
    W_GGML_API           size_t                w_ggml_backend_buft_get_alignment   (w_ggml_backend_buffer_type_t buft);
    W_GGML_API           size_t                w_ggml_backend_buft_get_max_size    (w_ggml_backend_buffer_type_t buft);
    W_GGML_API W_GGML_CALL size_t                w_ggml_backend_buft_get_alloc_size  (w_ggml_backend_buffer_type_t buft, struct w_ggml_tensor * tensor);
    W_GGML_API           bool                  w_ggml_backend_buft_supports_backend(w_ggml_backend_buffer_type_t buft, w_ggml_backend_t backend);
    W_GGML_API           bool                  w_ggml_backend_buft_is_host         (w_ggml_backend_buffer_type_t buft);

    // buffer
    enum w_ggml_backend_buffer_usage {
        W_GGML_BACKEND_BUFFER_USAGE_ANY = 0,
        W_GGML_BACKEND_BUFFER_USAGE_WEIGHTS = 1,
    };

    W_GGML_API           const char *               w_ggml_backend_buffer_name          (w_ggml_backend_buffer_t buffer);
    W_GGML_API           void                       w_ggml_backend_buffer_free          (w_ggml_backend_buffer_t buffer);
    W_GGML_API           void *                     w_ggml_backend_buffer_get_base      (w_ggml_backend_buffer_t buffer);
    W_GGML_API           size_t                     w_ggml_backend_buffer_get_size      (w_ggml_backend_buffer_t buffer);
    W_GGML_API W_GGML_CALL void                       w_ggml_backend_buffer_init_tensor   (w_ggml_backend_buffer_t buffer, struct w_ggml_tensor * tensor);
    W_GGML_API           size_t                     w_ggml_backend_buffer_get_alignment (w_ggml_backend_buffer_t buffer);
    W_GGML_API           size_t                     w_ggml_backend_buffer_get_max_size  (w_ggml_backend_buffer_t buffer);
    W_GGML_API           size_t                     w_ggml_backend_buffer_get_alloc_size(w_ggml_backend_buffer_t buffer, struct w_ggml_tensor * tensor);
    W_GGML_API           void                       w_ggml_backend_buffer_clear         (w_ggml_backend_buffer_t buffer, uint8_t value);
    W_GGML_API           bool                       w_ggml_backend_buffer_is_host       (w_ggml_backend_buffer_t buffer);
    W_GGML_API           void                       w_ggml_backend_buffer_set_usage     (w_ggml_backend_buffer_t buffer, enum w_ggml_backend_buffer_usage usage);
    W_GGML_API           w_ggml_backend_buffer_type_t w_ggml_backend_buffer_get_type      (w_ggml_backend_buffer_t buffer);
    W_GGML_API           void                       w_ggml_backend_buffer_reset         (w_ggml_backend_buffer_t buffer);

    //
    // Backend
    //

    W_GGML_API w_ggml_guid_t  w_ggml_backend_guid(w_ggml_backend_t backend);
    W_GGML_API const char * w_ggml_backend_name(w_ggml_backend_t backend);
    W_GGML_API void         w_ggml_backend_free(w_ggml_backend_t backend);

    W_GGML_API w_ggml_backend_buffer_type_t w_ggml_backend_get_default_buffer_type(w_ggml_backend_t backend);
    W_GGML_API w_ggml_backend_buffer_t      w_ggml_backend_alloc_buffer(w_ggml_backend_t backend, size_t size);
    W_GGML_API size_t                     w_ggml_backend_get_alignment(w_ggml_backend_t backend);
    W_GGML_API size_t                     w_ggml_backend_get_max_size(w_ggml_backend_t backend);

    W_GGML_API void w_ggml_backend_tensor_set_async(w_ggml_backend_t backend,       struct w_ggml_tensor * tensor, const void * data, size_t offset, size_t size);
    W_GGML_API void w_ggml_backend_tensor_get_async(w_ggml_backend_t backend, const struct w_ggml_tensor * tensor,       void * data, size_t offset, size_t size);

    W_GGML_API W_GGML_CALL void w_ggml_backend_tensor_set(      struct w_ggml_tensor * tensor, const void * data, size_t offset, size_t size);
    W_GGML_API W_GGML_CALL void w_ggml_backend_tensor_get(const struct w_ggml_tensor * tensor,       void * data, size_t offset, size_t size);

    W_GGML_API void w_ggml_backend_synchronize(w_ggml_backend_t backend);

    W_GGML_API w_ggml_backend_graph_plan_t w_ggml_backend_graph_plan_create(w_ggml_backend_t backend, struct w_ggml_cgraph * cgraph);
    W_GGML_API void                      w_ggml_backend_graph_plan_free  (w_ggml_backend_t backend, w_ggml_backend_graph_plan_t plan);

    W_GGML_API enum w_ggml_status w_ggml_backend_graph_plan_compute (w_ggml_backend_t backend, w_ggml_backend_graph_plan_t plan);
    W_GGML_API enum w_ggml_status w_ggml_backend_graph_compute      (w_ggml_backend_t backend, struct w_ggml_cgraph * cgraph);
    W_GGML_API enum w_ggml_status w_ggml_backend_graph_compute_async(w_ggml_backend_t backend, struct w_ggml_cgraph * cgraph);
    W_GGML_API bool w_ggml_backend_supports_op(w_ggml_backend_t backend, const struct w_ggml_tensor * op);
    W_GGML_API bool w_ggml_backend_offload_op(w_ggml_backend_t backend, const struct w_ggml_tensor * op);

    // tensor copy between different backends
    W_GGML_API void w_ggml_backend_tensor_copy(struct w_ggml_tensor * src, struct w_ggml_tensor * dst);

    // asynchronous copy
    // the copy is performed after all the currently queued operations in backend_src
    // backend_dst will wait for the copy to complete before performing other operations
    // automatic fallback to sync copy if async is not supported
    W_GGML_API void w_ggml_backend_tensor_copy_async(w_ggml_backend_t backend_src, w_ggml_backend_t backend_dst, struct w_ggml_tensor * src, struct w_ggml_tensor * dst);

    // events
    W_GGML_API w_ggml_backend_event_t   w_ggml_backend_event_new        (w_ggml_backend_t backend);
    W_GGML_API void                   w_ggml_backend_event_free       (w_ggml_backend_event_t event);
    W_GGML_API void                   w_ggml_backend_event_record     (w_ggml_backend_event_t event);
    W_GGML_API void                   w_ggml_backend_event_synchronize(w_ggml_backend_event_t event);
    W_GGML_API void                   w_ggml_backend_event_wait       (w_ggml_backend_t backend, w_ggml_backend_event_t event); // wait async on event

    //
    // CPU backend
    //

    W_GGML_API w_ggml_backend_t w_ggml_backend_cpu_init(void);

    W_GGML_API W_GGML_CALL bool w_ggml_backend_is_cpu                (w_ggml_backend_t backend);
    W_GGML_API           void w_ggml_backend_cpu_set_n_threads     (w_ggml_backend_t backend_cpu, int n_threads);
    W_GGML_API           void w_ggml_backend_cpu_set_abort_callback(w_ggml_backend_t backend_cpu, w_ggml_abort_callback abort_callback, void * abort_callback_data);

    // Create a backend buffer from an existing pointer
    W_GGML_API W_GGML_CALL w_ggml_backend_buffer_t w_ggml_backend_cpu_buffer_from_ptr(void * ptr, size_t size);

    W_GGML_API W_GGML_CALL w_ggml_backend_buffer_type_t w_ggml_backend_cpu_buffer_type(void);

#ifdef W_GGML_USE_CPU_HBM
    W_GGML_API w_ggml_backend_buffer_type_t w_ggml_backend_cpu_hbm_buffer_type(void);
#endif

    //
    // Backend registry
    //

    // The backend registry is a registry of all the available backends, and allows initializing backends in a generic way

    W_GGML_API size_t                     w_ggml_backend_reg_get_count(void);
    W_GGML_API size_t                     w_ggml_backend_reg_find_by_name(const char * name);
    W_GGML_API w_ggml_backend_t             w_ggml_backend_reg_init_backend_from_str(const char * backend_str); // str is name[:params]
    W_GGML_API const char *               w_ggml_backend_reg_get_name(size_t i);
    W_GGML_API w_ggml_backend_t             w_ggml_backend_reg_init_backend(size_t i, const char * params); // params is backend-specific
    W_GGML_API w_ggml_backend_buffer_type_t w_ggml_backend_reg_get_default_buffer_type(size_t i);
    W_GGML_API w_ggml_backend_buffer_t      w_ggml_backend_reg_alloc_buffer(size_t i, size_t size);

    //
    // Backend scheduler
    //

    // The backend scheduler allows for multiple backends to be used together
    // Handles compute buffer allocation, assignment of tensors to backends, and copying of tensors between backends
    // The backends are selected based on:
    // - the backend that supports the operation
    // - the location of the pre-allocated tensors (e.g. the weights)
    /*
      Example usage:

        // operations that use tensors allocated in a buffer with USAGE_WEIGHTS will be assigned
        // preferrably to run on the same backend as the buffer
        w_ggml_backend_buffer_set_usage(buf_weights, W_GGML_BACKEND_BUFFER_USAGE_WEIGHTS);

        sched = w_ggml_backend_sched_new({backend_gpu, backend_gpu2, backend_cpu}, NULL, num_backends, W_GGML_DEFAULT_GRAPH_SIZE, false);

        // initialize buffers from a max size graph (optional)
        reserve_graph = build_graph(sched, max_batch_size);

        // manually assign nodes to a backend (optional, should not be needed in most cases)
        struct w_ggml_tensor * node = w_ggml_mul_mat(ctx, ...);
        w_ggml_backend_sched_set_tensor_backend(sched, node, backend_gpu);

        w_ggml_backend_sched_reserve(sched, reserve_graph);

        // compute
        graph = build_graph(sched);
        w_ggml_backend_sched_graph_compute(sched, graph);

        // if there are graph inputs:
        w_ggml_backend_sched_reset(sched);
        w_ggml_backend_sched_alloc_graph(sched, graph);
        w_ggml_backend_tensor_set(input_tensor, ...);
        w_ggml_backend_sched_graph_compute(sched, graph);
    }
    */

    struct w_ggml_backend_sched;
    typedef struct w_ggml_backend_sched * w_ggml_backend_sched_t;

    // when ask == true, the scheduler wants to know if the user wants to observe this node
    // this allows the scheduler to batch nodes together in order to evaluate them in a single call
    //
    // when ask == false, the scheduler is passing the node tensor to the user for observation
    // if the user returns false, the scheduler will cancel the graph compute
    //
    typedef bool (*w_ggml_backend_sched_eval_callback)(struct w_ggml_tensor * t, bool ask, void * user_data);

    // Initialize a backend scheduler
    W_GGML_API w_ggml_backend_sched_t w_ggml_backend_sched_new(w_ggml_backend_t * backends, w_ggml_backend_buffer_type_t * bufts, int n_backends, size_t graph_size, bool parallel);
    W_GGML_API void                 w_ggml_backend_sched_free(w_ggml_backend_sched_t sched);

    // Initialize backend buffers from a measure graph
    W_GGML_API bool                 w_ggml_backend_sched_reserve(w_ggml_backend_sched_t sched, struct w_ggml_cgraph * measure_graph);

    // Get the number of splits of the last graph
    W_GGML_API int                  w_ggml_backend_sched_get_n_splits(w_ggml_backend_sched_t sched);
    W_GGML_API int                  w_ggml_backend_sched_get_n_copies(w_ggml_backend_sched_t sched);

    W_GGML_API size_t               w_ggml_backend_sched_get_buffer_size(w_ggml_backend_sched_t sched, w_ggml_backend_t backend);

    W_GGML_API void                 w_ggml_backend_sched_set_tensor_backend(w_ggml_backend_sched_t sched, struct w_ggml_tensor * node, w_ggml_backend_t backend);
    W_GGML_API w_ggml_backend_t       w_ggml_backend_sched_get_tensor_backend(w_ggml_backend_sched_t sched, struct w_ggml_tensor * node);

    // Allocate and compute graph on the backend scheduler
    W_GGML_API bool                 w_ggml_backend_sched_alloc_graph(w_ggml_backend_sched_t sched, struct w_ggml_cgraph * graph);
    W_GGML_API enum w_ggml_status     w_ggml_backend_sched_graph_compute(w_ggml_backend_sched_t sched, struct w_ggml_cgraph * graph);
    W_GGML_API enum w_ggml_status     w_ggml_backend_sched_graph_compute_async(w_ggml_backend_sched_t sched, struct w_ggml_cgraph * graph);
    W_GGML_API void                 w_ggml_backend_sched_synchronize(w_ggml_backend_sched_t sched);

    // Reset all assignments and allocators - must be called before changing the node backends
    W_GGML_API void                 w_ggml_backend_sched_reset(w_ggml_backend_sched_t sched);

    // Set a callback to be called for each resulting node during graph compute
    W_GGML_API void                 w_ggml_backend_sched_set_eval_callback(w_ggml_backend_sched_t sched, w_ggml_backend_sched_eval_callback callback, void * user_data);

    //
    // Utils
    //

    struct w_ggml_backend_graph_copy {
        w_ggml_backend_buffer_t buffer;
        struct w_ggml_context * ctx_allocated;
        struct w_ggml_context * ctx_unallocated;
        struct w_ggml_cgraph * graph;
    };

    // Copy a graph to a different backend
    W_GGML_API struct w_ggml_backend_graph_copy w_ggml_backend_graph_copy(w_ggml_backend_t backend, struct w_ggml_cgraph * graph);
    W_GGML_API void                           w_ggml_backend_graph_copy_free(struct w_ggml_backend_graph_copy copy);

    typedef bool (*W_GGML_CALL w_ggml_backend_eval_callback)(int node_index, struct w_ggml_tensor * t1, struct w_ggml_tensor * t2, void * user_data);

    // Compare the output of two backends
    W_GGML_API bool w_ggml_backend_compare_graph_backend(w_ggml_backend_t backend1, w_ggml_backend_t backend2, struct w_ggml_cgraph * graph, w_ggml_backend_eval_callback callback, void * user_data);

    // Tensor initialization
    W_GGML_API void w_ggml_backend_tensor_alloc(w_ggml_backend_buffer_t buffer, struct w_ggml_tensor * tensor, void * addr);
    W_GGML_API void w_ggml_backend_view_init(w_ggml_backend_buffer_t buffer, struct w_ggml_tensor * tensor);


#ifdef  __cplusplus
}
#endif
