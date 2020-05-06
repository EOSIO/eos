#pragma once

#include <boost/multi_index_container_fwd.hpp>
#include <boost/intrusive/set.hpp>
#include <boost/intrusive/avltree.hpp>
#include <boost/intrusive/slist.hpp>
#include <boost/container/deque.hpp>
#include <boost/throw_exception.hpp>
#include <boost/mpl/fold.hpp>
#include <boost/mp11/list.hpp>
#include <boost/mp11/algorithm.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/core/demangle.hpp>
#include <boost/interprocess/interprocess_fwd.hpp>
#include <cassert>
#include <memory>
#include <type_traits>
#include <sstream>

namespace chainbase {

   template<typename F>
   struct scope_exit {
    public:
      scope_exit(F&& f) : _f(f) {}
      scope_exit(const scope_exit&) = delete;
      scope_exit& operator=(const scope_exit&) = delete;
      ~scope_exit() { if(!_canceled) _f(); }
      void cancel() { _canceled = true; }
    private:
      F _f;
      bool _canceled = false;
   };

   // Adapts multi_index's idea of keys to intrusive
   template<typename KeyExtractor, typename T>
   struct get_key {
      using type = std::decay_t<decltype(KeyExtractor{}(std::declval<const T&>()))>;
      decltype(auto) operator()(const T& arg) const { return KeyExtractor{}(arg); }
   };

   template<typename T>
   struct value_holder {
      template<typename... A>
      value_holder(A&&... a) : _item(static_cast<A&&>(a)...) {}
      T _item;
   };

   template<class Tag>
   struct offset_node_base {
      offset_node_base() = default;
      offset_node_base(const offset_node_base&) {}
      constexpr offset_node_base& operator=(const offset_node_base&) { return *this; }
      std::ptrdiff_t _parent;
      std::ptrdiff_t _left;
      std::ptrdiff_t _right;
      int _color;
   };

   template<class Tag>
   struct offset_node_traits {
      using node = offset_node_base<Tag>;
      using node_ptr = node*;
      using const_node_ptr = const node*;
      using color = int;
      static node_ptr get_parent(const_node_ptr n) {
         if(n->_parent == 1) return nullptr;
         return (node_ptr)((char*)n + n->_parent);
      }
      static void set_parent(node_ptr n, node_ptr parent) {
         if(parent == nullptr) n->_parent = 1;
         else n->_parent = (char*)parent - (char*)n;
      }
      static node_ptr get_left(const_node_ptr n) {
         if(n->_left == 1) return nullptr;
         return (node_ptr)((char*)n + n->_left);
      }
      static void set_left(node_ptr n, node_ptr left) {
         if(left == nullptr) n->_left = 1;
         else n->_left = (char*)left - (char*)n;
      }
      static node_ptr get_right(const_node_ptr n) {
         if(n->_right == 1) return nullptr;
         return (node_ptr)((char*)n + n->_right);
      }
      static void set_right(node_ptr n, node_ptr right) {
         if(right == nullptr) n->_right = 1;
         else n->_right = (char*)right - (char*)n;
      }
      // red-black tree
      static color get_color(node_ptr n) {
         return n->_color;
      }
      static void set_color(node_ptr n, color c) {
         n->_color = c;
      }
      static color black() { return 0; }
      static color red() { return 1; }
      // avl tree
      using balance = int;
      static balance get_balance(node_ptr n) {
         return n->_color;
      }
      static void set_balance(node_ptr n, balance c) {
         n->_color = c;
      }
      static balance negative() { return -1; }
      static balance zero() { return 0; }
      static balance positive() { return 1; }

      // list
      static node_ptr get_next(const_node_ptr n) { return get_right(n); }
      static void set_next(node_ptr n, node_ptr next) { set_right(n, next); }
      static node_ptr get_previous(const_node_ptr n) { return get_left(n); }
      static void set_previous(node_ptr n, node_ptr previous) { set_left(n, previous); }
   };

   template<typename Node, typename Tag>
   struct offset_node_value_traits {
      using node_traits = offset_node_traits<Tag>;
      using node_ptr = typename node_traits::node_ptr;
      using const_node_ptr = typename node_traits::const_node_ptr;
      using value_type = typename Node::value_type;
      using pointer = value_type*;
      using const_pointer = const value_type*;

