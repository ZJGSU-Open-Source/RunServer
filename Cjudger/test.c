#include <stdlib.h>
#include <stdio.h>

char oj_home[1024];
int main() {
        printf("%s", getenv("GOPATH"));
        sprintf(oj_home, "%s/src", getenv("GOPATH"));
        printf("%s", oj_home);
        return 0;
}
