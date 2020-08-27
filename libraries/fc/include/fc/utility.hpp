#pragma once
#include <stdint.h>
#include <algorithm>
#include <functional>
#include <new>
#include <vector>
#include <type_traits>
#include <tuple>
#include <memory>

#ifdef _MSC_VER
#pragma warning(disable: 4482) // nonstandard extension used enum Name::Val, standard in C++11
#define NO_RETURN __declspec(noreturn)
#else
#define NO_RETURN __attribute__((noreturn))
#endif

#define MAX_NUM_ARRAY_ELEMENTS (1024*1024)
#define MAX_SIZE_OF_BYTE_ARRAYS (20*1024*1024)

//namespace std {
//  typedef decltype(sizeof(int)) size_t;
//  typedef decltype(nullptr) nullptr_t;
//}

namespace fc {
  using std::size_t;
  typedef decltype(nullptr) nullptr_t;

  template<typename T> struct remove_reference           { typedef T type;       };
  template<typename T> struct remove_reference<T&>       { typedef T type;       };
  template<typename T> struct remove_reference<T&&>      { typedef T type;       };

  template<typename T> struct deduce           { typedef T type; };
  template<typename T> struct deduce<T&>       { typedef T type; };
  template<typename T> struct deduce<const T&> { typedef T type; };
  template<typename T> struct deduce<T&&>      { typedef T type; };
  template<typename T> struct deduce<const T&&>{ typedef T type; };


  using std::move;
  using std::forward;

  struct true_type  { enum _value { value = 1 }; };
  struct false_type { enum _value { value = 0 }; };

   namespace detail {
      template<typename T> fc::true_type is_class_helper(void(T::*)());
      template<typename T> fc::false_type is_class_helper(...);

      template<typename T, typename A, typename... Args>
      struct supports_allocator {
      public:
         static constexpr bool value = std::uses_allocator<T, A>::value;
         static constexpr bool leading_allocator = std::is_constructible< T, std::allocator_arg_t, A, Args... >::value;
         static constexpr bool trailing_allocator = std::is_constructible< T, Args..., A>::value;
      };

      template<typename T, typename A, typename... Args>
      auto construct_maybe_with_allocator( A&& allocator, Args&&... args )
      -> std::enable_if_t<supports_allocator<T, A>::value && supports_allocator<T, A>::leading_allocator, T>
      {
         return T( std::allocator_arg, std::forward<A>(allocator), std::forward<Args>(args)... );
      }

      template<typename T, typename A, typename... Args>
      auto construct_maybe_with_allocator( A&& allocator, Args&&... args )
      -> std::enable_if_t<supports_allocator<T, A>::value && !supports_allocator<T, A>::leading_allocator, T>
      {
         static_assert( supports_allocator<T, A>::trailing_allocator, "type supposedly supports allocators but cannot be constructed by either the leading- or trailing-allocator convention" );
         return T( std::forward<Args>(args)..., std::forward<A>(allocator) );
      }

      template<typename T, typename A, typename... Args>
      auto construct_maybe_with_allocator( A&&, Args&&... args )
      -> std::enable_if_t<!supports_allocator<T, A>::value , T>
      {
         return T( std::forward<Args>(args)... );
      }

      template<typename T, typename A, typename... Args>
      auto maybe_augment_constructor_arguments_with_allocator(
               A&& allocator, std::tuple<Args&&...> args,
               std::enable_if_t<supports_allocator<T, A>::value && supports_allocator<T, A>::leading_allocator, int> = 0
      ) {
         return std::tuple_cat( std::forward_as_tuple<std::allocator_arg_t, A>( std::allocator_arg, allocator ), args );
      }

      template<typename T, typename A, typename... Args>
      auto maybe_augment_constructor_arguments_with_allocator(
               A&& allocator, std::tuple<>,
               std::enable_if_t<supports_allocator<T, A>::value && supports_allocator<T, A>::leading_allocator, int> = 0
      ) {
         return std::forward_as_tuple<std::allocator_arg_t, A>( std::allocator_arg, allocator );
      }

