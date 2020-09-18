#pragma once

#include <queue>
#include <set>
#include <stack>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <variant>

#include <chain_kv/cache.hpp>
#include <chain_kv/shared_bytes.hpp>

namespace eosio::chain_kv {
   // helpers for std::visit
   template <class... Ts>
   struct overloaded : Ts... {
      using Ts::operator()...;
   };
   template <class... Ts>
   overloaded(Ts...)->overloaded<Ts...>;

   struct derived_tag{};
   constexpr static inline auto derived_tag_v = derived_tag{};

   // Defines a session for reading/write data to a cache and persistent data store.
   //
   // \tparam Parent The parent of this session
   template <typename DB>
   class session {
      public:
         struct iterator_state {
            shared_bytes next_in_cache;
            shared_bytes previous_in_cache;
            bool deleted{ false };
            bool is_updated{ false };
         };

         class session_iterator;
         using db_type               = DB;
         using parent_type           = std::variant<session*, db_type*>;
         using parent_type_iterator  = std::variant<typename DB::iterator, session_iterator>;
         using cache_type            = std::map<shared_bytes, shared_bytes>;
         using iterator_cache_type   = std::unordered_map<shared_bytes, iterator_state>;

         struct parent_tag{};
         struct cache_tag{};

         constexpr static inline auto parent_tag_v  = parent_tag{};
         constexpr static inline auto cache_tag_v   = cache_tag{};

         class session_iterator {
            using iterator_cache_iterator = typename iterator_cache_type::const_iterator;
          public:
            using value_type = std::pair<shared_bytes, shared_bytes>;
            template <typename Iter>
            session_iterator( parent_tag, Iter&& iter, session* s )
               : m_cache_iterator(),
                 m_parent_iterator(std::forward<Iter>(iter)),
                 m_active_session(s),
                 m_is_cache(false) {}
            template <typename Iter>
            session_iterator( cache_tag, Iter&& iter, session* s )
               : m_cache_iterator(std::forward<Iter>(iter)),
                 m_parent_iterator(),
                 m_active_session(s),
                 m_is_cache(true) {}
            session_iterator(const session_iterator& it) = default;
            session_iterator(session_iterator&&)         = default;

            session_iterator& operator=(const session_iterator& it) = default;
            session_iterator& operator=(session_iterator&&) = default;

            template <typename Op, auto& Field>
            auto& move(Op&& op) {
               const auto& info = m_active_session->m_iterator_cache[get_key()];
               do {
                  if ( std::invoke(Field, info) ) {
                     if (m_is_cache) {
                        op(m_cache_iterator);
                     } else {
                        std::visit(std::move(op), m_parent_iterator);
                        m_cache_iterator = m_active_session->find(get_key());
                        m_is_cache = true;
                     }
                  } else {
                     if (m_is_cache) {
                        m_parent_iterator = m_active_session->parent_find(get_key());
                        m_is_cache = false;
                        std::visit(std::move(op), m_parent_iterator);
                     } else {
                        std::visit(std::move(op), m_parent_iterator);
                     }
                  }
                  info = m_active_session->m_iterator_cache[get_key()];
               } while (!info.deleted);
               return *this;
            }

            session_iterator& operator++() { return move([&](auto& it) { ++it; }, &iterator_state::next_in_cache); }
            session_iterator& operator--() { return move([&](auto& it) { --it; }, &iterator_state::prev_in_cache); }
            value_type        operator*() const { return std::pair{ get_key(), get_value() }; }
            value_type        operator->() const { return std::pair{ get_key(), get_value() }; }
            bool              operator==(const session_iterator& other) const { return get_key() == other.get_key(); }
            bool              operator!=(const session_iterator& other) const { return !((*this) == other); }

            inline bool deleted() const { return m_active_session->m_iterator_cache[get_key()].deleted; }

            inline const auto& get_key() const {
               return m_is_cache ? m_cache_iterator->first :
                      std::visit([](auto& i) { i.get_key(); }, m_parent_iterator);
            }
            inline const auto& get_value() const {
               return m_is_cache ? m_cache_iterator->second :
                      std::visit(
                            overloaded( [&]( session::iterator& i ) { return i.get_value(); },
                                        [&]( typename db_type::iterator& i ) { return m_active_session->read(i->first); } ),
                            m_parent_iterator );
             }
         private:
            cache_type::iterator  m_cache_iterator;
            parent_type_iterator  m_parent_iterator;
            session*              m_active_session{ nullptr };
            bool                  m_is_cache{false};
      };

      using iterator = session_iterator;

      template <typename Parent>
      auto* setup_parent(Parent* p) {
         p->set_child(this);
         return p;
      }

      public:
         session(derived_tag, DB& d)
            : m_parent(setup_parent(&d)) {}
         session(derived_tag, session& parent)
            : m_parent(setup_parent(&parent)) {}
         session(const session&) = default;
         session(session&&)      = default;
         ~session() { if (!invalidated) commit(); }

         session& operator=(const session&) = default;
         session& operator=(session&&) = default;

         inline set_child(session* c) { child = c; }

         inline parent_type parent() { return m_parent; }
         inline bool has_parent() const { return true; }
         // undo stack operations
         inline void undo() { invalidated = true; }

         void commit() {
            std::visit( [&](auto* p) { for (const auto [k, v] : m_cache) p->write(k,v); }, m_parent );
            invalidated = true;
         }

