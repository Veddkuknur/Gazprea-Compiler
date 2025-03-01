#include <stdio.h>

void checkStdin(int *state) {
  int ch = fgetc(stdin);
  if (ch == EOF) {
    if (feof(stdin)) {
      // stdin is empty (EOF)
      *state = 2;
    }
  } else {
    ungetc(ch, stdin);
    // stdin has data
  }
}
