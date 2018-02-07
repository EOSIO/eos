__attribute__((__visibility__("hidden")))
int __shcall(void *arg, int (*func)(void *))
{
	return func(arg);
}
