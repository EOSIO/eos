#pragma once
#include <eoslib/system.h>
#include <eoslib/memory.h>

namespace eos {

template<typename T>
class datastream {
   public:
      datastream( T start, size_t s )
      :_start(start),_pos(start),_end(start+s){};
      
      
      inline void skip( size_t s ){ _pos += s; }
      inline bool read( char* d, size_t s ) {
        assert( size_t(_end - _pos) >= (size_t)s, "read" );
        memcpy( d, _pos, s );
        _pos += s;
        return true;
      }
      
      inline bool write( const char* d, size_t s ) {
        assert( _end - _pos >= (int32_t)s, "write" );
        memcpy( _pos, d, s );
        _pos += s;
        return true;
      }
      
      inline bool put(char c) { 
        assert( _pos < _end, "put" );
        *_pos = c; 
        ++_pos; 
        return true;
      }
      
      inline bool get( unsigned char& c ) { return get( *(char*)&c ); }
      inline bool get( char& c ) 
      {
        assert( _pos < _end, "get" );
        c = *_pos;
        ++_pos; 
        return true;
      }
      
      T               pos()const        { return _pos;                               }
      inline bool     valid()const      { return _pos <= _end && _pos >= _start;  }
      inline bool     seekp(size_t p) { _pos = _start + p; return _pos <= _end; }
      inline size_t   tellp()const      { return _pos - _start;                     }
      inline size_t   remaining()const  { return _end - _pos;                       }
    private:
      T _start;
      T _pos;
      T _end;
};

template<>
class datastream<size_t> {
   public:
     datastream( size_t init_size = 0):_size(init_size){};
     inline bool     skip( size_t s )                 { _size += s; return true;  }
     inline bool     write( const char* ,size_t s )  { _size += s; return true;  }
     inline bool     put(char )                      { ++_size; return  true;    }
     inline bool     valid()const                     { return true;              }
     inline bool     seekp(size_t p)                  { _size = p;  return true;  }
     inline size_t   tellp()const                     { return _size;             }
     inline size_t   remaining()const                 { return 0;                 }
  private:
     size_t _size;
};


}