#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void SizeError(const char *description);

#define MATRIX_ROWS(m) (*((int *)(m)))
#define MATRIX_COLS(m) (*((int *)((char *)(m) + 4)))
#define MATRIX_DATA_PTR(m) (*((void **)((char *)(m) + 8)))

// A: pointer to the first matrix
// B: pointer to the second matrix
// type: 'i' for int, 'f' for float
void* matrixMultiply(void* A, void* B, char type) {

  int A_rows = MATRIX_ROWS(A);
  int A_cols = MATRIX_COLS(A);
  int B_rows = MATRIX_ROWS(B);
  int B_cols = MATRIX_COLS(B);

  if (A_cols != B_rows) {
    SizeError("Incompatible matrix dimensions");
    return NULL;
  }

  void *A_data = MATRIX_DATA_PTR(A);
  void *B_data = MATRIX_DATA_PTR(B);

  size_t header_size = sizeof(int) * 2 + sizeof(void*);
  void *C = malloc(header_size);

  MATRIX_ROWS(C) = A_rows;
  MATRIX_COLS(C) = B_cols;

  size_t elem_size = (type == 'i') ? sizeof(int) : sizeof(float);

  void *C_data = calloc(A_rows * B_cols, elem_size);
  MATRIX_DATA_PTR(C) = C_data;

  if (type == 'i') {
    int *A_int = (int *)A_data;
    int *B_int = (int *)B_data;
    int *C_int = (int *)C_data;

    for (int i = 0; i < A_rows; i++) {
      for (int j = 0; j < B_cols; j++) {
        int sum = 0;
        for (int k = 0; k < A_cols; k++) {
          sum += A_int[i * A_cols + k] * B_int[k * B_cols + j];
        }
        C_int[i * B_cols + j] = sum;
      }
    }
  } else if (type == 'f') {
    float *A_float = (float *)A_data;
    float *B_float = (float *)B_data;
    float *C_float = (float *)C_data;

    for (int i = 0; i < A_rows; i++) {
      for (int j = 0; j < B_cols; j++) {
        float sum = 0.0f;
        for (int k = 0; k < A_cols; k++) {
          sum += A_float[i * A_cols + k] * B_float[k * B_cols + j];
        }
        C_float[i * B_cols + j] = sum;
      }
    }
  }

  free(A_data);
  free(A);
  free(B_data);
  free(B);

  return C;
}

void* matrixAdd(void* A, void* B, char type) {

  int A_rows = MATRIX_ROWS(A);
  int A_cols = MATRIX_COLS(A);
  int B_rows = MATRIX_ROWS(B);
  int B_cols = MATRIX_COLS(B);

  if (A_rows != B_rows || A_cols != B_cols) {
    SizeError("Incompatible matrix dimensions");
    return NULL;
  }

  void *A_data = MATRIX_DATA_PTR(A);
  void *B_data = MATRIX_DATA_PTR(B);

  // Allocate space for the result matrix header + data pointer
  size_t header_size = sizeof(int)*2 + sizeof(void*);
  void *C = malloc(header_size);

  MATRIX_ROWS(C) = A_rows;
  MATRIX_COLS(C) = A_cols;

  // Determine element size
  size_t elem_size = (type == 'i') ? sizeof(int) : sizeof(float);

  // Allocate memory for C_data
  void *C_data = malloc(A_rows * A_cols * elem_size);
  MATRIX_DATA_PTR(C) = C_data;

  // Perform the addition
  if (type == 'i') {
    int *A_int = (int *)A_data;
    int *B_int = (int *)B_data;
    int *C_int = (int *)C_data;

    for (int i = 0; i < A_rows * A_cols; i++) {
      C_int[i] = A_int[i] + B_int[i];
    }
  } else if (type == 'f') {
    float *A_float = (float *)A_data;
    float *B_float = (float *)B_data;
    float *C_float = (float *)C_data;

    for (int i = 0; i < A_rows * A_cols; i++) {
      C_float[i] = A_float[i] + B_float[i];
    }
  }

  return C;
}

int main2() {
  int A_rows = 2, A_cols = 3;
  int B_rows = 3, B_cols = 2;

  void *A = malloc(sizeof(int)*2 + sizeof(void*));
  void *B = malloc(sizeof(int)*2 + sizeof(void*));

  MATRIX_ROWS(A) = A_rows;
  MATRIX_COLS(A) = A_cols;
  MATRIX_ROWS(B) = B_rows;
  MATRIX_COLS(B) = B_cols;

  int *A_data = malloc(sizeof(int)*A_rows*A_cols);
  int *B_data = malloc(sizeof(int)*B_rows*B_cols);

  // Initialize A: (2x3)
  // [1 2 3]
  // [4 5 6]
  for (int i = 0; i < A_rows*A_cols; i++) {
    A_data[i] = i + 1;
  }

  // Initialize B: (3x2)
  // [7 8]
  // [9 10]
  // [11 12]
  for (int i = 0; i < B_rows*B_cols; i++) {
    B_data[i] = i + 7;
  }

  MATRIX_DATA_PTR(A) = A_data;
  MATRIX_DATA_PTR(B) = B_data;

  // Multiply A (2x3) by B (3x2) = C (2x2)
  void *C = matrixMultiply(A, B, 'i');
  if (C) {
    int C_rows = MATRIX_ROWS(C);
    int C_cols = MATRIX_COLS(C);
    int *C_data = (int *)MATRIX_DATA_PTR(C);

    printf("Result matrix C:\n");
    for (int i = 0; i < C_rows; i++) {
      for (int j = 0; j < C_cols; j++) {
        printf("%d ", C_data[i*C_cols+j]);
      }
      printf("\n");
    }

    free(MATRIX_DATA_PTR(C));
    free(C);
  }

  return 0;
}
