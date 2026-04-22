#pragma once

// ggml-backend internal header

#include "ggml-backend.h"

#ifdef  __cplusplus
extern "C" {
#endif

    //
    // Backend buffer
    //

    // buffer type
    typedef void * w_ggml_backend_buffer_type_context_t;

    struct w_ggml_backend_buffer_type_i {
        const char *          (*W_GGML_CALL get_name)        (w_ggml_backend_buffer_type_t buft);
        w_ggml_backend_buffer_t (*W_GGML_CALL alloc_buffer)    (w_ggml_backend_buffer_type_t buft, size_t size);
        size_t                (*W_GGML_CALL get_alignment)   (w_ggml_backend_buffer_type_t buft); // tensor alignment
        size_t                (*W_GGML_CALL get_max_size)    (w_ggml_backend_buffer_type_t buft); // allocation max size
        size_t                (*W_GGML_CALL get_alloc_size)  (w_ggml_backend_buffer_type_t buft, const struct w_ggml_tensor * tensor); // data size needed to allocate the tensor, including padding
        bool                  (*W_GGML_CALL supports_backend)(w_ggml_backend_buffer_type_t buft, w_ggml_backend_t backend); // check if the buffer type is usable by the backend
        // check if tensor data is in host memory
        // should be equivalent to supports_backend(buft, w_ggml_backend_cpu_init())
        bool                  (*W_GGML_CALL is_host)         (w_ggml_backend_buffer_type_t buft);
    };

    struct w_ggml_backend_buffer_type {
        struct w_ggml_backend_buffer_type_i  iface;
        w_ggml_backend_buffer_type_context_t context;
    };

    // buffer
    typedef void * w_ggml_backend_buffer_context_t;

    struct w_ggml_backend_buffer_i {
        const char * (*W_GGML_CALL get_name)   (w_ggml_backend_buffer_t buffer);
        void         (*W_GGML_CALL free_buffer)(w_ggml_backend_buffer_t buffer);
        void *       (*W_GGML_CALL get_base)   (w_ggml_backend_buffer_t buffer);
        void         (*W_GGML_CALL init_tensor)(w_ggml_backend_buffer_t buffer, struct w_ggml_tensor * tensor);
        void         (*W_GGML_CALL set_tensor) (w_ggml_backend_buffer_t buffer,       struct w_ggml_tensor * tensor, const void * data, size_t offset, size_t size);
        void         (*W_GGML_CALL get_tensor) (w_ggml_backend_buffer_t buffer, const struct w_ggml_tensor * tensor,       void * data, size_t offset, size_t size);
        bool         (*W_GGML_CALL cpy_tensor) (w_ggml_backend_buffer_t buffer, const struct w_ggml_tensor * src, struct w_ggml_tensor * dst); // dst is in the buffer, src may be in any buffer
        void         (*W_GGML_CALL clear)      (w_ggml_backend_buffer_t buffer, uint8_t value);
        void         (*W_GGML_CALL reset)      (w_ggml_backend_buffer_t buffer); // reset any internal state due to tensor initialization, such as tensor extras
    };

    struct w_ggml_backend_buffer {
        struct w_ggml_backend_buffer_i  iface;
        w_ggml_backend_buffer_type_t    buft;
        w_ggml_backend_buffer_context_t context;
        size_t size;
        enum w_ggml_backend_buffer_usage usage;
    };

    W_GGML_CALL w_ggml_backend_buffer_t w_ggml_backend_buffer_init(
                   w_ggml_backend_buffer_type_t      buft,
            struct w_ggml_backend_buffer_i           iface,
                   w_ggml_backend_buffer_context_t   context,
                   size_t                          size);

    // do not use directly, use w_ggml_backend_tensor_copy instead
    bool w_ggml_backend_buffer_copy_tensor(const struct w_ggml_tensor * src, struct w_ggml_tensor * dst);

    // buffer that contains a collection of buffers
    W_GGML_CALL w_ggml_backend_buffer_t w_ggml_backend_multi_buffer_alloc_buffer(w_ggml_backend_buffer_t * buffers, size_t n_buffers);
    W_GGML_CALL bool                  w_ggml_backend_buffer_is_multi_buffer(w_ggml_backend_buffer_t buffer);
    W_GGML_CALL void                  w_ggml_backend_multi_buffer_set_usage(w_ggml_backend_buffer_t buffer, enum w_ggml_backend_buffer_usage usage);

    //
    // Backend
    //

    typedef void * w_ggml_backend_context_t;

    struct w_ggml_backend_i {
        const char * (*W_GGML_CALL get_name)(w_ggml_backend_t backend);

        void (*W_GGML_CALL free)(w_ggml_backend_t backend);

        // buffer allocation
        w_ggml_backend_buffer_type_t (*W_GGML_CALL get_default_buffer_type)(w_ggml_backend_t backend);

        // (optional) asynchronous tensor data access
        void (*W_GGML_CALL set_tensor_async)(w_ggml_backend_t backend,       struct w_ggml_tensor * tensor, const void * data, size_t offset, size_t size);
        void (*W_GGML_CALL get_tensor_async)(w_ggml_backend_t backend, const struct w_ggml_tensor * tensor,       void * data, size_t offset, size_t size);
        bool (*W_GGML_CALL cpy_tensor_async)(w_ggml_backend_t backend_src, w_ggml_backend_t backend_dst, const struct w_ggml_tensor * src, struct w_ggml_tensor * dst);

        // (optional) complete all pending operations
        void (*W_GGML_CALL synchronize)(w_ggml_backend_t backend);

        // compute graph with a plan (not used currently)
        w_ggml_backend_graph_plan_t (*W_GGML_CALL graph_plan_create) (w_ggml_backend_t backend, const struct w_ggml_cgraph * cgraph);
        void                      (*W_GGML_CALL graph_plan_free)   (w_ggml_backend_t backend, w_ggml_backend_graph_plan_t plan);

        // compute graph with a plan
        enum w_ggml_status (*W_GGML_CALL graph_plan_compute)(w_ggml_backend_t backend, w_ggml_backend_graph_plan_t plan);
        // compute graph without a plan (async)
        enum w_ggml_status (*W_GGML_CALL graph_compute)     (w_ggml_backend_t backend, struct w_ggml_cgraph * cgraph);

        // check if the backend supports an operation
        bool (*W_GGML_CALL supports_op)(w_ggml_backend_t backend, const struct w_ggml_tensor * op);

        // check if the backend wants to run an operation, even if the weights are allocated in a CPU buffer
        // these should be expensive operations with large batch sizes that may benefit from running on this backend
        // even if the weight has to be copied from the CPU temporarily
        bool (*W_GGML_CALL offload_op)(w_ggml_backend_t backend, const struct w_ggml_tensor * op);

        // (optional) event synchronization
        w_ggml_backend_event_t (*W_GGML_CALL event_new)         (w_ggml_backend_t backend);
        void                 (*W_GGML_CALL event_free)        (w_ggml_backend_event_t event);
        void                 (*W_GGML_CALL event_record)      (w_ggml_backend_event_t event);
        void                 (*W_GGML_CALL event_wait)        (w_ggml_backend_t backend, w_ggml_backend_event_t event);
        void                 (*W_GGML_CALL event_synchronize) (w_ggml_backend_event_t event);
    };

    struct w_ggml_backend {
        w_ggml_guid_t guid;

        struct w_ggml_backend_i iface;
        w_ggml_backend_context_t context;
    };

    struct w_ggml_backend_event {
        w_ggml_backend_t backend;
        void * context;
    };

    //
    // Backend registry
    //

    typedef w_ggml_backend_t (*W_GGML_CALL w_ggml_backend_init_fn)(const char * params, void * user_data);

    W_GGML_CALL void w_ggml_backend_register(const char * name, w_ggml_backend_init_fn init_fn, w_ggml_backend_buffer_type_t default_buffer_type, void * user_data);

#ifdef  __cplusplus
}
#endif
