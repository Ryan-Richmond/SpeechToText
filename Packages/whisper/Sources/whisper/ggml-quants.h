#pragma once

#define W_GGML_COMMON_DECL_C
#include "ggml-common.h"

#include "ggml.h"

// GGML internal header

#ifdef __cplusplus
extern "C" {
#endif

// Quantization
void quantize_row_q4_0_reference(const float * W_GGML_RESTRICT x, block_q4_0 * W_GGML_RESTRICT y, int64_t k);
void quantize_row_q4_1_reference(const float * W_GGML_RESTRICT x, block_q4_1 * W_GGML_RESTRICT y, int64_t k);
void quantize_row_q5_0_reference(const float * W_GGML_RESTRICT x, block_q5_0 * W_GGML_RESTRICT y, int64_t k);
void quantize_row_q5_1_reference(const float * W_GGML_RESTRICT x, block_q5_1 * W_GGML_RESTRICT y, int64_t k);
void quantize_row_q8_0_reference(const float * W_GGML_RESTRICT x, block_q8_0 * W_GGML_RESTRICT y, int64_t k);
void quantize_row_q8_1_reference(const float * W_GGML_RESTRICT x, block_q8_1 * W_GGML_RESTRICT y, int64_t k);

void quantize_row_q2_K_reference(const float * W_GGML_RESTRICT x, block_q2_K * W_GGML_RESTRICT y, int64_t k);
void quantize_row_q3_K_reference(const float * W_GGML_RESTRICT x, block_q3_K * W_GGML_RESTRICT y, int64_t k);
void quantize_row_q4_K_reference(const float * W_GGML_RESTRICT x, block_q4_K * W_GGML_RESTRICT y, int64_t k);
void quantize_row_q5_K_reference(const float * W_GGML_RESTRICT x, block_q5_K * W_GGML_RESTRICT y, int64_t k);
void quantize_row_q6_K_reference(const float * W_GGML_RESTRICT x, block_q6_K * W_GGML_RESTRICT y, int64_t k);
void quantize_row_q8_K_reference(const float * W_GGML_RESTRICT x, block_q8_K * W_GGML_RESTRICT y, int64_t k);

void quantize_row_iq3_xxs_reference(const float * W_GGML_RESTRICT x, block_iq3_xxs * W_GGML_RESTRICT y, int64_t k);
void quantize_row_iq4_nl_reference (const float * W_GGML_RESTRICT x, block_iq4_nl  * W_GGML_RESTRICT y, int64_t k);
void quantize_row_iq4_xs_reference (const float * W_GGML_RESTRICT x, block_iq4_xs  * W_GGML_RESTRICT y, int64_t k);
void quantize_row_iq3_s_reference  (const float * W_GGML_RESTRICT x, block_iq3_s   * W_GGML_RESTRICT y, int64_t k);
void quantize_row_iq2_s_reference  (const float * W_GGML_RESTRICT x, block_iq2_s   * W_GGML_RESTRICT y, int64_t k);

void quantize_row_q4_0(const float * W_GGML_RESTRICT x, void * W_GGML_RESTRICT y, int64_t k);
void quantize_row_q4_1(const float * W_GGML_RESTRICT x, void * W_GGML_RESTRICT y, int64_t k);
void quantize_row_q5_0(const float * W_GGML_RESTRICT x, void * W_GGML_RESTRICT y, int64_t k);
void quantize_row_q5_1(const float * W_GGML_RESTRICT x, void * W_GGML_RESTRICT y, int64_t k);
void quantize_row_q8_0(const float * W_GGML_RESTRICT x, void * W_GGML_RESTRICT y, int64_t k);
void quantize_row_q8_1(const float * W_GGML_RESTRICT x, void * W_GGML_RESTRICT y, int64_t k);

void quantize_row_q2_K(const float * W_GGML_RESTRICT x, void * W_GGML_RESTRICT y, int64_t k);
void quantize_row_q3_K(const float * W_GGML_RESTRICT x, void * W_GGML_RESTRICT y, int64_t k);
void quantize_row_q4_K(const float * W_GGML_RESTRICT x, void * W_GGML_RESTRICT y, int64_t k);
void quantize_row_q5_K(const float * W_GGML_RESTRICT x, void * W_GGML_RESTRICT y, int64_t k);
void quantize_row_q6_K(const float * W_GGML_RESTRICT x, void * W_GGML_RESTRICT y, int64_t k);
void quantize_row_q8_K(const float * W_GGML_RESTRICT x, void * W_GGML_RESTRICT y, int64_t k);

void quantize_row_iq3_xxs(const float * W_GGML_RESTRICT x, void * W_GGML_RESTRICT y, int64_t k);
void quantize_row_iq4_nl (const float * W_GGML_RESTRICT x, void * W_GGML_RESTRICT y, int64_t k);
void quantize_row_iq4_xs (const float * W_GGML_RESTRICT x, void * W_GGML_RESTRICT y, int64_t k);
void quantize_row_iq3_s  (const float * W_GGML_RESTRICT x, void * W_GGML_RESTRICT y, int64_t k);
void quantize_row_iq2_s  (const float * W_GGML_RESTRICT x, void * W_GGML_RESTRICT y, int64_t k);

// Dequantization
void dequantize_row_q4_0(const block_q4_0 * W_GGML_RESTRICT x, float * W_GGML_RESTRICT y, int64_t k);
void dequantize_row_q4_1(const block_q4_1 * W_GGML_RESTRICT x, float * W_GGML_RESTRICT y, int64_t k);
void dequantize_row_q5_0(const block_q5_0 * W_GGML_RESTRICT x, float * W_GGML_RESTRICT y, int64_t k);
void dequantize_row_q5_1(const block_q5_1 * W_GGML_RESTRICT x, float * W_GGML_RESTRICT y, int64_t k);
void dequantize_row_q8_0(const block_q8_0 * W_GGML_RESTRICT x, float * W_GGML_RESTRICT y, int64_t k);
//void dequantize_row_q8_1(const block_q8_1 * W_GGML_RESTRICT x, float * W_GGML_RESTRICT y, int64_t k);

void dequantize_row_q2_K(const block_q2_K * W_GGML_RESTRICT x, float * W_GGML_RESTRICT y, int64_t k);
void dequantize_row_q3_K(const block_q3_K * W_GGML_RESTRICT x, float * W_GGML_RESTRICT y, int64_t k);
void dequantize_row_q4_K(const block_q4_K * W_GGML_RESTRICT x, float * W_GGML_RESTRICT y, int64_t k);
void dequantize_row_q5_K(const block_q5_K * W_GGML_RESTRICT x, float * W_GGML_RESTRICT y, int64_t k);
void dequantize_row_q6_K(const block_q6_K * W_GGML_RESTRICT x, float * W_GGML_RESTRICT y, int64_t k);
void dequantize_row_q8_K(const block_q8_K * W_GGML_RESTRICT x, float * W_GGML_RESTRICT y, int64_t k);

void dequantize_row_iq2_xxs(const block_iq2_xxs * W_GGML_RESTRICT x, float * W_GGML_RESTRICT y, int64_t k);
void dequantize_row_iq2_xs (const block_iq2_xs  * W_GGML_RESTRICT x, float * W_GGML_RESTRICT y, int64_t k);
void dequantize_row_iq2_s  (const block_iq2_s   * W_GGML_RESTRICT x, float * W_GGML_RESTRICT y, int64_t k);
void dequantize_row_iq3_xxs(const block_iq3_xxs * W_GGML_RESTRICT x, float * W_GGML_RESTRICT y, int64_t k);
void dequantize_row_iq1_s  (const block_iq1_s   * W_GGML_RESTRICT x, float * W_GGML_RESTRICT y, int64_t k);
void dequantize_row_iq1_m  (const block_iq1_m   * W_GGML_RESTRICT x, float * W_GGML_RESTRICT y, int64_t k);
void dequantize_row_iq4_nl (const block_iq4_nl  * W_GGML_RESTRICT x, float * W_GGML_RESTRICT y, int64_t k);
void dequantize_row_iq4_xs (const block_iq4_xs  * W_GGML_RESTRICT x, float * W_GGML_RESTRICT y, int64_t k);
void dequantize_row_iq3_s  (const block_iq3_s   * W_GGML_RESTRICT x, float * W_GGML_RESTRICT y, int64_t k);

// Dot product
void w_ggml_vec_dot_q4_0_q8_0(int n, float * W_GGML_RESTRICT s, size_t bs, const void * W_GGML_RESTRICT vx, size_t bx, const void * W_GGML_RESTRICT vy, size_t by, int nrc);
void w_ggml_vec_dot_q4_1_q8_1(int n, float * W_GGML_RESTRICT s, size_t bs, const void * W_GGML_RESTRICT vx, size_t bx, const void * W_GGML_RESTRICT vy, size_t by, int nrc);
void w_ggml_vec_dot_q5_0_q8_0(int n, float * W_GGML_RESTRICT s, size_t bs, const void * W_GGML_RESTRICT vx, size_t bx, const void * W_GGML_RESTRICT vy, size_t by, int nrc);
void w_ggml_vec_dot_q5_1_q8_1(int n, float * W_GGML_RESTRICT s, size_t bs, const void * W_GGML_RESTRICT vx, size_t bx, const void * W_GGML_RESTRICT vy, size_t by, int nrc);
void w_ggml_vec_dot_q8_0_q8_0(int n, float * W_GGML_RESTRICT s, size_t bs, const void * W_GGML_RESTRICT vx, size_t bx, const void * W_GGML_RESTRICT vy, size_t by, int nrc);

void w_ggml_vec_dot_q2_K_q8_K(int n, float * W_GGML_RESTRICT s, size_t bs, const void * W_GGML_RESTRICT vx, size_t bx, const void * W_GGML_RESTRICT vy, size_t by, int nrc);
void w_ggml_vec_dot_q3_K_q8_K(int n, float * W_GGML_RESTRICT s, size_t bs, const void * W_GGML_RESTRICT vx, size_t bx, const void * W_GGML_RESTRICT vy, size_t by, int nrc);
void w_ggml_vec_dot_q4_K_q8_K(int n, float * W_GGML_RESTRICT s, size_t bs, const void * W_GGML_RESTRICT vx, size_t bx, const void * W_GGML_RESTRICT vy, size_t by, int nrc);
void w_ggml_vec_dot_q5_K_q8_K(int n, float * W_GGML_RESTRICT s, size_t bs, const void * W_GGML_RESTRICT vx, size_t bx, const void * W_GGML_RESTRICT vy, size_t by, int nrc);
void w_ggml_vec_dot_q6_K_q8_K(int n, float * W_GGML_RESTRICT s, size_t bs, const void * W_GGML_RESTRICT vx, size_t bx, const void * W_GGML_RESTRICT vy, size_t by, int nrc);

void w_ggml_vec_dot_iq2_xxs_q8_K(int n, float * W_GGML_RESTRICT s, size_t bs, const void * W_GGML_RESTRICT vx, size_t bx, const void * W_GGML_RESTRICT vy, size_t by, int nrc);
void w_ggml_vec_dot_iq2_xs_q8_K (int n, float * W_GGML_RESTRICT s, size_t bs, const void * W_GGML_RESTRICT vx, size_t bx, const void * W_GGML_RESTRICT vy, size_t by, int nrc);
void w_ggml_vec_dot_iq2_s_q8_K  (int n, float * W_GGML_RESTRICT s, size_t bs, const void * W_GGML_RESTRICT vx, size_t bx, const void * W_GGML_RESTRICT vy, size_t by, int nrc);
void w_ggml_vec_dot_iq3_xxs_q8_K(int n, float * W_GGML_RESTRICT s, size_t bs, const void * W_GGML_RESTRICT vx, size_t bx, const void * W_GGML_RESTRICT vy, size_t by, int nrc);
void w_ggml_vec_dot_iq1_s_q8_K  (int n, float * W_GGML_RESTRICT s, size_t bs, const void * W_GGML_RESTRICT vx, size_t bx, const void * W_GGML_RESTRICT vy, size_t by, int nrc);
void w_ggml_vec_dot_iq1_m_q8_K  (int n, float * W_GGML_RESTRICT s, size_t bs, const void * W_GGML_RESTRICT vx, size_t bx, const void * W_GGML_RESTRICT vy, size_t by, int nrc);
void w_ggml_vec_dot_iq4_nl_q8_0 (int n, float * W_GGML_RESTRICT s, size_t bs, const void * W_GGML_RESTRICT vx, size_t bx, const void * W_GGML_RESTRICT vy, size_t by, int nrc);
void w_ggml_vec_dot_iq4_xs_q8_K (int n, float * W_GGML_RESTRICT s, size_t bs, const void * W_GGML_RESTRICT vx, size_t bx, const void * W_GGML_RESTRICT vy, size_t by, int nrc);
void w_ggml_vec_dot_iq3_s_q8_K  (int n, float * W_GGML_RESTRICT s, size_t bs, const void * W_GGML_RESTRICT vx, size_t bx, const void * W_GGML_RESTRICT vy, size_t by, int nrc);

// Quantization utilizing an importance matrix (a.k.a. "Activation aWare Quantization")
size_t quantize_iq2_xxs(const float * W_GGML_RESTRICT src, void * W_GGML_RESTRICT dst, int64_t nrows, int64_t n_per_row, const float * imatrix);
size_t quantize_iq2_xs (const float * W_GGML_RESTRICT src, void * W_GGML_RESTRICT dst, int64_t nrows, int64_t n_per_row, const float * imatrix);
size_t quantize_iq2_s  (const float * W_GGML_RESTRICT src, void * W_GGML_RESTRICT dst, int64_t nrows, int64_t n_per_row, const float * imatrix);
size_t quantize_iq3_xxs(const float * W_GGML_RESTRICT src, void * W_GGML_RESTRICT dst, int64_t nrows, int64_t n_per_row, const float * imatrix);
size_t quantize_iq1_s  (const float * W_GGML_RESTRICT src, void * W_GGML_RESTRICT dst, int64_t nrows, int64_t n_per_row, const float * imatrix);
size_t quantize_iq1_m  (const float * W_GGML_RESTRICT src, void * W_GGML_RESTRICT dst, int64_t nrows, int64_t n_per_row, const float * imatrix);
size_t quantize_iq4_nl (const float * W_GGML_RESTRICT src, void * W_GGML_RESTRICT dst, int64_t nrows, int64_t n_per_row, const float * imatrix);
size_t quantize_iq4_xs (const float * W_GGML_RESTRICT src, void * W_GGML_RESTRICT dst, int64_t nrows, int64_t n_per_row, const float * imatrix);
size_t quantize_iq3_s  (const float * W_GGML_RESTRICT src, void * W_GGML_RESTRICT dst, int64_t nrows, int64_t n_per_row, const float * imatrix);

size_t quantize_q2_K(const float * W_GGML_RESTRICT src, void * W_GGML_RESTRICT dst, int64_t nrows, int64_t n_per_row, const float * imatrix);
size_t quantize_q3_K(const float * W_GGML_RESTRICT src, void * W_GGML_RESTRICT dst, int64_t nrows, int64_t n_per_row, const float * imatrix);
size_t quantize_q4_K(const float * W_GGML_RESTRICT src, void * W_GGML_RESTRICT dst, int64_t nrows, int64_t n_per_row, const float * imatrix);
size_t quantize_q5_K(const float * W_GGML_RESTRICT src, void * W_GGML_RESTRICT dst, int64_t nrows, int64_t n_per_row, const float * imatrix);
size_t quantize_q6_K(const float * W_GGML_RESTRICT src, void * W_GGML_RESTRICT dst, int64_t nrows, int64_t n_per_row, const float * imatrix);
size_t quantize_q4_0(const float * W_GGML_RESTRICT src, void * W_GGML_RESTRICT dst, int64_t nrows, int64_t n_per_row, const float * imatrix);
size_t quantize_q4_1(const float * W_GGML_RESTRICT src, void * W_GGML_RESTRICT dst, int64_t nrows, int64_t n_per_row, const float * imatrix);
size_t quantize_q5_0(const float * W_GGML_RESTRICT src, void * W_GGML_RESTRICT dst, int64_t nrows, int64_t n_per_row, const float * imatrix);
size_t quantize_q5_1(const float * W_GGML_RESTRICT src, void * W_GGML_RESTRICT dst, int64_t nrows, int64_t n_per_row, const float * imatrix);
size_t quantize_q8_0(const float * W_GGML_RESTRICT src, void * W_GGML_RESTRICT dst, int64_t nrows, int64_t n_per_row, const float * imatrix);

void iq2xs_init_impl(enum w_ggml_type type);
void iq2xs_free_impl(enum w_ggml_type type);
void iq3xs_init_impl(int grid_size);
void iq3xs_free_impl(int grid_size);

#ifdef __cplusplus
}
#endif

