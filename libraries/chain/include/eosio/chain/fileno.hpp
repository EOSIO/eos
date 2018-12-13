#pragma once

#include <cstdio>  // declaration of ::fileno
#include <fstream>  // for basic_filebuf template
#include <cerrno>

#if defined(__GLIBCXX__) || (defined(__GLIBCPP__) && __GLIBCPP__>=20020514)  // GCC >= 3.1.0
# include <ext/stdio_filebuf.h>
#endif
#if defined(__GLIBCXX__) // GCC >= 3.4.0
# include <ext/stdio_sync_filebuf.h>
#endif

#if defined(_LIBCPP_VERSION)
// Generate a static data member of type Tag::type in which to store
// the address of a private member. It is crucial that Tag does not
// depend on the /value/ of the the stored address in any way so that
// we can access it from ordinary code without directly touching
// private data.
template < class Tag >
struct stowed
{
  static typename Tag::type value;
};

template < class Tag >
typename Tag::type stowed< Tag >::value;

// Generate a static data member whose constructor initializes
// stowed< Tag >::value. This type will only be named in an explicit
// instantiation, where it is legal to pass the address of a private
// member.
template < class Tag, typename Tag::type x >
struct stow_private
{
  stow_private() { stowed< Tag >::value = x; }
  static stow_private instance;
};
template < class Tag, typename Tag::type x >
stow_private< Tag, x > stow_private< Tag, x >::instance;

struct filebuf_file { typedef FILE*( std::filebuf::*type ); };
template struct stow_private< filebuf_file, &std::filebuf::__file_ >;
#endif

