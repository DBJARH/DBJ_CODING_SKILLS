// C23, gcc -std=c23 jobserve_client.c -lcurl -o jobserve_client
// Note: `defer` is NOT in standard C23 (rejected, targeted for C2y).
// GCC 15+ supports it experimentally under -fdefer-ts. This uses
// [[gnu::cleanup]] instead — standard-adjacent, works today on GCC 13+.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

    static struct {
        unsigned char data[0xFF];
    } ff = {};


    static struct {
        unsigned char data[0xFFF];
    } fff = {};

#define ss_cap(ss_ ) ((size_t)sizeof(ss_.data))
#define ss_len(ss_ ) ((size_t)strnlen(ss_,data, ss_cap(ss_)))

#define ss_set(ss_, literal ) ( strncpy(ss_.data, literal, ss_cap(ss_)) , sizeof(literal) )

#define ss_print(ss_) printf("\n%8s : %s", #ss_ , ss_.data)

#define ss_cpy( dest_ , src_) ( strncpy(dest_.data, src_.data, ss_cap(dest_)) , ss_cap(src_) )

int main(int argc, char** argv) {

    ss_set( ff, "ABC") ;
    ss_print(ff);

    ss_cpy(fff,ff);
    ss_print(fff);

    auto ff2 = ff ;

    ss_print(ff2);

    return 42;
}