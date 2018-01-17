#pragma once

// forward declarations, to be filled in by the compiler, for placement new.
extern "C" {
   void* operator new(size_t, void*);
   void* operator new[](size_t, void*);
}

// compatibility for things declared by the c++ standard libraries that compilers replace
namespace std {

// forward declaration of initializer_list
template<class E> class initializer_list {
   public:
      using value_type = E;
      using reference = const E&;
      using const_reference = const E&;
      using size_type = size_t;
      using iterator = const E*;
      using const_iterator = const E*;
      constexpr initializer_list() noexcept;
      constexpr size_t size() const noexcept {
         return _len;
      }
      constexpr const E* begin() const noexcept {
         return _iter;
      }
      constexpr const E* end() const noexcept {
         return _iter + _len;
      }

   private:
      iterator  _iter;
      size_type _len;

      constexpr initializer_list(const_iterator array, size_type len)
      :_iter(array),_len(len)
      {}
};
// initializer list range access
template<class E> constexpr const E* begin(initializer_list<E> il) noexcept;
template<class E> constexpr const E* end(initializer_list<E> il) noexcept;

}