      static node_ptr to_node_ptr(value_type &value) {
         return node_ptr{static_cast<Node*>(boost::intrusive::get_parent_from_member(&value, &value_holder<value_type>::_item))};
      }
      static const_node_ptr to_node_ptr(const value_type &value) {
         return const_node_ptr{static_cast<const Node*>(boost::intrusive::get_parent_from_member(&value, &value_holder<value_type>::_item))};
      }
      static pointer to_value_ptr(node_ptr n) { return pointer{&static_cast<Node*>(&*n)->_item}; }
      static const_pointer to_value_ptr(const_node_ptr n) { return const_pointer{&static_cast<const Node*>(&*n)->_item}; }

      static constexpr boost::intrusive::link_mode_type link_mode = boost::intrusive::normal_link;
   };
   template<typename Allocator, typename T>
   using rebind_alloc_t = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;

   template<typename Index>
   struct index_tag_impl { using type = void; };
   template<template<typename...> class Index, typename Tag, typename... T>
   struct index_tag_impl<Index<boost::multi_index::tag<Tag>, T...>> { using type = Tag; };
   template<typename Index>
   using index_tag = typename index_tag_impl<Index>::type;

   template<typename Tag, typename... Indices>
   using find_tag = boost::mp11::mp_find<boost::mp11::mp_list<index_tag<Indices>...>, Tag>;

   template<typename K, typename Allocator>
   using hook = offset_node_base<K>;

   template<typename Node, typename OrderedIndex>
   using set_base = boost::intrusive::avltree<
      typename Node::value_type,
      boost::intrusive::value_traits<offset_node_value_traits<Node, OrderedIndex>>,
      boost::intrusive::key_of_value<get_key<typename OrderedIndex::key_from_value_type, typename Node::value_type>>,
      boost::intrusive::compare<typename OrderedIndex::compare_type>>;

   template<typename OrderedIndex>
   constexpr bool is_valid_index = false;
   template<typename... T>
   constexpr bool is_valid_index<boost::multi_index::ordered_unique<T...>> = true;

   template<typename Node, typename Tag>
   using list_base = boost::intrusive::slist<
      typename Node::value_type,
      boost::intrusive::value_traits<offset_node_value_traits<Node, Tag>>>;

   template<typename L, typename It, typename Pred, typename Disposer>
   void remove_if_after_and_dispose(L& l, It it, It end, Pred&& p, Disposer&& d) {
      for(;;) {
         It next = it;
         ++next;
         if(next == end) break;
         if(p(*next)) { l.erase_after_and_dispose(it, d); }
         else { it = next; }
      }
   }

   template<typename T, typename Allocator, typename... Indices>
   class undo_index;
  
   template<typename Node, typename OrderedIndex>
   struct set_impl : private set_base<Node, OrderedIndex> {
      using base_type = set_base<Node, OrderedIndex>;
      // Allow compatible keys to match multi_index
      template<typename K>
      auto find(K&& k) const {
         return base_type::find(static_cast<K&&>(k), this->key_comp());
      }
      template<typename K>
      auto lower_bound(K&& k) const {
         return base_type::lower_bound(static_cast<K&&>(k), this->key_comp());
      }
      template<typename K>
      auto upper_bound(K&& k) const {
         return base_type::upper_bound(static_cast<K&&>(k), this->key_comp());
      }
      template<typename K>
      auto equal_range(K&& k) const {
         return base_type::equal_range(static_cast<K&&>(k), this->key_comp());
      }
      using base_type::begin;
      using base_type::end;
      using base_type::rbegin;
      using base_type::rend;
      using base_type::size;
      using base_type::iterator_to;
      using base_type::empty;
      template<typename T, typename Allocator, typename... Indices>
      friend class undo_index;
   };

   template<typename T, typename S>
   class chainbase_node_allocator;

   // Allows nested object to use a different allocator from the container.
   template<template<typename> class A, typename T>
   auto& propagate_allocator(A<T>& a) { return a; }
   template<typename T, typename S>
   auto& propagate_allocator(boost::interprocess::allocator<T, S>& a) { return a; }
   template<typename T, typename S, std::size_t N>
   auto propagate_allocator(boost::interprocess::node_allocator<T, S, N>& a) { return boost::interprocess::allocator<T, S>{a.get_segment_manager()}; }
   template<typename T, typename S, std::size_t N>
   auto propagate_allocator(boost::interprocess::private_node_allocator<T, S, N>& a) { return boost::interprocess::allocator<T, S>{a.get_segment_manager()}; }
   template<typename T, typename S>
   auto propagate_allocator(chainbase::chainbase_node_allocator<T, S>& a) { return boost::interprocess::allocator<T, S>{a.get_segment_manager()}; }