//! Similar to fileno(3), but taking a C++ stream as argument instead of a
//! FILE*.  Note that there is no way for the library to track what you do with
//! the descriptor, so be careful.
//! \return  The integer file descriptor associated with the stream, or -1 if
//!   that stream is invalid.  In the latter case, for the sake of keeping the
//!   code as similar to fileno(3), errno is set to EBADF.
//! \see  The <A HREF="http://www.ginac.de/~kreckel/fileno/">upstream page at
//!   http://www.ginac.de/~kreckel/fileno/</A> of this code provides more
//!   detailed information.
template <typename charT, typename traits>
inline int
fileno_hack(const std::basic_ios<charT, traits>& stream)
{
    // Some C++ runtime libraries shipped with ancient GCC, Sun Pro,
    // Sun WS/Forte 5/6, Compaq C++ supported non-standard file descriptor
    // access basic_filebuf<>::fd().  Alas, starting from GCC 3.1, the GNU C++
    // runtime removes all non-standard std::filebuf methods and provides an
    // extension template class __gnu_cxx::stdio_filebuf on all systems where
    // that appears to make sense (i.e. at least all Unix systems).  Starting
    // from GCC 3.4, there is an __gnu_cxx::stdio_sync_filebuf, in addition.
    // Sorry, darling, I must get brutal to fetch the darn file descriptor!
    // Please complain to your compiler/libstdc++ vendor...
#if defined(__GLIBCXX__) || defined(__GLIBCPP__)
    // OK, stop reading here, because it's getting obscene.  Cross fingers!
# if defined(__GLIBCXX__)  // >= GCC 3.4.0
    // This applies to cin, cout and cerr when not synced with stdio:
    typedef __gnu_cxx::stdio_filebuf<charT, traits> unix_filebuf_t;
    unix_filebuf_t* fbuf = dynamic_cast<unix_filebuf_t*>(stream.rdbuf());
    if (fbuf != NULL) {
        return fbuf->fd();
    }

    // This applies to filestreams:
    typedef std::basic_filebuf<charT, traits> filebuf_t;
    filebuf_t* bbuf = dynamic_cast<filebuf_t*>(stream.rdbuf());
    if (bbuf != NULL) {
        // This subclass is only there for accessing the FILE*.  Ouuwww, sucks!
        struct my_filebuf : public std::basic_filebuf<charT, traits> {
            // Note: _M_file is of type __basic_file<char> which has a
            // FILE* as its first (but private) member variable.
            FILE* c_file() { return *(FILE**)(&this->_M_file); }
        };
        FILE* c_file = static_cast<my_filebuf*>(bbuf)->c_file();
        if (c_file != NULL) {  // Could be NULL for failed ifstreams.
            return ::fileno(c_file);
        }
    }

    // This applies to cin, cout and cerr when synced with stdio:
    typedef __gnu_cxx::stdio_sync_filebuf<charT, traits> sync_filebuf_t;
    sync_filebuf_t* sbuf = dynamic_cast<sync_filebuf_t*>(stream.rdbuf());
    if (sbuf != NULL) {
#  if (__GLIBCXX__<20040906) // GCC < 3.4.2
        // This subclass is only there for accessing the FILE*.
        // See GCC PR#14600 and PR#16411.
        struct my_filebuf : public sync_filebuf_t {
            my_filebuf();  // Dummy ctor keeps the compiler happy.
            // Note: stdio_sync_filebuf has a FILE* as its first (but private)
            // member variable.  However, it is derived from basic_streambuf<>
            // and the FILE* is the first non-inherited member variable.
            FILE* c_file() {
                return *(FILE**)((char*)this + sizeof(std::basic_streambuf<charT, traits>));
            }
        };
        return ::fileno(static_cast<my_filebuf*>(sbuf)->c_file());
#  else
        return ::fileno(sbuf->file());
#  endif
    }
# else  // GCC < 3.4.0 used __GLIBCPP__
#  if (__GLIBCPP__>=20020514)  // GCC >= 3.1.0
    // This applies to cin, cout and cerr:
    typedef __gnu_cxx::stdio_filebuf<charT, traits> unix_filebuf_t;
    unix_filebuf_t* buf = dynamic_cast<unix_filebuf_t*>(stream.rdbuf());
    if (buf != NULL) {
        return buf->fd();
    }

    // This applies to filestreams:
    typedef std::basic_filebuf<charT, traits> filebuf_t;
    filebuf_t* bbuf = dynamic_cast<filebuf_t*>(stream.rdbuf());
    if (bbuf != NULL) {
        // This subclass is only there for accessing the FILE*.  Ouuwww, sucks!
        struct my_filebuf : public std::basic_filebuf<charT, traits> {
            // Note: _M_file is of type __basic_file<char> which has a
            // FILE* as its first (but private) member variable.
            FILE* c_file() { return *(FILE**)(&this->_M_file); }
        };
        FILE* c_file = static_cast<my_filebuf*>(bbuf)->c_file();
        if (c_file != NULL) {  // Could be NULL for failed ifstreams.
            return ::fileno(c_file);
        }
    }
#  else  // GCC 3.0.x
    typedef std::basic_filebuf<charT, traits> filebuf_t;
    filebuf_t* fbuf = dynamic_cast<filebuf_t*>(stream.rdbuf());
    if (fbuf != NULL) {
        struct my_filebuf : public filebuf_t {
            // Note: basic_filebuf<charT, traits> has a __basic_file<charT>* as
            // its first (but private) member variable.  Since it is derived
            // from basic_streambuf<charT, traits> we can guess its offset.
            // __basic_file<charT> in turn has a FILE* as its first (but
            // private) member variable.  Get it by brute force.  Oh, geez!
            FILE* c_file() {
                std::__basic_file<charT>* ptr_M_file = *(std::__basic_file<charT>**)((char*)this + sizeof(std::basic_streambuf<charT, traits>));
#  if _GLIBCPP_BASIC_FILE_INHERITANCE
                // __basic_file<charT> inherits from __basic_file_base<charT>
                return *(FILE**)((char*)ptr_M_file + sizeof(std::__basic_file_base<charT>));
#  else
                // __basic_file<charT> is base class, but with vptr.
                return *(FILE**)((char*)ptr_M_file + sizeof(void*));
#  endif
            }
        };
        return ::fileno(static_cast<my_filebuf*>(fbuf)->c_file());
    }
#  endif
# endif
#elif defined(_LIBCPP_VERSION)
    return ::fileno(stream.rdbuf()->*stowed< filebuf_file >::value);
#else
#  error "Does anybody know how to fetch the bloody file descriptor?"
    return stream.rdbuf()->fd();  // Maybe a good start?
#endif
#if !defined(_LIBCPP_VERSION)
    errno = EBADF;
#endif
    return -1;
}

#if defined(_LIBCPP_VERSION)
//! 8-Bit character instantiation: fileno(ios).
template <>
int
fileno_hack<char>(const std::ios& stream)
{
    return fileno_hack(stream);
}

//! Wide character instantiation: fileno(wios).
template <>
int
fileno_hack<wchar_t>(const std::wios& stream)
{
    return fileno_hack(stream);
}
#endif