         void update_iter_cache(const shared_bytes& key, parent_tag& p) const {
            const auto& update_with_it = [](auto& k, const auto& i) { k = i->first; };
            const auto& update_key = [](auto& k, const auto& b) { k = b; };
            auto& info = m_iterator_cache[key];
            if (!info.is_updated) {
               auto it = p.lower_bound(key);
               info.is_updated = true;
               if ( it == p.end() ) {
                  //++it->first[0]; // TODO Tim you will need to swap this to the user defined prefix++, not sure which byte that will be
                  update_with_it(info.next_in_cache, --p.lower_bound(it->first));
                  // reset the prefix --it->first[0];
                  update_with_it(info.prev_in_cache, --it);
                  return;
               }

               bool dec = false;
               if ( it->first == key ) {
                  --it;
                  dec = true;
               }
               auto& prev_info = m_iterator_cache[key];
               update_with_it(info.prev_in_cache, it);
               if (prev_info.is_updated) {
                  const auto& nk = prev_info.next_in_cache;
                  update_key(info, nk);
                  auto& next_info = m_iterator_cache[nk];
                  if (next_info.is_updated) {
                     update_key(next_info.prev_in_cache, key);
                  }
                  return;
               }
               ++it;
               if (dec)
                  ++it;
               update_key(info.next_in_cache, it);
            }
         }

         template <typename OnLocal, typename OnParent, typename OnError>
         decltype(auto) work_on(OnLocal&& local_func,
                                OnParent&& parent_func,
                                OnError&&  error_func,
                                const shared_bytes& key) {
            if ( contains_local(key) ) {
               return local_func();
            } else {
               auto p = m_parent;
               // TODO need to evaluate these visits.  They look terrible
               while ( std::visit([](auto* x) { return x->has_parent(); }, p) ) {
                  if ( std::visit([&](auto* x) { return x->contains_local(key); }, p) ) {
                     return std::visit([&](auto* x) { return parent_func(x); }, p);
                  }
                  if (std::holds_alternative<session*>(p))
                     p = std::get<session*>(p)->parent();
               }
            }
            return error_func();
         }

         // this part also identifies a concept.  don't know what to call it yet
         shared_bytes read(const shared_bytes& key) const {
            return work_on( [&](){ return m_cache.find(key); },
                            [&](auto* p){
                              // TODO disable this cache priming for performance testing
                              if constexpr (std::is_same_v<std::decay_t<decltype(p)>, session>) {
                                 update_iter_cache(key, *p);
                                 const auto& b = p->find(key);
                                 m_cache[key] = b;
                                 return b;
                              }
                            },
                            [](){ return shared_bytes::invalid(); },
                            key );
         }

         inline bool contains_local(const shared_bytes& key) const { return m_iterator_cache.count(key) > 0; }
         void write(const shared_bytes& key, const shared_bytes& value) {
            return work_on( [&]() { m_cache[key] = value; },
                            [&](auto* p) {
                               if constexpr (std::is_same_v<std::decay_t<decltype(p)>, session>) {
                                  update_iter_cache(key, *p);
                                  m_cache[key] = value;
                               }
                               return;
                            },
                            [](){},
                            key );
         }

         bool contains(const shared_bytes& key) const {
            return work_on( []() { return true; },
                            [](auto* p) { return true; },
                            []() { return false; },
                            key );
         }

         void erase(const shared_bytes& key) {
            if ( contains_local(key) ) {
               m_cache.erase(key);
            }
            m_iterator_cache[key].is_updated = true;
            m_iterator_cache[key].deleted = true;
         }
         inline void clear() {
            m_cache.clear();
            m_iterator_cache.clear();
         }

         //std::pair<shared_bytes, shared_bytes> bounds(const shared_bytes& key) const;

         inline bool is_deleted(const shared_bytes& key) const { return m_iterator_cache[key].deleted; }

         iterator find(const shared_bytes& key) {
            work_on( [&]() { return iterator{ cache_tag_v, m_cache.find(key), this }; },
                     [&](auto* p) { return iterator{ parent_tag_v, p->find(key), this}; },
                     [&]() { return iterator { cache_tag_v, m_cache.end(), this}; }, key );
         }
         iterator parent_find(const shared_bytes& key) {
            return std::visit([&](auto& p) { return iterator{p->find(key), this}; }, m_parent);
         }
#if 0
         // TODO this needs a contract prefix
         iterator begin() {
         }
         // TODO this needs a contract prefix
         iterator       end();
         // TODO this needs a contract prefix
         iterator       lower_bound(const shared_bytes& key);
         // TODO this needs a contract prefix
         iterator       upper_bound(const shared_bytes& key);
#endif

      private:
         parent_type m_parent;
         session* child = nullptr;

         // TODO you can go back to your cache type, if you want.  I couldn't get your iterators to work
         // correctly through the system

         // The cache used by this session instance.  This will include all new/update key/value pairs
         // and may include values read from the persistent data store.
         cache_type m_cache;

         // Indicates if the next/previous key in lexicographical order for a given key exists in the cache.
         // The first value in the pair indicates if the previous key in lexicographical order exists in the cache.
         // The second value in the pair indicates if the next key lexicographical order exists in the cache.
         iterator_cache_type m_iterator_cache;

         bool invalidated = false;
   };
} // namespace eosio::chain_kv