   // Similar to boost::multi_index_container with an undo stack.
   // Indices should be instances of ordered_unique.
   template<typename T, typename Allocator, typename... Indices>
   class undo_index {
    public:
      using id_type = std::decay_t<decltype(std::declval<T>().id)>;
      using value_type = T;
      using allocator_type = Allocator;

      static_assert((... && is_valid_index<Indices>), "Only ordered_unique indices are supported");

      undo_index() = default;
      explicit undo_index(const Allocator& a) : _undo_stack{a}, _allocator{a}, _old_values_allocator{a} {}
      ~undo_index() {
         dispose_undo();
         clear_impl<1>();
         std::get<0>(_indices).clear_and_dispose([&](pointer p){ dispose_node(*p); });
      }

      void validate()const {
         if( sizeof(node) != _size_of_value_type || sizeof(*this) != _size_of_this )
            BOOST_THROW_EXCEPTION( std::runtime_error("content of memory does not match data expected by executable") );
      }
    
      struct node : hook<Indices, Allocator>..., value_holder<T> {
         using value_type = T;
         using allocator_type = Allocator;
         template<typename... A>
         explicit node(A&&... a) : value_holder<T>{static_cast<A&&>(a)...} {}
         const T& item() const { return *this; }
         uint64_t _mtime = 0; // _monotonic_revision when the node was last modified or created.
      };
      static constexpr int erased_flag = 2; // 0,1,and -1 are used by the tree

      using indices_type = std::tuple<set_impl<node, Indices>...>;

      using index0_set_type = std::tuple_element_t<0, indices_type>;
      using alloc_traits = typename std::allocator_traits<Allocator>::template rebind_traits<node>;

      static_assert(std::is_same_v<typename index0_set_type::key_type, id_type>, "first index must be id");

      using index0_type = boost::mp11::mp_first<boost::mp11::mp_list<Indices...>>;
      struct old_node : hook<index0_type, Allocator>, value_holder<T> {
         using value_type = T;
         using allocator_type = Allocator;
         template<typename... A>
         explicit old_node(const T& t) : value_holder<T>{t} {}
         uint64_t _mtime = 0; // Backup of the node's _mtime, to be restored on undo
         typename alloc_traits::pointer _current; // pointer to the actual node
      };

      using id_pointer = id_type*;
      using pointer = value_type*;
      using const_iterator = typename index0_set_type::const_iterator;

      // The undo stack is implemented as a deque of undo_states
      // that index into a pair of singly linked lists.
      //
      // The primary key (id) is managed by the undo_index.  The id's are assigned sequentially to
      // objects in the order of insertion.  User code MUST NOT modify the primary key.
      // A given id can only be reused if its insertion is undone.
      //
      // Each undo session remembers the state of the table at the point when it was created.
      //
      // Within the undo state at the top of the undo stack:
      // A primary key is new if it is at least old_next_id.
      //
      // A primary key is removed if it exists in the removed_values list before removed_values_end.
      // A node has a flag which indicates whether it has been removed.
      //
      // A primary key is modified if it exists in the old_values list before old_values_end
      //
      // A primary key exists at most once in either the main table or removed values.
      // Every primary key in old_values also exists in either the main table OR removed_values.
      // If a primary key exists in both removed_values AND old_values, undo will restore the value from old_values.
      // A primary key may appear in old_values any number of times.  If it appears more than once
      //   within a single undo session, undo will restore the oldest value.
      //
      // The following describes the minimal set of operations required to maintain the undo stack:
      // start session: remember next_id and the current heads of old_values and removed_values.
      // squash: drop the last undo state
      // create: nop
      // modify: push a copy of the object onto old_values
      // remove: move node to removed index and set removed flag
      //
      // Operations on a given key MUST always follow the sequence: CREATE MODIFY* REMOVE?
      // When undoing multiple operations on the same key, the final result is determined
      // by the oldest operation.  Therefore, the following optimizations can be applied:
      //  - A primary key which is new may be discarded from old_values and removed_values
      //  - If a primary key has multiple modifications, all but the oldest can be discarded.
      //  - If a primary key is both modified and removed, the modified value can replace
      //    the removed value, and can then be discarded.
      // These optimizations may be applied at any time, but are not required by the class
      // invariants.
      //
      // Notes regarding memory:
      // Nodes in the main table share the same layout as nodes in removed_values and may
      // be freely moved between the two.  This permits undo to restore removed nodes
      // without allocating memory.
      //
      struct undo_state {
         typename std::allocator_traits<Allocator>::pointer old_values_end;
         typename std::allocator_traits<Allocator>::pointer removed_values_end;
         id_type old_next_id = 0;
         uint64_t ctime = 0; // _monotonic_revision at the point the undo_state was created
      };

