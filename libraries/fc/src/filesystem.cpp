//#define BOOST_NO_SCOPED_ENUMS
#include <fc/filesystem.hpp>
#include <fc/exception/exception.hpp>
#include <fc/fwd_impl.hpp>
#include <fc/utility.hpp>

#include <fc/utf8.hpp>
#include <fc/variant.hpp>

#include <boost/config.hpp>
#include <boost/filesystem.hpp>

#ifdef _WIN32
# include <windows.h>
# include <userenv.h>
# include <shlobj.h>
#else
  #include <sys/types.h>
  #include <sys/stat.h>
  #include <pwd.h>
# ifdef FC_HAS_SIMPLE_FILE_LOCK
  #include <sys/file.h>
  #include <fcntl.h>
# endif
#endif

namespace fc {
  // when converting to and from a variant, store utf-8 in the variant
  void to_variant( const fc::path& path_to_convert, variant& variant_output )
  {
    std::wstring wide_string = path_to_convert.generic_wstring();
    std::string utf8_string;
    fc::encodeUtf8(wide_string, &utf8_string);
    variant_output = utf8_string;

    //std::string path = t.to_native_ansi_path();
    //std::replace(path.begin(), path.end(), '\\', '/');
    //v = path;
  }

  void from_variant( const fc::variant& variant_to_convert, fc::path& path_output )
  {
    std::wstring wide_string;
    fc::decodeUtf8(variant_to_convert.as_string(), &wide_string);
    path_output = path(wide_string);
  }

   // Note: we can do this cast because the separator should be an ASCII character
   char path::separator_char = static_cast<char>(boost::filesystem::path("/").make_preferred().native()[0]);

   path::path(){}
   path::~path(){};
   path::path( const boost::filesystem::path& p )
   :_p(p){}

   path::path( const char* p )
   :_p(p){}
   path::path( const fc::string& p )
   :_p(p.c_str()){}

   path::path(const std::wstring& p)
   :_p(p) {}

   path::path( const path& p )
   :_p(p){}

   path::path( path&& p )
   :_p(std::move(p)){}

   path& path::operator =( const path& p ) {
    *_p = *p._p;
    return *this;
   }
   path& path::operator =( path&& p ) {
    *_p = fc::move( *p._p );
    return *this;
   }

   bool operator <( const fc::path& l, const fc::path& r ) { return *l._p < *r._p; }
   bool operator ==( const fc::path& l, const fc::path& r ) { return *l._p == *r._p; }
   bool operator !=( const fc::path& l, const fc::path& r ) { return *l._p != *r._p; }

   path& path::operator /=( const fc::path& p ) {
      *_p /= *p._p;
      return *this;
   }
   path   operator /( const fc::path& p, const fc::path& o ) {
      path tmp;
      tmp = *p._p / *o._p;
      return tmp;
   }

   path::operator boost::filesystem::path& () {
    return *_p;
   }
   path::operator const boost::filesystem::path& ()const {
    return *_p;
   }
   fc::string path::generic_string()const {
    return _p->generic_string();
   }

   fc::string path::preferred_string() const
   {
     return boost::filesystem::path(*_p).make_preferred().string();
   }

  std::wstring path::wstring() const
    {
    return _p->wstring();
    }

  std::wstring path::generic_wstring() const
    {
    return _p->generic_wstring();
    }

  std::wstring path::preferred_wstring() const
  {
    return boost::filesystem::path(*_p).make_preferred().wstring();
  }

  std::string path::to_native_ansi_path() const
    {
    std::wstring path = generic_wstring();

#ifdef WIN32
    const size_t maxPath = 32*1024;
    std::vector<wchar_t> short_path;
    short_path.resize(maxPath + 1);

    wchar_t* buffer = short_path.data();
    DWORD res = GetShortPathNameW(path.c_str(), buffer, maxPath);
    if(res != 0)
      path = buffer;
#endif
    std::string filePath;
    fc::encodeUtf8(path, &filePath);
    return filePath;
    }

   /**
    *  @todo use iterators instead of indexes for
    *  faster performance
    */
   fc::string path::windows_string()const {
     std::string result = _p->generic_string();
     std::replace(result.begin(), result.end(), '/', '\\');
     return result;
   }

   fc::string path::string()const {
    return _p->string();
   }
   fc::path path::filename()const {
    return _p->filename();
   }
   void     path::replace_extension( const fc::path& e ) {
        _p->replace_extension(e);
   }
   fc::path path::extension()const {
    return _p->extension();
   }
   fc::path path::stem()const {
    return _p->stem();
   }
   fc::path path::parent_path()const {
    return _p->parent_path();
   }
  bool path::is_relative()const { return _p->is_relative(); }
  bool path::is_absolute()const { return _p->is_absolute(); }

