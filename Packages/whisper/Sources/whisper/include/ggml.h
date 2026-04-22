#pragma once

//
// GGML Tensor Library
//
// This documentation is still a work in progress.
// If you wish some specific topics to be covered, feel free to drop a comment:
//
//   https://github.com/ggerganov/whisper.cpp/issues/40
//
// ## Overview
//
// This library implements:
//
//  - a set of tensor operations
//  - automatic differentiation
//  - basic optimization algorithms
//
// The aim of this library is to provide a minimalistic approach for various machine learning tasks. This includes,
// but is not limited to, the following:
//
//  - linear regression
//  - support vector machines
//  - neural networks
//
// The library allows the user to define a certain function using the available tensor operations. This function
// definition is represented internally via a computation graph. Each tensor operation in the function definition
// corresponds to a node in the graph. Having the computation graph defined, the user can choose to compute the
// function's value and/or its gradient with respect to the input variables. Optionally, the function can be optimized
// using one of the available optimization algorithms.
//
// For example, here we define the function: f(x) = a*x^2 + b
//
//   {
//       struct w_ggml_init_params params = {
//           .mem_size   = 16*1024*1024,
//           .mem_buffer = NULL,
//       };
//
//       // memory allocation happens here
//       struct w_ggml_context * ctx = w_ggml_init(params);
//
//       struct w_ggml_tensor * x = w_ggml_new_tensor_1d(ctx, W_GGML_TYPE_F32, 1);
//
//       w_ggml_set_param(ctx, x); // x is an input variable
//
//       struct w_ggml_tensor * a  = w_ggml_new_tensor_1d(ctx, W_GGML_TYPE_F32, 1);
//       struct w_ggml_tensor * b  = w_ggml_new_tensor_1d(ctx, W_GGML_TYPE_F32, 1);
//       struct w_ggml_tensor * x2 = w_ggml_mul(ctx, x, x);
//       struct w_ggml_tensor * f  = w_ggml_add(ctx, w_ggml_mul(ctx, a, x2), b);
//
//       ...
//   }
//
// Notice that the function definition above does not involve any actual computation. The computation is performed only
// when the user explicitly requests it. For example, to compute the function's value at x = 2.0:
//
//   {
//       ...
//
//       struct w_ggml_cgraph * gf = w_ggml_new_graph(ctx);
//       w_ggml_build_forward_expand(gf, f);
//
//       // set the input variable and parameter values
//       w_ggml_set_f32(x, 2.0f);
//       w_ggml_set_f32(a, 3.0f);
//       w_ggml_set_f32(b, 4.0f);
//
//       w_ggml_graph_compute_with_ctx(ctx, &gf, n_threads);
//
//       printf("f = %f\n", w_ggml_get_f32_1d(f, 0));
//
//       ...
//   }
//
// The actual computation is performed in the w_ggml_graph_compute() function.
//
// The w_ggml_new_tensor_...() functions create new tensors. They are allocated in the memory buffer provided to the
// w_ggml_init() function. You have to be careful not to exceed the memory buffer size. Therefore, you have to know
// in advance how much memory you need for your computation. Alternatively, you can allocate a large enough memory
// and after defining the computation graph, call the w_ggml_used_mem() function to find out how much memory was
// actually needed.
//
// The w_ggml_set_param() function marks a tensor as an input variable. This is used by the automatic
// differentiation and optimization algorithms.
//
// The described approach allows to define the function graph once and then compute its forward or backward graphs
// multiple times. All computations will use the same memory buffer allocated in the w_ggml_init() function. This way
// the user can avoid the memory allocation overhead at runtime.
//
// The library supports multi-dimensional tensors - up to 4 dimensions. The FP16 and FP32 data types are first class
// citizens, but in theory the library can be extended to support FP8 and integer data types.
//
// Each tensor operation produces a new tensor. Initially the library was envisioned to support only the use of unary
// and binary operations. Most of the available operations fall into one of these two categories. With time, it became
// clear that the library needs to support more complex operations. The way to support these operations is not clear
// yet, but a few examples are demonstrated in the following operations:
//
//   - w_ggml_permute()
//   - w_ggml_conv_1d_1s()
//   - w_ggml_conv_1d_2s()
//
// For each tensor operator, the library implements a forward and backward computation function. The forward function
// computes the output tensor value given the input tensor values. The backward function computes the adjoint of the
// input tensors given the adjoint of the output tensor. For a detailed explanation of what this means, take a
// calculus class, or watch the following video:
//
//   What is Automatic Differentiation?
//   https://www.youtube.com/watch?v=wG_nF1awSSY
//
//
// ## Tensor data (struct w_ggml_tensor)
//
// The tensors are stored in memory via the w_ggml_tensor struct. The structure provides information about the size of
// the tensor, the data type, and the memory buffer where the tensor data is stored. Additionally, it contains
// pointers to the "source" tensors - i.e. the tensors that were used to compute the current tensor. For example:
//
//   {
//       struct w_ggml_tensor * c = w_ggml_add(ctx, a, b);
//
//       assert(c->src[0] == a);
//       assert(c->src[1] == b);
//   }
//
// The multi-dimensional tensors are stored in row-major order. The w_ggml_tensor struct contains fields for the
// number of elements in each dimension ("ne") as well as the number of bytes ("nb", a.k.a. stride). This allows
// to store tensors that are not contiguous in memory, which is useful for operations such as transposition and
// permutation. All tensor operations have to take the stride into account and not assume that the tensor is
// contiguous in memory.
//
// The data of the tensor is accessed via the "data" pointer. For example:
//
//   {
//       const int nx = 2;
//       const int ny = 3;
//
//       struct w_ggml_tensor * a = w_ggml_new_tensor_2d(ctx, W_GGML_TYPE_F32, nx, ny);
//
//       for (int y = 0; y < ny; y++) {
//           for (int x = 0; x < nx; x++) {
//               *(float *) ((char *) a->data + y*a->nb[1] + x*a->nb[0]) = x + y;
//           }
//       }
//
//       ...
//   }
//
// Alternatively, there are helper functions, such as w_ggml_get_f32_1d() and w_ggml_set_f32_1d() that can be used.
//
// ## The matrix multiplication operator (w_ggml_mul_mat)
//
// TODO
//
//
// ## Multi-threading
//
// TODO
//
//
// ## Overview of ggml.c
//
// TODO
//
//
// ## SIMD optimizations
//
// TODO
//
//
// ## Debugging ggml
//
// TODO
//
//

#ifdef W_GGML_SHARED
#    if defined(_WIN32) && !defined(__MINGW32__)
#        ifdef W_GGML_BUILD
#            define W_GGML_API __declspec(dllexport)
#        else
#            define W_GGML_API __declspec(dllimport)
#        endif
#    else
#        define W_GGML_API __attribute__ ((visibility ("default")))
#    endif
#else
#    define W_GGML_API
#endif

#ifdef W_GGML_MULTIPLATFORM
#    if defined(_WIN32)
#        define W_GGML_CALL
#    else
#        define W_GGML_CALL __attribute__((__ms_abi__))
#    endif
#else
#    define W_GGML_CALL
#endif

// TODO: support for clang
#ifdef __GNUC__
#    define W_GGML_DEPRECATED(func, hint) func __attribute__((deprecated(hint)))
#elif defined(_MSC_VER)
#    define W_GGML_DEPRECATED(func, hint) __declspec(deprecated(hint)) func
#else
#    define W_GGML_DEPRECATED(func, hint) func
#endif

#ifndef __GNUC__
#    define W_GGML_ATTRIBUTE_FORMAT(...)
#elif defined(__MINGW32__)
#    define W_GGML_ATTRIBUTE_FORMAT(...) __attribute__((format(gnu_printf, __VA_ARGS__)))
#else
#    define W_GGML_ATTRIBUTE_FORMAT(...) __attribute__((format(printf, __VA_ARGS__)))
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define W_GGML_FILE_MAGIC   0x67676d6c // "ggml"
#define W_GGML_FILE_VERSION 1

#define W_GGML_QNT_VERSION        2    // bump this on quantization format changes
#define W_GGML_QNT_VERSION_FACTOR 1000 // do not change this

#define W_GGML_MAX_DIMS           4
#define W_GGML_MAX_PARAMS         2048
#define W_GGML_MAX_CONTEXTS       64
#define W_GGML_MAX_SRC            10
#ifndef W_GGML_MAX_NAME
#define W_GGML_MAX_NAME           64
#endif
#define W_GGML_MAX_OP_PARAMS      64
#define W_GGML_DEFAULT_N_THREADS  4
#define W_GGML_DEFAULT_GRAPH_SIZE 2048
#if UINTPTR_MAX == 0xFFFFFFFF
    #define W_GGML_MEM_ALIGN 4
#else
    #define W_GGML_MEM_ALIGN 16
#endif

#define W_GGML_EXIT_SUCCESS 0
#define W_GGML_EXIT_ABORTED 1

#define GGUF_MAGIC "GGUF"

#define GGUF_VERSION 3

#define GGUF_DEFAULT_ALIGNMENT 32

#define W_GGML_UNUSED(x) (void)(x)

#define W_GGML_PAD(x, n) (((x) + (n) - 1) & ~((n) - 1))