      // Exception safety: strong
      template<typename Constructor>
      const value_type& emplace( Constructor&& c ) {
         auto p = alloc_traits::allocate(_allocator, 1);
         auto guard0 = scope_exit{[&]{ alloc_traits::deallocate(_allocator, p, 1); }};
         auto new_id = _next_id;
         auto constructor = [&]( value_type& v ) {
            v.id = new_id;
            c( v );
         };
         alloc_traits::construct(_allocator, &*p, constructor, propagate_allocator(_allocator));
         auto guard1 = scope_exit{[&]{ alloc_traits::destroy(_allocator, &*p); }};
         if(!insert_impl<1>(p->_item))
            BOOST_THROW_EXCEPTION( std::logic_error{ "could not insert object, most likely a uniqueness constraint was violated" } );
         std::get<0>(_indices).push_back(p->_item); // cannot fail and we know that it will definitely insert at the end.
         on_create(p->_item);
         ++_next_id;
         guard1.cancel();
         guard0.cancel();
         return p->_item;
      }

      // Exception safety: basic.
      // If the modifier leaves the object in a state that conflicts
      // with another object, it will either be reverted or erased.
      template<typename Modifier>
      void modify( const value_type& obj, Modifier&& m) {
         value_type* backup = on_modify(obj);
         value_type& node_ref = const_cast<value_type&>(obj);
         bool success = false;
         {
            auto guard0 = scope_exit{[&]{
               if(!post_modify<true, 1>(node_ref)) { // The object id cannot be modified
                  if(backup) {
                     node_ref = std::move(*backup);
                     bool success = post_modify<true, 1>(node_ref);
                     (void)success;
                     assert(success);
                     assert(backup == &_old_values.front());
                     _old_values.pop_front_and_dispose([this](pointer p){ dispose_old(*p); });
                  } else {
                     remove(obj);
                  }
               } else {
                  success = true;
               }
            }};
            auto old_id = obj.id;
            m(node_ref);
            (void)old_id;
            assert(obj.id == old_id);
         }
         if(!success)
            BOOST_THROW_EXCEPTION( std::logic_error{ "could not modify object, most likely a uniqueness constraint was violated" } );
      }

      // Allows testing whether a value has been removed from the undo_index.
      //
      // The lifetime of an object removed through a removed_nodes_tracker
      // does not end before the removed_nodes_tracker is destroyed or invalidated.
      //
      // A removed_nodes_tracker is invalidated by the following members of undo_index:
      // start_undo_session, commit, squash, and undo.
      class removed_nodes_tracker {
       public:
         explicit removed_nodes_tracker(undo_index& idx) : _self(&idx) {}
         ~removed_nodes_tracker() {
            _removed_values.clear_and_dispose([this](value_type* obj) { _self->dispose_node(*obj); });
         }
         removed_nodes_tracker(const removed_nodes_tracker&) = delete;
         removed_nodes_tracker& operator=(const removed_nodes_tracker&) = delete;
         bool is_removed(const value_type& obj) const {
            return undo_index::get_removed_field(obj) == erased_flag;
         }
         // Must be used in place of undo_index::remove
         void remove(const value_type& obj) {
            _self->remove(obj, *this);
         }
       private:
         friend class undo_index;
         void save(value_type& obj) {
            undo_index::get_removed_field(obj) = erased_flag;
            _removed_values.push_front(obj);
         }
         undo_index* _self;
         list_base<node, index0_type> _removed_values;
      };
      auto track_removed() {
         return removed_nodes_tracker(*this);
      }

      void remove( const value_type& obj ) noexcept {
         auto& node_ref = const_cast<value_type&>(obj);
         erase_impl(node_ref);
         if(on_remove(node_ref)) {
            dispose_node(node_ref);
         }
      }

    private:

      void remove( const value_type& obj, removed_nodes_tracker& tracker ) noexcept {
         auto& node_ref = const_cast<value_type&>(obj);
         erase_impl(node_ref);
         if(on_remove(node_ref)) {
            tracker.save(node_ref);
         }
      }

