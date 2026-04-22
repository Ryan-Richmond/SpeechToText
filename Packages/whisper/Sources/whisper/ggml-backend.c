#include "ggml-backend-impl.h"
#include "ggml-alloc.h"
#include "ggml-impl.h"

#include <assert.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define MAX(a, b) ((a) > (b) ? (a) : (b))

// backend buffer type

const char * w_ggml_backend_buft_name(w_ggml_backend_buffer_type_t buft) {
    return buft->iface.get_name(buft);
}

W_GGML_CALL w_ggml_backend_buffer_t w_ggml_backend_buft_alloc_buffer(w_ggml_backend_buffer_type_t buft, size_t size) {
    return buft->iface.alloc_buffer(buft, size);
}

size_t w_ggml_backend_buft_get_alignment(w_ggml_backend_buffer_type_t buft) {
    return buft->iface.get_alignment(buft);
}

size_t w_ggml_backend_buft_get_max_size(w_ggml_backend_buffer_type_t buft) {
    // get_max_size is optional, defaults to SIZE_MAX
    if (buft->iface.get_max_size) {
        return buft->iface.get_max_size(buft);
    }
    return SIZE_MAX;
}

W_GGML_CALL size_t w_ggml_backend_buft_get_alloc_size(w_ggml_backend_buffer_type_t buft, struct w_ggml_tensor * tensor) {
    // get_alloc_size is optional, defaults to w_ggml_nbytes
    if (buft->iface.get_alloc_size) {
        size_t size = buft->iface.get_alloc_size(buft, tensor);
        assert(size >= w_ggml_nbytes(tensor));
        return size;
    }
    return w_ggml_nbytes(tensor);
}

bool w_ggml_backend_buft_supports_backend(w_ggml_backend_buffer_type_t buft, w_ggml_backend_t backend) {
    return buft->iface.supports_backend(buft, backend);
}

bool w_ggml_backend_buft_is_host(w_ggml_backend_buffer_type_t buft) {
    if (buft->iface.is_host) {
        return buft->iface.is_host(buft);
    }
    return false;
}

// backend buffer

W_GGML_CALL w_ggml_backend_buffer_t w_ggml_backend_buffer_init(
               w_ggml_backend_buffer_type_t      buft,
        struct w_ggml_backend_buffer_i           iface,
               w_ggml_backend_buffer_context_t   context,
               size_t                          size) {
    w_ggml_backend_buffer_t buffer = malloc(sizeof(struct w_ggml_backend_buffer));

    (*buffer) = (struct w_ggml_backend_buffer) {
        /* .interface = */ iface,
        /* .buft      = */ buft,
        /* .context   = */ context,
        /* .size      = */ size,
        /* .usage     = */ W_GGML_BACKEND_BUFFER_USAGE_ANY
    };

    return buffer;
}

const char * w_ggml_backend_buffer_name(w_ggml_backend_buffer_t buffer) {
    return buffer->iface.get_name(buffer);
}

void w_ggml_backend_buffer_free(w_ggml_backend_buffer_t buffer) {
    if (buffer == NULL) {
        return;
    }

    if (buffer->iface.free_buffer != NULL) {
        buffer->iface.free_buffer(buffer);
    }
    free(buffer);
}

size_t w_ggml_backend_buffer_get_size(w_ggml_backend_buffer_t buffer) {
    return buffer->size;
}

void * w_ggml_backend_buffer_get_base(w_ggml_backend_buffer_t buffer) {
    void * base = buffer->iface.get_base(buffer);

    W_GGML_ASSERT(base != NULL && "backend buffer base cannot be NULL");

    return base;
}

W_GGML_CALL void w_ggml_backend_buffer_init_tensor(w_ggml_backend_buffer_t buffer, struct w_ggml_tensor * tensor) {
    // init_tensor is optional
    if (buffer->iface.init_tensor) {
        buffer->iface.init_tensor(buffer, tensor);
    }
}

size_t w_ggml_backend_buffer_get_alignment (w_ggml_backend_buffer_t buffer) {
    return w_ggml_backend_buft_get_alignment(w_ggml_backend_buffer_get_type(buffer));
}

size_t w_ggml_backend_buffer_get_max_size(w_ggml_backend_buffer_t buffer) {
    return w_ggml_backend_buft_get_max_size(w_ggml_backend_buffer_get_type(buffer));
}

size_t w_ggml_backend_buffer_get_alloc_size(w_ggml_backend_buffer_t buffer, struct w_ggml_tensor * tensor) {
    return w_ggml_backend_buft_get_alloc_size(w_ggml_backend_buffer_get_type(buffer), tensor);
}

void w_ggml_backend_buffer_clear(w_ggml_backend_buffer_t buffer, uint8_t value) {
    buffer->iface.clear(buffer, value);
}

bool w_ggml_backend_buffer_is_host(w_ggml_backend_buffer_t buffer) {
    return w_ggml_backend_buft_is_host(w_ggml_backend_buffer_get_type(buffer));
}

void w_ggml_backend_buffer_set_usage(w_ggml_backend_buffer_t buffer, enum w_ggml_backend_buffer_usage usage) {
    buffer->usage = usage;

    // FIXME: add a generic callback to the buffer interface
    if (w_ggml_backend_buffer_is_multi_buffer(buffer)) {
        w_ggml_backend_multi_buffer_set_usage(buffer, usage);
    }
}

w_ggml_backend_buffer_type_t w_ggml_backend_buffer_get_type(w_ggml_backend_buffer_t buffer) {
    return buffer->buft;
}

void w_ggml_backend_buffer_reset(w_ggml_backend_buffer_t buffer) {
    if (buffer->iface.reset) {
        buffer->iface.reset(buffer);
    }
}

bool w_ggml_backend_buffer_copy_tensor(const struct w_ggml_tensor * src, struct w_ggml_tensor * dst) {
    w_ggml_backend_buffer_t dst_buf = dst->view_src ? dst->view_src->buffer : dst->buffer;
    if (dst_buf->iface.cpy_tensor) {
        return src->buffer->iface.cpy_tensor(dst_buf, src, dst);
    }
    return false;
}

// backend

w_ggml_guid_t w_ggml_backend_guid(w_ggml_backend_t backend) {
    if (backend == NULL) {
        return NULL;
    }
    return backend->guid;
}

const char * w_ggml_backend_name(w_ggml_backend_t backend) {
    if (backend == NULL) {
        return "NULL";
    }
    return backend->iface.get_name(backend);
}

void w_ggml_backend_free(w_ggml_backend_t backend) {
    if (backend == NULL) {
        return;
    }

    backend->iface.free(backend);
}

w_ggml_backend_buffer_type_t w_ggml_backend_get_default_buffer_type(w_ggml_backend_t backend) {
    return backend->iface.get_default_buffer_type(backend);
}

w_ggml_backend_buffer_t w_ggml_backend_alloc_buffer(w_ggml_backend_t backend, size_t size) {
    return w_ggml_backend_buft_alloc_buffer(w_ggml_backend_get_default_buffer_type(backend), size);
}

size_t w_ggml_backend_get_alignment(w_ggml_backend_t backend) {
    return w_ggml_backend_buft_get_alignment(w_ggml_backend_get_default_buffer_type(backend));
}

size_t w_ggml_backend_get_max_size(w_ggml_backend_t backend) {
    return w_ggml_backend_buft_get_max_size(w_ggml_backend_get_default_buffer_type(backend));
}

void w_ggml_backend_tensor_set_async(w_ggml_backend_t backend, struct w_ggml_tensor * tensor, const void * data, size_t offset, size_t size) {
    W_GGML_ASSERT(tensor->data != NULL && "tensor not allocated");
    W_GGML_ASSERT(offset + size <= w_ggml_nbytes(tensor) && "tensor write out of bounds");

    if (backend->iface.set_tensor_async == NULL) {
        w_ggml_backend_tensor_set(tensor, data, offset, size);
    } else {
        backend->iface.set_tensor_async(backend, tensor, data, offset, size);
    }
}

void w_ggml_backend_tensor_get_async(w_ggml_backend_t backend, const struct w_ggml_tensor * tensor, void * data, size_t offset, size_t size) {
    W_GGML_ASSERT(tensor->data != NULL && "tensor not allocated");
    W_GGML_ASSERT(offset + size <= w_ggml_nbytes(tensor) && "tensor read out of bounds");

    if (backend->iface.get_tensor_async == NULL) {
        w_ggml_backend_tensor_get(tensor, data, offset, size);
    } else {
        backend->iface.get_tensor_async(backend, tensor, data, offset, size);
    }
}

W_GGML_CALL void w_ggml_backend_tensor_set(struct w_ggml_tensor * tensor, const void * data, size_t offset, size_t size) {
    w_ggml_backend_buffer_t buf = tensor->view_src ? tensor->view_src->buffer : tensor->buffer;

    W_GGML_ASSERT(buf != NULL && "tensor buffer not set");
    W_GGML_ASSERT(tensor->data != NULL && "tensor not allocated");
    W_GGML_ASSERT(offset + size <= w_ggml_nbytes(tensor) && "tensor write out of bounds");

    if (!size) {
        return;
    }

    buf->iface.set_tensor(buf, tensor, data, offset, size);
}

W_GGML_CALL void w_ggml_backend_tensor_get(const struct w_ggml_tensor * tensor, void * data, size_t offset, size_t size) {
    w_ggml_backend_buffer_t buf = tensor->view_src ? tensor->view_src->buffer : tensor->buffer;

    W_GGML_ASSERT(buf != NULL && "tensor buffer not set");
    W_GGML_ASSERT(tensor->data != NULL && "tensor not allocated");
    W_GGML_ASSERT(offset + size <= w_ggml_nbytes(tensor) && "tensor read out of bounds");

    if (!size) {
        return;
    }

    buf->iface.get_tensor(buf, tensor, data, offset, size);
}

void w_ggml_backend_synchronize(w_ggml_backend_t backend) {
    if (backend->iface.synchronize == NULL) {
        return;
    }

    backend->iface.synchronize(backend);
}

w_ggml_backend_graph_plan_t w_ggml_backend_graph_plan_create(w_ggml_backend_t backend, struct w_ggml_cgraph * cgraph) {
    W_GGML_ASSERT(backend->iface.graph_plan_create != NULL);

    return backend->iface.graph_plan_create(backend, cgraph);
}

void w_ggml_backend_graph_plan_free(w_ggml_backend_t backend, w_ggml_backend_graph_plan_t plan) {
    W_GGML_ASSERT(backend->iface.graph_plan_free != NULL);

    backend->iface.graph_plan_free(backend, plan);
}

enum w_ggml_status w_ggml_backend_graph_plan_compute(w_ggml_backend_t backend, w_ggml_backend_graph_plan_t plan) {
    W_GGML_ASSERT(backend->iface.graph_plan_compute != NULL);

    return backend->iface.graph_plan_compute(backend, plan);
}

enum w_ggml_status w_ggml_backend_graph_compute(w_ggml_backend_t backend, struct w_ggml_cgraph * cgraph) {
    enum w_ggml_status err = w_ggml_backend_graph_compute_async(backend, cgraph);
    w_ggml_backend_synchronize(backend);
    return err;
}

enum w_ggml_status w_ggml_backend_graph_compute_async(w_ggml_backend_t backend, struct w_ggml_cgraph * cgraph) {
    return backend->iface.graph_compute(backend, cgraph);
}

