// Minimal test to check if sapf can even start
// Prints to stderr at each stage to identify crash location

#include <cstdio>

int main(int argc, char** argv) {
    fprintf(stderr, "DEBUG: main() entered\n");
    fflush(stderr);

    fprintf(stderr, "DEBUG: About to print to stdout\n");
    fflush(stderr);

    printf("Hello from test\n");
    fflush(stdout);

    fprintf(stderr, "DEBUG: main() exiting normally\n");
    fflush(stderr);

    return 0;
}