    public:

      template<typename CompatibleKey>
      const value_type* find( CompatibleKey&& key) const {
         const auto& index = std::get<0>(_indices);
         auto iter = index.find(static_cast<CompatibleKey&&>(key));
         if (iter != index.end()) {
            return &*iter;
         } else {
            return nullptr;
         }
      }

      template<typename CompatibleKey>
      const value_type& get( CompatibleKey&& key )const {
         auto ptr = find( static_cast<CompatibleKey&&>(key) );
         if( !ptr ) {
            std::stringstream ss;
            ss << "key not found (" << boost::core::demangle( typeid( key ).name() ) << "): " << key;
            BOOST_THROW_EXCEPTION( std::out_of_range( ss.str().c_str() ) );
         }
         return *ptr;
      }

      void remove_object( int64_t id ) {
         const value_type* val = find( typename value_type::id_type(id) );
         if( !val ) BOOST_THROW_EXCEPTION( std::out_of_range( boost::lexical_cast<std::string>(id) ) );
         remove( *val );
      }

      class session {
       public:
         session(undo_index& idx, bool enabled)
          : _index(idx),
            _apply(enabled) {
            if(enabled) idx.add_session();
         }
         session(session&& other)
           : _index(other._index),
             _apply(other._apply)
         {
            other._apply = false;
         }
         session& operator=(session&& other) {
            if(this != &other) {
               undo();
               _apply = other._apply;
               other._apply = false;
            }
            return *this;
         }
         ~session() { if(_apply) _index.undo(); }
         void push() { _apply = false; }
         void squash() {
            if ( _apply ) _index.squash();
            _apply = false;
         }
         void undo() {
            if ( _apply ) _index.undo();
            _apply = false;
         }
       private:
         undo_index& _index;
         bool _apply = true;
      };

      int64_t revision() const { return _revision; }

      session start_undo_session( bool enabled ) {
         return session{*this, enabled};
      }

      void set_revision( uint64_t revision ) {
         if( _undo_stack.size() != 0 )
            BOOST_THROW_EXCEPTION( std::logic_error("cannot set revision while there is an existing undo stack") );

         if( revision > std::numeric_limits<int64_t>::max() )
            BOOST_THROW_EXCEPTION( std::logic_error("revision to set is too high") );

         if( revision < _revision )
            BOOST_THROW_EXCEPTION( std::logic_error("revision cannot decrease") );

         _revision = static_cast<int64_t>(revision);
      }

      std::pair<int64_t, int64_t> undo_stack_revision_range() const {
         return { _revision - _undo_stack.size(), _revision };
      }

      /**
       * Discards all undo history prior to revision
       */
      void commit( int64_t revision ) noexcept {
         revision = std::min(revision, _revision);
         if (revision == _revision) {
            dispose_undo();
            _undo_stack.clear();
         } else if( (_revision - revision) < _undo_stack.size() ) {
            auto iter = _undo_stack.begin() + (_undo_stack.size() - (_revision - revision));
            dispose(get_old_values_end(*iter), get_removed_values_end(*iter));
            _undo_stack.erase(_undo_stack.begin(), iter);
         }
      }

      const undo_index& indices() const { return *this; }
      template<typename Tag>
      const auto& get() const { return std::get<find_tag<Tag, Indices...>::value>(_indices); }

      template<int N>
      const auto& get() const { return std::get<N>(_indices); }

      std::size_t size() const {
         return std::get<0>(_indices).size();
      }

      bool empty() const {
         return std::get<0>(_indices).empty();
      }

      template<typename Tag, typename Iter>
      auto project(Iter iter) const {
         return project<find_tag<Tag, Indices...>::value>(iter);
      }

      template<int N, typename Iter>
      auto project(Iter iter) const {
         if(iter == get<boost::mp11::mp_find<boost::mp11::mp_list<typename set_impl<node, Indices>::const_iterator...>, Iter>::value>().end())
            return get<N>().end();
         return get<N>().iterator_to(*iter);
      }

      bool has_undo_session() const { return !_undo_stack.empty(); }

      struct delta {
         boost::iterator_range<typename index0_set_type::const_iterator> new_values;
         boost::iterator_range<typename list_base<old_node, index0_type>::const_iterator> old_values;
         boost::iterator_range<typename list_base<node, index0_type>::const_iterator> removed_values;
      };