bool w_ggml_backend_supports_op(w_ggml_backend_t backend, const struct w_ggml_tensor * op) {
    return backend->iface.supports_op(backend, op);
}

bool w_ggml_backend_offload_op(w_ggml_backend_t backend, const struct w_ggml_tensor * op) {
    if (backend->iface.offload_op != NULL) {
        return backend->iface.offload_op(backend, op);
    }
    return false;
}

// backend copy

static bool w_ggml_are_same_layout(const struct w_ggml_tensor * a, const struct w_ggml_tensor * b) {
    if (a->type != b->type) {
        return false;
    }
    for (int i = 0; i < W_GGML_MAX_DIMS; i++) {
        if (a->ne[i] != b->ne[i]) {
            return false;
        }
        if (a->nb[i] != b->nb[i]) {
            return false;
        }
    }
    return true;
}

void w_ggml_backend_tensor_copy(struct w_ggml_tensor * src, struct w_ggml_tensor * dst) {
    W_GGML_ASSERT(w_ggml_are_same_layout(src, dst) && "cannot copy tensors with different layouts");

    if (src == dst) {
        return;
    }

    if (w_ggml_backend_buffer_is_host(src->buffer)) {
        w_ggml_backend_tensor_set(dst, src->data, 0, w_ggml_nbytes(src));
    } else if (w_ggml_backend_buffer_is_host(dst->buffer)) {
        w_ggml_backend_tensor_get(src, dst->data, 0, w_ggml_nbytes(src));
    } else if (!w_ggml_backend_buffer_copy_tensor(src, dst)) {
#ifndef NDEBUG
        fprintf(stderr, "%s: warning: slow copy from %s to %s\n", __func__, w_ggml_backend_buffer_name(src->buffer), w_ggml_backend_buffer_name(dst->buffer));
#endif
        size_t nbytes = w_ggml_nbytes(src);
        void * data = malloc(nbytes);
        w_ggml_backend_tensor_get(src, data, 0, nbytes);
        w_ggml_backend_tensor_set(dst, data, 0, nbytes);
        free(data);
    }
}

void w_ggml_backend_tensor_copy_async(w_ggml_backend_t backend_src, w_ggml_backend_t backend_dst, struct w_ggml_tensor * src, struct w_ggml_tensor * dst) {
    W_GGML_ASSERT(w_ggml_are_same_layout(src, dst) && "cannot copy tensors with different layouts");

    if (src == dst) {
        return;
    }

    if (backend_dst->iface.cpy_tensor_async != NULL) {
        if (backend_dst->iface.cpy_tensor_async(backend_src, backend_dst, src, dst)) {
            return;
        }
    }

    // an async copy would normally happen after all the queued operations on both backends are completed
    // sync src, set_async dst
    if (w_ggml_backend_buffer_is_host(src->buffer)) {
        w_ggml_backend_synchronize(backend_src);
        w_ggml_backend_tensor_set_async(backend_dst, dst, src->data, 0, w_ggml_nbytes(src));
    } else {
        w_ggml_backend_synchronize(backend_src);
        w_ggml_backend_tensor_copy(src, dst);
        w_ggml_backend_synchronize(backend_dst);
    }
}

// events

w_ggml_backend_event_t w_ggml_backend_event_new(w_ggml_backend_t backend) {
    if (backend->iface.event_new == NULL) {
        return NULL;
    }
    return backend->iface.event_new(backend);
}

void w_ggml_backend_event_free(w_ggml_backend_event_t event) {
    if (event == NULL) {
        return;
    }
    event->backend->iface.event_free(event);
}

void w_ggml_backend_event_record(w_ggml_backend_event_t event) {
    W_GGML_ASSERT(event->backend->iface.event_record != NULL);

    event->backend->iface.event_record(event);
}

void w_ggml_backend_event_synchronize(w_ggml_backend_event_t event) {
    W_GGML_ASSERT(event->backend->iface.event_synchronize != NULL);

    event->backend->iface.event_synchronize(event);
}

void w_ggml_backend_event_wait(w_ggml_backend_t backend, w_ggml_backend_event_t event) {
    W_GGML_ASSERT(backend->iface.event_wait != NULL);

    backend->iface.event_wait(backend, event);
}

// backend registry

#define W_GGML_REG_MAX_BACKENDS 16

struct w_ggml_backend_reg {
    char name[128];
    w_ggml_backend_init_fn init_fn;
    w_ggml_backend_buffer_type_t default_buffer_type;
    void * user_data;
};

static struct w_ggml_backend_reg w_ggml_backend_registry[W_GGML_REG_MAX_BACKENDS];
static size_t w_ggml_backend_registry_count = 0;

W_GGML_CALL static w_ggml_backend_t w_ggml_backend_reg_cpu_init(const char * params, void * user_data);

W_GGML_CALL static void w_ggml_backend_registry_init(void) {
    static bool initialized = false;

    if (initialized) {
        return;
    }

    initialized = true;

    w_ggml_backend_register("CPU", w_ggml_backend_reg_cpu_init, w_ggml_backend_cpu_buffer_type(), NULL);

    // add forward decls here to avoid including the backend headers
#ifdef W_GGML_USE_CUDA
    extern W_GGML_CALL void w_ggml_backend_cuda_reg_devices(void);
    w_ggml_backend_cuda_reg_devices();
#endif

#ifdef W_GGML_USE_SYCL
    extern void w_ggml_backend_sycl_reg_devices(void);
    w_ggml_backend_sycl_reg_devices();
#endif

#ifdef W_GGML_USE_METAL
    extern W_GGML_CALL w_ggml_backend_t w_ggml_backend_reg_metal_init(const char * params, void * user_data);
    extern W_GGML_CALL w_ggml_backend_buffer_type_t w_ggml_backend_metal_buffer_type(void);
    w_ggml_backend_register("Metal", w_ggml_backend_reg_metal_init, w_ggml_backend_metal_buffer_type(), NULL);
#endif

#ifdef W_GGML_USE_VULKAN
    extern W_GGML_CALL int w_ggml_backend_vk_reg_devices(void);
    w_ggml_backend_vk_reg_devices();
#endif

#ifdef W_GGML_USE_KOMPUTE
    extern W_GGML_CALL void w_ggml_backend_kompute_reg_devices(void);
    w_ggml_backend_kompute_reg_devices();
#endif
}

W_GGML_CALL void w_ggml_backend_register(const char * name, w_ggml_backend_init_fn init_fn, w_ggml_backend_buffer_type_t default_buffer_type, void * user_data) {
    W_GGML_ASSERT(w_ggml_backend_registry_count < W_GGML_REG_MAX_BACKENDS);

    size_t id = w_ggml_backend_registry_count;

    w_ggml_backend_registry[id] = (struct w_ggml_backend_reg) {
        /* .name                = */ {0},
        /* .fn                  = */ init_fn,
        /* .default_buffer_type = */ default_buffer_type,
        /* .user_data           = */ user_data,
    };

    snprintf(w_ggml_backend_registry[id].name, sizeof(w_ggml_backend_registry[id].name), "%s", name);

#ifndef NDEBUG
    fprintf(stderr, "%s: registered backend %s\n", __func__, name);
#endif

    w_ggml_backend_registry_count++;
}

size_t w_ggml_backend_reg_get_count(void) {
    w_ggml_backend_registry_init();

    return w_ggml_backend_registry_count;
}

size_t w_ggml_backend_reg_find_by_name(const char * name) {
    w_ggml_backend_registry_init();

    for (size_t i = 0; i < w_ggml_backend_registry_count; i++) {
        // TODO: case insensitive in a portable way
        if (strcmp(w_ggml_backend_registry[i].name, name) == 0) {
            return i;
        }
    }

    // not found
    return SIZE_MAX;
}

// init from backend:params string
w_ggml_backend_t w_ggml_backend_reg_init_backend_from_str(const char * backend_str) {
    w_ggml_backend_registry_init();

    const char * params = strchr(backend_str, ':');
    char backend_name[128];
    if (params == NULL) {
        snprintf(backend_name, sizeof(backend_name), "%s", backend_str);
        params = "";
    } else {
        snprintf(backend_name, sizeof(backend_name), "%.*s", (int)(params - backend_str), backend_str);
        params++;
    }

    size_t backend_i = w_ggml_backend_reg_find_by_name(backend_name);

    if (backend_i == SIZE_MAX) {
        fprintf(stderr, "%s: backend %s not found\n", __func__, backend_name);
        return NULL;
    }

    return w_ggml_backend_reg_init_backend(backend_i, params);
}

const char * w_ggml_backend_reg_get_name(size_t i) {
    w_ggml_backend_registry_init();

    W_GGML_ASSERT(i < w_ggml_backend_registry_count);
    return w_ggml_backend_registry[i].name;
}

w_ggml_backend_t w_ggml_backend_reg_init_backend(size_t i, const char * params) {
    w_ggml_backend_registry_init();

    W_GGML_ASSERT(i < w_ggml_backend_registry_count);
    return w_ggml_backend_registry[i].init_fn(params, w_ggml_backend_registry[i].user_data);
}

w_ggml_backend_buffer_type_t w_ggml_backend_reg_get_default_buffer_type(size_t i) {
    w_ggml_backend_registry_init();

    W_GGML_ASSERT(i < w_ggml_backend_registry_count);
    return w_ggml_backend_registry[i].default_buffer_type;
}

w_ggml_backend_buffer_t w_ggml_backend_reg_alloc_buffer(size_t i, size_t size) {
    w_ggml_backend_registry_init();

    W_GGML_ASSERT(i < w_ggml_backend_registry_count);
    return w_ggml_backend_buft_alloc_buffer(w_ggml_backend_registry[i].default_buffer_type, size);
}

// backend CPU

static const size_t TENSOR_ALIGNMENT = 32; // required for mmap as gguf only guarantees 32-byte alignment

W_GGML_CALL static const char * w_ggml_backend_cpu_buffer_name(w_ggml_backend_buffer_t buffer) {
    return "CPU";

    W_GGML_UNUSED(buffer);
}

W_GGML_CALL static void * w_ggml_backend_cpu_buffer_get_base(w_ggml_backend_buffer_t buffer) {
    uintptr_t data = (uintptr_t)buffer->context;

    // align the buffer
    if (data % TENSOR_ALIGNMENT != 0) {
        data = W_GGML_PAD(data, TENSOR_ALIGNMENT);
    }

    return (void *)data;
}

W_GGML_CALL static void w_ggml_backend_cpu_buffer_free_buffer(w_ggml_backend_buffer_t buffer) {
    free(buffer->context);
}

W_GGML_CALL static void w_ggml_backend_cpu_buffer_set_tensor(w_ggml_backend_buffer_t buffer, struct w_ggml_tensor * tensor, const void * data, size_t offset, size_t size) {
    memcpy((char *)tensor->data + offset, data, size);

    W_GGML_UNUSED(buffer);
}

W_GGML_CALL static void w_ggml_backend_cpu_buffer_get_tensor(w_ggml_backend_buffer_t buffer, const struct w_ggml_tensor * tensor, void * data, size_t offset, size_t size) {
    memcpy(data, (const char *)tensor->data + offset, size);

    W_GGML_UNUSED(buffer);
}

