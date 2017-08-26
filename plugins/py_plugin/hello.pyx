#cdef extern from <stdio.h>:
#    void printf(char *)

cdef extern from "hello.h":
    int get_info_ (char *result,int len)
    
def sayHello():
    print('hello,world')

def get_info():
    cdef char info[1024]
    get_info_(info,1024)
    return info

cdef extern char* c_printf(char *s):
    sayHello()