      delta last_undo_session() const {
        if(_undo_stack.empty())
           return { { get<0>().end(), get<0>().end() },
                    { _old_values.end(), _old_values.end() },
                    { _removed_values.end(), _removed_values.end() } };
         // Warning: This is safe ONLY as long as nothing exposes the undo stack to client code.
         // Compressing the undo stack does not change the logical state of the undo_index.
         const_cast<undo_index*>(this)->compress_last_undo_session();
         return { { get<0>().lower_bound(_undo_stack.back().old_next_id), get<0>().end() },
                  { _old_values.begin(), get_old_values_end(_undo_stack.back()) },
                  { _removed_values.begin(), get_removed_values_end(_undo_stack.back()) } };
      }

      auto begin() const { return get<0>().begin(); }
      auto end() const { return get<0>().end(); }

      void undo_all() {
         while(!_undo_stack.empty()) {
            undo();
         }
      }

      // Resets the contents to the state at the top of the undo stack.
      void undo() noexcept {
         if (_undo_stack.empty()) return;
         undo_state& undo_info = _undo_stack.back();
         // erase all new_ids
         auto& by_id = std::get<0>(_indices);
         auto new_ids_iter = by_id.lower_bound(undo_info.old_next_id);
         by_id.erase_and_dispose(new_ids_iter, by_id.end(), [this](pointer p){
            erase_impl<1>(*p);
            dispose_node(*p);
         });
         // replace old_values
         _old_values.erase_after_and_dispose(_old_values.before_begin(), get_old_values_end(undo_info), [this, &undo_info](pointer p) {
            auto restored_mtime = to_old_node(*p)._mtime;
            // Skip restoring values that overwrite an earlier modify in the same session.
            // Duplicate modifies can only happen because of squash.
            if(restored_mtime < undo_info.ctime) {
               auto iter = &to_old_node(*p)._current->_item;
               *iter = std::move(*p);
               auto& node_mtime = to_node(*iter)._mtime;
               node_mtime = restored_mtime;
               if (get_removed_field(*iter) != erased_flag) {
                  // Non-unique items are transient and are guaranteed to be fixed
                  // by the time we finish processing old_values.
                  post_modify<false, 1>(*iter);
               } else {
                  // The item was removed.  It will be inserted when we process removed_values
               }
            }
            dispose_old(*p);
         });
         // insert all removed_values
         _removed_values.erase_after_and_dispose(_removed_values.before_begin(), get_removed_values_end(undo_info), [this, &undo_info](pointer p) {
            if (p->id < undo_info.old_next_id) {
               get_removed_field(*p) = 0; // Will be overwritten by tree algorithms, because we're reusing the color.
               insert_impl(*p);
            } else {
               dispose_node(*p);
            }
         });
         _next_id = undo_info.old_next_id;
         _undo_stack.pop_back();
         --_revision;
      }

      // Combines the top two states on the undo stack
      void squash() noexcept {
         squash_and_compress();
      }

      void squash_fast() noexcept {
         if (_undo_stack.empty()) {
            return;
         } else if (_undo_stack.size() == 1) {
            dispose_undo();
         }
         _undo_stack.pop_back();
         --_revision;
      }

      void squash_and_compress() noexcept {
         if(_undo_stack.size() >= 2) {
            compress_impl(_undo_stack[_undo_stack.size() - 2]);
         }
         squash_fast();
      }

      void compress_last_undo_session() noexcept {
         compress_impl(_undo_stack.back());
      }

    private:

      // Removes elements of the last undo session that would be redundant
      // if all the sessions after @c session were squashed.
      //
      // WARNING: This function leaves any undo sessions after @c session in
      // an indeterminate state.  The caller MUST use squash to restore the
      // undo stack to a sane state.
      void compress_impl(undo_state& session) noexcept {
         auto session_start = session.ctime;
         auto old_next_id = session.old_next_id;
         remove_if_after_and_dispose(_old_values, _old_values.before_begin(), get_old_values_end(_undo_stack.back()),
                                     [session_start](value_type& v){
                                        if(to_old_node(v)._mtime >= session_start) return true;
                                        auto& item = to_old_node(v)._current->_item;
                                        if (get_removed_field(item) == erased_flag) {
                                           item = std::move(v);
                                           to_node(item)._mtime = to_old_node(v)._mtime;
                                           return true;
                                        }
                                        return false;
                                     },
                                     [&](pointer p) { dispose_old(*p); });
         remove_if_after_and_dispose(_removed_values, _removed_values.before_begin(), get_removed_values_end(_undo_stack.back()),
                                     [old_next_id](value_type& v){
                                        return v.id >= old_next_id;
                                     },
                                     [this](pointer p) { dispose_node(*p); });
      }

