//
// Created by judafer2000 on 12/8/24.
//

void IndexError(const char *description);
void MathError(const char *description);
void SizeError(const char *description);
void StrideError(const char *description);


void vectorSizeCheck(int size1, int size2) {
    if (size1 != size2) {
        SizeError("Size mismatch between vectors");
    }
    if (size1 < 0 || size2 < 0) {
        SizeError("At least one vector has negative size");
    }
}


void vectorIndexCheck(int index, int vector_size) {
    if (index > vector_size || index < 0) {
        IndexError("Index out of bounds");
    }

}



void declVectorSizeCheck(int decl_size, int vec_size) {
    if (vec_size > decl_size) {
        SizeError("Vector size is greater than declaration size");
    }

    if (decl_size < 0) {
        SizeError("Vector declaration size is negative");
    }
    if (vec_size <0) {
        SizeError("Vector has negative size");
    }
}


void matrixSizeCheck(int row1, int col1, int row2, int col2) {
    if (row1 != row2 || col1 != col2) {
        SizeError("Size mismatch between matrices");

    }
    if (row1 < 0 || row2 < 0 || col1 < 0 || col2 < 0) {
        SizeError("At least one matrix has negative size");
    }
}

void matrixDeclSizeCheck(int decl_row1, int decl_col1, int row2, int col2) {
    if (decl_row1 != row2 || decl_col1 != col2) {
        SizeError("Size mismatch between matrices");

    }
    if (decl_row1 < 0 || decl_col1 < 0 || row2 < 0 || col2 < 0) {
        SizeError("At least one matrix has negative size");
    }
}

void throwMatrixSizeError(){
  SizeError("in matrix operation");
}

void throwMatrixIndexError(){
  IndexError("Invalid index provided to vector/matrix");
}