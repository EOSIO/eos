#include <string>

#define CONN_NUM     1024
#define MSG_NUM   1024
#define MSG_LEN    1024

uint64_t my_clock()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);        
    uint64_t us = tv.tv_sec *1000000 ;
    us +=  tv.tv_usec;
    return us;
}

std::string get_req_string(int len)
{
   char * preq = new char[len+1];
   if(!preq) return {};
   for(int i = 0; i < len; ++i){
      preq[i] = ';' + (i&0x3f);
   }
   preq[len] = '\0';
   std::string ret = std::string(preq, len);
   delete [] preq;
   return ret;
}

std::string get_random_req_string(int len)
{
   char * preq = new char[len+1];
   if(!preq) return {};
   for(int i = 0; i < len; ++i){
      preq[i] = ';' + (rand()&0x3f);
   }
   preq[len] = '\0';
   std::string ret = std::string(preq, len);
   delete [] preq;
   return ret;
}