W_GGML_CALL static bool w_ggml_backend_cpu_buffer_cpy_tensor(w_ggml_backend_buffer_t buffer, const struct w_ggml_tensor * src, struct w_ggml_tensor * dst) {
    if (w_ggml_backend_buffer_is_host(src->buffer)) {
        memcpy(dst->data, src->data, w_ggml_nbytes(src));
        return true;
    }
    return false;

    W_GGML_UNUSED(buffer);
}

W_GGML_CALL static void w_ggml_backend_cpu_buffer_clear(w_ggml_backend_buffer_t buffer, uint8_t value) {
    memset(buffer->context, value, buffer->size);
}

static struct w_ggml_backend_buffer_i cpu_backend_buffer_i = {
    /* .get_name        = */ w_ggml_backend_cpu_buffer_name,
    /* .free_buffer     = */ w_ggml_backend_cpu_buffer_free_buffer,
    /* .get_base        = */ w_ggml_backend_cpu_buffer_get_base,
    /* .init_tensor     = */ NULL, // no initialization required
    /* .set_tensor      = */ w_ggml_backend_cpu_buffer_set_tensor,
    /* .get_tensor      = */ w_ggml_backend_cpu_buffer_get_tensor,
    /* .cpy_tensor      = */ w_ggml_backend_cpu_buffer_cpy_tensor,
    /* .clear           = */ w_ggml_backend_cpu_buffer_clear,
    /* .reset           = */ NULL,
};

// for buffers from ptr, free is not called
static struct w_ggml_backend_buffer_i cpu_backend_buffer_i_from_ptr = {
    /* .get_name        = */ w_ggml_backend_cpu_buffer_name,
    /* .free_buffer     = */ NULL, // ptr is not owned by the buffer, so it does not need to be freed
    /* .get_base        = */ w_ggml_backend_cpu_buffer_get_base,
    /* .init_tensor     = */ NULL, // no initialization required
    /* .set_tensor      = */ w_ggml_backend_cpu_buffer_set_tensor,
    /* .get_tensor      = */ w_ggml_backend_cpu_buffer_get_tensor,
    /* .cpy_tensor      = */ w_ggml_backend_cpu_buffer_cpy_tensor,
    /* .clear           = */ w_ggml_backend_cpu_buffer_clear,
    /* .reset           = */ NULL,
};

W_GGML_CALL static const char * w_ggml_backend_cpu_buffer_type_get_name(w_ggml_backend_buffer_type_t buft) {
    return "CPU";

    W_GGML_UNUSED(buft);
}

W_GGML_CALL static w_ggml_backend_buffer_t w_ggml_backend_cpu_buffer_type_alloc_buffer(w_ggml_backend_buffer_type_t buft, size_t size) {
    size += TENSOR_ALIGNMENT;   // malloc may return an address that is not aligned
    void * data = malloc(size); // TODO: use W_GGML_ALIGNED_MALLOC (move to ggml-impl.h)
    if (data == NULL) {
        fprintf(stderr, "%s: failed to allocate buffer of size %zu\n", __func__, size);
        return NULL;
    }

    return w_ggml_backend_buffer_init(buft, cpu_backend_buffer_i, data, size);
}

W_GGML_CALL static size_t w_ggml_backend_cpu_buffer_type_get_alignment(w_ggml_backend_buffer_type_t buft) {
    return TENSOR_ALIGNMENT;

    W_GGML_UNUSED(buft);
}

W_GGML_CALL static bool w_ggml_backend_cpu_buffer_type_supports_backend(w_ggml_backend_buffer_type_t buft, w_ggml_backend_t backend) {
    return w_ggml_backend_is_cpu(backend);

    W_GGML_UNUSED(buft);
}

W_GGML_CALL static bool w_ggml_backend_cpu_buffer_type_is_host(w_ggml_backend_buffer_type_t buft) {
    return true;

    W_GGML_UNUSED(buft);
}

W_GGML_CALL w_ggml_backend_buffer_type_t w_ggml_backend_cpu_buffer_type(void) {
    static struct w_ggml_backend_buffer_type w_ggml_backend_cpu_buffer_type = {
        /* .iface = */ {
            /* .get_name         = */ w_ggml_backend_cpu_buffer_type_get_name,
            /* .alloc_buffer     = */ w_ggml_backend_cpu_buffer_type_alloc_buffer,
            /* .get_alignment    = */ w_ggml_backend_cpu_buffer_type_get_alignment,
            /* .get_max_size     = */ NULL, // defaults to SIZE_MAX
            /* .get_alloc_size   = */ NULL, // defaults to w_ggml_nbytes
            /* .supports_backend = */ w_ggml_backend_cpu_buffer_type_supports_backend,
            /* .is_host          = */ w_ggml_backend_cpu_buffer_type_is_host,
        },
        /* .context = */ NULL,
    };

    return &w_ggml_backend_cpu_buffer_type;
}

#ifdef W_GGML_USE_CPU_HBM

// buffer type HBM

#include <hbwmalloc.h>

W_GGML_CALL static const char * w_ggml_backend_cpu_hbm_buffer_type_get_name(w_ggml_backend_buffer_type_t buft) {
    return "CPU_HBM";

    W_GGML_UNUSED(buft);
}

W_GGML_CALL static const char * w_ggml_backend_cpu_hbm_buffer_get_name(w_ggml_backend_buffer_t buf) {
    return "CPU_HBM";

    W_GGML_UNUSED(buf);
}

W_GGML_CALL static void w_ggml_backend_cpu_hbm_buffer_free_buffer(w_ggml_backend_buffer_t buffer) {
    hbw_free(buffer->context);
}

W_GGML_CALL static w_ggml_backend_buffer_t w_ggml_backend_cpu_hbm_buffer_type_alloc_buffer(w_ggml_backend_buffer_type_t buft, size_t size) {
    //void * ptr = hbw_malloc(size);
    void * ptr;
    int result = hbw_posix_memalign(&ptr, w_ggml_backend_cpu_buffer_type_get_alignment(buft), size);
    if (result != 0) {
        fprintf(stderr, "failed to allocate HBM buffer of size %zu\n", size);
        return NULL;
    }

    w_ggml_backend_buffer_t buffer = w_ggml_backend_cpu_buffer_from_ptr(ptr, size);
    buffer->buft = buft;
    buffer->iface.get_name = w_ggml_backend_cpu_hbm_buffer_get_name;
    buffer->iface.free_buffer = w_ggml_backend_cpu_hbm_buffer_free_buffer;

    return buffer;
}

w_ggml_backend_buffer_type_t w_ggml_backend_cpu_hbm_buffer_type(void) {
    static struct w_ggml_backend_buffer_type w_ggml_backend_cpu_buffer_type_hbm = {
        /* .iface    = */ {
            /* .get_name         = */ w_ggml_backend_cpu_hbm_buffer_type_get_name,
            /* .alloc_buffer     = */ w_ggml_backend_cpu_hbm_buffer_type_alloc_buffer,
            /* .get_alignment    = */ w_ggml_backend_cpu_buffer_type_get_alignment,
            /* .get_max_size     = */ NULL, // defaults to SIZE_MAX
            /* .get_alloc_size   = */ NULL, // defaults to w_ggml_nbytes
            /* .supports_backend = */ w_ggml_backend_cpu_buffer_type_supports_backend,
            /* .is_host          = */ w_ggml_backend_cpu_buffer_type_is_host,
        },
        /* .context  = */ NULL,
    };

    return &w_ggml_backend_cpu_buffer_type_hbm;
}
#endif

struct w_ggml_backend_cpu_context {
    int n_threads;
    void * work_data;
    size_t work_size;

    w_ggml_abort_callback abort_callback;
    void *              abort_callback_data;
};

W_GGML_CALL static const char * w_ggml_backend_cpu_name(w_ggml_backend_t backend) {
    return "CPU";

    W_GGML_UNUSED(backend);
}

W_GGML_CALL static void w_ggml_backend_cpu_free(w_ggml_backend_t backend) {
    struct w_ggml_backend_cpu_context * cpu_ctx = (struct w_ggml_backend_cpu_context *)backend->context;
    free(cpu_ctx->work_data);
    free(cpu_ctx);
    free(backend);
}

W_GGML_CALL static w_ggml_backend_buffer_type_t w_ggml_backend_cpu_get_default_buffer_type(w_ggml_backend_t backend) {
    return w_ggml_backend_cpu_buffer_type();

    W_GGML_UNUSED(backend);
}

struct w_ggml_backend_plan_cpu {
    struct w_ggml_cplan cplan;
    struct w_ggml_cgraph cgraph;
};

W_GGML_CALL static w_ggml_backend_graph_plan_t w_ggml_backend_cpu_graph_plan_create(w_ggml_backend_t backend, const struct w_ggml_cgraph * cgraph) {
    struct w_ggml_backend_cpu_context * cpu_ctx = (struct w_ggml_backend_cpu_context *)backend->context;

    struct w_ggml_backend_plan_cpu * cpu_plan = malloc(sizeof(struct w_ggml_backend_plan_cpu));

    cpu_plan->cplan = w_ggml_graph_plan(cgraph, cpu_ctx->n_threads);
    cpu_plan->cgraph = *cgraph; // FIXME: deep copy

    if (cpu_plan->cplan.work_size > 0) {
        cpu_plan->cplan.work_data = malloc(cpu_plan->cplan.work_size);
        if (cpu_plan->cplan.work_data == NULL) {
            free(cpu_plan);
            return NULL;
        }
    }

    cpu_plan->cplan.abort_callback      = cpu_ctx->abort_callback;
    cpu_plan->cplan.abort_callback_data = cpu_ctx->abort_callback_data;

    return cpu_plan;
}

W_GGML_CALL static void w_ggml_backend_cpu_graph_plan_free(w_ggml_backend_t backend, w_ggml_backend_graph_plan_t plan) {
    struct w_ggml_backend_plan_cpu * cpu_plan = (struct w_ggml_backend_plan_cpu *)plan;

    free(cpu_plan->cplan.work_data);
    free(cpu_plan);

    W_GGML_UNUSED(backend);
}

W_GGML_CALL static enum w_ggml_status w_ggml_backend_cpu_graph_plan_compute(w_ggml_backend_t backend, w_ggml_backend_graph_plan_t plan) {
    struct w_ggml_backend_plan_cpu * cpu_plan = (struct w_ggml_backend_plan_cpu *)plan;

    return w_ggml_graph_compute(&cpu_plan->cgraph, &cpu_plan->cplan);

    W_GGML_UNUSED(backend);
}

W_GGML_CALL static enum w_ggml_status w_ggml_backend_cpu_graph_compute(w_ggml_backend_t backend, struct w_ggml_cgraph * cgraph) {
    struct w_ggml_backend_cpu_context * cpu_ctx = (struct w_ggml_backend_cpu_context *)backend->context;

    struct w_ggml_cplan cplan = w_ggml_graph_plan(cgraph, cpu_ctx->n_threads);

    if (cpu_ctx->work_size < cplan.work_size) {
        free(cpu_ctx->work_data);
        cpu_ctx->work_data = malloc(cplan.work_size);
        if (cpu_ctx->work_data == NULL) {
            cpu_ctx->work_size = 0;
            return W_GGML_STATUS_ALLOC_FAILED;
        }
        cpu_ctx->work_size = cplan.work_size;
    }
    cplan.work_data = cpu_ctx->work_data;

    cplan.abort_callback      = cpu_ctx->abort_callback;
    cplan.abort_callback_data = cpu_ctx->abort_callback_data;

    return w_ggml_graph_compute(cgraph, &cplan);
}

