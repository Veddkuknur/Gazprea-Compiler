#include "run_time_errors.h"

void intDivZero(int value) {
    if (value == 0) {
        MathError("Division by zero");
    }
}

void floatDivZero(float value) {
    if (value == 0) {
        MathError("Division by zero");
    }
}

void intZeroExp(int left, int right) {
    if (left ==0 && right <=0) {
        MathError("Tried to raise 0 to the power of a number less than or equal to zero");
    }
}

void floatZeroExp(float left, float right) {
    if (left == 0 && right <=0) {
        MathError("Tried to raise 0 to the power of a number less than or equal to zero");
    }
}