  bool path::empty() const { return _p->empty(); }

      directory_iterator::directory_iterator( const fc::path& p )
      :_p(p){}

      directory_iterator::directory_iterator(){}
      directory_iterator::~directory_iterator(){}

      fc::path            directory_iterator::operator*()const { return boost::filesystem::path(*(*_p)); }
      detail::path_wrapper directory_iterator::operator->() const { return detail::path_wrapper(boost::filesystem::path(*(*_p))); }
      directory_iterator& directory_iterator::operator++(int)  { (*_p)++; return *this; }
      directory_iterator& directory_iterator::operator++()     { (*_p)++; return *this; }

      bool operator==( const directory_iterator& r, const directory_iterator& l) {
        return *r._p == *l._p;
      }
      bool operator!=( const directory_iterator& r, const directory_iterator& l) {
        return *r._p != *l._p;
      }


      recursive_directory_iterator::recursive_directory_iterator( const fc::path& p )
      :_p(p){}

      recursive_directory_iterator::recursive_directory_iterator(){}
      recursive_directory_iterator::~recursive_directory_iterator(){}

      fc::path            recursive_directory_iterator::operator*()const { return boost::filesystem::path(*(*_p)); }
      recursive_directory_iterator& recursive_directory_iterator::operator++(int)  { (*_p)++; return *this; }
      recursive_directory_iterator& recursive_directory_iterator::operator++()     { (*_p)++; return *this; }

      void recursive_directory_iterator::pop() { (*_p).pop(); }
      int recursive_directory_iterator::level() { return _p->level(); }

      bool operator==( const recursive_directory_iterator& r, const recursive_directory_iterator& l) {
        return *r._p == *l._p;
      }
      bool operator!=( const recursive_directory_iterator& r, const recursive_directory_iterator& l) {
        return *r._p != *l._p;
      }


  bool exists( const path& p ) { return boost::filesystem::exists(p); }
  void create_directories( const path& p ) {
    try {
      boost::filesystem::create_directories(p);
    } catch ( ... ) {
      FC_THROW( "Unable to create directories ${path}", ("path", p )("inner", fc::except_str() ) );
    }
  }
  bool is_directory( const path& p ) { return boost::filesystem::is_directory(p); }
  bool is_regular_file( const path& p ) { return boost::filesystem::is_regular_file(p); }
  uint64_t file_size( const path& p ) { return boost::filesystem::file_size(p); }

  uint64_t directory_size(const path& p)
  {
    try {
      FC_ASSERT( is_directory( p ) );

      recursive_directory_iterator end;
      uint64_t size = 0;
      for( recursive_directory_iterator itr( p ); itr != end; ++itr )
      {
        if( is_regular_file( *itr ) )
          size += file_size( *itr );
      }

      return size;
    } catch ( ... ) {
      FC_THROW( "Unable to calculate size of directory ${path}", ("path", p )("inner", fc::except_str() ) );
    }
  }

  void remove_all( const path& p ) { boost::filesystem::remove_all(p); }
  void copy( const path& f, const path& t ) {
     boost::system::error_code ec;
     try {
  	      boost::filesystem::copy( boost::filesystem::path(f), boost::filesystem::path(t), ec );
     } catch ( boost::system::system_error& e ) {
     	FC_THROW( "Copy from ${srcfile} to ${dstfile} failed because ${reason}",
	         ("srcfile",f)("dstfile",t)("reason",e.what() ) );
     } catch ( ... ) {
     	FC_THROW( "Copy from ${srcfile} to ${dstfile} failed",
	         ("srcfile",f)("dstfile",t)("inner", fc::except_str() ) );
     }
     if( ec ) {
        FC_THROW( "Copy from ${srcfile} to ${dstfile} failed because ${reason}",
              ("srcfile",f)("dstfile",t)("reason", ec.category().name() ) );
     }
  }
  void resize_file( const path& f, size_t t )
  {
    try {
      boost::filesystem::resize_file( f, t );
    }
    catch ( boost::system::system_error& e )
    {
      FC_THROW( "Resize file '${f}' to size ${s} failed: ${reason}",
                ("f",f)("s",t)( "reason", e.what() ) );
    }
    catch ( ... )
    {
      FC_THROW( "Resize file '${f}' to size ${s} failed: ${reason}",
                ("f",f)("s",t)( "reason", fc::except_str() ) );
    }
  }

