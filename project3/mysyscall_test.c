#include <linux/unistd.h>
#include <stdio.h>

#define __NR_mycall 322

int main(int argc, const char *argv[])
{
    int n;
    int a, b;
    printf("enter two intgers: ");
    scanf("%d %d", &a, &b);
    n = syscall(__NR_mycall, a, b);
    printf("myscall return value: %d\n", n);
    return 0;
}
