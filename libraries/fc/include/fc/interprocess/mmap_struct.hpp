#pragma once
#include <fc/interprocess/file_mapping.hpp>
#include <memory>

namespace fc
{
   class path;
   namespace detail
   {
       /**
        *  Base class used to hide common implementation details.
        */
       class mmap_struct_base
       {
          public:
            size_t size()const;
            void flush();

          protected:
            void open( const fc::path& file, size_t s, bool create );
            std::unique_ptr<fc::file_mapping>  _file_mapping;
            std::unique_ptr<fc::mapped_region> _mapped_region;
       };
   };

   /**
    *  @class mmap_struct
    *  @brief A struct that has been mapped from a file.
    *
    *  @note T must be POD 
    */
   template<typename T>
   class mmap_struct : public detail::mmap_struct_base
   {
      public:
        mmap_struct():_mapped_struct(nullptr){}
        /**
         *  Create the file if it does not exist or is of the wrong size if create is true, then maps
         *  the file to memory.
         *
         *  @throw an exception if the file does not exist or is the wrong size and create is false
         */
        void open( const fc::path& file, bool create = false )
        {
            detail::mmap_struct_base::open( file, sizeof(T), create ); 
            _mapped_struct = (T*)_mapped_region->get_address();
        }

        T*       operator->()      { return _mapped_struct; }
        const T* operator->()const { return _mapped_struct; }

        T&       operator*()       { return *_mapped_struct; }
        const T& operator*()const  { return *_mapped_struct; }

      private:
        T*                                 _mapped_struct;
   };

}