  // setuid, setgid not implemented.
  // translates octal permission like 0755 to S_ stuff defined in sys/stat.h
  // no-op on Windows.
  void chmod( const path& p, int perm )
  {
#ifndef WIN32
    mode_t actual_perm =
      ((perm & 0400) ? S_IRUSR : 0)
    | ((perm & 0200) ? S_IWUSR : 0)
    | ((perm & 0100) ? S_IXUSR : 0)

    | ((perm & 0040) ? S_IRGRP : 0)
    | ((perm & 0020) ? S_IWGRP : 0)
    | ((perm & 0010) ? S_IXGRP : 0)

    | ((perm & 0004) ? S_IROTH : 0)
    | ((perm & 0002) ? S_IWOTH : 0)
    | ((perm & 0001) ? S_IXOTH : 0)
    ;

    int result = ::chmod( p.string().c_str(), actual_perm );
    if( result != 0 )
        FC_THROW( "chmod operation failed on ${p}", ("p",p) );
#endif
    return;
  }

  void rename( const path& f, const path& t ) {
     try {
  	    boost::filesystem::rename( boost::filesystem::path(f), boost::filesystem::path(t) );
     } catch ( boost::system::system_error& ) {
         try{
             boost::filesystem::copy( boost::filesystem::path(f), boost::filesystem::path(t) );
             boost::filesystem::remove( boost::filesystem::path(f));
         } catch ( boost::system::system_error& e ) {
             FC_THROW( "Rename from ${srcfile} to ${dstfile} failed because ${reason}",
                     ("srcfile",f)("dstfile",t)("reason",e.what() ) );
         }
     } catch ( ... ) {
     	FC_THROW( "Rename from ${srcfile} to ${dstfile} failed",
	         ("srcfile",f)("dstfile",t)("inner", fc::except_str() ) );
     }
  }
  void create_hard_link( const path& f, const path& t ) {
     try {
        boost::filesystem::create_hard_link( f, t );
     } catch ( ... ) {
         FC_THROW( "Unable to create hard link from '${from}' to '${to}'",
                          ( "from", f )("to",t)("exception", fc::except_str() ) );
     }
  }
  bool remove( const path& f ) {
     try {
        return boost::filesystem::remove( f );
     } catch ( ... ) {
         FC_THROW( "Unable to remove '${path}'", ( "path", f )("exception", fc::except_str() ) );
     }
  }
  fc::path canonical( const fc::path& p ) {
     try {
        return boost::filesystem::canonical(p);
     } catch ( ... ) {
         FC_THROW( "Unable to resolve path '${path}'", ( "path", p )("exception", fc::except_str() ) );
     }
  }
  fc::path absolute( const fc::path& p ) { return boost::filesystem::absolute(p); }
  path     unique_path() { return boost::filesystem::unique_path(); }
  path     temp_directory_path() { return boost::filesystem::temp_directory_path(); }

  // Return path when appended to a_From will resolve to same as a_To
  fc::path make_relative(const fc::path& from, const fc::path& to) {
    boost::filesystem::path a_From = boost::filesystem::absolute(from);
    boost::filesystem::path a_To = boost::filesystem::absolute(to);
    boost::filesystem::path ret;
    boost::filesystem::path::const_iterator itrFrom(a_From.begin()), itrTo(a_To.begin());
    // Find common base
    for( boost::filesystem::path::const_iterator toEnd( a_To.end() ), fromEnd( a_From.end() ) ; itrFrom != fromEnd && itrTo != toEnd && *itrFrom == *itrTo; ++itrFrom, ++itrTo );
    // Navigate backwards in directory to reach previously found base
    for( boost::filesystem::path::const_iterator fromEnd( a_From.end() ); itrFrom != fromEnd; ++itrFrom ) {
      if( (*itrFrom) != "." )
         ret /= "..";
    }
    // Now navigate down the directory branch
    for (; itrTo != a_To.end(); ++itrTo)
      ret /= *itrTo;
    return ret;
  }

   temp_file::temp_file(const fc::path& p, bool create)
   : temp_file_base(p / fc::unique_path())
   {
      if (fc::exists(*_path))
      {
         FC_THROW( "Name collision: ${path}", ("path", _path->string()) );
      }
      if (create)
      {
         std::ofstream ofs(_path->generic_string().c_str(), std::ofstream::out | std::ofstream::binary);
         ofs.close();
      }
   }

   temp_file::temp_file(temp_file&& other)
      : temp_file_base(std::move(other._path))
   {
   }

   temp_file& temp_file::operator=(temp_file&& other)
   {
      if (this != &other)
      {
         remove();
         _path = std::move(other._path);
      }
      return *this;
   }

   temp_directory::temp_directory(const fc::path& p)
   : temp_file_base(p / fc::unique_path())
   {
      if (fc::exists(*_path))
      {
         FC_THROW( "Name collision: ${path}", ("path", _path->string()) );
      }
      fc::create_directories(*_path);
   }

   temp_directory::temp_directory(temp_directory&& other)
      : temp_file_base(std::move(other._path))
   {
   }