W_GGML_CALL static bool w_ggml_backend_cpu_supports_op(w_ggml_backend_t backend, const struct w_ggml_tensor * op) {
    switch (op->op) {
        case W_GGML_OP_CPY:
            return
                op->type != W_GGML_TYPE_IQ2_XXS &&
                op->type != W_GGML_TYPE_IQ2_XS  &&
                op->type != W_GGML_TYPE_IQ1_S   &&
                op->type != W_GGML_TYPE_IQ1_M; // missing type_traits.from_float
        case W_GGML_OP_MUL_MAT:
            return op->src[1]->type == W_GGML_TYPE_F32 || op->src[1]->type == w_ggml_internal_get_type_traits(op->src[0]->type).vec_dot_type;
        default:
            return true;
    }

    W_GGML_UNUSED(backend);
}

static struct w_ggml_backend_i cpu_backend_i = {
    /* .get_name                = */ w_ggml_backend_cpu_name,
    /* .free                    = */ w_ggml_backend_cpu_free,
    /* .get_default_buffer_type = */ w_ggml_backend_cpu_get_default_buffer_type,
    /* .set_tensor_async        = */ NULL,
    /* .get_tensor_async        = */ NULL,
    /* .cpy_tensor_async        = */ NULL,
    /* .synchronize             = */ NULL,
    /* .graph_plan_create       = */ w_ggml_backend_cpu_graph_plan_create,
    /* .graph_plan_free         = */ w_ggml_backend_cpu_graph_plan_free,
    /* .graph_plan_compute      = */ w_ggml_backend_cpu_graph_plan_compute,
    /* .graph_compute           = */ w_ggml_backend_cpu_graph_compute,
    /* .supports_op             = */ w_ggml_backend_cpu_supports_op,
    /* .offload_op              = */ NULL,
    /* .event_new               = */ NULL,
    /* .event_free              = */ NULL,
    /* .event_record            = */ NULL,
    /* .event_wait              = */ NULL,
    /* .event_synchronize       = */ NULL,
};

static w_ggml_guid_t w_ggml_backend_cpu_guid(void) {
    static w_ggml_guid guid = { 0xaa, 0x67, 0xc7, 0x43, 0x96, 0xe6, 0xa3, 0x8a, 0xe3, 0xaf, 0xea, 0x92, 0x36, 0xbc, 0xfc, 0x89 };
    return &guid;
}

w_ggml_backend_t w_ggml_backend_cpu_init(void) {
    struct w_ggml_backend_cpu_context * ctx = malloc(sizeof(struct w_ggml_backend_cpu_context));
    if (ctx == NULL) {
        return NULL;
    }

    ctx->n_threads           = W_GGML_DEFAULT_N_THREADS;
    ctx->work_data           = NULL;
    ctx->work_size           = 0;
    ctx->abort_callback      = NULL;
    ctx->abort_callback_data = NULL;

    w_ggml_backend_t cpu_backend = malloc(sizeof(struct w_ggml_backend));
    if (cpu_backend == NULL) {
        free(ctx);
        return NULL;
    }

    *cpu_backend = (struct w_ggml_backend) {
        /* .guid      = */ w_ggml_backend_cpu_guid(),
        /* .interface = */ cpu_backend_i,
        /* .context   = */ ctx
    };
    return cpu_backend;
}

W_GGML_CALL bool w_ggml_backend_is_cpu(w_ggml_backend_t backend) {
    return backend != NULL && w_ggml_guid_matches(backend->guid, w_ggml_backend_cpu_guid());
}

void w_ggml_backend_cpu_set_n_threads(w_ggml_backend_t backend_cpu, int n_threads) {
    W_GGML_ASSERT(w_ggml_backend_is_cpu(backend_cpu));

    struct w_ggml_backend_cpu_context * ctx = (struct w_ggml_backend_cpu_context *)backend_cpu->context;
    ctx->n_threads = n_threads;
}

void w_ggml_backend_cpu_set_abort_callback(w_ggml_backend_t backend_cpu, w_ggml_abort_callback abort_callback, void * abort_callback_data) {
    W_GGML_ASSERT(w_ggml_backend_is_cpu(backend_cpu));

    struct w_ggml_backend_cpu_context * ctx = (struct w_ggml_backend_cpu_context *)backend_cpu->context;
    ctx->abort_callback = abort_callback;
    ctx->abort_callback_data = abort_callback_data;
}

W_GGML_CALL w_ggml_backend_buffer_t w_ggml_backend_cpu_buffer_from_ptr(void * ptr, size_t size) {
    W_GGML_ASSERT((uintptr_t)ptr % TENSOR_ALIGNMENT == 0 && "buffer pointer must be aligned");
    return w_ggml_backend_buffer_init(w_ggml_backend_cpu_buffer_type(), cpu_backend_buffer_i_from_ptr, ptr, size);
}

W_GGML_CALL static w_ggml_backend_t w_ggml_backend_reg_cpu_init(const char * params, void * user_data) {
    return w_ggml_backend_cpu_init();

    W_GGML_UNUSED(params);
    W_GGML_UNUSED(user_data);
}

// multi-buffer buffer

struct w_ggml_backend_multi_buffer_context {
    w_ggml_backend_buffer_t * buffers;
    size_t n_buffers;
};

typedef struct w_ggml_backend_multi_buffer_context * w_ggml_backend_multi_buffer_context_t;

W_GGML_CALL static const char * w_ggml_backend_multi_buffer_get_name(w_ggml_backend_buffer_t buffer) {
    w_ggml_backend_multi_buffer_context_t ctx = (w_ggml_backend_multi_buffer_context_t) buffer->context;

    return ctx->buffers[0]->iface.get_name(ctx->buffers[0]);
}

W_GGML_CALL static void w_ggml_backend_multi_buffer_free_buffer(w_ggml_backend_buffer_t buffer) {
    w_ggml_backend_multi_buffer_context_t ctx = (w_ggml_backend_multi_buffer_context_t) buffer->context;
    for (size_t i = 0; i < ctx->n_buffers; i++) {
        w_ggml_backend_buffer_free(ctx->buffers[i]);
    }

    free(ctx->buffers);
    free(ctx);
}

W_GGML_CALL static void w_ggml_backend_multi_buffer_clear(w_ggml_backend_buffer_t buffer, uint8_t value) {
    w_ggml_backend_multi_buffer_context_t ctx = (w_ggml_backend_multi_buffer_context_t) buffer->context;
    for (size_t i = 0; i < ctx->n_buffers; i++) {
        w_ggml_backend_buffer_clear(ctx->buffers[i], value);
    }
}

static struct w_ggml_backend_buffer_i w_ggml_backend_multi_buffer_context_interface(void) {
    static struct w_ggml_backend_buffer_i multi_backend_buffer_i = {
        /* .get_name        = */ w_ggml_backend_multi_buffer_get_name,
        /* .free_buffer     = */ w_ggml_backend_multi_buffer_free_buffer,
        /* .get_base        = */ NULL,
        /* .init_tensor     = */ NULL,
        /* .set_tensor      = */ NULL,
        /* .get_tensor      = */ NULL,
        /* .cpy_tensor      = */ NULL,
        /* .clear           = */ w_ggml_backend_multi_buffer_clear,
        /* .reset           = */ NULL,
    };

    return multi_backend_buffer_i;
}

W_GGML_CALL w_ggml_backend_buffer_t w_ggml_backend_multi_buffer_alloc_buffer(w_ggml_backend_buffer_t * buffers, size_t n_buffers) {
    w_ggml_backend_multi_buffer_context_t ctx = (w_ggml_backend_multi_buffer_context_t) malloc(sizeof(struct w_ggml_backend_multi_buffer_context));
    ctx->n_buffers = n_buffers;
    ctx->buffers = (w_ggml_backend_buffer_t *) malloc(n_buffers * sizeof(w_ggml_backend_buffer_t));

    W_GGML_ASSERT(ctx->buffers != NULL);

    size_t total_size = 0;
    for (size_t i = 0; i < n_buffers; i++) {
        ctx->buffers[i] = buffers[i];
        total_size += w_ggml_backend_buffer_get_size(buffers[i]);
    }

    return w_ggml_backend_buffer_init(buffers[0]->buft, w_ggml_backend_multi_buffer_context_interface(), ctx, total_size);
}

W_GGML_CALL bool w_ggml_backend_buffer_is_multi_buffer(w_ggml_backend_buffer_t buffer) {
    return buffer->iface.get_name == w_ggml_backend_multi_buffer_get_name;
}

W_GGML_CALL void w_ggml_backend_multi_buffer_set_usage(w_ggml_backend_buffer_t buffer, enum w_ggml_backend_buffer_usage usage) {
    W_GGML_ASSERT(w_ggml_backend_buffer_is_multi_buffer(buffer));
    w_ggml_backend_multi_buffer_context_t ctx = (w_ggml_backend_multi_buffer_context_t) buffer->context;
    for (size_t i = 0; i < ctx->n_buffers; i++) {
        w_ggml_backend_buffer_set_usage(ctx->buffers[i], usage);
    }
}

// creates a copy of the tensor with the same memory layout
static struct w_ggml_tensor * w_ggml_dup_tensor_layout(struct w_ggml_context * ctx, const struct w_ggml_tensor * tensor) {
    struct w_ggml_tensor * dup = w_ggml_dup_tensor(ctx, tensor);
    for (int i = 0; i < W_GGML_MAX_DIMS; i++) {
        dup->nb[i] = tensor->nb[i];
    }
    return dup;
}

static bool w_ggml_is_view_op(enum w_ggml_op op) {
    return op == W_GGML_OP_VIEW || op == W_GGML_OP_RESHAPE || op == W_GGML_OP_PERMUTE || op == W_GGML_OP_TRANSPOSE;
}

// scheduler

#ifndef W_GGML_SCHED_MAX_BACKENDS
#define W_GGML_SCHED_MAX_BACKENDS 16
#endif

#ifndef W_GGML_SCHED_MAX_SPLITS
#define W_GGML_SCHED_MAX_SPLITS 2048
#endif

#ifndef W_GGML_SCHED_MAX_SPLIT_INPUTS
#define W_GGML_SCHED_MAX_SPLIT_INPUTS W_GGML_MAX_SRC
#endif

#ifndef W_GGML_SCHED_MAX_COPIES
#define W_GGML_SCHED_MAX_COPIES 4
#endif

struct w_ggml_backend_sched_split {
    int backend_id;
    int i_start;
    int i_end;
    struct w_ggml_tensor * inputs[W_GGML_SCHED_MAX_SPLIT_INPUTS];
    int n_inputs;
    // graph view of this split
    struct w_ggml_cgraph graph;
};

struct w_ggml_backend_sched {
    bool is_reset; // true if the scheduler has been reset since the last graph split
    bool is_alloc;

    int n_backends;

    w_ggml_backend_t backends[W_GGML_SCHED_MAX_BACKENDS];
    w_ggml_backend_buffer_type_t bufts[W_GGML_SCHED_MAX_BACKENDS];
    w_ggml_gallocr_t galloc;

