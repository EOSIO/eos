static inline struct pthread *__pthread_self()
{
   static struct pthread self = { 0 };
   return &self;
}
