#pragma once

#include <boost/container/container_fwd.hpp>
#include <boost/interprocess/offset_ptr.hpp>
#include <boost/interprocess/allocators/allocator.hpp>

#include <cstddef>
#include <cstring>
#include <algorithm>
#include <string>

#include <chainbase/pinnable_mapped_file.hpp>

namespace chainbase {

   namespace bip = boost::interprocess;

   class shared_cow_string {
      struct impl {
         uint32_t reference_count;
         uint32_t size;
         char data[0];
      };
    public:
      using allocator_type = bip::allocator<char, pinnable_mapped_file::segment_manager>;
      using iterator = const char*;
      using const_iterator = const char*;
      explicit shared_cow_string(const allocator_type& alloc) : _data(nullptr), _alloc(alloc) {}
      template<typename Iter>
      explicit shared_cow_string(Iter begin, Iter end, const allocator_type& alloc) : shared_cow_string(alloc) {
         std::size_t size = std::distance(begin, end);
         impl* new_data = (impl*)&*_alloc.allocate(sizeof(impl) + size + 1);
         new_data->reference_count = 1;
         new_data->size = size;
         std::copy(begin, end, new_data->data);
         new_data->data[size] = '\0';
         _data = new_data;
      }
      explicit shared_cow_string(const char* ptr, std::size_t size, const allocator_type& alloc) : shared_cow_string(alloc) {
         impl* new_data = (impl*)&*_alloc.allocate(sizeof(impl) + size + 1);
         new_data->reference_count = 1;
         new_data->size = size;
         std::memcpy(new_data->data, ptr, size);
         new_data->data[size] = '\0';
         _data = new_data;
      }
      explicit shared_cow_string(std::size_t size, boost::container::default_init_t, const allocator_type& alloc) : shared_cow_string(alloc) {
         impl* new_data = (impl*)&*_alloc.allocate(sizeof(impl) + size + 1);
         new_data->reference_count = 1;
         new_data->size = size;
         new_data->data[size] = '\0';
         _data = new_data;
      }
      shared_cow_string(const shared_cow_string& other) : _data(other._data), _alloc(other._alloc) {
         if(_data != nullptr) {
            ++_data->reference_count;
         }
      }
      shared_cow_string(shared_cow_string&& other) : _data(other._data), _alloc(other._alloc) {
         other._data = nullptr;
      }
      shared_cow_string& operator=(const shared_cow_string& other) {
         *this = shared_cow_string{other};
         return *this;
      }
      shared_cow_string& operator=(shared_cow_string&& other) {
         if (this != &other) {
            dec_refcount();
            _data = other._data;
            other._data = nullptr;
         }
         return *this;
      }
      ~shared_cow_string() {
         dec_refcount();
      }
      void resize(std::size_t new_size, boost::container::default_init_t) {
         impl* new_data = (impl*)&*_alloc.allocate(sizeof(impl) + new_size + 1);
         new_data->reference_count = 1;
         new_data->size = new_size;
         new_data->data[new_size] = '\0';
         dec_refcount();
         _data = new_data;
      }
      template<typename F>
      void resize_and_fill(std::size_t new_size, F&& f) {
         resize(new_size, boost::container::default_init);
         static_cast<F&&>(f)(_data->data, new_size);
      }
      void assign(const char* ptr, std::size_t size) {
         impl* new_data = (impl*)&*_alloc.allocate(sizeof(impl) + size + 1);
         new_data->reference_count = 1;
         new_data->size = size;
         if(size)
            std::memcpy(new_data->data, ptr, size);
         new_data->data[size] = '\0';
         dec_refcount();
         _data = new_data;
      }
      void assign(const unsigned char* ptr, std::size_t size) {
         assign((char*)ptr, size);
      }
      const char * data() const {
         if (_data) return _data->data;
         else return nullptr;
      }
      std::size_t size() const {
         if (_data) return _data->size;
         else return 0;
      }
      const_iterator begin() const { return data(); }
      const_iterator end() const {
         if (_data) return _data->data + _data->size;
         else return nullptr;
      }
      int compare(std::size_t start, std::size_t count, const char* other, std::size_t other_size) const {
         std::size_t sz = size();
         if(start > sz) BOOST_THROW_EXCEPTION(std::out_of_range{"shared_cow_string::compare"});
         count = std::min(count, sz - start);
         std::size_t cmp_len = std::min(count, other_size);
         const char* start_ptr = data() + start;
         int result = std::char_traits<char>::compare(start_ptr, other, cmp_len);
         if (result != 0) return result;
         else if (count < other_size) return -1;
         else if(count > other_size) return 1;
         else return 0;
      }
      bool operator==(const shared_cow_string& rhs) const {
        return size() == rhs.size() && std::memcmp(data(), rhs.data(), size()) == 0;
      }
      bool operator!=(const shared_cow_string& rhs) const { return !(*this == rhs); }
      const allocator_type& get_allocator() const { return _alloc; }
    private:
      void dec_refcount() {
         if(_data && --_data->reference_count == 0) {
            _alloc.deallocate((char*)&*_data, sizeof(shared_cow_string) + _data->size + 1);
         }
      }
      bip::offset_ptr<impl> _data;
      allocator_type _alloc;
   };

}  // namepsace chainbase