    // hash keys of the nodes in the graph
    struct w_ggml_hash_set    hash_set;
    // hash values
    int * tensor_backend_id;
    struct w_ggml_tensor * (* tensor_copies)[W_GGML_SCHED_MAX_BACKENDS][W_GGML_SCHED_MAX_COPIES];

    int * node_backend_ids; // [graph_size]
    int * leaf_backend_ids; // [graph_size]

    // copy of the graph with modified inputs
    struct w_ggml_cgraph * graph;

    // graph splits
    struct w_ggml_backend_sched_split * splits;
    int n_splits;
    int splits_capacity;

    // pipeline parallelism support
    int n_copies;
    int cur_copy;
    w_ggml_backend_event_t events[W_GGML_SCHED_MAX_BACKENDS][W_GGML_SCHED_MAX_COPIES];
    struct w_ggml_tensor * graph_inputs[W_GGML_SCHED_MAX_SPLIT_INPUTS];
    int n_graph_inputs;

    struct w_ggml_context * ctx;

    w_ggml_backend_sched_eval_callback callback_eval;
    void * callback_eval_user_data;

    // align context_buffer to W_GGML_MEM_ALIGN
#ifdef _MSC_VER
    __declspec(align(W_GGML_MEM_ALIGN))
#else
    __attribute__((aligned(W_GGML_MEM_ALIGN)))
#endif
    char context_buffer[W_GGML_SCHED_MAX_SPLITS*W_GGML_SCHED_MAX_SPLIT_INPUTS*2*sizeof(struct w_ggml_tensor) + sizeof(struct w_ggml_cgraph)];
};

#define hash_id(tensor) w_ggml_hash_find_or_insert(sched->hash_set, tensor)
#define tensor_backend_id(tensor) sched->tensor_backend_id[hash_id(tensor)]

// returns the priority of the backend, lower id is higher priority
static int w_ggml_backend_sched_backend_id(w_ggml_backend_sched_t sched, w_ggml_backend_t backend) {
    for (int i = 0; i < sched->n_backends; i++) {
        if (sched->backends[i] == backend) {
            return i;
        }
    }
    return -1;
}

static int w_ggml_backend_sched_backend_from_buffer(w_ggml_backend_sched_t sched, const struct w_ggml_tensor * tensor) {
    w_ggml_backend_buffer_t buffer = tensor->buffer;
    if (buffer == NULL) {
        return -1;
    }

    // find highest prio backend that supports the buffer type
    for (int i = 0; i < sched->n_backends; i++) {
        if (w_ggml_backend_buft_supports_backend(buffer->buft, sched->backends[i])) {
            return i;
        }
    }

    fprintf(stderr, "%s: error: no backend supports buffer type %s used in tensor %s\n",
        __func__, w_ggml_backend_buffer_name(buffer), tensor->name);
    W_GGML_ASSERT(false);

    return -1;
}

#if 0
static char causes[W_GGML_DEFAULT_GRAPH_SIZE*16 + W_GGML_SCHED_MAX_SPLITS*W_GGML_SCHED_MAX_SPLIT_INPUTS][128]; // debug only
#define SET_CAUSE(node, ...) sprintf(causes[hash_id(node)], __VA_ARGS__)
#define GET_CAUSE(node) causes[hash_id(node)]
#else
#define SET_CAUSE(node, ...)
#define GET_CAUSE(node) ""
#endif

// returns the backend that should be used for the node based on the current locations
static int w_ggml_backend_sched_backend_id_from_cur(w_ggml_backend_sched_t sched, struct w_ggml_tensor * tensor) {
    // TODO: use supports_op to check if the backend supports the op

    // assign pre-allocated nodes to their backend
    int cur_backend_id = w_ggml_backend_sched_backend_from_buffer(sched, tensor);
    if (cur_backend_id != -1) {
        SET_CAUSE(tensor, "1.dst");
        return cur_backend_id;
    }

    // view_src
    if (tensor->view_src != NULL) {
        cur_backend_id = w_ggml_backend_sched_backend_from_buffer(sched, tensor->view_src);
        if (cur_backend_id != -1) {
            SET_CAUSE(tensor, "1.vsrc");
            return cur_backend_id;
        }
    }

    // graph input
    if (tensor->flags & W_GGML_TENSOR_FLAG_INPUT) {
        cur_backend_id = sched->n_backends - 1; // last backend (assumed CPU)
        SET_CAUSE(tensor, "1.inp");
        return cur_backend_id;
    }

    // assign nodes that use weights to the backend of the weights
    // operations with weights are preferably run on the same backend as the weights
    for (int i = 0; i < W_GGML_MAX_SRC; i++) {
        const struct w_ggml_tensor * src = tensor->src[i];
        if (src == NULL) {
            continue;
        }
        if (src->buffer != NULL && src->buffer->usage == W_GGML_BACKEND_BUFFER_USAGE_WEIGHTS) {
            int src_backend_id = w_ggml_backend_sched_backend_from_buffer(sched, src);
            // check if a backend with higher prio wants to offload the op
            if (src_backend_id == sched->n_backends - 1) {
                for (int b = 0; b < src_backend_id; b++) {
                    if (w_ggml_backend_offload_op(sched->backends[b], tensor)) {
                        SET_CAUSE(tensor, "1.off");
                        return b;
                    }
                }
            }
            SET_CAUSE(tensor, "1.wgt%d", i);
            return src_backend_id;
        }
    }

    return -1;
}

static char * fmt_size(size_t size) {
    static char buffer[128];
    if (size >= 1024*1024) {
        snprintf(buffer, sizeof(buffer), "%zuM", size/1024/1024);
    } else {
        snprintf(buffer, sizeof(buffer), "%zuK", size/1024);
    }
    return buffer;
}

static void w_ggml_backend_sched_print_assignments(w_ggml_backend_sched_t sched, struct w_ggml_cgraph * graph) {
    int cur_split = 0;
    for (int i = 0; i < graph->n_nodes; i++) {
        if (cur_split < sched->n_splits && i == sched->splits[cur_split].i_start) {
            w_ggml_backend_t split_backend = sched->backends[sched->splits[cur_split].backend_id];
            fprintf(stderr, "\n## SPLIT #%d: %s # %d inputs: ", cur_split, w_ggml_backend_name(split_backend),
                sched->splits[cur_split].n_inputs);
            for (int j = 0; j < sched->splits[cur_split].n_inputs; j++) {
                fprintf(stderr, "[%s (%5.5s)] ", sched->splits[cur_split].inputs[j]->name,
                    fmt_size(w_ggml_nbytes(sched->splits[cur_split].inputs[j])));
            }
            fprintf(stderr, "\n");
            cur_split++;
        }
        struct w_ggml_tensor * node = graph->nodes[i];
        if (w_ggml_is_view_op(node->op)) {
            continue;
        }
        w_ggml_backend_t tensor_backend = w_ggml_backend_sched_get_tensor_backend(sched, node);
        fprintf(stderr, "node #%3d (%10.10s): %20.20s (%5.5s) [%5.5s %8.8s]:", i, w_ggml_op_name(node->op), node->name,
            fmt_size(w_ggml_nbytes(node)), tensor_backend ? w_ggml_backend_name(tensor_backend) : "NULL", GET_CAUSE(node));
        for (int j = 0; j < W_GGML_MAX_SRC; j++) {
            struct w_ggml_tensor * src = node->src[j];
            if (src == NULL) {
                continue;
            }
            w_ggml_backend_t src_backend = w_ggml_backend_sched_get_tensor_backend(sched, src);
            fprintf(stderr, " %20.20s (%5.5s) [%5.5s %8.8s]", src->name,
                fmt_size(w_ggml_nbytes(src)), src_backend ? w_ggml_backend_name(src_backend) : "NULL", GET_CAUSE(src));
        }
        fprintf(stderr, "\n");
    }
}

//#define DEBUG_PASS1
//#define DEBUG_PASS2
//#define DEBUG_PASS3
//#define DEBUG_PASS4

