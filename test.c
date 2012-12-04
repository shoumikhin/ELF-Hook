#include <stdio.h>
#include <dlfcn.h>

#include "elf_hook.h"

#define LIBTEST1_PATH "libtest1.so"  //position dependent code (for 32 bit only)
#define LIBTEST2_PATH "libtest2.so"  //position independent code

void libtest1();  //from libtest1.so
void libtest2();  //from libtest2.so

int hooked_puts(char const *s)
{
    puts(s);  //calls the original puts() from libc.so because our main executable module called "test" is intact by hook
    puts("is HOOKED!");
}

int main()
{
    void *handle1 = dlopen(LIBTEST1_PATH, RTLD_LAZY);
    void *handle2 = dlopen(LIBTEST2_PATH, RTLD_LAZY);
    void *base1 = NULL, *base2 = NULL;
    void *original1, *original2;

    if (NULL == handle1 || NULL == handle2)
        fprintf(stderr, "Failed to open \"%s\" or \"%s\"!\n", LIBTEST1_PATH, LIBTEST2_PATH);

    if(get_module_base_address(LIBTEST1_PATH, handle1, &base1) ||
       get_module_base_address(LIBTEST2_PATH, handle2, &base2)) 
        fprintf(stderr, "Failed to get module base addresses\n");

    libtest1();  //calls puts() from libc.so twice
    libtest2();  //calls puts() from libc.so twice
    puts("-----------------------------");

    original1 = elf_hook(LIBTEST1_PATH, base1, "puts", hooked_puts);
    original2 = elf_hook(LIBTEST2_PATH, base2, "puts", hooked_puts);

    if (NULL == original1 || NULL == original2)
        fprintf(stderr, "Redirection failed!\n");

    libtest1();  //calls hooked_puts() twice
    libtest2();  //calls hooked_puts() twice
    puts("-----------------------------");

    original1 = elf_hook(LIBTEST1_PATH, base1, "puts", original1);
    original2 = elf_hook(LIBTEST2_PATH, base2, "puts", original2);

    if (NULL == original1 || original1 != original2)  //both pointers should contain hooked_puts() address now
        fprintf(stderr, "Restoration failed!\n");

    libtest1();  //again calls puts() from libc.so twice
    libtest2();  //again calls puts() from libc.so twice

    dlclose(handle1);
    dlclose(handle2);

    return 0;
}