   temp_directory& temp_directory::operator=(temp_directory&& other)
   {
      if (this != &other)
      {
         remove();
         _path = std::move(other._path);
      }
      return *this;
   }

   const fc::path& temp_file_base::path() const
   {
      if (!_path)
      {
         FC_THROW( "Temporary directory has been released." );
      }
      return *_path;
   }

   void temp_file_base::remove()
   {
      if (_path.valid())
      {
         try
         {
            fc::remove_all(*_path);
         }
         catch (...)
         {
            // eat errors on cleanup
         }
         release();
      }
   }

   void temp_file_base::release()
   {
      _path = fc::optional<fc::path>();
   }

   const fc::path& home_path()
   {
      static fc::path p = []()
      {
#ifdef WIN32
          HANDLE access_token;
          if (!OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &access_token))
            FC_ASSERT(false, "Unable to open an access token for the current process");
          wchar_t user_profile_dir[MAX_PATH];
          DWORD user_profile_dir_len = sizeof(user_profile_dir);
          BOOL success = GetUserProfileDirectoryW(access_token, user_profile_dir, &user_profile_dir_len);
          CloseHandle(access_token);
          if (!success)
            FC_ASSERT(false, "Unable to get the user profile directory");
          return fc::path(std::wstring(user_profile_dir));
#else
          char* home = getenv( "HOME" );
          if( nullptr == home )
          {
             struct passwd* pwd = getpwuid(getuid());
             if( pwd )
             {
                 return fc::path( std::string( pwd->pw_dir ) );
             }
             FC_ASSERT( home != nullptr, "The HOME environment variable is not set" );
          }
          return fc::path( std::string(home) );
#endif
      }();
      return p;
   }

   const fc::path& app_path()
   {
#ifdef __APPLE__
         static fc::path appdir = [](){  return home_path() / "Library" / "Application Support"; }();
#elif defined( WIN32 )
         static fc::path appdir = [](){
           wchar_t app_data_dir[MAX_PATH];

           if (!SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE, NULL, 0, app_data_dir)))
             FC_ASSERT(false, "Unable to get the current AppData directory");
           return fc::path(std::wstring(app_data_dir));
         }();
#else
        static fc::path appdir = home_path() / ".local/share";
#endif
      return appdir;
   }

   const fc::path& current_path()
   {
     static fc::path appCurrentPath = boost::filesystem::current_path();
     return appCurrentPath;
   }


#ifdef FC_HAS_SIMPLE_FILE_LOCK
  class simple_lock_file::impl
  {
  public:
#ifdef _WIN32
    HANDLE file_handle;
#else
    int file_handle;
#endif
    bool is_locked;
    path lock_file_path;

    impl(const path& lock_file_path);
    ~impl();

    bool try_lock();
    void unlock();
  };

  simple_lock_file::impl::impl(const path& lock_file_path) :
#ifdef _WIN32
    file_handle(INVALID_HANDLE_VALUE),
#else
    file_handle(-1),
#endif
    is_locked(false),
    lock_file_path(lock_file_path)
  {}

  simple_lock_file::impl::~impl()
  {
    unlock();
  }

  bool simple_lock_file::impl::try_lock()
  {
#ifdef _WIN32
    HANDLE fh = CreateFileA(lock_file_path.to_native_ansi_path().c_str(),
                            GENERIC_READ | GENERIC_WRITE,
                            0, 0,
                            OPEN_ALWAYS, 0, NULL);
    if (fh == INVALID_HANDLE_VALUE)
      return false;
    is_locked = true;
    file_handle = fh;
    return true;
#else
    int fd = open(lock_file_path.string().c_str(), O_RDWR|O_CREAT, 0644);
    if (fd < 0)
      return false;
    if (flock(fd, LOCK_EX|LOCK_NB) == -1)
    {
      close(fd);
      return false;
    }
    is_locked = true;
    file_handle = fd;
    return true;
#endif
  }

  void simple_lock_file::impl::unlock()
  {
#ifdef WIN32
    CloseHandle(file_handle);
    file_handle = INVALID_HANDLE_VALUE;
    is_locked = false;
#else
    flock(file_handle, LOCK_UN);
    close(file_handle);
    file_handle = -1;
    is_locked = false;
#endif
  }


  simple_lock_file::simple_lock_file(const path& lock_file_path) :
    my(new impl(lock_file_path))
  {
  }

  simple_lock_file::~simple_lock_file()
  {
  }

  bool simple_lock_file::try_lock()
  {
    return my->try_lock();
  }

  void simple_lock_file::unlock()
  {
    my->unlock();
  }
#endif // FC_HAS_SIMPLE_FILE_LOCK

}
