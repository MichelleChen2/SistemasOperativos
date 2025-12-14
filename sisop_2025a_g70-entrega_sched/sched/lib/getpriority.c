
#include <inc/lib.h>

int 
getpriority(int envid) 
{
    if (envid < 0 ) {
        return -E_INVAL; 
    }

    int r = sys_getpriority((envid_t) envid); 

    if (r == -E_BAD_ENV) {
        return -E_BAD_ENV; 
    }

    return r; 
}