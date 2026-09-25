#include "../fixtures/probe.h"
#include "../fixtures/baseline.h"
int main(void) {
    FILE *f = tmpfile();
    assert(f);
    probe(f);
    rewind(f);
    char data[32768];
    size_t n = fread(data, 1, sizeof(data) - 1, f);
    data[n] = 0;
    fclose(f);
    if (strcmp(data, baseline)) {
        fputs(data, stderr);
        return 1;
    }
    return 0;
}