      template<typename T, typename A, typename... Args>
      auto maybe_augment_constructor_arguments_with_allocator(
               A&& allocator, std::tuple<Args&&...> args,
               std::enable_if_t<supports_allocator<T, A>::value && !supports_allocator<T, A>::leading_allocator, int> = 0
      ) {
         static_assert( supports_allocator<T, A>::trailing_allocator, "type supposedly supports allocators but cannot be constructed by either the leading- or trailing-allocator convention" );
         return std::tuple_cat( args, std::forward_as_tuple<A>( allocator ) );
      }

      template<typename T, typename A, typename... Args>
      auto maybe_augment_constructor_arguments_with_allocator(
               A&& allocator, std::tuple<>,
               std::enable_if_t<supports_allocator<T, A>::value && !supports_allocator<T, A>::leading_allocator, int> = 0
      ) {
         static_assert( supports_allocator<T, A>::trailing_allocator, "type supposedly supports allocators but cannot be constructed by either the leading- or trailing-allocator convention" );
         return std::forward_as_tuple<A>( allocator );
      }

      template<typename T, typename A, typename... Args>
      auto maybe_augment_constructor_arguments_with_allocator(
               A&&, std::tuple<Args&&...> args,
               std::enable_if_t<!supports_allocator<T, A>::value, int> = 0
      ) {
         return args;
      }

      template<typename T1, typename T2, typename A>
      std::pair<T1,T2> default_construct_pair_maybe_with_allocator( A&& allocator )
      {
         return std::pair<T1,T2>(
            std::piecewise_construct,
            maybe_augment_constructor_arguments_with_allocator<T1>( allocator, std::make_tuple() ),
            maybe_augment_constructor_arguments_with_allocator<T2>( allocator, std::make_tuple() )
         );
      }
   }

  template<typename T>
  struct is_class { typedef decltype(detail::is_class_helper<T>(0)) type; enum value_enum { value = type::value }; };
#ifdef min
#undef min
#endif
  template<typename T>
  const T& min( const T& a, const T& b ) { return a < b ? a: b; }

  constexpr size_t const_strlen(const char* str) {
     int i = 0;
     while(*(str+i) != '\0')
        i++;
     return i;
  }


  template<typename Container>
  void move_append(Container& dest, Container&& src ) {
    if (src.empty()) {
      return;
    } else if (dest.empty()) {
      dest = std::move(src);
    } else {
      dest.insert(std::end(dest), std::make_move_iterator(std::begin(src)), std::make_move_iterator(std::end(src)));
    }
  }

  template<typename Container>
  void copy_append(Container& dest, const Container& src ) {
    if (src.empty()) {
      return;
    } else {
      dest.insert(std::end(dest), std::begin(src), std::end(src));
    }
  }

  template<typename Container>
  void deduplicate( Container& entries ) {
    if (entries.size() > 1) {
      std::sort( entries.begin(), entries.end() );
      auto itr = std::unique( entries.begin(), entries.end() );
      entries.erase( itr, entries.end() );
    }
  }

  /**
   * std::function type that verifies std::function is set before invocation
   */
  template<typename Signature>
  class optional_delegate;

  template<typename R, typename ...Args>
  class optional_delegate<R(Args...)> : private std::function<R(Args...)> {
  public:
     using std::function<R(Args...)>::function;

     auto operator()( Args... args ) const -> R {
        if (static_cast<bool>(*this)) {
           if constexpr( std::is_move_constructible_v<R> ) {
              return std::function<R(Args...)>::operator()(std::move(args)...);
           } else {
              return std::function<R(Args...)>::operator()(args...);
           }
        } else {
           if constexpr( !std::is_void_v<R> ) {
              return {};
           }
        }
     }
  };

  using yield_function_t = optional_delegate<void()>;

}

  // outside of namespace fc becuase of VC++ conflict with std::swap
  template<typename T>
  void fc_swap( T& a, T& b ) {
    T tmp = fc::move(a);
    a = fc::move(b);
    b = fc::move(tmp);
  }

#define LLCONST(constant)   static_cast<int64_t>(constant##ll)
#define ULLCONST(constant)  static_cast<uint64_t>(constant##ull)
