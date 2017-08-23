#cdef extern from <stdio.h>:
#    void printf(char *)
    
def sayHello():
    print('hello,world')

cdef extern char* c_printf(char *s):
    sayHello()