#define W_GGML_ASSERT(x) \
    do { \
        if (!(x)) { \
            fflush(stdout); \
            fprintf(stderr, "W_GGML_ASSERT: %s:%d: %s\n", __FILE__, __LINE__, #x); \
            w_ggml_print_backtrace(); \
            abort(); \
        } \
    } while (0)

#ifndef NDEBUG
#define W_GGML_UNREACHABLE() W_GGML_ASSERT(!"statement should not be reached")
#elif defined(__GNUC__)
#define W_GGML_UNREACHABLE() __builtin_unreachable()
#elif defined(_MSC_VER)
#define W_GGML_UNREACHABLE() __assume(0)
#else
#define W_GGML_UNREACHABLE() ((void) 0)
#endif

// used to copy the number of elements and stride in bytes of tensors into local variables.
// main purpose is to reduce code duplication and improve readability.
//
// example:
//
//    W_GGML_TENSOR_LOCALS(int64_t, ne1, src1, ne);
//    W_GGML_TENSOR_LOCALS(size_t,  nb1, src1, nb);
//
#define W_GGML_TENSOR_LOCALS_1(type, prefix, pointer, array) \
    const type prefix##0 = (pointer)->array[0]; \
    W_GGML_UNUSED(prefix##0);
#define W_GGML_TENSOR_LOCALS_2(type, prefix, pointer, array) \
    W_GGML_TENSOR_LOCALS_1    (type, prefix, pointer, array) \
    const type prefix##1 = (pointer)->array[1]; \
    W_GGML_UNUSED(prefix##1);
#define W_GGML_TENSOR_LOCALS_3(type, prefix, pointer, array) \
    W_GGML_TENSOR_LOCALS_2    (type, prefix, pointer, array) \
    const type prefix##2 = (pointer)->array[2]; \
    W_GGML_UNUSED(prefix##2);
#define W_GGML_TENSOR_LOCALS(type, prefix, pointer, array) \
    W_GGML_TENSOR_LOCALS_3  (type, prefix, pointer, array) \
    const type prefix##3 = (pointer)->array[3]; \
    W_GGML_UNUSED(prefix##3);

#define W_GGML_TENSOR_UNARY_OP_LOCALS \
    W_GGML_TENSOR_LOCALS(int64_t, ne0, src0, ne) \
    W_GGML_TENSOR_LOCALS(size_t,  nb0, src0, nb) \
    W_GGML_TENSOR_LOCALS(int64_t, ne,  dst,  ne) \
    W_GGML_TENSOR_LOCALS(size_t,  nb,  dst,  nb)

#define W_GGML_TENSOR_BINARY_OP_LOCALS \
    W_GGML_TENSOR_LOCALS(int64_t, ne0, src0, ne) \
    W_GGML_TENSOR_LOCALS(size_t,  nb0, src0, nb) \
    W_GGML_TENSOR_LOCALS(int64_t, ne1, src1, ne) \
    W_GGML_TENSOR_LOCALS(size_t,  nb1, src1, nb) \
    W_GGML_TENSOR_LOCALS(int64_t, ne,  dst,  ne) \
    W_GGML_TENSOR_LOCALS(size_t,  nb,  dst,  nb)

#ifdef  __cplusplus
extern "C" {
#endif

    enum w_ggml_status {
        W_GGML_STATUS_ALLOC_FAILED = -2,
        W_GGML_STATUS_FAILED = -1,
        W_GGML_STATUS_SUCCESS = 0,
        W_GGML_STATUS_ABORTED = 1,
    };

    // get w_ggml_status name string
    W_GGML_API W_GGML_CALL const char * w_ggml_status_to_string(enum w_ggml_status status);

    // ieee 754-2008 half-precision float16
    // todo: make this not an integral type
    typedef uint16_t w_ggml_fp16_t;
    W_GGML_API float       w_ggml_fp16_to_fp32(w_ggml_fp16_t);
    W_GGML_API w_ggml_fp16_t w_ggml_fp32_to_fp16(float);
    W_GGML_API void        w_ggml_fp16_to_fp32_row(const w_ggml_fp16_t *, float *, int64_t);
    W_GGML_API void        w_ggml_fp32_to_fp16_row(const float *, w_ggml_fp16_t *, int64_t);

    // google brain half-precision bfloat16
    typedef struct { uint16_t bits; } w_ggml_bf16_t;
    W_GGML_API w_ggml_bf16_t w_ggml_fp32_to_bf16(float);
    W_GGML_API float       w_ggml_bf16_to_fp32(w_ggml_bf16_t);  // consider just doing << 16
    W_GGML_API void        w_ggml_bf16_to_fp32_row(const w_ggml_bf16_t *, float *, int64_t);
    W_GGML_API void        w_ggml_fp32_to_bf16_row(const float *, w_ggml_bf16_t *, int64_t);

    struct w_ggml_object;
    struct w_ggml_context;

    // NOTE: always add types at the end of the enum to keep backward compatibility
    enum w_ggml_type {
        W_GGML_TYPE_F32     = 0,
        W_GGML_TYPE_F16     = 1,
        W_GGML_TYPE_Q4_0    = 2,
        W_GGML_TYPE_Q4_1    = 3,
        // W_GGML_TYPE_Q4_2 = 4, support has been removed
        // W_GGML_TYPE_Q4_3 = 5, support has been removed
        W_GGML_TYPE_Q5_0    = 6,
        W_GGML_TYPE_Q5_1    = 7,
        W_GGML_TYPE_Q8_0    = 8,
        W_GGML_TYPE_Q8_1    = 9,
        W_GGML_TYPE_Q2_K    = 10,
        W_GGML_TYPE_Q3_K    = 11,
        W_GGML_TYPE_Q4_K    = 12,
        W_GGML_TYPE_Q5_K    = 13,
        W_GGML_TYPE_Q6_K    = 14,
        W_GGML_TYPE_Q8_K    = 15,
        W_GGML_TYPE_IQ2_XXS = 16,
        W_GGML_TYPE_IQ2_XS  = 17,
        W_GGML_TYPE_IQ3_XXS = 18,
        W_GGML_TYPE_IQ1_S   = 19,
        W_GGML_TYPE_IQ4_NL  = 20,
        W_GGML_TYPE_IQ3_S   = 21,
        W_GGML_TYPE_IQ2_S   = 22,
        W_GGML_TYPE_IQ4_XS  = 23,
        W_GGML_TYPE_I8      = 24,
        W_GGML_TYPE_I16     = 25,
        W_GGML_TYPE_I32     = 26,
        W_GGML_TYPE_I64     = 27,
        W_GGML_TYPE_F64     = 28,
        W_GGML_TYPE_IQ1_M   = 29,
        W_GGML_TYPE_BF16    = 30,
        W_GGML_TYPE_COUNT,
    };

    // precision
    enum w_ggml_prec {
        W_GGML_PREC_DEFAULT,
        W_GGML_PREC_F32,
    };

    enum w_ggml_backend_type {
        W_GGML_BACKEND_TYPE_CPU = 0,
        W_GGML_BACKEND_TYPE_GPU = 10,
        W_GGML_BACKEND_TYPE_GPU_SPLIT = 20,
    };

    // model file types
    enum w_ggml_ftype {
        W_GGML_FTYPE_UNKNOWN        = -1,
        W_GGML_FTYPE_ALL_F32        = 0,
        W_GGML_FTYPE_MOSTLY_F16     = 1,  // except 1d tensors
        W_GGML_FTYPE_MOSTLY_Q4_0    = 2,  // except 1d tensors
        W_GGML_FTYPE_MOSTLY_Q4_1    = 3,  // except 1d tensors
        W_GGML_FTYPE_MOSTLY_Q4_1_SOME_F16 = 4, // tok_embeddings.weight and output.weight are F16
        W_GGML_FTYPE_MOSTLY_Q8_0    = 7,  // except 1d tensors
        W_GGML_FTYPE_MOSTLY_Q5_0    = 8,  // except 1d tensors
        W_GGML_FTYPE_MOSTLY_Q5_1    = 9,  // except 1d tensors
        W_GGML_FTYPE_MOSTLY_Q2_K    = 10, // except 1d tensors
        W_GGML_FTYPE_MOSTLY_Q3_K    = 11, // except 1d tensors
        W_GGML_FTYPE_MOSTLY_Q4_K    = 12, // except 1d tensors
        W_GGML_FTYPE_MOSTLY_Q5_K    = 13, // except 1d tensors
        W_GGML_FTYPE_MOSTLY_Q6_K    = 14, // except 1d tensors
        W_GGML_FTYPE_MOSTLY_IQ2_XXS = 15, // except 1d tensors
        W_GGML_FTYPE_MOSTLY_IQ2_XS  = 16, // except 1d tensors
        W_GGML_FTYPE_MOSTLY_IQ3_XXS = 17, // except 1d tensors
        W_GGML_FTYPE_MOSTLY_IQ1_S   = 18, // except 1d tensors
        W_GGML_FTYPE_MOSTLY_IQ4_NL  = 19, // except 1d tensors
        W_GGML_FTYPE_MOSTLY_IQ3_S   = 20, // except 1d tensors
        W_GGML_FTYPE_MOSTLY_IQ2_S   = 21, // except 1d tensors
        W_GGML_FTYPE_MOSTLY_IQ4_XS  = 22, // except 1d tensors
        W_GGML_FTYPE_MOSTLY_IQ1_M   = 23, // except 1d tensors
        W_GGML_FTYPE_MOSTLY_BF16    = 24, // except 1d tensors
    };

    // available tensor operations:
    enum w_ggml_op {
        W_GGML_OP_NONE = 0,

        W_GGML_OP_DUP,
        W_GGML_OP_ADD,
        W_GGML_OP_ADD1,
        W_GGML_OP_ACC,
        W_GGML_OP_SUB,
        W_GGML_OP_MUL,
        W_GGML_OP_DIV,
        W_GGML_OP_SQR,
        W_GGML_OP_SQRT,
        W_GGML_OP_LOG,
        W_GGML_OP_SUM,
        W_GGML_OP_SUM_ROWS,
        W_GGML_OP_MEAN,
        W_GGML_OP_ARGMAX,
        W_GGML_OP_REPEAT,
        W_GGML_OP_REPEAT_BACK,
        W_GGML_OP_CONCAT,
        W_GGML_OP_SILU_BACK,
        W_GGML_OP_NORM, // normalize
        W_GGML_OP_RMS_NORM,
        W_GGML_OP_RMS_NORM_BACK,
        W_GGML_OP_GROUP_NORM,

        W_GGML_OP_MUL_MAT,
        W_GGML_OP_MUL_MAT_ID,
        W_GGML_OP_OUT_PROD,

        W_GGML_OP_SCALE,
        W_GGML_OP_SET,
        W_GGML_OP_CPY,
        W_GGML_OP_CONT,
        W_GGML_OP_RESHAPE,
        W_GGML_OP_VIEW,
        W_GGML_OP_PERMUTE,
        W_GGML_OP_TRANSPOSE,
        W_GGML_OP_GET_ROWS,
        W_GGML_OP_GET_ROWS_BACK,
        W_GGML_OP_DIAG,
        W_GGML_OP_DIAG_MASK_INF,
        W_GGML_OP_DIAG_MASK_ZERO,
        W_GGML_OP_SOFT_MAX,
        W_GGML_OP_SOFT_MAX_BACK,
        W_GGML_OP_ROPE,
        W_GGML_OP_ROPE_BACK,
        W_GGML_OP_CLAMP,
        W_GGML_OP_CONV_TRANSPOSE_1D,
        W_GGML_OP_IM2COL,
        W_GGML_OP_CONV_TRANSPOSE_2D,
        W_GGML_OP_POOL_1D,
        W_GGML_OP_POOL_2D,
        W_GGML_OP_UPSCALE, // nearest interpolate
        W_GGML_OP_PAD,
        W_GGML_OP_ARANGE,
        W_GGML_OP_TIMESTEP_EMBEDDING,
        W_GGML_OP_ARGSORT,
        W_GGML_OP_LEAKY_RELU,

        W_GGML_OP_FLASH_ATTN,
        W_GGML_OP_FLASH_ATTN_EXT,
        W_GGML_OP_FLASH_FF,
        W_GGML_OP_FLASH_ATTN_BACK,
        W_GGML_OP_SSM_CONV,
        W_GGML_OP_SSM_SCAN,
        W_GGML_OP_WIN_PART,
        W_GGML_OP_WIN_UNPART,
        W_GGML_OP_GET_REL_POS,
        W_GGML_OP_ADD_REL_POS,

        W_GGML_OP_UNARY,

        W_GGML_OP_MAP_UNARY,
        W_GGML_OP_MAP_BINARY,

        W_GGML_OP_MAP_CUSTOM1_F32,
        W_GGML_OP_MAP_CUSTOM2_F32,
        W_GGML_OP_MAP_CUSTOM3_F32,

        W_GGML_OP_MAP_CUSTOM1,
        W_GGML_OP_MAP_CUSTOM2,
        W_GGML_OP_MAP_CUSTOM3,

        W_GGML_OP_CROSS_ENTROPY_LOSS,
        W_GGML_OP_CROSS_ENTROPY_LOSS_BACK,

        W_GGML_OP_COUNT,
    };

    enum w_ggml_unary_op {
        W_GGML_UNARY_OP_ABS,
        W_GGML_UNARY_OP_SGN,
        W_GGML_UNARY_OP_NEG,
        W_GGML_UNARY_OP_STEP,
        W_GGML_UNARY_OP_TANH,
        W_GGML_UNARY_OP_ELU,
        W_GGML_UNARY_OP_RELU,
        W_GGML_UNARY_OP_SIGMOID,
        W_GGML_UNARY_OP_GELU,
        W_GGML_UNARY_OP_GELU_QUICK,
        W_GGML_UNARY_OP_SILU,
        W_GGML_UNARY_OP_HARDSWISH,
        W_GGML_UNARY_OP_HARDSIGMOID,

        W_GGML_UNARY_OP_COUNT,
    };

    enum w_ggml_object_type {
        W_GGML_OBJECT_TYPE_TENSOR,
        W_GGML_OBJECT_TYPE_GRAPH,
        W_GGML_OBJECT_TYPE_WORK_BUFFER
    };

    enum w_ggml_log_level {
        W_GGML_LOG_LEVEL_ERROR = 2,
        W_GGML_LOG_LEVEL_WARN  = 3,
        W_GGML_LOG_LEVEL_INFO  = 4,
        W_GGML_LOG_LEVEL_DEBUG = 5
    };

    enum w_ggml_tensor_flag {
        W_GGML_TENSOR_FLAG_INPUT  = 1,
        W_GGML_TENSOR_FLAG_OUTPUT = 2,
        W_GGML_TENSOR_FLAG_PARAM  = 4,
    };

    // ggml object
    struct w_ggml_object {
        size_t offs;
        size_t size;

        struct w_ggml_object * next;

        enum w_ggml_object_type type;

        char padding[4];
    };

    static const size_t W_GGML_OBJECT_SIZE = sizeof(struct w_ggml_object);

    // n-dimensional tensor
    struct w_ggml_tensor {
        enum w_ggml_type         type;
        enum w_ggml_backend_type backend;

        struct w_ggml_backend_buffer * buffer;

        int64_t ne[W_GGML_MAX_DIMS]; // number of elements
        size_t  nb[W_GGML_MAX_DIMS]; // stride in bytes:
                                   // nb[0] = w_ggml_type_size(type)
                                   // nb[1] = nb[0]   * (ne[0] / w_ggml_blck_size(type)) + padding
                                   // nb[i] = nb[i-1] * ne[i-1]

        // compute data
        enum w_ggml_op op;

        // op params - allocated as int32_t for alignment
        int32_t op_params[W_GGML_MAX_OP_PARAMS / sizeof(int32_t)];

        int32_t flags;

        struct w_ggml_tensor * grad;
        struct w_ggml_tensor * src[W_GGML_MAX_SRC];

        // performance
        int     perf_runs;
        int64_t perf_cycles;
        int64_t perf_time_us;

        struct w_ggml_tensor * view_src;
        size_t               view_offs;

        void * data;

        char name[W_GGML_MAX_NAME];

        void * extra; // extra things e.g. for ggml-cuda.cu

        char padding[8];
    };

    static const size_t W_GGML_TENSOR_SIZE = sizeof(struct w_ggml_tensor);

    // Abort callback
    // If not NULL, called before ggml computation
    // If it returns true, the computation is aborted
    typedef bool (*w_ggml_abort_callback)(void * data);

    // the compute plan that needs to be prepared for w_ggml_graph_compute()
    // since https://github.com/ggerganov/ggml/issues/287
    struct w_ggml_cplan {
        size_t    work_size; // size of work buffer, calculated by `w_ggml_graph_plan()`
        uint8_t * work_data; // work buffer, to be allocated by caller before calling to `w_ggml_graph_compute()`

        int n_threads;

        // abort w_ggml_graph_compute when true
        w_ggml_abort_callback abort_callback;
        void *              abort_callback_data;
    };

    enum w_ggml_cgraph_eval_order {
        W_GGML_CGRAPH_EVAL_ORDER_LEFT_TO_RIGHT = 0,
        W_GGML_CGRAPH_EVAL_ORDER_RIGHT_TO_LEFT,
        W_GGML_CGRAPH_EVAL_ORDER_COUNT
    };

    struct w_ggml_hash_set {
        size_t size;
        struct w_ggml_tensor ** keys;
    };

    // computation graph
    struct w_ggml_cgraph {
        int size;
        int n_nodes;
        int n_leafs;

        struct w_ggml_tensor ** nodes;
        struct w_ggml_tensor ** grads;
        struct w_ggml_tensor ** leafs;

        struct w_ggml_hash_set visited_hash_table;

        enum w_ggml_cgraph_eval_order order;

        // performance
        int     perf_runs;
        int64_t perf_cycles;
        int64_t perf_time_us;
    };

    // scratch buffer
    struct w_ggml_scratch {
        size_t offs;
        size_t size;
        void * data;
    };

    struct w_ggml_init_params {
        // memory pool
        size_t mem_size;   // bytes
        void * mem_buffer; // if NULL, memory will be allocated internally
        bool   no_alloc;   // don't allocate memory for the tensor data
    };


    // compute types

    // NOTE: the INIT or FINALIZE pass is not scheduled unless explicitly enabled.
    // This behavior was changed since https://github.com/ggerganov/llama.cpp/pull/1995.
    enum w_ggml_task_type {
        W_GGML_TASK_TYPE_INIT = 0,
        W_GGML_TASK_TYPE_COMPUTE,
        W_GGML_TASK_TYPE_FINALIZE,
    };

    struct w_ggml_compute_params {
        enum w_ggml_task_type type;

        // ith = thread index, nth = number of threads
        int ith, nth;

        // work buffer for all threads
        size_t wsize;
        void * wdata;
    };

    // numa strategies
    enum w_ggml_numa_strategy {
        W_GGML_NUMA_STRATEGY_DISABLED   = 0,
        W_GGML_NUMA_STRATEGY_DISTRIBUTE = 1,
        W_GGML_NUMA_STRATEGY_ISOLATE    = 2,
        W_GGML_NUMA_STRATEGY_NUMACTL    = 3,
        W_GGML_NUMA_STRATEGY_MIRROR     = 4,
        W_GGML_NUMA_STRATEGY_COUNT
    };

    //
    // GUID
    //

    // GUID types
    typedef uint8_t w_ggml_guid[16];
    typedef w_ggml_guid * w_ggml_guid_t;

    W_GGML_API bool w_ggml_guid_matches(w_ggml_guid_t guid_a, w_ggml_guid_t guid_b);

    // misc

    W_GGML_API void    w_ggml_time_init(void); // call this once at the beginning of the program
    W_GGML_API int64_t w_ggml_time_ms(void);
    W_GGML_API int64_t w_ggml_time_us(void);
    W_GGML_API int64_t w_ggml_cycles(void);
    W_GGML_API int64_t w_ggml_cycles_per_ms(void);

    W_GGML_API void    w_ggml_print_backtrace(void);

    // accepts a UTF-8 path, even on Windows
    W_GGML_API FILE *  w_ggml_fopen(const char * fname, const char * mode);

    W_GGML_API void    w_ggml_numa_init(enum w_ggml_numa_strategy numa); // call once for better performance on NUMA systems
    W_GGML_API bool    w_ggml_is_numa(void); // true if init detected that system has >1 NUMA node

    W_GGML_API void    w_ggml_print_object (const struct w_ggml_object * obj);
    W_GGML_API void    w_ggml_print_objects(const struct w_ggml_context * ctx);

    W_GGML_API W_GGML_CALL int64_t w_ggml_nelements   (const struct w_ggml_tensor * tensor);
    W_GGML_API W_GGML_CALL int64_t w_ggml_nrows       (const struct w_ggml_tensor * tensor);
    W_GGML_API W_GGML_CALL size_t  w_ggml_nbytes      (const struct w_ggml_tensor * tensor);
    W_GGML_API           size_t  w_ggml_nbytes_pad  (const struct w_ggml_tensor * tensor); // same as w_ggml_nbytes() but padded to W_GGML_MEM_ALIGN

    W_GGML_API W_GGML_CALL int    w_ggml_blck_size(enum w_ggml_type type);
    W_GGML_API W_GGML_CALL size_t w_ggml_type_size(enum w_ggml_type type);             // size in bytes for all elements in a block
    W_GGML_API W_GGML_CALL size_t w_ggml_row_size (enum w_ggml_type type, int64_t ne); // size in bytes for all elements in a row

    W_GGML_DEPRECATED(
    W_GGML_API double w_ggml_type_sizef(enum w_ggml_type type), // w_ggml_type_size()/w_ggml_blck_size() as float
    "use w_ggml_row_size() instead");

    W_GGML_API W_GGML_CALL const char * w_ggml_type_name(enum w_ggml_type type);
    W_GGML_API W_GGML_CALL const char * w_ggml_op_name  (enum w_ggml_op   op);
    W_GGML_API           const char * w_ggml_op_symbol(enum w_ggml_op   op);

    W_GGML_API           const char * w_ggml_unary_op_name(enum w_ggml_unary_op op);
    W_GGML_API W_GGML_CALL const char * w_ggml_op_desc(const struct w_ggml_tensor * t); // unary or op name

    W_GGML_API W_GGML_CALL size_t  w_ggml_element_size(const struct w_ggml_tensor * tensor);

    W_GGML_API W_GGML_CALL bool    w_ggml_is_quantized(enum w_ggml_type type);

    // TODO: temporary until model loading of ggml examples is refactored
    W_GGML_API enum w_ggml_type w_ggml_ftype_to_w_ggml_type(enum w_ggml_ftype ftype);

    W_GGML_API W_GGML_CALL bool w_ggml_is_transposed(const struct w_ggml_tensor * tensor);
    W_GGML_API W_GGML_CALL bool w_ggml_is_contiguous(const struct w_ggml_tensor * tensor);
    W_GGML_API W_GGML_CALL bool w_ggml_is_permuted  (const struct w_ggml_tensor * tensor);
    W_GGML_API W_GGML_CALL bool w_ggml_is_empty     (const struct w_ggml_tensor * tensor);
    W_GGML_API           bool w_ggml_is_scalar    (const struct w_ggml_tensor * tensor);
    W_GGML_API           bool w_ggml_is_vector    (const struct w_ggml_tensor * tensor);
    W_GGML_API           bool w_ggml_is_matrix    (const struct w_ggml_tensor * tensor);
    W_GGML_API           bool w_ggml_is_3d        (const struct w_ggml_tensor * tensor);
    W_GGML_API           int  w_ggml_n_dims       (const struct w_ggml_tensor * tensor); // returns 1 for scalars

    W_GGML_API bool w_ggml_are_same_shape (const struct w_ggml_tensor * t0, const struct w_ggml_tensor * t1);
    W_GGML_API bool w_ggml_are_same_stride(const struct w_ggml_tensor * t0, const struct w_ggml_tensor * t1);

    // use this to compute the memory overhead of a tensor
    W_GGML_API size_t w_ggml_tensor_overhead(void);

    W_GGML_API bool w_ggml_validate_row_data(enum w_ggml_type type, const void * data, size_t nbytes);

    // main

    W_GGML_API struct w_ggml_context * w_ggml_init(struct w_ggml_init_params params);
    W_GGML_API void                  w_ggml_free(struct w_ggml_context * ctx);

    W_GGML_API size_t  w_ggml_used_mem(const struct w_ggml_context * ctx);

    W_GGML_API size_t  w_ggml_set_scratch (struct w_ggml_context * ctx, struct w_ggml_scratch scratch);
    W_GGML_API bool    w_ggml_get_no_alloc(struct w_ggml_context * ctx);
    W_GGML_API void    w_ggml_set_no_alloc(struct w_ggml_context * ctx, bool no_alloc);

    W_GGML_API void *  w_ggml_get_mem_buffer     (const struct w_ggml_context * ctx);
    W_GGML_API size_t  w_ggml_get_mem_size       (const struct w_ggml_context * ctx);
    W_GGML_API size_t  w_ggml_get_max_tensor_size(const struct w_ggml_context * ctx);

    W_GGML_API struct w_ggml_tensor * w_ggml_new_tensor(
            struct w_ggml_context * ctx,
            enum   w_ggml_type type,
            int    n_dims,
            const int64_t *ne);

    W_GGML_API struct w_ggml_tensor * w_ggml_new_tensor_1d(
            struct w_ggml_context * ctx,
            enum   w_ggml_type type,
            int64_t ne0);

    W_GGML_API struct w_ggml_tensor * w_ggml_new_tensor_2d(
            struct w_ggml_context * ctx,
            enum   w_ggml_type type,
            int64_t ne0,
            int64_t ne1);

    W_GGML_API struct w_ggml_tensor * w_ggml_new_tensor_3d(
            struct w_ggml_context * ctx,
            enum   w_ggml_type type,
            int64_t ne0,
            int64_t ne1,
            int64_t ne2);

    W_GGML_API struct w_ggml_tensor * w_ggml_new_tensor_4d(
            struct w_ggml_context * ctx,
            enum   w_ggml_type type,
            int64_t ne0,
            int64_t ne1,
            int64_t ne2,
            int64_t ne3);

    W_GGML_API struct w_ggml_tensor * w_ggml_new_i32(struct w_ggml_context * ctx, int32_t value);
    W_GGML_API struct w_ggml_tensor * w_ggml_new_f32(struct w_ggml_context * ctx, float value);

    W_GGML_API struct w_ggml_tensor * w_ggml_dup_tensor (struct w_ggml_context * ctx, const struct w_ggml_tensor * src);
    W_GGML_API struct w_ggml_tensor * w_ggml_view_tensor(struct w_ggml_context * ctx, struct w_ggml_tensor * src);

    // Context tensor enumeration and lookup
    W_GGML_API struct w_ggml_tensor * w_ggml_get_first_tensor(const struct w_ggml_context * ctx);
    W_GGML_API struct w_ggml_tensor * w_ggml_get_next_tensor (const struct w_ggml_context * ctx, struct w_ggml_tensor * tensor);
    W_GGML_API struct w_ggml_tensor * w_ggml_get_tensor(struct w_ggml_context * ctx, const char * name);

    W_GGML_API struct w_ggml_tensor * w_ggml_set_zero(struct w_ggml_tensor * tensor);
    W_GGML_API struct w_ggml_tensor * w_ggml_set_i32 (struct w_ggml_tensor * tensor, int32_t value);
    W_GGML_API struct w_ggml_tensor * w_ggml_set_f32 (struct w_ggml_tensor * tensor, float value);

    // Converts a flat index into coordinates
    W_GGML_API void    w_ggml_unravel_index(const struct w_ggml_tensor * tensor, int64_t i, int64_t * i0, int64_t * i1, int64_t * i2, int64_t * i3);

    W_GGML_API int32_t w_ggml_get_i32_1d(const struct w_ggml_tensor * tensor, int i);
    W_GGML_API void    w_ggml_set_i32_1d(const struct w_ggml_tensor * tensor, int i, int32_t value);

    W_GGML_API int32_t w_ggml_get_i32_nd(const struct w_ggml_tensor * tensor, int i0, int i1, int i2, int i3);
    W_GGML_API void    w_ggml_set_i32_nd(const struct w_ggml_tensor * tensor, int i0, int i1, int i2, int i3, int32_t value);

    W_GGML_API float   w_ggml_get_f32_1d(const struct w_ggml_tensor * tensor, int i);
    W_GGML_API void    w_ggml_set_f32_1d(const struct w_ggml_tensor * tensor, int i, float value);

    W_GGML_API float   w_ggml_get_f32_nd(const struct w_ggml_tensor * tensor, int i0, int i1, int i2, int i3);
    W_GGML_API void    w_ggml_set_f32_nd(const struct w_ggml_tensor * tensor, int i0, int i1, int i2, int i3, float value);

    W_GGML_API void *  w_ggml_get_data    (const struct w_ggml_tensor * tensor);
    W_GGML_API float * w_ggml_get_data_f32(const struct w_ggml_tensor * tensor);

    W_GGML_API W_GGML_CALL enum w_ggml_unary_op w_ggml_get_unary_op(const struct w_ggml_tensor * tensor);

    W_GGML_API const char *         w_ggml_get_name   (const struct w_ggml_tensor * tensor);
    W_GGML_API struct w_ggml_tensor * w_ggml_set_name   (      struct w_ggml_tensor * tensor, const char * name);
    W_GGML_ATTRIBUTE_FORMAT(2, 3)
    W_GGML_API struct w_ggml_tensor * w_ggml_format_name(      struct w_ggml_tensor * tensor, const char * fmt, ...);

    //
    // operations on tensors with backpropagation
    //

    W_GGML_API struct w_ggml_tensor * w_ggml_dup(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a);

    // in-place, returns view(a)
    W_GGML_API struct w_ggml_tensor * w_ggml_dup_inplace(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a);

    W_GGML_API struct w_ggml_tensor * w_ggml_add(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            struct w_ggml_tensor  * b);

    W_GGML_API struct w_ggml_tensor * w_ggml_add_inplace(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            struct w_ggml_tensor  * b);

    W_GGML_API struct w_ggml_tensor * w_ggml_add_cast(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            struct w_ggml_tensor  * b,
            enum   w_ggml_type      type);

    W_GGML_API struct w_ggml_tensor * w_ggml_add1(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            struct w_ggml_tensor  * b);

    W_GGML_API struct w_ggml_tensor * w_ggml_add1_inplace(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            struct w_ggml_tensor  * b);

    // dst = a
    // view(dst, nb1, nb2, nb3, offset) += b
    // return dst
    W_GGML_API struct w_ggml_tensor * w_ggml_acc(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            struct w_ggml_tensor  * b,
            size_t                nb1,
            size_t                nb2,
            size_t                nb3,
            size_t                offset);

    W_GGML_API struct w_ggml_tensor * w_ggml_acc_inplace(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            struct w_ggml_tensor  * b,
            size_t                nb1,
            size_t                nb2,
            size_t                nb3,
            size_t                offset);

    W_GGML_API struct w_ggml_tensor * w_ggml_sub(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            struct w_ggml_tensor  * b);

    W_GGML_API struct w_ggml_tensor * w_ggml_sub_inplace(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            struct w_ggml_tensor  * b);

    W_GGML_API struct w_ggml_tensor * w_ggml_mul(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            struct w_ggml_tensor  * b);

    W_GGML_API struct w_ggml_tensor * w_ggml_mul_inplace(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            struct w_ggml_tensor  * b);

    W_GGML_API struct w_ggml_tensor * w_ggml_div(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            struct w_ggml_tensor  * b);

    W_GGML_API struct w_ggml_tensor * w_ggml_div_inplace(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            struct w_ggml_tensor  * b);

    W_GGML_API struct w_ggml_tensor * w_ggml_sqr(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a);

    W_GGML_API struct w_ggml_tensor * w_ggml_sqr_inplace(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a);

    W_GGML_API struct w_ggml_tensor * w_ggml_sqrt(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a);

    W_GGML_API struct w_ggml_tensor * w_ggml_sqrt_inplace(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a);

    W_GGML_API struct w_ggml_tensor * w_ggml_log(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a);

    W_GGML_API struct w_ggml_tensor * w_ggml_log_inplace(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a);

    // return scalar
    W_GGML_API struct w_ggml_tensor * w_ggml_sum(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a);

    // sums along rows, with input shape [a,b,c,d] return shape [1,b,c,d]
    W_GGML_API struct w_ggml_tensor * w_ggml_sum_rows(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a);

    // mean along rows
    W_GGML_API struct w_ggml_tensor * w_ggml_mean(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a);

    // argmax along rows
    W_GGML_API struct w_ggml_tensor * w_ggml_argmax(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a);

    // if a is the same shape as b, and a is not parameter, return a
    // otherwise, return a new tensor: repeat(a) to fit in b
    W_GGML_API struct w_ggml_tensor * w_ggml_repeat(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            struct w_ggml_tensor  * b);

    // sums repetitions in a into shape of b
    W_GGML_API struct w_ggml_tensor * w_ggml_repeat_back(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            struct w_ggml_tensor  * b);

    // concat a and b on dim 2
    // used in stable-diffusion
    W_GGML_API struct w_ggml_tensor * w_ggml_concat(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            struct w_ggml_tensor  * b);

    W_GGML_API struct w_ggml_tensor * w_ggml_abs(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a);

    W_GGML_API struct w_ggml_tensor * w_ggml_abs_inplace(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a);

    W_GGML_API struct w_ggml_tensor * w_ggml_sgn(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a);

    W_GGML_API struct w_ggml_tensor * w_ggml_sgn_inplace(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a);

    W_GGML_API struct w_ggml_tensor * w_ggml_neg(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a);

    W_GGML_API struct w_ggml_tensor * w_ggml_neg_inplace(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a);

    W_GGML_API struct w_ggml_tensor * w_ggml_step(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a);

    W_GGML_API struct w_ggml_tensor * w_ggml_step_inplace(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a);

    W_GGML_API struct w_ggml_tensor * w_ggml_tanh(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a);

    W_GGML_API struct w_ggml_tensor * w_ggml_tanh_inplace(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a);

    W_GGML_API struct w_ggml_tensor * w_ggml_elu(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a);

    W_GGML_API struct w_ggml_tensor * w_ggml_elu_inplace(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a);

    W_GGML_API struct w_ggml_tensor * w_ggml_relu(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a);

    W_GGML_API struct w_ggml_tensor * w_ggml_leaky_relu(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a, float negative_slope, bool inplace);

    W_GGML_API struct w_ggml_tensor * w_ggml_relu_inplace(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a);

    W_GGML_API struct w_ggml_tensor * w_ggml_sigmoid(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a);

    W_GGML_API struct w_ggml_tensor * w_ggml_sigmoid_inplace(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a);

    W_GGML_API struct w_ggml_tensor * w_ggml_gelu(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a);

    W_GGML_API struct w_ggml_tensor * w_ggml_gelu_inplace(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a);

    W_GGML_API struct w_ggml_tensor * w_ggml_gelu_quick(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a);

    W_GGML_API struct w_ggml_tensor * w_ggml_gelu_quick_inplace(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a);

    W_GGML_API struct w_ggml_tensor * w_ggml_silu(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a);

    W_GGML_API struct w_ggml_tensor * w_ggml_silu_inplace(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a);

    // a - x
    // b - dy
    W_GGML_API struct w_ggml_tensor * w_ggml_silu_back(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            struct w_ggml_tensor  * b);

    // hardswish(x) = x * relu6(x + 3) / 6
    W_GGML_API struct w_ggml_tensor * w_ggml_hardswish(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a);

    // hardsigmoid(x) = relu6(x + 3) / 6
    W_GGML_API struct w_ggml_tensor * w_ggml_hardsigmoid(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a);

    // normalize along rows
    W_GGML_API struct w_ggml_tensor * w_ggml_norm(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            float                 eps);

    W_GGML_API struct w_ggml_tensor * w_ggml_norm_inplace(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            float                 eps);

    W_GGML_API struct w_ggml_tensor * w_ggml_rms_norm(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            float                 eps);

    W_GGML_API struct w_ggml_tensor * w_ggml_rms_norm_inplace(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            float                 eps);

    // group normalize along ne0*ne1*n_groups
    // used in stable-diffusion
    // TODO: eps is hardcoded to 1e-6 for now
    W_GGML_API struct w_ggml_tensor * w_ggml_group_norm(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            int                   n_groups);

    W_GGML_API struct w_ggml_tensor * w_ggml_group_norm_inplace(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            int                   n_groups);

    // a - x
    // b - dy
    W_GGML_API struct w_ggml_tensor * w_ggml_rms_norm_back(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            struct w_ggml_tensor  * b,
            float                 eps);

    // A: k columns, n rows => [ne03, ne02, n, k]
    // B: k columns, m rows  (i.e. we transpose it internally) => [ne03 * x, ne02 * y, m, k]
    // result is n columns, m rows => [ne03 * x, ne02 * y, m, n]
    W_GGML_API struct w_ggml_tensor * w_ggml_mul_mat(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            struct w_ggml_tensor  * b);

    // change the precision of a matrix multiplication
    // set to W_GGML_PREC_F32 for higher precision (useful for phi-2)
    W_GGML_API void w_ggml_mul_mat_set_prec(
            struct w_ggml_tensor * a,
            enum w_ggml_prec       prec);

    // indirect matrix multiplication
    W_GGML_API struct w_ggml_tensor * w_ggml_mul_mat_id(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * as,
            struct w_ggml_tensor  * b,
            struct w_ggml_tensor  * ids);

    // A: m columns, n rows,
    // B: p columns, n rows,
    // result is m columns, p rows
    W_GGML_API struct w_ggml_tensor * w_ggml_out_prod(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            struct w_ggml_tensor  * b);

    //
    // operations on tensors without backpropagation
    //

    W_GGML_API struct w_ggml_tensor * w_ggml_scale(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            float                 s);

    // in-place, returns view(a)
    W_GGML_API struct w_ggml_tensor * w_ggml_scale_inplace(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            float                 s);

    // b -> view(a,offset,nb1,nb2,3), return modified a
    W_GGML_API struct w_ggml_tensor * w_ggml_set(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            struct w_ggml_tensor  * b,
            size_t                nb1,
            size_t                nb2,
            size_t                nb3,
            size_t                offset);

    // b -> view(a,offset,nb1,nb2,3), return view(a)
    W_GGML_API struct w_ggml_tensor * w_ggml_set_inplace(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            struct w_ggml_tensor  * b,
            size_t                nb1,
            size_t                nb2,
            size_t                nb3,
            size_t                offset);

    W_GGML_API struct w_ggml_tensor * w_ggml_set_1d(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            struct w_ggml_tensor  * b,
            size_t                offset);

    W_GGML_API struct w_ggml_tensor * w_ggml_set_1d_inplace(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            struct w_ggml_tensor  * b,
            size_t                offset);

    // b -> view(a,offset,nb1,nb2,3), return modified a
    W_GGML_API struct w_ggml_tensor * w_ggml_set_2d(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            struct w_ggml_tensor  * b,
            size_t                nb1,
            size_t                offset);

    // b -> view(a,offset,nb1,nb2,3), return view(a)
    W_GGML_API struct w_ggml_tensor * w_ggml_set_2d_inplace(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            struct w_ggml_tensor  * b,
            size_t                nb1,
            size_t                offset);

    // a -> b, return view(b)
    W_GGML_API struct w_ggml_tensor * w_ggml_cpy(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            struct w_ggml_tensor  * b);

    W_GGML_API struct w_ggml_tensor * w_ggml_cast(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            enum   w_ggml_type      type);

    // make contiguous
    W_GGML_API struct w_ggml_tensor * w_ggml_cont(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a);

    // make contiguous, with new shape
    W_GGML_API struct w_ggml_tensor * w_ggml_cont_1d(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            int64_t               ne0);

    W_GGML_API struct w_ggml_tensor * w_ggml_cont_2d(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            int64_t               ne0,
            int64_t               ne1);

    W_GGML_API struct w_ggml_tensor * w_ggml_cont_3d(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            int64_t               ne0,
            int64_t               ne1,
            int64_t               ne2);

    W_GGML_API struct w_ggml_tensor * w_ggml_cont_4d(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            int64_t               ne0,
            int64_t               ne1,
            int64_t               ne2,
            int64_t               ne3);

    // return view(a), b specifies the new shape
    // TODO: when we start computing gradient, make a copy instead of view
    W_GGML_API struct w_ggml_tensor * w_ggml_reshape(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            struct w_ggml_tensor  * b);

    // return view(a)
    // TODO: when we start computing gradient, make a copy instead of view
    W_GGML_API struct w_ggml_tensor * w_ggml_reshape_1d(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            int64_t               ne0);

    W_GGML_API struct w_ggml_tensor * w_ggml_reshape_2d(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            int64_t               ne0,
            int64_t               ne1);

    // return view(a)
    // TODO: when we start computing gradient, make a copy instead of view
    W_GGML_API struct w_ggml_tensor * w_ggml_reshape_3d(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            int64_t               ne0,
            int64_t               ne1,
            int64_t               ne2);

    W_GGML_API struct w_ggml_tensor * w_ggml_reshape_4d(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            int64_t               ne0,
            int64_t               ne1,
            int64_t               ne2,
            int64_t               ne3);

    // offset in bytes
    W_GGML_API struct w_ggml_tensor * w_ggml_view_1d(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            int64_t               ne0,
            size_t                offset);

    W_GGML_API struct w_ggml_tensor * w_ggml_view_2d(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            int64_t               ne0,
            int64_t               ne1,
            size_t                nb1, // row stride in bytes
            size_t                offset);

    W_GGML_API struct w_ggml_tensor * w_ggml_view_3d(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            int64_t               ne0,
            int64_t               ne1,
            int64_t               ne2,
            size_t                nb1, // row   stride in bytes
            size_t                nb2, // slice stride in bytes
            size_t                offset);

    W_GGML_API struct w_ggml_tensor * w_ggml_view_4d(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            int64_t               ne0,
            int64_t               ne1,
            int64_t               ne2,
            int64_t               ne3,
            size_t                nb1, // row   stride in bytes
            size_t                nb2, // slice stride in bytes
            size_t                nb3,
            size_t                offset);

    W_GGML_API struct w_ggml_tensor * w_ggml_permute(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            int                   axis0,
            int                   axis1,
            int                   axis2,
            int                   axis3);

    // alias for w_ggml_permute(ctx, a, 1, 0, 2, 3)
    W_GGML_API struct w_ggml_tensor * w_ggml_transpose(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a);

    // supports 3D: a->ne[2] == b->ne[1]
    W_GGML_API struct w_ggml_tensor * w_ggml_get_rows(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            struct w_ggml_tensor  * b);

    W_GGML_API struct w_ggml_tensor * w_ggml_get_rows_back(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            struct w_ggml_tensor  * b,
            struct w_ggml_tensor  * c);

    W_GGML_API struct w_ggml_tensor * w_ggml_diag(
        struct w_ggml_context     * ctx,
        struct w_ggml_tensor      * a);

    // set elements above the diagonal to -INF
    W_GGML_API struct w_ggml_tensor * w_ggml_diag_mask_inf(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            int                   n_past);

    // in-place, returns view(a)
    W_GGML_API struct w_ggml_tensor * w_ggml_diag_mask_inf_inplace(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            int                   n_past);

    // set elements above the diagonal to 0
    W_GGML_API struct w_ggml_tensor * w_ggml_diag_mask_zero(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            int                   n_past);

    // in-place, returns view(a)
    W_GGML_API struct w_ggml_tensor * w_ggml_diag_mask_zero_inplace(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            int                   n_past);

    W_GGML_API struct w_ggml_tensor * w_ggml_soft_max(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a);

    // in-place, returns view(a)
    W_GGML_API struct w_ggml_tensor * w_ggml_soft_max_inplace(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a);

    // fused soft_max(a*scale + mask*(ALiBi slope))
    // mask is optional
    // max_bias = 0.0f for no ALiBi
    W_GGML_API struct w_ggml_tensor * w_ggml_soft_max_ext(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            struct w_ggml_tensor  * mask,
            float                 scale,
            float                 max_bias);

    W_GGML_API struct w_ggml_tensor * w_ggml_soft_max_back(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            struct w_ggml_tensor  * b);

    // in-place, returns view(a)
    W_GGML_API struct w_ggml_tensor * w_ggml_soft_max_back_inplace(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            struct w_ggml_tensor  * b);

    // rotary position embedding
    // if mode & 1 == 1, skip n_past elements (DEPRECATED)
    // if mode & 2 == 1, GPT-NeoX style
    // if mode & 4 == 1, ChatGLM style
    //
    // b is an int32 vector with size a->ne[2], it contains the positions
    W_GGML_API struct w_ggml_tensor * w_ggml_rope(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            struct w_ggml_tensor  * b,
            int                   n_dims,
            int                   mode,
            int                   n_ctx);

    // in-place, returns view(a)
    W_GGML_API struct w_ggml_tensor * w_ggml_rope_inplace(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            struct w_ggml_tensor  * b,
            int                   n_dims,
            int                   mode,
            int                   n_ctx);

    // custom RoPE
    W_GGML_API struct w_ggml_tensor * w_ggml_rope_custom(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            struct w_ggml_tensor  * b,
            int                   n_dims,
            int                   mode,
            int                   n_ctx,
            int                   n_orig_ctx,
            float                 freq_base,
            float                 freq_scale,
            float                 ext_factor,
            float                 attn_factor,
            float                 beta_fast,
            float                 beta_slow);

    // in-place, returns view(a)
    W_GGML_API struct w_ggml_tensor * w_ggml_rope_custom_inplace(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            struct w_ggml_tensor  * b,
            int                   n_dims,
            int                   mode,
            int                   n_ctx,
            int                   n_orig_ctx,
            float                 freq_base,
            float                 freq_scale,
            float                 ext_factor,
            float                 attn_factor,
            float                 beta_fast,
            float                 beta_slow);

    // compute correction dims for YaRN RoPE scaling
    W_GGML_CALL void w_ggml_rope_yarn_corr_dims(
        int n_dims, int n_orig_ctx, float freq_base, float beta_fast, float beta_slow, float dims[2]);

    // xPos RoPE, in-place, returns view(a)
    W_GGML_API struct w_ggml_tensor * w_ggml_rope_xpos_inplace(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            struct w_ggml_tensor  * b,
            int                   n_dims,
            float                 base,
            bool                  down);

    // rotary position embedding backward, i.e compute dx from dy
    // a - dy
    W_GGML_API struct w_ggml_tensor * w_ggml_rope_back(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            struct w_ggml_tensor  * b,
            int                   n_dims,
            int                   mode,
            int                   n_ctx,
            int                   n_orig_ctx,
            float                 freq_base,
            float                 freq_scale,
            float                 ext_factor,
            float                 attn_factor,
            float                 beta_fast,
            float                 beta_slow,
            float                 xpos_base,
            bool                  xpos_down);

    // clamp
    // in-place, returns view(a)
    W_GGML_API struct w_ggml_tensor * w_ggml_clamp(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            float                 min,
            float                 max);

    W_GGML_API struct w_ggml_tensor * w_ggml_im2col(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            struct w_ggml_tensor  * b,
            int                  s0,
            int                  s1,
            int                  p0,
            int                  p1,
            int                  d0,
            int                  d1,
            bool                 is_2D,
            enum w_ggml_type       dst_type);

    W_GGML_API struct w_ggml_tensor * w_ggml_conv_depthwise_2d(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            struct w_ggml_tensor  * b,
            int                  s0,
            int                  s1,
            int                  p0,
            int                  p1,
            int                  d0,
            int                  d1);

    W_GGML_API struct w_ggml_tensor * w_ggml_conv_1d(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            struct w_ggml_tensor  * b,
            int                   s0,  // stride
            int                   p0,  // padding
            int                   d0); // dilation

    // conv_1d with padding = half
    // alias for w_ggml_conv_1d(a, b, s, a->ne[0]/2, d)
    W_GGML_API struct w_ggml_tensor* w_ggml_conv_1d_ph(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            struct w_ggml_tensor  * b,
            int                   s,
            int                   d);

    W_GGML_API struct w_ggml_tensor * w_ggml_conv_transpose_1d(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            struct w_ggml_tensor  * b,
            int                   s0,
            int                   p0,
            int                   d0);

    W_GGML_API struct w_ggml_tensor * w_ggml_conv_2d(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            struct w_ggml_tensor  * b,
            int                   s0,
            int                   s1,
            int                   p0,
            int                   p1,
            int                   d0,
            int                   d1);


    // kernel size is a->ne[0] x a->ne[1]
    // stride is equal to kernel size
    // padding is zero
    // example:
    // a:     16   16    3  768
    // b:   1024 1024    3    1
    // res:   64   64  768    1
    // used in sam
    W_GGML_API struct w_ggml_tensor * w_ggml_conv_2d_sk_p0(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            struct w_ggml_tensor  * b);

    // kernel size is a->ne[0] x a->ne[1]
    // stride is 1
    // padding is half
    // example:
    // a:      3    3    256  256
    // b:     64   64    256    1
    // res:   64   64    256    1
    // used in sam
    W_GGML_API struct w_ggml_tensor * w_ggml_conv_2d_s1_ph(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            struct w_ggml_tensor  * b);

    W_GGML_API struct w_ggml_tensor * w_ggml_conv_transpose_2d_p0(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            struct w_ggml_tensor  * b,
            int                   stride);

    enum w_ggml_op_pool {
        W_GGML_OP_POOL_MAX,
        W_GGML_OP_POOL_AVG,
        W_GGML_OP_POOL_COUNT,
    };

    W_GGML_API struct w_ggml_tensor * w_ggml_pool_1d(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            enum w_ggml_op_pool     op,
            int                   k0, // kernel size
            int                   s0, // stride
            int                   p0); // padding

    // the result will have 2*p0 padding for the first dimension
    // and 2*p1 padding for the second dimension
    W_GGML_API struct w_ggml_tensor * w_ggml_pool_2d(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            enum w_ggml_op_pool     op,
            int                   k0,
            int                   k1,
            int                   s0,
            int                   s1,
            float                 p0,
            float                 p1);

    // nearest interpolate
    // used in stable-diffusion
    W_GGML_API struct w_ggml_tensor * w_ggml_upscale(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            int                   scale_factor);

    // pad each dimension with zeros: [x, ..., x] -> [x, ..., x, 0, ..., 0]
    W_GGML_API struct w_ggml_tensor * w_ggml_pad(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            int                  p0,
            int                  p1,
            int                  p2,
            int                  p3);

    // Ref: https://github.com/CompVis/stable-diffusion/blob/main/ldm/modules/diffusionmodules/util.py#L151
    // timesteps: [N,]
    // return: [N, dim]
    W_GGML_API struct w_ggml_tensor * w_ggml_timestep_embedding(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * timesteps,
            int                   dim,
            int                   max_period);

    // sort rows
    enum w_ggml_sort_order {
        W_GGML_SORT_ORDER_ASC,
        W_GGML_SORT_ORDER_DESC,
    };

    W_GGML_API struct w_ggml_tensor * w_ggml_argsort(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            enum w_ggml_sort_order  order);

    W_GGML_API struct w_ggml_tensor * w_ggml_arange(
            struct w_ggml_context * ctx,
            float                 start,
            float                 stop,
            float                 step);

    // top k elements per row
    W_GGML_API struct w_ggml_tensor * w_ggml_top_k(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            int                   k);

    W_GGML_API struct w_ggml_tensor * w_ggml_flash_attn(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * q,
            struct w_ggml_tensor  * k,
            struct w_ggml_tensor  * v,
            bool                  masked);

#define W_GGML_KQ_MASK_PAD 32

    // q:    [n_embd, n_batch,     n_head,    1]
    // k:    [n_embd, n_kv,        n_head_kv, 1]
    // v:    [n_embd, n_kv,        n_head_kv, 1] !! not transposed !!
    // mask: [n_kv,   n_batch_pad, 1,         1] !! n_batch_pad = W_GGML_PAD(n_batch, W_GGML_KQ_MASK_PAD) !!
    // res:  [n_embd, n_head,      n_batch,   1] !! permuted !!
    W_GGML_API struct w_ggml_tensor * w_ggml_flash_attn_ext(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * q,
            struct w_ggml_tensor  * k,
            struct w_ggml_tensor  * v,
            struct w_ggml_tensor  * mask,
            float                 scale,
            float                 max_bias);

    W_GGML_API void w_ggml_flash_attn_ext_set_prec(
            struct w_ggml_tensor * a,
            enum w_ggml_prec       prec);

    W_GGML_API struct w_ggml_tensor * w_ggml_flash_attn_back(
           struct w_ggml_context * ctx,
           struct w_ggml_tensor  * q,
           struct w_ggml_tensor  * k,
           struct w_ggml_tensor  * v,
           struct w_ggml_tensor  * d,
           bool                  masked);

    W_GGML_API struct w_ggml_tensor * w_ggml_flash_ff(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            struct w_ggml_tensor  * b0,
            struct w_ggml_tensor  * b1,
            struct w_ggml_tensor  * c0,
            struct w_ggml_tensor  * c1);

    W_GGML_API struct w_ggml_tensor * w_ggml_ssm_conv(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * s,
            struct w_ggml_tensor  * x,
            struct w_ggml_tensor  * c,
            struct w_ggml_tensor  * sq);

    W_GGML_API struct w_ggml_tensor * w_ggml_ssm_scan(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * s,
            struct w_ggml_tensor  * x,
            struct w_ggml_tensor  * dt,
            struct w_ggml_tensor  * A,
            struct w_ggml_tensor  * B,
            struct w_ggml_tensor  * C,
            struct w_ggml_tensor  * sq);

    // partition into non-overlapping windows with padding if needed
    // example:
    // a:   768   64   64    1
    // w:    14
    // res: 768   14   14    25
    // used in sam
    W_GGML_API struct w_ggml_tensor * w_ggml_win_part(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            int                   w);

    // reverse of w_ggml_win_part
    // used in sam
    W_GGML_API struct w_ggml_tensor * w_ggml_win_unpart(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            int                   w0,
            int                   h0,
            int                   w);

    W_GGML_API struct w_ggml_tensor * w_ggml_unary(
            struct w_ggml_context * ctx,
             struct w_ggml_tensor * a,
             enum w_ggml_unary_op op);

    W_GGML_API struct w_ggml_tensor * w_ggml_unary_inplace(
        struct w_ggml_context * ctx,
        struct w_ggml_tensor  * a,
        enum w_ggml_unary_op op);

    // used in sam
    W_GGML_API struct w_ggml_tensor * w_ggml_get_rel_pos(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            int                   qh,
            int                   kh);

    // used in sam
    W_GGML_API struct w_ggml_tensor * w_ggml_add_rel_pos(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            struct w_ggml_tensor  * pw,
            struct w_ggml_tensor  * ph);

    W_GGML_API struct w_ggml_tensor * w_ggml_add_rel_pos_inplace(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * a,
            struct w_ggml_tensor  * pw,
            struct w_ggml_tensor  * ph);

    // custom operators

    typedef void (*w_ggml_unary_op_f32_t) (const int, float *, const float *);
    typedef void (*w_ggml_binary_op_f32_t)(const int, float *, const float *, const float *);

    typedef void (*w_ggml_custom1_op_f32_t)(struct w_ggml_tensor *, const struct w_ggml_tensor *);
    typedef void (*w_ggml_custom2_op_f32_t)(struct w_ggml_tensor *, const struct w_ggml_tensor *, const struct w_ggml_tensor *);
    typedef void (*w_ggml_custom3_op_f32_t)(struct w_ggml_tensor *, const struct w_ggml_tensor *, const struct w_ggml_tensor *, const struct w_ggml_tensor *);

    W_GGML_DEPRECATED(W_GGML_API struct w_ggml_tensor * w_ggml_map_unary_f32(
            struct w_ggml_context        * ctx,
            struct w_ggml_tensor         * a,
                   w_ggml_unary_op_f32_t   fun),
        "use w_ggml_map_custom1 instead");

    W_GGML_DEPRECATED(W_GGML_API struct w_ggml_tensor * w_ggml_map_unary_inplace_f32(
            struct w_ggml_context        * ctx,
            struct w_ggml_tensor         * a,
                   w_ggml_unary_op_f32_t   fun),
        "use w_ggml_map_custom1_inplace instead");

    W_GGML_DEPRECATED(W_GGML_API struct w_ggml_tensor * w_ggml_map_binary_f32(
            struct w_ggml_context         * ctx,
            struct w_ggml_tensor          * a,
            struct w_ggml_tensor          * b,
                   w_ggml_binary_op_f32_t   fun),
        "use w_ggml_map_custom2 instead");

    W_GGML_DEPRECATED(W_GGML_API struct w_ggml_tensor * w_ggml_map_binary_inplace_f32(
            struct w_ggml_context         * ctx,
            struct w_ggml_tensor          * a,
            struct w_ggml_tensor          * b,
                   w_ggml_binary_op_f32_t   fun),
        "use w_ggml_map_custom2_inplace instead");

    W_GGML_DEPRECATED(W_GGML_API struct w_ggml_tensor * w_ggml_map_custom1_f32(
            struct w_ggml_context          * ctx,
            struct w_ggml_tensor           * a,
                   w_ggml_custom1_op_f32_t   fun),
        "use w_ggml_map_custom1 instead");

    W_GGML_DEPRECATED(W_GGML_API struct w_ggml_tensor * w_ggml_map_custom1_inplace_f32(
            struct w_ggml_context          * ctx,
            struct w_ggml_tensor           * a,
                   w_ggml_custom1_op_f32_t   fun),
        "use w_ggml_map_custom1_inplace instead");

    W_GGML_DEPRECATED(W_GGML_API struct w_ggml_tensor * w_ggml_map_custom2_f32(
            struct w_ggml_context          * ctx,
            struct w_ggml_tensor           * a,
            struct w_ggml_tensor           * b,
                   w_ggml_custom2_op_f32_t   fun),
        "use w_ggml_map_custom2 instead");

    W_GGML_DEPRECATED(W_GGML_API struct w_ggml_tensor * w_ggml_map_custom2_inplace_f32(
            struct w_ggml_context          * ctx,
            struct w_ggml_tensor           * a,
            struct w_ggml_tensor           * b,
                   w_ggml_custom2_op_f32_t   fun),
        "use w_ggml_map_custom2_inplace instead");

    W_GGML_DEPRECATED(W_GGML_API struct w_ggml_tensor * w_ggml_map_custom3_f32(
            struct w_ggml_context          * ctx,
            struct w_ggml_tensor           * a,
            struct w_ggml_tensor           * b,
            struct w_ggml_tensor           * c,
                   w_ggml_custom3_op_f32_t   fun),
        "use w_ggml_map_custom3 instead");

    W_GGML_DEPRECATED(W_GGML_API struct w_ggml_tensor * w_ggml_map_custom3_inplace_f32(
            struct w_ggml_context          * ctx,
            struct w_ggml_tensor           * a,
            struct w_ggml_tensor           * b,
            struct w_ggml_tensor           * c,
                   w_ggml_custom3_op_f32_t   fun),
        "use w_ggml_map_custom3_inplace instead");

    // custom operators v2

    typedef void (*w_ggml_custom1_op_t)(struct w_ggml_tensor * dst , const struct w_ggml_tensor * a, int ith, int nth, void * userdata);
    typedef void (*w_ggml_custom2_op_t)(struct w_ggml_tensor * dst , const struct w_ggml_tensor * a, const struct w_ggml_tensor * b, int ith, int nth, void * userdata);
    typedef void (*w_ggml_custom3_op_t)(struct w_ggml_tensor * dst , const struct w_ggml_tensor * a, const struct w_ggml_tensor * b, const struct w_ggml_tensor * c, int ith, int nth, void * userdata);

    #define W_GGML_N_TASKS_MAX -1

    W_GGML_API struct w_ggml_tensor * w_ggml_map_custom1(
            struct w_ggml_context   * ctx,
            struct w_ggml_tensor    * a,
            w_ggml_custom1_op_t       fun,
            int                     n_tasks,
            void                  * userdata);

    W_GGML_API struct w_ggml_tensor * w_ggml_map_custom1_inplace(
            struct w_ggml_context   * ctx,
            struct w_ggml_tensor    * a,
            w_ggml_custom1_op_t       fun,
            int                     n_tasks,
            void                  * userdata);

    W_GGML_API struct w_ggml_tensor * w_ggml_map_custom2(
            struct w_ggml_context   * ctx,
            struct w_ggml_tensor    * a,
            struct w_ggml_tensor    * b,
            w_ggml_custom2_op_t       fun,
            int                     n_tasks,
            void                  * userdata);

    W_GGML_API struct w_ggml_tensor * w_ggml_map_custom2_inplace(
            struct w_ggml_context   * ctx,
            struct w_ggml_tensor    * a,
            struct w_ggml_tensor    * b,
            w_ggml_custom2_op_t       fun,
            int                     n_tasks,
            void                  * userdata);

    W_GGML_API struct w_ggml_tensor * w_ggml_map_custom3(
            struct w_ggml_context   * ctx,
            struct w_ggml_tensor    * a,
            struct w_ggml_tensor    * b,
            struct w_ggml_tensor    * c,
            w_ggml_custom3_op_t       fun,
            int                     n_tasks,
            void                  * userdata);

    W_GGML_API struct w_ggml_tensor * w_ggml_map_custom3_inplace(
            struct w_ggml_context   * ctx,
            struct w_ggml_tensor    * a,
            struct w_ggml_tensor    * b,
            struct w_ggml_tensor    * c,
            w_ggml_custom3_op_t       fun,
            int                     n_tasks,
            void                  * userdata);

    // loss function

    W_GGML_API struct w_ggml_tensor * w_ggml_cross_entropy_loss(
            struct w_ggml_context         * ctx,
            struct w_ggml_tensor          * a,
            struct w_ggml_tensor          * b);

    W_GGML_API struct w_ggml_tensor * w_ggml_cross_entropy_loss_back(
            struct w_ggml_context         * ctx,
            struct w_ggml_tensor          * a,
            struct w_ggml_tensor          * b,
            struct w_ggml_tensor          * c);

    //
    // automatic differentiation
    //

    W_GGML_API void w_ggml_set_param(
            struct w_ggml_context * ctx,
            struct w_ggml_tensor  * tensor);


    W_GGML_API void w_ggml_build_forward_expand (struct w_ggml_cgraph * cgraph, struct w_ggml_tensor * tensor);
    W_GGML_API void w_ggml_build_backward_expand(struct w_ggml_context * ctx, struct w_ggml_cgraph * gf, struct w_ggml_cgraph * gb, bool keep);

    // graph allocation in a context
    W_GGML_API struct w_ggml_cgraph * w_ggml_new_graph         (struct w_ggml_context * ctx); // size = W_GGML_DEFAULT_GRAPH_SIZE, grads = false
    W_GGML_API struct w_ggml_cgraph * w_ggml_new_graph_custom  (struct w_ggml_context * ctx, size_t size, bool grads);
    W_GGML_API struct w_ggml_cgraph * w_ggml_graph_dup         (struct w_ggml_context * ctx, struct w_ggml_cgraph * cgraph);
    W_GGML_API struct w_ggml_cgraph   w_ggml_graph_view        (struct w_ggml_cgraph * cgraph, int i0, int i1);
    W_GGML_API void                 w_ggml_graph_cpy         (struct w_ggml_cgraph * src, struct w_ggml_cgraph * dst);
    W_GGML_API void                 w_ggml_graph_reset       (struct w_ggml_cgraph * cgraph);  // zero grads
    W_GGML_API void                 w_ggml_graph_clear       (struct w_ggml_cgraph * cgraph);

    W_GGML_API size_t w_ggml_graph_overhead(void);
    W_GGML_API size_t w_ggml_graph_overhead_custom(size_t size, bool grads);

    // w_ggml_graph_plan() has to be called before w_ggml_graph_compute()
    // when plan.work_size > 0, caller must allocate memory for plan.work_data
    W_GGML_API struct w_ggml_cplan w_ggml_graph_plan            (const struct w_ggml_cgraph * cgraph, int n_threads /*= W_GGML_DEFAULT_N_THREADS*/);
    W_GGML_API enum w_ggml_status  w_ggml_graph_compute         (      struct w_ggml_cgraph * cgraph, struct w_ggml_cplan * cplan);
    // same as w_ggml_graph_compute() but the work data is allocated as a part of the context
    // note: the drawback of this API is that you must have ensured that the context has enough memory for the work data
    W_GGML_API enum w_ggml_status  w_ggml_graph_compute_with_ctx(struct w_ggml_context * ctx, struct w_ggml_cgraph * cgraph, int n_threads);

    W_GGML_API struct w_ggml_tensor * w_ggml_graph_get_tensor(struct w_ggml_cgraph * cgraph, const char * name);

    W_GGML_API void                 w_ggml_graph_export(const struct w_ggml_cgraph * cgraph, const char * fname);
    W_GGML_API struct w_ggml_cgraph * w_ggml_graph_import(const char * fname, struct w_ggml_context ** ctx_data, struct w_ggml_context ** ctx_eval);

    // print info and performance information for the graph
    W_GGML_API void w_ggml_graph_print(const struct w_ggml_cgraph * cgraph);

    // dump the graph into a file using the dot format
    W_GGML_API void w_ggml_graph_dump_dot(const struct w_ggml_cgraph * gb, const struct w_ggml_cgraph * gf, const char * filename);

    // build gradient checkpointing backward graph gb for gf using provided checkpoints
    // gb_tmp will contain original backward graph with rewritten backward process nodes,
    // but without the second forward pass nodes.
    W_GGML_API void w_ggml_build_backward_gradient_checkpointing(
            struct w_ggml_context   * ctx,
            struct w_ggml_cgraph    * gf,
            struct w_ggml_cgraph    * gb,
            struct w_ggml_cgraph    * gb_tmp,
            struct w_ggml_tensor  * * checkpoints,
            int                     n_checkpoints);
    //
    // optimization
    //

    // optimization methods
    enum w_ggml_opt_type {
        W_GGML_OPT_TYPE_ADAM,
        W_GGML_OPT_TYPE_LBFGS,
    };

    // linesearch methods
    enum w_ggml_linesearch {
        W_GGML_LINESEARCH_DEFAULT = 1,

        W_GGML_LINESEARCH_BACKTRACKING_ARMIJO       = 0,
        W_GGML_LINESEARCH_BACKTRACKING_WOLFE        = 1,
        W_GGML_LINESEARCH_BACKTRACKING_STRONG_WOLFE = 2,
    };

    // optimization return values
    enum w_ggml_opt_result {
        W_GGML_OPT_RESULT_OK = 0,
        W_GGML_OPT_RESULT_DID_NOT_CONVERGE,
        W_GGML_OPT_RESULT_NO_CONTEXT,
        W_GGML_OPT_RESULT_INVALID_WOLFE,
        W_GGML_OPT_RESULT_FAIL,
        W_GGML_OPT_RESULT_CANCEL,

        W_GGML_LINESEARCH_FAIL = -128,
        W_GGML_LINESEARCH_MINIMUM_STEP,
        W_GGML_LINESEARCH_MAXIMUM_STEP,
        W_GGML_LINESEARCH_MAXIMUM_ITERATIONS,
        W_GGML_LINESEARCH_INVALID_PARAMETERS,
    };

    typedef void (*w_ggml_opt_callback)(void * data, int accum_step, float * sched, bool * cancel);
    typedef void (*w_ggml_log_callback)(enum w_ggml_log_level level, const char * text, void * user_data);

    // optimization parameters
    //
    //   see ggml.c (w_ggml_opt_default_params) for default values
    //
    struct w_ggml_opt_params {
        enum w_ggml_opt_type type;

        size_t graph_size;

        int n_threads;

        // delta-based convergence test
        //
        //   if past == 0 - disabled
        //   if past > 0:
        //     stop if |f(x) - f(x_past)| < delta * max(1, |f(x)|)
        //
        int past;
        float delta;

        // maximum number of iterations without improvement
        //
        //   if 0 - disabled
        //   if > 0:
        //     assume convergence if no cost improvement in this number of iterations
        //
        int max_no_improvement;

        bool print_forward_graph;
        bool print_backward_graph;

        int n_gradient_accumulation;

        // ADAM parameters
        struct {
            int n_iter;

            float sched; // schedule multiplier (fixed, decay or warmup)
            float decay; // weight decay for AdamW, use 0.0f to disable
            int   decay_min_ndim; // minimum number of tensor dimension to apply weight decay
            float alpha; // learning rate
            float beta1;
            float beta2;
            float eps;   // epsilon for numerical stability
            float eps_f; // epsilon for convergence test
            float eps_g; // epsilon for convergence test
            float gclip; // gradient clipping
        } adam;

        // LBFGS parameters
        struct {
            int m; // number of corrections to approximate the inv. Hessian
            int n_iter;
            int max_linesearch;

            float eps;      // convergence tolerance
            float ftol;     // line search tolerance
            float wolfe;
            float min_step;
            float max_step;

            enum w_ggml_linesearch linesearch;
        } lbfgs;
    };

    struct w_ggml_opt_context {
        struct w_ggml_context * ctx;
        struct w_ggml_opt_params params;

        int iter;
        int64_t nx; // number of parameter elements

        bool just_initialized;

        float loss_before;
        float loss_after;

        struct {
            struct w_ggml_tensor * g;  // current gradient
            struct w_ggml_tensor * m;  // first moment
            struct w_ggml_tensor * v;  // second moment
            struct w_ggml_tensor * pf; // past function values
            float fx_best;
            float fx_prev;
            int n_no_improvement;
        } adam;

        struct {
            struct w_ggml_tensor * x;    // current parameters
            struct w_ggml_tensor * xp;   // previous parameters
            struct w_ggml_tensor * g;    // current gradient
            struct w_ggml_tensor * gp;   // previous gradient
            struct w_ggml_tensor * d;    // search direction
            struct w_ggml_tensor * pf;   // past function values
            struct w_ggml_tensor * lmal; // the L-BFGS memory alpha
            struct w_ggml_tensor * lmys; // the L-BFGS memory ys
            struct w_ggml_tensor * lms;  // the L-BFGS memory s
            struct w_ggml_tensor * lmy;  // the L-BFGS memory y
            float fx_best;
            float step;
            int j;
            int k;
            int end;
            int n_no_improvement;
        } lbfgs;
    };

    W_GGML_API struct w_ggml_opt_params w_ggml_opt_default_params(enum w_ggml_opt_type type);

    // optimize the function defined by the tensor f
    W_GGML_API enum w_ggml_opt_result w_ggml_opt(
            struct w_ggml_context * ctx,
            struct w_ggml_opt_params params,
            struct w_ggml_tensor * f);

    // initialize optimizer context
    W_GGML_API void w_ggml_opt_init(
            struct w_ggml_context     * ctx,
            struct w_ggml_opt_context * opt,
            struct w_ggml_opt_params    params,
            int64_t                   nx);

    // continue optimizing the function defined by the tensor f
    W_GGML_API enum w_ggml_opt_result w_ggml_opt_resume(
            struct w_ggml_context * ctx,
            struct w_ggml_opt_context * opt,
            struct w_ggml_tensor * f);

    // continue optimizing the function defined by the tensor f
    W_GGML_API enum w_ggml_opt_result w_ggml_opt_resume_g(
            struct w_ggml_context * ctx,
            struct w_ggml_opt_context * opt,
            struct w_ggml_tensor * f,
            struct w_ggml_cgraph * gf,
            struct w_ggml_cgraph * gb,
            w_ggml_opt_callback callback,
            void * callback_data);

    //
    // tensor flags
    //
    W_GGML_API void w_ggml_set_input(struct w_ggml_tensor * tensor);
    W_GGML_API void w_ggml_set_output(struct w_ggml_tensor * tensor);

    //
    // quantization
    //

    // - w_ggml_quantize_init can be called multiple times with the same type
    //   it will only initialize the quantization tables for the first call or after w_ggml_quantize_free
    //   automatically called by w_ggml_quantize_chunk for convenience
    //
    // - w_ggml_quantize_free will free any memory allocated by w_ggml_quantize_init
    //   call this at the end of the program to avoid memory leaks
    //
    // note: these are thread-safe
    //
    W_GGML_API void w_ggml_quantize_init(enum w_ggml_type type);
    W_GGML_API void w_ggml_quantize_free(void);

    // some quantization type cannot be used without an importance matrix
    W_GGML_API bool w_ggml_quantize_requires_imatrix(enum w_ggml_type type);

    // calls w_ggml_quantize_init internally (i.e. can allocate memory)
    W_GGML_API size_t w_ggml_quantize_chunk(
            enum w_ggml_type   type,
               const float * src,
                      void * dst,
                   int64_t   start,
                   int64_t   nrows,
                   int64_t   n_per_row,
               const float * imatrix);

    //
    // gguf
    //

    enum gguf_type {
        GGUF_TYPE_UINT8   = 0,
        GGUF_TYPE_INT8    = 1,
        GGUF_TYPE_UINT16  = 2,
        GGUF_TYPE_INT16   = 3,
        GGUF_TYPE_UINT32  = 4,
        GGUF_TYPE_INT32   = 5,
        GGUF_TYPE_FLOAT32 = 6,
        GGUF_TYPE_BOOL    = 7,
        GGUF_TYPE_STRING  = 8,
        GGUF_TYPE_ARRAY   = 9,
        GGUF_TYPE_UINT64  = 10,
        GGUF_TYPE_INT64   = 11,
        GGUF_TYPE_FLOAT64 = 12,
        GGUF_TYPE_COUNT,       // marks the end of the enum
    };

    struct gguf_context;

    struct gguf_init_params {
        bool no_alloc;

        // if not NULL, create a w_ggml_context and allocate the tensor data in it
        struct w_ggml_context ** ctx;
    };

    W_GGML_API struct gguf_context * gguf_init_empty(void);
    W_GGML_API struct gguf_context * gguf_init_from_file(const char * fname, struct gguf_init_params params);
    //W_GGML_API struct gguf_context * gguf_init_from_buffer(..);

    W_GGML_API void gguf_free(struct gguf_context * ctx);

    W_GGML_API const char * gguf_type_name(enum gguf_type type);

    W_GGML_API int    gguf_get_version    (const struct gguf_context * ctx);
    W_GGML_API size_t gguf_get_alignment  (const struct gguf_context * ctx);
    W_GGML_API size_t gguf_get_data_offset(const struct gguf_context * ctx);
    W_GGML_API void * gguf_get_data       (const struct gguf_context * ctx);

    W_GGML_API int          gguf_get_n_kv(const struct gguf_context * ctx);
    W_GGML_API int          gguf_find_key(const struct gguf_context * ctx, const char * key);
    W_GGML_API const char * gguf_get_key (const struct gguf_context * ctx, int key_id);

    W_GGML_API enum gguf_type gguf_get_kv_type (const struct gguf_context * ctx, int key_id);
    W_GGML_API enum gguf_type gguf_get_arr_type(const struct gguf_context * ctx, int key_id);

    // will abort if the wrong type is used for the key
    W_GGML_API uint8_t      gguf_get_val_u8  (const struct gguf_context * ctx, int key_id);
    W_GGML_API int8_t       gguf_get_val_i8  (const struct gguf_context * ctx, int key_id);
    W_GGML_API uint16_t     gguf_get_val_u16 (const struct gguf_context * ctx, int key_id);
    W_GGML_API int16_t      gguf_get_val_i16 (const struct gguf_context * ctx, int key_id);
    W_GGML_API uint32_t     gguf_get_val_u32 (const struct gguf_context * ctx, int key_id);
    W_GGML_API int32_t      gguf_get_val_i32 (const struct gguf_context * ctx, int key_id);
    W_GGML_API float        gguf_get_val_f32 (const struct gguf_context * ctx, int key_id);
    W_GGML_API uint64_t     gguf_get_val_u64 (const struct gguf_context * ctx, int key_id);
    W_GGML_API int64_t      gguf_get_val_i64 (const struct gguf_context * ctx, int key_id);
    W_GGML_API double       gguf_get_val_f64 (const struct gguf_context * ctx, int key_id);
    W_GGML_API bool         gguf_get_val_bool(const struct gguf_context * ctx, int key_id);
    W_GGML_API const char * gguf_get_val_str (const struct gguf_context * ctx, int key_id);
    W_GGML_API const void * gguf_get_val_data(const struct gguf_context * ctx, int key_id);
    W_GGML_API int          gguf_get_arr_n   (const struct gguf_context * ctx, int key_id);
    W_GGML_API const void * gguf_get_arr_data(const struct gguf_context * ctx, int key_id);
    W_GGML_API const char * gguf_get_arr_str (const struct gguf_context * ctx, int key_id, int i);

    W_GGML_API int            gguf_get_n_tensors    (const struct gguf_context * ctx);
    W_GGML_API int            gguf_find_tensor      (const struct gguf_context * ctx, const char * name);
    W_GGML_API size_t         gguf_get_tensor_offset(const struct gguf_context * ctx, int i);
    W_GGML_API char *         gguf_get_tensor_name  (const struct gguf_context * ctx, int i);
    W_GGML_API enum w_ggml_type gguf_get_tensor_type  (const struct gguf_context * ctx, int i);

    // removes key if it exists
    W_GGML_API void gguf_remove_key(struct gguf_context * ctx, const char * key);

    // overrides existing values or adds a new one
    W_GGML_API void gguf_set_val_u8  (struct gguf_context * ctx, const char * key, uint8_t  val);
    W_GGML_API void gguf_set_val_i8  (struct gguf_context * ctx, const char * key, int8_t   val);
    W_GGML_API void gguf_set_val_u16 (struct gguf_context * ctx, const char * key, uint16_t val);
    W_GGML_API void gguf_set_val_i16 (struct gguf_context * ctx, const char * key, int16_t  val);
    W_GGML_API void gguf_set_val_u32 (struct gguf_context * ctx, const char * key, uint32_t val);
    W_GGML_API void gguf_set_val_i32 (struct gguf_context * ctx, const char * key, int32_t  val);
    W_GGML_API void gguf_set_val_f32 (struct gguf_context * ctx, const char * key, float    val);
    W_GGML_API void gguf_set_val_u64 (struct gguf_context * ctx, const char * key, uint64_t val);
    W_GGML_API void gguf_set_val_i64 (struct gguf_context * ctx, const char * key, int64_t  val);
    W_GGML_API void gguf_set_val_f64 (struct gguf_context * ctx, const char * key, double   val);
    W_GGML_API void gguf_set_val_bool(struct gguf_context * ctx, const char * key, bool     val);
    W_GGML_API void gguf_set_val_str (struct gguf_context * ctx, const char * key, const char * val);
    W_GGML_API void gguf_set_arr_data(struct gguf_context * ctx, const char * key, enum gguf_type type, const void * data, int n);
    W_GGML_API void gguf_set_arr_str (struct gguf_context * ctx, const char * key, const char ** data, int n);

    // set or add KV pairs from another context
    W_GGML_API void gguf_set_kv(struct gguf_context * ctx, struct gguf_context * src);

    // manage tensor info
    W_GGML_API void gguf_add_tensor(struct gguf_context * ctx, const struct w_ggml_tensor * tensor);
    W_GGML_API void gguf_set_tensor_type(struct gguf_context * ctx, const char * name, enum w_ggml_type type);
    W_GGML_API void gguf_set_tensor_data(struct gguf_context * ctx, const char * name, const void * data, size_t size);

    // writing gguf files can be done in 2 ways:
    //
    // - write the entire gguf_context to a binary file in a single pass:
    //
    //   gguf_write_to_file(ctx, fname);
    //
    // - first prepare a file with a placeholder for the meta data, write the tensor data, then write the meta data:
    //
    //   FILE * f = fopen(fname, "wb");
    //   fseek(f, gguf_get_meta_size(ctx), SEEK_SET);
    //   fwrite(f, ...);
    //   void * data = gguf_meta_get_meta_data(ctx);
    //   fseek(f, 0, SEEK_SET);
    //   fwrite(f, data, gguf_get_meta_size(ctx));
    //   free(data);
    //   fclose(f);
    //

    // write the entire context to a binary file
    W_GGML_API void gguf_write_to_file(const struct gguf_context * ctx, const char * fname, bool only_meta);

    // get the size in bytes of the meta data (header, kv pairs, tensor info) including padding
    W_GGML_API size_t gguf_get_meta_size(const struct gguf_context * ctx);
    W_GGML_API void   gguf_get_meta_data(const struct gguf_context * ctx, void * data);

    //
    // system info
    //

    W_GGML_API int w_ggml_cpu_has_avx        (void);
    W_GGML_API int w_ggml_cpu_has_avx_vnni   (void);
    W_GGML_API int w_ggml_cpu_has_avx2       (void);
    W_GGML_API int w_ggml_cpu_has_avx512     (void);
    W_GGML_API int w_ggml_cpu_has_avx512_vbmi(void);
    W_GGML_API int w_ggml_cpu_has_avx512_vnni(void);
    W_GGML_API int w_ggml_cpu_has_fma        (void);
    W_GGML_API int w_ggml_cpu_has_neon       (void);
    W_GGML_API int w_ggml_cpu_has_arm_fma    (void);
    W_GGML_API int w_ggml_cpu_has_metal      (void);
    W_GGML_API int w_ggml_cpu_has_f16c       (void);
    W_GGML_API int w_ggml_cpu_has_fp16_va    (void);
    W_GGML_API int w_ggml_cpu_has_wasm_simd  (void);
    W_GGML_API int w_ggml_cpu_has_blas       (void);
    W_GGML_API int w_ggml_cpu_has_cuda       (void);
    W_GGML_API int w_ggml_cpu_has_clblast    (void);
    W_GGML_API int w_ggml_cpu_has_vulkan     (void);
    W_GGML_API int w_ggml_cpu_has_kompute    (void);
    W_GGML_API int w_ggml_cpu_has_gpublas    (void);
    W_GGML_API int w_ggml_cpu_has_sse3       (void);
    W_GGML_API int w_ggml_cpu_has_ssse3      (void);
    W_GGML_API int w_ggml_cpu_has_sycl       (void);
    W_GGML_API int w_ggml_cpu_has_vsx        (void);
    W_GGML_API int w_ggml_cpu_has_matmul_int8(void);

    //
    // Internal types and functions exposed for tests and benchmarks
    //

#ifdef  __cplusplus
// restrict not standard in C++
#define W_GGML_RESTRICT
#else
#define W_GGML_RESTRICT restrict
#endif
    typedef void (*w_ggml_to_float_t)  (const void  * W_GGML_RESTRICT x, float * W_GGML_RESTRICT y, int64_t k);
    typedef void (*w_ggml_from_float_t)(const float * W_GGML_RESTRICT x, void  * W_GGML_RESTRICT y, int64_t k);
    typedef void (*w_ggml_vec_dot_t)   (int n, float * W_GGML_RESTRICT s, size_t bs, const void * W_GGML_RESTRICT x, size_t bx,
                                      const void * W_GGML_RESTRICT y, size_t by, int nrc);

    typedef struct {
        const char      * type_name;
        int               blck_size;
        size_t            type_size;
        bool              is_quantized;
        w_ggml_to_float_t   to_float;
        w_ggml_from_float_t from_float;
        w_ggml_from_float_t from_float_reference;
        w_ggml_vec_dot_t    vec_dot;
        enum w_ggml_type    vec_dot_type;
        int64_t           nrows; // number of rows to process simultaneously;
    } w_ggml_type_traits_t;

    W_GGML_API w_ggml_type_traits_t w_ggml_internal_get_type_traits(enum w_ggml_type type);

#ifdef  __cplusplus
}
#endif