      // starts a new undo session.
      // Exception safety: strong
      int64_t add_session() {
         _undo_stack.emplace_back();
         _undo_stack.back().old_values_end = _old_values.empty()?nullptr:&*_old_values.begin();
         _undo_stack.back().removed_values_end = _removed_values.empty()?nullptr:&*_removed_values.begin();
         _undo_stack.back().old_next_id = _next_id;
         _undo_stack.back().ctime = ++_monotonic_revision;
         return ++_revision;
      }

      template<int N = 0>
      bool insert_impl(value_type& p) {
         if constexpr (N < sizeof...(Indices)) {
            auto [iter, inserted] = std::get<N>(_indices).insert_unique(p);
            if(!inserted) return false;
            auto guard = scope_exit{[this,iter=iter]{ std::get<N>(_indices).erase(iter); }};
            if(insert_impl<N+1>(p)) {
               guard.cancel();
               return true;
            }
            return false;
         }
         return true;
      }

      // Moves a modified node into the correct location
      template<bool unique, int N = 0>
      bool post_modify(value_type& p) {
         if constexpr (N < sizeof...(Indices)) {
            auto& idx = std::get<N>(_indices);
            auto iter = idx.iterator_to(p);
            bool fixup = false;
            if (iter != idx.begin()) {
               auto copy = iter;
               --copy;
               if (!idx.value_comp()(*copy, p)) fixup = true;
            }
            ++iter;
            if (iter != idx.end()) {
               if(!idx.value_comp()(p, *iter)) fixup = true;
            }
            if(fixup) {
               auto iter2 = idx.iterator_to(p);
               idx.erase(iter2);
               if constexpr (unique) {
                  auto [new_pos, inserted] = idx.insert_unique(p);
                  if (!inserted) {
                     idx.insert_before(new_pos, p);
                     return false;
                  }
               } else {
                  idx.insert_equal(p);
               }
            }
            return post_modify<unique, N+1>(p);
         }
         return true;
      }

      template<int N = 0>
      void erase_impl(value_type& p) {
         if constexpr (N < sizeof...(Indices)) {
            auto& setN = std::get<N>(_indices);
            setN.erase(setN.iterator_to(p));
            erase_impl<N+1>(p);
         }
      }

      void on_create(const value_type& value) noexcept {
         if(!_undo_stack.empty()) {
            // Not in old_values, removed_values, or new_ids
            to_node(value)._mtime = _monotonic_revision;
         }
      }

      value_type* on_modify( const value_type& obj) {
         if (!_undo_stack.empty()) {
            auto& undo_info = _undo_stack.back();
            if ( to_node(obj)._mtime >= undo_info.ctime ) {
               // Nothing to do
            } else {
               // Not in removed_values
               auto p = old_alloc_traits::allocate(_old_values_allocator, 1);
               auto guard0 = scope_exit{[&]{ _old_values_allocator.deallocate(p, 1); }};
               old_alloc_traits::construct(_old_values_allocator, &*p, obj);
               p->_mtime = to_node(obj)._mtime;
               p->_current = &to_node(obj);
               guard0.cancel();
               _old_values.push_front(p->_item);
               to_node(obj)._mtime = _monotonic_revision;
               return &p->_item;
            }
         }
         return nullptr;
      }
      template<int N = 0>
      void clear_impl() noexcept {
         if constexpr(N < sizeof...(Indices)) {
            std::get<N>(_indices).clear();
            clear_impl<N+1>();
         }
      }
      void dispose_node(node& node_ref) noexcept {
         node* p{&node_ref};
         alloc_traits::destroy(_allocator, p);
         alloc_traits::deallocate(_allocator, p, 1);
      }
      void dispose_node(value_type& node_ref) noexcept {
         dispose_node(static_cast<node&>(*boost::intrusive::get_parent_from_member(&node_ref, &value_holder<value_type>::_item)));
      }
      void dispose_old(old_node& node_ref) noexcept {
         old_node* p{&node_ref};
         old_alloc_traits::destroy(_old_values_allocator, p);
         old_alloc_traits::deallocate(_old_values_allocator, p, 1);
      }
      void dispose_old(value_type& node_ref) noexcept {
         dispose_old(static_cast<old_node&>(*boost::intrusive::get_parent_from_member(&node_ref, &value_holder<value_type>::_item)));
      }
      void dispose(typename list_base<old_node, index0_type>::iterator old_start, typename list_base<node, index0_type>::iterator removed_start) noexcept {
         // This will leave one element around.  That's okay, because we'll clean it up the next time.
         if(old_start != _old_values.end())
            _old_values.erase_after_and_dispose(old_start, _old_values.end(), [this](pointer p){ dispose_old(*p); });
         if(removed_start != _removed_values.end())
            _removed_values.erase_after_and_dispose(removed_start, _removed_values.end(), [this](pointer p){ dispose_node(*p); });
      }
      void dispose_undo() noexcept {
         _old_values.clear_and_dispose([this](pointer p){ dispose_old(*p); });
         _removed_values.clear_and_dispose([this](pointer p){ dispose_node(*p); });
      }
      static node& to_node(value_type& obj) {
         return static_cast<node&>(*boost::intrusive::get_parent_from_member(&obj, &value_holder<value_type>::_item));
      }
      static node& to_node(const value_type& obj) {
         return to_node(const_cast<value_type&>(obj));
      }
      static old_node& to_old_node(value_type& obj) {
         return static_cast<old_node&>(*boost::intrusive::get_parent_from_member(&obj, &value_holder<value_type>::_item));
      }