// assigns backends to ops and splits the graph into subgraphs that can be computed on the same backend
static void w_ggml_backend_sched_split_graph(w_ggml_backend_sched_t sched, struct w_ggml_cgraph * graph) {
    // reset splits
    sched->n_splits = 0;
    sched->n_graph_inputs = 0;
    sched->is_reset = false;

    struct w_ggml_init_params params = {
        /* .mem_size =   */ sizeof(sched->context_buffer),
        /* .mem_buffer = */ sched->context_buffer,
        /* .no_alloc =   */ true
    };

    w_ggml_free(sched->ctx);

    sched->ctx = w_ggml_init(params);
    if (sched->ctx == NULL) {
        fprintf(stderr, "%s: failed to initialize context\n", __func__);
        W_GGML_ASSERT(false);
    }

    // pass 1: assign backends to ops with pre-allocated inputs
    for (int i = 0; i < graph->n_leafs; i++) {
        struct w_ggml_tensor * leaf = graph->leafs[i];
        int * leaf_backend_id = &tensor_backend_id(leaf);
        if (*leaf_backend_id != -1) {
            // do not overwrite user assignments
            continue;
        }
        *leaf_backend_id = w_ggml_backend_sched_backend_id_from_cur(sched, leaf);
    }

    for (int i = 0; i < graph->n_nodes; i++) {
        struct w_ggml_tensor * node = graph->nodes[i];
        int * node_backend_id = &tensor_backend_id(node);
        if (*node_backend_id != -1) {
            // do not overwrite user assignments
            continue;
        }
        *node_backend_id = w_ggml_backend_sched_backend_id_from_cur(sched, node);
        // src
        for (int j = 0; j < W_GGML_MAX_SRC; j++) {
            struct w_ggml_tensor * src = node->src[j];
            if (src == NULL) {
                continue;
            }
            int * src_backend_id = &tensor_backend_id(src);
            if (*src_backend_id == -1) {
                *src_backend_id = w_ggml_backend_sched_backend_id_from_cur(sched, src);
            }
        }
    }
#ifdef DEBUG_PASS1
    fprintf(stderr, "PASS 1 ASSIGNMENTS\n"); w_ggml_backend_sched_print_assignments(sched, graph);
#endif

    // pass 2: expand current backend assignments
    // assign the same backend to adjacent nodes
    // expand gpu backends (i.e. non last prio) up and down, ignoring cpu (the lowest priority backend)
    // thus, cpu will never be used unless weights are on cpu, or there are no gpu ops between cpu ops


    // pass 2.2 expand gpu down
    {
        int cur_backend_id = -1;
        for (int i = 0; i < graph->n_nodes; i++) {
            struct w_ggml_tensor * node = graph->nodes[i];
            if (w_ggml_is_view_op(node->op)) {
                continue;
            }
            int * node_backend_id = &tensor_backend_id(node);
            if (*node_backend_id != -1) {
                if (*node_backend_id == sched->n_backends - 1) {
                    // skip cpu (lowest prio backend)
                    cur_backend_id = -1;
                } else {
                    cur_backend_id = *node_backend_id;
                }
            } else {
                *node_backend_id = cur_backend_id;
                SET_CAUSE(node, "2.2");
            }
        }
    }
    // pass 2.1 expand gpu up
    {
        int cur_backend_id = -1;
        for (int i = graph->n_nodes - 1; i >= 0; i--) {
            struct w_ggml_tensor * node = graph->nodes[i];
            if (w_ggml_is_view_op(node->op)) {
                continue;
            }
            int * node_backend_id = &tensor_backend_id(node);
            if (*node_backend_id != -1) {
                if (*node_backend_id == sched->n_backends - 1) {
                    // skip cpu (lowest prio backend)
                    cur_backend_id = -1;
                } else {
                    cur_backend_id = *node_backend_id;
                }
            } else {
                *node_backend_id = cur_backend_id;
                SET_CAUSE(node, "2.1");
            }
        }
    }
    // pass 2.4 expand rest down
    {
        int cur_backend_id = -1;
        for (int i = 0; i < graph->n_nodes; i++) {
            struct w_ggml_tensor * node = graph->nodes[i];
            if (w_ggml_is_view_op(node->op)) {
                continue;
            }
            int * node_backend_id = &tensor_backend_id(node);
            if (*node_backend_id != -1) {
                cur_backend_id = *node_backend_id;
            } else {
                *node_backend_id = cur_backend_id;
                SET_CAUSE(node, "2.4");
            }
        }
    }
    // pass 2.3 expand rest up
    {
        int cur_backend_id = -1;
        for (int i = graph->n_nodes - 1; i >= 0; i--) {
            struct w_ggml_tensor * node = graph->nodes[i];
            if (w_ggml_is_view_op(node->op)) {
                continue;
            }
            int * node_backend_id = &tensor_backend_id(node);
            if (*node_backend_id != -1) {
                cur_backend_id = *node_backend_id;
            } else {
                *node_backend_id = cur_backend_id;
                SET_CAUSE(node, "2.3");
            }
        }
    }

#ifdef DEBUG_PASS2
    fprintf(stderr, "PASS 2 ASSIGNMENTS\n"); w_ggml_backend_sched_print_assignments(sched, graph);
#endif

    // pass 3: assign backends to remaining src from dst and view_src
    for (int i = 0; i < graph->n_nodes; i++) {
        struct w_ggml_tensor * node = graph->nodes[i];
        int * cur_backend_id = &tensor_backend_id(node);
        if (node->view_src != NULL && *cur_backend_id == -1) {
            *cur_backend_id = tensor_backend_id(node->view_src);
            SET_CAUSE(node, "3.vsrc");
        }
        for (int j = 0; j < W_GGML_MAX_SRC; j++) {
            struct w_ggml_tensor * src = node->src[j];
            if (src == NULL) {
                continue;
            }
            int * src_backend_id = &tensor_backend_id(src);
            if (*src_backend_id == -1) {
                if (src->view_src != NULL) {
                    // views are always on the same backend as the source
                    *src_backend_id = tensor_backend_id(src->view_src);
                    SET_CAUSE(src, "3.vsrc");
                } else {
                    *src_backend_id = *cur_backend_id;
                    SET_CAUSE(src, "3.cur");
                }
            }
        }
    }
#ifdef DEBUG_PASS3
    fprintf(stderr, "PASS 3 ASSIGNMENTS\n"); w_ggml_backend_sched_print_assignments(sched, graph);
#endif

    // pass 4: split graph, find tensors that need to be copied
    {
        int i_split = 0;
        struct w_ggml_backend_sched_split * split = &sched->splits[0];
        // find the backend of the first split, skipping view ops
        for (int i = 0; i < graph->n_nodes; i++) {
            struct w_ggml_tensor * node = graph->nodes[i];
            if (!w_ggml_is_view_op(node->op)) {
                split->backend_id = tensor_backend_id(node);
                break;
            }
        }
        split->i_start = 0;
        split->n_inputs = 0;
        memset(split->inputs, 0, sizeof(split->inputs)); //HACK
        int cur_backend_id = split->backend_id;
        for (int i = 0; i < graph->n_nodes; i++) {
            struct w_ggml_tensor * node = graph->nodes[i];

            if (w_ggml_is_view_op(node->op)) {
                continue;
            }

            const int node_backend_id = tensor_backend_id(node);

            W_GGML_ASSERT(node_backend_id != -1); // all nodes should be assigned by now

            // check if we should start a new split based on the sources of the current node
            bool need_new_split = false;
            if (node_backend_id == cur_backend_id && split->n_inputs > 0) {
                for (int j = 0; j < W_GGML_MAX_SRC; j++) {
                    struct w_ggml_tensor * src = node->src[j];
                    if (src == NULL) {
                        continue;
                    }
                    // check if a weight is on a different backend
                    // by starting a new split, the memory of the previously offloaded weights can be reused
                    if (src->buffer != NULL && src->buffer->usage == W_GGML_BACKEND_BUFFER_USAGE_WEIGHTS) {
                        int src_backend_id = tensor_backend_id(src);
                        if (src_backend_id != -1 && src_backend_id != cur_backend_id) {
                            need_new_split = true;
                            break;
                        }
                    }
                    // check if the split has too many inputs
                    if (split->n_inputs == W_GGML_SCHED_MAX_SPLIT_INPUTS) {
                        const size_t id = hash_id(src);
                        int src_backend_id = sched->tensor_backend_id[id];
                        if (src_backend_id != cur_backend_id && sched->tensor_copies[hash_id(src)][cur_backend_id][0] == NULL) {
                            //printf("starting new split because of too many inputs: node %s, input %s\n", node->name, src->name);
                            need_new_split = true;
                            break;
                        }
                    }
                }
            }

            if (node_backend_id != cur_backend_id || need_new_split) {
                split->i_end = i;
                i_split++;
                if (i_split >= sched->splits_capacity) {
                    sched->splits_capacity *= 2;
                    sched->splits = realloc(sched->splits, sched->splits_capacity * sizeof(struct w_ggml_backend_sched_split));
                    W_GGML_ASSERT(sched->splits != NULL);
                }
                W_GGML_ASSERT(i_split < W_GGML_SCHED_MAX_SPLITS);
                split = &sched->splits[i_split];
                split->backend_id = node_backend_id;
                split->i_start = i;
                split->n_inputs = 0;
                cur_backend_id = node_backend_id;
            }

            // find inputs that are not on the same backend
            for (int j = 0; j < W_GGML_MAX_SRC; j++) {
                struct w_ggml_tensor * src = node->src[j];
                if (src == NULL) {
                    continue;
                }

                const int src_backend_id = tensor_backend_id(src);
                assert(src_backend_id != -1); // all inputs should be assigned by now

                if (src->flags & W_GGML_TENSOR_FLAG_INPUT && sched->n_copies > 1)  {
                    size_t id = hash_id(src);
                    if (sched->tensor_copies[id][src_backend_id][0] == NULL) {
                        w_ggml_backend_t backend = sched->backends[src_backend_id];
                        for (int c = 0; c < sched->n_copies; c++) {
                            struct w_ggml_tensor * tensor_copy;
                            if (c == sched->cur_copy) {
                                tensor_copy = src; // use the original tensor as the current copy
                            } else {
                                tensor_copy = w_ggml_dup_tensor_layout(sched->ctx, src);
                                w_ggml_format_name(tensor_copy, "%s#%s#%d", w_ggml_backend_name(backend), src->name, c);
                            }
                            if (sched->n_copies > 1) {
                                w_ggml_set_input(tensor_copy);
                                w_ggml_set_output(tensor_copy); // prevent ggml-alloc from overwriting the tensor
                            }
                            sched->tensor_copies[id][src_backend_id][c] = tensor_copy;
                            SET_CAUSE(tensor_copy, "4.cpy");
                        }
                        int n_graph_inputs = sched->n_graph_inputs++;
                        W_GGML_ASSERT(n_graph_inputs < W_GGML_SCHED_MAX_SPLIT_INPUTS);
                        sched->graph_inputs[n_graph_inputs] = src;
                    }
                }

                if (src_backend_id != node_backend_id) {
                    // create a copy of the input in the split's backend
                    const size_t id = hash_id(src);
                    if (sched->tensor_copies[id][cur_backend_id][0] == NULL) {
                        w_ggml_backend_t backend = sched->backends[cur_backend_id];
                        for (int c = 0; c < sched->n_copies; c++) {
                            struct w_ggml_tensor * tensor_copy = w_ggml_dup_tensor_layout(sched->ctx, src);
                            w_ggml_format_name(tensor_copy, "%s#%s#%d", w_ggml_backend_name(backend), src->name, c);
                            if (sched->n_copies > 1) {
                                w_ggml_set_input(tensor_copy);
                                w_ggml_set_output(tensor_copy); // prevent ggml-alloc from overwriting the tensor
                            }
                            sched->tensor_copies[id][cur_backend_id][c] = tensor_copy;
                            SET_CAUSE(tensor_copy, "4.cpy");
                        }
                        int n_inputs = split->n_inputs++;
                        W_GGML_ASSERT(n_inputs < W_GGML_SCHED_MAX_SPLIT_INPUTS);
                        split->inputs[n_inputs] = src;
                    }
                    node->src[j] = sched->tensor_copies[id][cur_backend_id][sched->cur_copy];
                }
            }
        }
        split->i_end = graph->n_nodes;
        sched->n_splits = i_split + 1;
    }
#ifdef DEBUG_PASS4
    fprintf(stderr, "PASS 4 ASSIGNMENTS\n"); w_ggml_backend_sched_print_assignments(sched, graph);
#endif

    // create copies of the graph for each split
    // TODO: avoid this copy
    struct w_ggml_cgraph * graph_copy = w_ggml_new_graph_custom(sched->ctx, graph->n_nodes + sched->n_splits*W_GGML_SCHED_MAX_SPLIT_INPUTS*2, false);
    for (int i = 0; i < sched->n_splits; i++) {
        struct w_ggml_backend_sched_split * split = &sched->splits[i];
        split->graph = w_ggml_graph_view(graph, split->i_start, split->i_end);

        // add inputs to the graph copy so that they are allocated by ggml-alloc at the start of the split
        for (int j = 0; j < split->n_inputs; j++) {
            assert(graph_copy->size > (graph_copy->n_nodes + 1));

            struct w_ggml_tensor * input = split->inputs[j];
            const size_t input_id = hash_id(input);
            struct w_ggml_tensor * input_cpy = sched->tensor_copies[input_id][split->backend_id][sched->cur_copy];

            // add a dependency to the input source so that it is not freed before the copy is done
            struct w_ggml_tensor * input_dep = w_ggml_view_tensor(sched->ctx, input);
            input_dep->src[0] = input;
            sched->node_backend_ids[graph_copy->n_nodes] = sched->tensor_backend_id[input_id];
            graph_copy->nodes[graph_copy->n_nodes++] = input_dep;

            // add a dependency to the input copy so that it is allocated at the start of the split
            sched->node_backend_ids[graph_copy->n_nodes] = split->backend_id;
            graph_copy->nodes[graph_copy->n_nodes++] = input_cpy;
        }

        for (int j = split->i_start; j < split->i_end; j++) {
            assert(graph_copy->size > graph_copy->n_nodes);
            sched->node_backend_ids[graph_copy->n_nodes] = tensor_backend_id(graph->nodes[j]);
            graph_copy->nodes[graph_copy->n_nodes++] = graph->nodes[j];
        }
    }

    if (sched->n_copies > 1) {
        // add input copies as leafs so that they are allocated first
        for (int i = 0; i < sched->n_graph_inputs; i++) {
            struct w_ggml_tensor * input = sched->graph_inputs[i];
            size_t id = hash_id(input);
            int backend_id = tensor_backend_id(input);
            for (int c = 0; c < sched->n_copies; c++) {
                struct w_ggml_tensor * input_cpy = sched->tensor_copies[id][backend_id][c];
                sched->leaf_backend_ids[graph_copy->n_leafs] = backend_id;
                graph_copy->leafs[graph_copy->n_leafs++] = input_cpy;
            }
        }

        for (int i = 0; i < sched->n_splits; i++) {
            struct w_ggml_backend_sched_split * split = &sched->splits[i];
            int backend_id = split->backend_id;
            for (int j = 0; j < split->n_inputs; j++) {
                struct w_ggml_tensor * input = split->inputs[j];
                size_t id = hash_id(input);
                for (int c = 0; c < sched->n_copies; c++) {
                    struct w_ggml_tensor * input_cpy = sched->tensor_copies[id][backend_id][c];
                    sched->leaf_backend_ids[graph_copy->n_leafs] = backend_id;
                    graph_copy->leafs[graph_copy->n_leafs++] = input_cpy;
                }
            }
        }
    }

    // add leafs from the original graph
    for (int i = 0; i < graph->n_leafs; i++) {
        struct w_ggml_tensor * leaf = graph->leafs[i];
        sched->leaf_backend_ids[graph_copy->n_leafs] = tensor_backend_id(leaf);
        graph_copy->leafs[graph_copy->n_leafs++] = leaf;
    }

    sched->graph = graph_copy;
}

