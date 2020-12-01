#pragma once

#include <b1/session/session.hpp>

namespace eosio::session {

template <typename... T>
class session_variant {
 public:
   template <typename Iterator_traits>
   class session_variant_iterator {
    public:
      using difference_type   = typename Iterator_traits::difference_type;
      using value_type        = typename Iterator_traits::value_type;
      using pointer           = typename Iterator_traits::pointer;
      using reference         = typename Iterator_traits::reference;
      using iterator_category = typename Iterator_traits::iterator_category;
      friend session_variant;

    public:
      template <typename U>
      session_variant_iterator(U iterator);
      session_variant_iterator()                                   = default;
      session_variant_iterator(const session_variant_iterator& it) = default;
      session_variant_iterator(session_variant_iterator&&)         = default;

      session_variant_iterator& operator=(const session_variant_iterator& it) = default;
      session_variant_iterator& operator=(session_variant_iterator&&) = default;

      session_variant_iterator& operator++();
      session_variant_iterator& operator--();
      value_type                operator*() const;
      bool                      operator==(const session_variant_iterator& other) const;
      bool                      operator!=(const session_variant_iterator& other) const;

      bool               deleted() const;
      const shared_bytes key() const;

    private:
      std::variant<typename T::iterator...> m_holder;
   };

   struct iterator_traits {
      using difference_type   = std::ptrdiff_t;
      using value_type        = std::pair<shared_bytes, std::optional<shared_bytes>>;
      using pointer           = value_type*;
      using reference         = value_type&;
      using iterator_category = std::bidirectional_iterator_tag;
   };
   using iterator = session_variant_iterator<iterator_traits>;

 public:
   session_variant() = default;
   template <typename U>
   session_variant(U& session, std::nullptr_t);
   session_variant(const session_variant&)  = default;
   session_variant(session_variant&& other) = default;
   ~session_variant()                       = default;

   session_variant& operator=(const session_variant&) = default;

   session_variant& operator=(session_variant&& other) = default;

   /// \brief Returns the set of keys that have been updated in this session.
   std::unordered_set<shared_bytes> updated_keys() const;

   /// \brief Returns the set of keys that have been deleted in this session.
   std::unordered_set<shared_bytes> deleted_keys() const;

   /// \brief Discards the changes in this session and detachs the session from its parent.
   void undo();
   /// \brief Commits the changes in this session into its parent.
   void commit();

   std::optional<shared_bytes> read(const shared_bytes& key);
   void                        write(const shared_bytes& key, const shared_bytes& value);
   bool                        contains(const shared_bytes& key);
   void                        erase(const shared_bytes& key);
   void                        clear();

   /// \brief Reads a batch of keys from this session.
   /// \param keys A type that supports iteration and returns in its iterator a shared_bytes type representing the key.
   template <typename Iterable>
   const std::pair<std::vector<std::pair<shared_bytes, shared_bytes>>, std::unordered_set<shared_bytes>>
   read(const Iterable& keys);

   /// \brief Writes a batch of key/value pairs into this session.
   /// \param key_values A type that supports iteration and returns in its iterator a pair containing shared_bytes
   /// instances that represents a key and a value.
   template <typename Iterable>
   void write(const Iterable& key_values);

   /// \brief Erases a batch of keys from this session.
   /// \param keys A type that supports iteration and returns in its iterator a shared_bytes type representing the key.
   template <typename Iterable>
   void erase(const Iterable& keys);

   /// \brief Writes a batch of key/values from this session into another container.
   /// \param ds The container to write into.  The type must implement a batch write method like the one defined in this
   /// type. \param keys A type that supports iteration and returns in its iterator a shared_bytes type representing the
   /// key.
   template <typename Other_data_store, typename Iterable>
   void write_to(Other_data_store& ds, const Iterable& keys);

   /// \brief Reads a batch of keys from another container into this session.
   /// \param ds The container to read from.  This type must implement a batch read method like the one defined in this
   /// type. \param keys A type that supports iteration and returns in its iterator a shared_bytes type representing the
   /// key.
   template <typename Other_data_store, typename Iterable>
   void read_from(Other_data_store& ds, const Iterable& keys);

   /// \brief Returns an iterator to the key, or the end iterator if the key is not in the cache or session heirarchy.
   /// \param key The key to search for.
   /// \return An iterator to the key if found, the end iterator otherwise.
   iterator find(const shared_bytes& key);
   iterator begin();
   iterator end();

   /// \brief Returns an iterator to the first key that is not less than, in lexicographical order, the given key.
   /// \param key The key to search on.
   /// \return An iterator to the first key that is not less than the given key, or the end iterator if there is no key
   /// that matches that criteria.
   iterator lower_bound(const shared_bytes& key);

   std::variant<T*...> holder();