      auto get_old_values_end(const undo_state& info) {
         if(info.old_values_end == nullptr) {
            return _old_values.end();
         } else {
            return _old_values.iterator_to(*info.old_values_end);
         }
      }

      auto get_old_values_end(const undo_state& info) const {
         return static_cast<decltype(_old_values.cend())>(const_cast<undo_index*>(this)->get_old_values_end(info));
      }

      auto get_removed_values_end(const undo_state& info) {
         if(info.removed_values_end == nullptr) {
            return _removed_values.end();
         } else {
            return _removed_values.iterator_to(*info.removed_values_end);
         }
      }

      auto get_removed_values_end(const undo_state& info) const {
         return static_cast<decltype(_removed_values.cend())>(const_cast<undo_index*>(this)->get_removed_values_end(info));
      }

      // returns true if the node should be destroyed
      bool on_remove( value_type& obj) {
         if (!_undo_stack.empty()) {
            auto& undo_info = _undo_stack.back();
            if ( obj.id >= undo_info.old_next_id ) {
               return true;
            }
            get_removed_field(obj) = erased_flag;

            _removed_values.push_front(obj);
            return false;
         }
         return true;
      }
      // Returns the field indicating whether the node has been removed
      static int& get_removed_field(const value_type& obj) {
         return static_cast<hook<index0_type, Allocator>&>(to_node(obj))._color;
      }
      using old_alloc_traits = typename std::allocator_traits<Allocator>::template rebind_traits<old_node>;
      indices_type _indices;
      boost::container::deque<undo_state, rebind_alloc_t<Allocator, undo_state>> _undo_stack;
      list_base<old_node, index0_type> _old_values;
      list_base<node, index0_type> _removed_values;
      rebind_alloc_t<Allocator, node> _allocator;
      rebind_alloc_t<Allocator, old_node> _old_values_allocator;
      id_type _next_id = 0;
      int64_t _revision = 0;
      uint64_t _monotonic_revision = 0;
      uint32_t                        _size_of_value_type = sizeof(node);
      uint32_t                        _size_of_this = sizeof(undo_index);
   };

   template<typename MultiIndexContainer>
   struct multi_index_to_undo_index_impl;

   template<typename T, typename I, typename A>
   struct mi_to_ui_ii;
   template<typename T, typename... I, typename A>
   struct mi_to_ui_ii<T, boost::mp11::mp_list<I...>, A> {
      using type = undo_index<T, A, I...>;
   };

   struct to_mp11 {
      template<typename State, typename T>
      using apply = boost::mpl::identity<boost::mp11::mp_push_back<State, T>>;
   };

   template<typename T, typename I, typename A>
   struct multi_index_to_undo_index_impl<boost::multi_index_container<T, I, A>> {
      using as_mp11 = typename boost::mpl::fold<I, boost::mp11::mp_list<>, to_mp11>::type;
      using type = typename mi_to_ui_ii<T, as_mp11, A>::type;
   };

   // Converts a multi_index_container to a corresponding undo_index.
   template<typename MultiIndexContainer>
   using multi_index_to_undo_index = typename multi_index_to_undo_index_impl<MultiIndexContainer>::type;
}