static bool w_ggml_backend_sched_alloc_splits(w_ggml_backend_sched_t sched) {
    // allocate graph
    if (!w_ggml_gallocr_alloc_graph(sched->galloc, sched->graph)) {
        // the re-allocation may cause the split inputs to be moved to a different address
        w_ggml_backend_sched_synchronize(sched);
#ifndef NDEBUG
        fprintf(stderr, "%s: failed to allocate graph, reserving\n", __func__);
#endif
        w_ggml_gallocr_reserve_n(sched->galloc, sched->graph, sched->node_backend_ids, sched->leaf_backend_ids);
        if (!w_ggml_gallocr_alloc_graph(sched->galloc, sched->graph)) {
            fprintf(stderr, "%s: failed to allocate graph\n", __func__);
            return false;
        }
    }

    return true;
}

static enum w_ggml_status w_ggml_backend_sched_compute_splits(w_ggml_backend_sched_t sched) {
    struct w_ggml_backend_sched_split * splits = sched->splits;

    for (int i = 0; i < sched->n_splits; i++) {
        struct w_ggml_backend_sched_split * split = &splits[i];
        int split_backend_id = split->backend_id;
        w_ggml_backend_t split_backend = sched->backends[split_backend_id];

        // copy the input tensors to the split backend
        for (int j = 0; j < split->n_inputs; j++) {
            w_ggml_backend_t input_backend = w_ggml_backend_sched_get_tensor_backend(sched, split->inputs[j]);
            struct w_ggml_tensor * input = split->inputs[j];
            struct w_ggml_tensor * input_cpy = sched->tensor_copies[hash_id(input)][split_backend_id][sched->cur_copy];

            if (input->flags & W_GGML_TENSOR_FLAG_INPUT) {
                // inputs from the user must be copied immediately to prevent the user overwriting the data before the copy is done
                if (sched->events[split_backend_id][sched->cur_copy] != NULL) {
                    w_ggml_backend_event_synchronize(sched->events[split_backend_id][sched->cur_copy]);
                } else {
                    w_ggml_backend_synchronize(split_backend);
                }
                w_ggml_backend_tensor_copy(input, input_cpy);
            } else {
                // wait for the split backend to finish using the input before overwriting it
                if (sched->events[split_backend_id][sched->cur_copy] != NULL) {
                    w_ggml_backend_event_wait(split_backend, sched->events[split_backend_id][sched->cur_copy]);
                } else {
                    w_ggml_backend_synchronize(split_backend);
                }
                w_ggml_backend_tensor_copy_async(input_backend, split_backend, input, input_cpy);
            }
        }

        if (!sched->callback_eval) {
            enum w_ggml_status ec = w_ggml_backend_graph_compute_async(split_backend, &split->graph);
            if (ec != W_GGML_STATUS_SUCCESS) {
                return ec;
            }
        } else {
            // similar to w_ggml_backend_compare_graph_backend
            for (int j0 = 0; j0 < split->graph.n_nodes; j0++) {
                struct w_ggml_tensor * t = split->graph.nodes[j0];

                // check if the user needs data from this node
                bool need = sched->callback_eval(t, true, sched->callback_eval_user_data);

                int j1 = j0;

                // determine the range [j0, j1] of nodes that can be computed together
                while (!need && j1 < split->graph.n_nodes - 1) {
                    t = split->graph.nodes[++j1];
                    need = sched->callback_eval(t, true, sched->callback_eval_user_data);
                }

                struct w_ggml_cgraph gv = w_ggml_graph_view(&split->graph, j0, j1 + 1);

                enum w_ggml_status ec = w_ggml_backend_graph_compute_async(split_backend, &gv);
                if (ec != W_GGML_STATUS_SUCCESS) {
                    return ec;
                }

                // TODO: pass backend to the callback, then the user can decide if they want to synchronize
                w_ggml_backend_synchronize(split_backend);

                if (need && !sched->callback_eval(t, false, sched->callback_eval_user_data)) {
                    break;
                }

                j0 = j1;
            }
        }

        // record the event of this copy
        if (split->n_inputs > 0) {
            if (sched->events[split_backend_id][sched->cur_copy] != NULL) {
                w_ggml_backend_event_record(sched->events[split_backend_id][sched->cur_copy]);
            }
        }
    }

    sched->cur_copy = (sched->cur_copy + 1) % sched->n_copies;

    return W_GGML_STATUS_SUCCESS;
}

w_ggml_backend_sched_t w_ggml_backend_sched_new(
        w_ggml_backend_t * backends,
        w_ggml_backend_buffer_type_t * bufts,
        int n_backends,
        size_t graph_size,
        bool parallel) {
    W_GGML_ASSERT(n_backends > 0);
    W_GGML_ASSERT(n_backends <= W_GGML_SCHED_MAX_BACKENDS);
    W_GGML_ASSERT(w_ggml_backend_is_cpu(backends[n_backends - 1])); // last backend must be CPU

    struct w_ggml_backend_sched * sched = calloc(1, sizeof(struct w_ggml_backend_sched));

    // initialize hash table
    sched->hash_set          = w_ggml_hash_set_new(graph_size);
    sched->tensor_backend_id = calloc(sched->hash_set.size, sizeof(sched->tensor_backend_id[0]));
    sched->tensor_copies     = calloc(sched->hash_set.size, sizeof(sched->tensor_copies[0]));

    const size_t nodes_size = graph_size + W_GGML_SCHED_MAX_SPLITS*W_GGML_SCHED_MAX_SPLIT_INPUTS*2;
    sched->node_backend_ids  = calloc(nodes_size, sizeof(sched->node_backend_ids[0]));
    sched->leaf_backend_ids  = calloc(nodes_size, sizeof(sched->leaf_backend_ids[0]));

    sched->n_backends = n_backends;

    sched->n_copies = parallel ? W_GGML_SCHED_MAX_COPIES : 1;

    const int initial_splits_capacity = 16;
    sched->splits = calloc(initial_splits_capacity, sizeof(sched->splits[0]));
    sched->splits_capacity = initial_splits_capacity;

    for (int b = 0; b < n_backends; b++) {
        sched->backends[b] = backends[b];
        sched->bufts[b] = bufts ? bufts[b] : w_ggml_backend_get_default_buffer_type(backends[b]);
        W_GGML_ASSERT(w_ggml_backend_buft_supports_backend(sched->bufts[b], backends[b]));
        if (sched->n_copies > 1) {
            for (int c = 0; c < sched->n_copies; c++) {
                sched->events[b][c] = w_ggml_backend_event_new(backends[b]);
            }
        }
    }

    sched->galloc = w_ggml_gallocr_new_n(sched->bufts, n_backends);

    w_ggml_backend_sched_reset(sched);

    return sched;
}

void w_ggml_backend_sched_free(w_ggml_backend_sched_t sched) {
    if (sched == NULL) {
        return;
    }
    for (int b = 0; b < sched->n_backends; b++) {
        for (int c = 0; c < sched->n_copies; c++) {
            w_ggml_backend_event_free(sched->events[b][c]);
        }
    }
    w_ggml_gallocr_free(sched->galloc);
    w_ggml_free(sched->ctx);
    free(sched->splits);
    free(sched->hash_set.keys);
    free(sched->tensor_backend_id);
    free(sched->tensor_copies);
    free(sched->node_backend_ids);
    free(sched->leaf_backend_ids);
    free(sched);
}

void w_ggml_backend_sched_reset(w_ggml_backend_sched_t sched) {
    // reset state for the next run
    if (!sched->is_reset) {
        size_t hash_size = sched->hash_set.size;
        memset(sched->hash_set.keys,      0, sizeof(sched->hash_set.keys[0])     * hash_size); // NOLINT
        memset(sched->tensor_backend_id, -1, sizeof(sched->tensor_backend_id[0]) * hash_size);
        memset(sched->tensor_copies,      0, sizeof(sched->tensor_copies[0])     * hash_size);

        sched->is_reset = true;
    }
    sched->is_alloc = false;
}

bool w_ggml_backend_sched_reserve(w_ggml_backend_sched_t sched, struct w_ggml_cgraph * measure_graph) {
    W_GGML_ASSERT((int)sched->hash_set.size >= measure_graph->n_nodes);

    w_ggml_backend_sched_split_graph(sched, measure_graph);

    // TODO: extract this to a separate function
    if (!w_ggml_gallocr_reserve_n(sched->galloc, sched->graph, sched->node_backend_ids, sched->leaf_backend_ids)) {
        return false;
    }

    w_ggml_backend_sched_reset(sched);
    w_ggml_backend_sched_synchronize(sched);

    return true;
}

bool w_ggml_backend_sched_alloc_graph(w_ggml_backend_sched_t sched, struct w_ggml_cgraph * graph) {
    W_GGML_ASSERT((int)sched->hash_set.size >= graph->n_nodes);

    w_ggml_backend_sched_split_graph(sched, graph);

    if (!w_ggml_backend_sched_alloc_splits(sched)) {
        return false;
    }

    sched->is_alloc = true;

    return true;
}

enum w_ggml_status w_ggml_backend_sched_graph_compute(w_ggml_backend_sched_t sched, struct w_ggml_cgraph * graph) {
    enum w_ggml_status err = w_ggml_backend_sched_graph_compute_async(sched, graph);
    w_ggml_backend_sched_synchronize(sched);
    return err;
}

enum w_ggml_status w_ggml_backend_sched_graph_compute_async(w_ggml_backend_sched_t sched, struct w_ggml_cgraph * graph) {
    if (!sched->is_reset && !sched->is_alloc) {
        w_ggml_backend_sched_reset(sched);
    }

    if (!sched->is_alloc) {
        if (!w_ggml_backend_sched_alloc_graph(sched, graph)) {
            return W_GGML_STATUS_ALLOC_FAILED;
        }
    }