 private:
   std::variant<T*...> m_holder;
};

template <typename... T>
template <typename U>
session_variant<T...>::session_variant(U& session, std::nullptr_t) : m_holder{ &session } {}

template <typename... T>
std::unordered_set<shared_bytes> session_variant<T...>::updated_keys() const {
   return std::visit([&](auto* session) { return session->updated_keys(); }, m_holder);
}

template <typename... T>
std::unordered_set<shared_bytes> session_variant<T...>::deleted_keys() const {
   return std::visit([&](auto* session) { return session->deleted_keys(); }, m_holder);
}

template <typename... T>
std::variant<T*...> session_variant<T...>::holder() {
   return m_holder;
}

template <typename... T>
void session_variant<T...>::undo() {
   std::visit([&](auto* session) { return session->undo(); }, m_holder);
}

template <typename... T>
void session_variant<T...>::commit() {
   std::visit([&](auto* session) { return session->commit(); }, m_holder);
}

template <typename... T>
std::optional<shared_bytes> session_variant<T...>::read(const shared_bytes& key) {
   return std::visit([&](auto* session) { return session->read(key); }, m_holder);
}

template <typename... T>
void session_variant<T...>::write(const shared_bytes& key, const shared_bytes& value) {
   std::visit([&](auto* session) { return session->write(key, value); }, m_holder);
}

template <typename... T>
bool session_variant<T...>::contains(const shared_bytes& key) {
   return std::visit([&](auto* session) { return session->contains(key); }, m_holder);
}

template <typename... T>
void session_variant<T...>::erase(const shared_bytes& key) {
   std::visit([&](auto* session) { return session->erase(key); }, m_holder);
}

template <typename... T>
void session_variant<T...>::clear() {
   std::visit([&](auto* session) { return session->clear(); }, m_holder);
}

template <typename... T>
template <typename Iterable>
const std::pair<std::vector<std::pair<shared_bytes, shared_bytes>>, std::unordered_set<shared_bytes>>
session_variant<T...>::read(const Iterable& keys) {
   return std::visit([&](auto* session) { return session->read(keys); }, m_holder);
}

template <typename... T>
template <typename Iterable>
void session_variant<T...>::write(const Iterable& key_values) {
   std::visit([&](auto* session) { return session->write(key_values); }, m_holder);
}

template <typename... T>
template <typename Iterable>
void session_variant<T...>::erase(const Iterable& keys) {
   std::visit([&](auto* session) { return session->erase(keys); }, m_holder);
}

template <typename... T>
template <typename Other_data_store, typename Iterable>
void session_variant<T...>::write_to(Other_data_store& ds, const Iterable& keys) {
   std::visit([&](auto* session) { return session->write_to(ds, keys); }, m_holder);
}

template <typename... T>
template <typename Other_data_store, typename Iterable>
void session_variant<T...>::read_from(Other_data_store& ds, const Iterable& keys) {
   std::visit([&](auto* session) { return session->read_from(ds, keys); }, m_holder);
}

template <typename... T>
typename session_variant<T...>::iterator session_variant<T...>::find(const shared_bytes& key) {
   return std::visit([&](auto* session) { return session_variant<T...>::iterator{ session->find(key) }; }, m_holder);
}

template <typename... T>
typename session_variant<T...>::iterator session_variant<T...>::begin() {
   return std::visit([&](auto* session) { return session_variant<T...>::iterator{ session->begin() }; }, m_holder);
}

template <typename... T>
typename session_variant<T...>::iterator session_variant<T...>::end() {
   return std::visit([&](auto* session) { return session_variant<T...>::iterator{ session->end() }; }, m_holder);
}

template <typename... T>
typename session_variant<T...>::iterator session_variant<T...>::lower_bound(const shared_bytes& key) {
   return std::visit([&](auto* session) { return session_variant<T...>::iterator{ session->lower_bound(key) }; },
                     m_holder);
}

template <typename... T>
template <typename Iterator_traits>
template <typename U>
session_variant<T...>::session_variant_iterator<Iterator_traits>::session_variant_iterator(U iterator)
    : m_holder{ std::move(iterator) } {}

template <typename... T>
template <typename Iterator_traits>
typename session_variant<T...>::template session_variant_iterator<Iterator_traits>&
session_variant<T...>::session_variant_iterator<Iterator_traits>::operator++() {
   std::visit([&](auto& it) { ++it; }, m_holder);
   return *this;
}

template <typename... T>
template <typename Iterator_traits>
typename session_variant<T...>::template session_variant_iterator<Iterator_traits>&
session_variant<T...>::session_variant_iterator<Iterator_traits>::operator--() {
   std::visit([&](auto& it) { --it; }, m_holder);
   return *this;
}

template <typename... T>
template <typename Iterator_traits>
typename session_variant<T...>::template session_variant_iterator<Iterator_traits>::value_type
session_variant<T...>::session_variant_iterator<Iterator_traits>::operator*() const {
   return std::visit([&](auto& it) { return *it; }, m_holder);
}

template <typename... T>
template <typename Iterator_traits>
bool session_variant<T...>::session_variant_iterator<Iterator_traits>::operator==(
      const session_variant_iterator& other) const {
   return m_holder == other.m_holder;
}

template <typename... T>
template <typename Iterator_traits>
bool session_variant<T...>::session_variant_iterator<Iterator_traits>::operator!=(
      const session_variant_iterator& other) const {
   return m_holder != other.m_holder;
}

template <typename... T>
template <typename Iterator_traits>
bool session_variant<T...>::session_variant_iterator<Iterator_traits>::deleted() const {
   return std::visit([&](auto& it) { return it.deleted(); }, m_holder);
}

template <typename... T>
template <typename Iterator_traits>
const shared_bytes session_variant<T...>::session_variant_iterator<Iterator_traits>::key() const {
   return std::visit([&](auto& it) { return it.key(); }, m_holder);
}

} // namespace eosio::session