    return w_ggml_backend_sched_compute_splits(sched);
}

void w_ggml_backend_sched_synchronize(w_ggml_backend_sched_t sched) {
    for (int i = 0; i < sched->n_backends; i++) {
        w_ggml_backend_synchronize(sched->backends[i]);
    }
}

void w_ggml_backend_sched_set_eval_callback(w_ggml_backend_sched_t sched, w_ggml_backend_sched_eval_callback callback, void * user_data) {
    sched->callback_eval = callback;
    sched->callback_eval_user_data = user_data;
}

int w_ggml_backend_sched_get_n_splits(w_ggml_backend_sched_t sched) {
    return sched->n_splits;
}

int w_ggml_backend_sched_get_n_copies(w_ggml_backend_sched_t sched) {
    return sched->n_copies;
}

size_t w_ggml_backend_sched_get_buffer_size(w_ggml_backend_sched_t sched, w_ggml_backend_t backend) {
    int backend_index = w_ggml_backend_sched_backend_id(sched, backend);
    W_GGML_ASSERT(backend_index >= 0 && backend_index < sched->n_backends);

    return w_ggml_gallocr_get_buffer_size(sched->galloc, backend_index);
}

void w_ggml_backend_sched_set_tensor_backend(w_ggml_backend_sched_t sched, struct w_ggml_tensor * node, w_ggml_backend_t backend) {
    int backend_index = w_ggml_backend_sched_backend_id(sched, backend);
    W_GGML_ASSERT(backend_index >= 0 && backend_index < sched->n_backends);
    tensor_backend_id(node) = backend_index;
}

w_ggml_backend_t w_ggml_backend_sched_get_tensor_backend(w_ggml_backend_sched_t sched, struct w_ggml_tensor * node) {
    int backend_index = tensor_backend_id(node);
    if (backend_index == -1) {
        return NULL;
    }
    return sched->backends[backend_index];
}

// utils

void w_ggml_backend_view_init(w_ggml_backend_buffer_t buffer, struct w_ggml_tensor * tensor) {
    W_GGML_ASSERT(tensor->buffer == NULL);
    W_GGML_ASSERT(tensor->view_src != NULL);
    W_GGML_ASSERT(tensor->view_src->buffer != NULL);
    W_GGML_ASSERT(tensor->view_src->data != NULL);

    tensor->buffer = buffer;
    tensor->data = (char *)tensor->view_src->data + tensor->view_offs;
    tensor->backend = tensor->view_src->backend;
    w_ggml_backend_buffer_init_tensor(buffer, tensor);
}

void w_ggml_backend_tensor_alloc(w_ggml_backend_buffer_t buffer, struct w_ggml_tensor * tensor, void * addr) {
    W_GGML_ASSERT(tensor->buffer == NULL);
    W_GGML_ASSERT(tensor->data == NULL);
    W_GGML_ASSERT(tensor->view_src == NULL);
    W_GGML_ASSERT(addr >= w_ggml_backend_buffer_get_base(buffer));
    W_GGML_ASSERT((char *)addr + w_ggml_backend_buffer_get_alloc_size(buffer, tensor) <=
                (char *)w_ggml_backend_buffer_get_base(buffer) + w_ggml_backend_buffer_get_size(buffer));

    tensor->buffer = buffer;
    tensor->data = addr;
    w_ggml_backend_buffer_init_tensor(buffer, tensor);
}

static struct w_ggml_tensor * graph_copy_dup_tensor(struct w_ggml_hash_set hash_set, struct w_ggml_tensor ** node_copies,
    struct w_ggml_context * ctx_allocated, struct w_ggml_context * ctx_unallocated, struct w_ggml_tensor * src) {

    W_GGML_ASSERT(src != NULL);
    W_GGML_ASSERT(src->data && "graph must be allocated");

    size_t id = w_ggml_hash_insert(hash_set, src);
    if (id == W_GGML_HASHTABLE_ALREADY_EXISTS) {
        return node_copies[w_ggml_hash_find(hash_set, src)];
    }

    struct w_ggml_tensor * dst = w_ggml_dup_tensor_layout(src->data && !src->view_src ? ctx_allocated : ctx_unallocated, src);
    if (src->view_src != NULL) {
        dst->view_src = graph_copy_dup_tensor(hash_set, node_copies, ctx_allocated, ctx_unallocated, src->view_src);
        dst->view_offs = src->view_offs;
    }
    dst->op = src->op;
    memcpy(dst->op_params, src->op_params, sizeof(dst->op_params));
    w_ggml_set_name(dst, src->name);

    // copy src
    for (int i = 0; i < W_GGML_MAX_SRC; i++) {
        struct w_ggml_tensor * s = src->src[i];
        if (s == NULL) {
            continue;
        }
        dst->src[i] = graph_copy_dup_tensor(hash_set, node_copies, ctx_allocated, ctx_unallocated, s);
    }

    node_copies[id] = dst;
    return dst;
}

static void graph_copy_init_tensor(struct w_ggml_hash_set hash_set, struct w_ggml_tensor ** node_copies, bool * node_init, struct w_ggml_tensor * src) {
    size_t id = w_ggml_hash_find(hash_set, src);
    if (node_init[id]) {
        return;
    }
    node_init[id] = true;

    struct w_ggml_tensor * dst = node_copies[id];
    if (dst->view_src != NULL) {
        graph_copy_init_tensor(hash_set, node_copies, node_init, src->view_src);
        w_ggml_backend_view_init(dst->view_src->buffer, dst);
    }
    else {
        w_ggml_backend_tensor_copy(src, dst);
    }

    // init src
    for (int i = 0; i < W_GGML_MAX_SRC; i++) {
        struct w_ggml_tensor * s = src->src[i];
        if (s == NULL) {
            continue;
        }
        graph_copy_init_tensor(hash_set, node_copies, node_init, s);
    }
}

struct w_ggml_backend_graph_copy w_ggml_backend_graph_copy(w_ggml_backend_t backend, struct w_ggml_cgraph * graph) {
    struct w_ggml_hash_set hash_set = {
        /* .size = */ graph->visited_hash_table.size,
        /* .keys = */ calloc(graph->visited_hash_table.size, sizeof(hash_set.keys[0])) // NOLINT
    };
    struct w_ggml_tensor ** node_copies = calloc(hash_set.size, sizeof(node_copies[0])); // NOLINT
    bool * node_init = calloc(hash_set.size, sizeof(node_init[0]));

    struct w_ggml_init_params params = {
        /* .mem_size   = */ w_ggml_tensor_overhead()*hash_set.size + w_ggml_graph_overhead_custom(graph->size, false),
        /* .mem_buffer = */ NULL,
        /* .no_alloc   = */ true
    };

    struct w_ggml_context * ctx_allocated = w_ggml_init(params);
    struct w_ggml_context * ctx_unallocated = w_ggml_init(params);

    if (ctx_allocated == NULL || ctx_unallocated == NULL) {
        fprintf(stderr, "failed to allocate context for graph copy\n");
        free(hash_set.keys);
        free(node_copies);
        free(node_init);
        w_ggml_free(ctx_allocated);
        w_ggml_free(ctx_unallocated);
        return (struct w_ggml_backend_graph_copy) {
            /* .buffer           = */ NULL,
            /* .ctx_allocated    = */ NULL,
            /* .ctx_unallocated  = */ NULL,
            /* .graph            = */ NULL,
        };
    }

    // dup nodes
    for (int i = 0; i < graph->n_nodes; i++) {
        struct w_ggml_tensor * node = graph->nodes[i];
        graph_copy_dup_tensor(hash_set, node_copies, ctx_allocated, ctx_unallocated, node);
    }

    // allocate nodes
    w_ggml_backend_buffer_t buffer = w_ggml_backend_alloc_ctx_tensors(ctx_allocated, backend);
    if (buffer == NULL) {
        fprintf(stderr, "failed to allocate buffer for graph copy\n");
        free(hash_set.keys);
        free(node_copies);
        free(node_init);
        w_ggml_free(ctx_allocated);
        w_ggml_free(ctx_unallocated);
        return (struct w_ggml_backend_graph_copy) {
            /* .buffer           = */ NULL,
            /* .ctx_allocated    = */ NULL,
            /* .ctx_unallocated  = */ NULL,
            /* .graph            = */ NULL,
        };
    }

    //printf("copy buffer size: %zu MB\n", w_ggml_backend_buffer_get_size(buffer) / 1024 / 1024);

    // copy data and init views
    for (int i = 0; i < graph->n_nodes; i++) {
        struct w_ggml_tensor * node = graph->nodes[i];
        graph_copy_init_tensor(hash_set, node_copies, node_init, node);
    }

    // build graph copy
    struct w_ggml_cgraph * graph_copy = w_ggml_new_graph_custom(ctx_allocated, graph->size, false);
    for (int i = 0; i < graph->n_nodes; i++) {
        struct w_ggml_tensor * node = graph->nodes[i];
        struct w_ggml_tensor * node_copy = node_copies[w_ggml_hash_find(hash_set, node)];
        graph_copy->nodes[i] = node_copy;
    }
    graph_copy->n_nodes = graph->n_nodes;

    free(hash_set.keys);
    free(node_copies);
    free(node_init);

    return (struct w_ggml_backend_graph_copy) {
        /* .buffer           = */ buffer,
        /* .ctx_allocated    = */ ctx_allocated,
        /* .ctx_unallocated  = */ ctx_unallocated,
        /* .graph            = */ graph_copy,
    };
}

void w_ggml_backend_graph_copy_free(struct w_ggml_backend_graph_copy copy) {
    w_ggml_backend_buffer_free(copy.buffer);
    w_ggml_free(copy.ctx_allocated);
    w_ggml_free(copy.ctx_unallocated);
}

bool w_ggml_backend_compare_graph_backend(w_ggml_backend_t backend1, w_ggml_backend_t backend2, struct w_ggml_cgraph * graph, w_ggml_backend_eval_callback callback, void * user_data) {
    struct w_ggml_backend_graph_copy copy = w_ggml_backend_graph_copy(backend2, graph);
    if (copy.buffer == NULL) {
        return false;
    }

    struct w_ggml_cgraph * g1 = graph;
    struct w_ggml_cgraph * g2 = copy.graph;

    assert(g1->n_nodes == g2->n_nodes);

    for (int i = 0; i < g1->n_nodes; i++) {
        //printf("eval %d/%d\n", i, g1->n_nodes);
        struct w_ggml_tensor * t1 = g1->nodes[i];
        struct w_ggml_tensor * t2 = g2->nodes[i];

        assert(t1->op == t2->op && w_ggml_are_same_layout(t1, t2));

        struct w_ggml_cgraph g1v = w_ggml_graph_view(g1, i, i + 1);
        struct w_ggml_cgraph g2v = w_ggml_graph_view(g2, i, i + 1);

        w_ggml_backend_graph_compute(backend1, &g1v);
        w_ggml_backend_graph_compute(backend2, &g2v);

        if (w_ggml_is_view_op(t1->op)) {
            continue;
        }

        // compare results, calculate rms etc
        if (!callback(i, t1, t2, user_data)) {
            break;
        }
    }

    w_ggml_backend_graph_copy_free(copy);

    return true;
}
