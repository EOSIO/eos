# Chain-KV aka Session API

### Table of contents
1. [Enable ChainKV in Nodeos](#enable-chainkv-in-nodeos)
2. [ChainKV or Chainbase](#chainkv-or-chainbase)
3. [Using a database table in a smart contract](#using-a-database-table-in-a-smart-contract)
4. [Using a kv table in a smart contract](#using-a-kv-table-in-a-smart-contract)
5. [What is a Session](#what-is-a-session)
6. [Extending Session](#extending-session)
7. [What is an Undo stack](#what-is-an-undo-stack)

## Enable ChainKV in Nodeos

To enable the use of the RocksDB backing store in nodeos provide this argument when starting nodeos `--backing-store rocksdb`.
By default Chainbase is the backing store of Nodeos.  So when this argument is not present when starting Nodeos, Chainbase will be used. You can also
be explicit in setting the use of Chainbase by specifying `--backing-store chainbase` when starting Nodeos.

## ChainKV or Chainbase


## Using a database table in a smart contract


## Using a kv table in a smart contract


## What is a Session

A Session is a lexicographically ordered (by key) in memory key-value datastore.  It provides methods for reading, writing and deleting key-value pairs and for ordered iteration over the keys.  Sessions can be laid out in a parent-child relationship (in the fashion of a linked list) in such a way that you can form the building blocks of a block chain, where the head block would be the permanent, irrevisible data store (such as RocksDB).  

Session was implemented with the assumption that once a Session instance has a child, that Session instance has become immutable and the child Session instance will contain delta changes that can either be committed up into the parent Session instance or discarded (undo) by either breaking the parent child relationship or by calling undo on the child session.  This abstraction allows for defining Session as either a block or a transaction within that block depending on the context.

The Session type is templated to allow template specialization for introducing new permament data stores into the EOS system.  Currently there are only two types of Sessions implemented, a RocksDB specialization to allow for persisting key value pairs in RocksDB and the default template which represents the in memory key-value datastore.

## Extending Session

The Session API depends mainly on duck typing and template specialization, so extending the Session API to introduce new data stores is a fairly straight forward process.  For example if you wanted to introduce a new permament data store then the basic setup would be something like the following code example.  For additional documentation refer to session.hpp.

```cpp
/// This could be any type of datastore that you'd want to introduce.
/// For example any relational database engine, a http end point that streams data to another server, etc...
struct my_datastore_t {};

/// To extend Session, we just provide a specialization tagged by the struct above.
template <>
class session<my_datastore_> {
  /// This API relies on duck typing, so this specialization needs to provide at least the same
  /// public interface as the templated session defininition.  Beyond that, you can provide
  /// any additional methods you want to implement your specialization.  Refer to session.hpp
  /// for documentation on these methods.

  std::unordered_map<shared_bytes, bool> deltas() const;
  void attach(Parent& parent);
  void attach(session& parent);
  void detach();
  void undo();
  void commit();

  std::optional<shared_bytes> read(const shared_bytes& key) const;
  void                        write(const shared_bytes& key, const shared_bytes& value);
  bool                        contains(const shared_bytes& key) const;
  void                        erase(const shared_bytes& key);
  void                        clear();

  template <typename Iterable>
  const std::pair<std::vector<std::pair<shared_bytes, shared_bytes>>, std::unordered_set<shared_bytes>>
  read(const Iterable& keys) const;

  template <typename Iterable>
  void write(const Iterable& key_values);

  template <typename Iterable>
  void erase(const Iterable& keys);

  template <typename Other_data_store, typename Iterable>
  void write_to(Other_data_store& ds, const Iterable& keys) const;
  
  template <typename Other_data_store, typename Iterable>
  void read_from(const Other_data_store& ds, const Iterable& keys);

  iterator find(const shared_bytes& key) const;
  iterator begin() const;
  iterator end() const;
  iterator lower_bound(const shared_bytes& key) const;
};
```

### What is an Undo stack

The undo stack is a container used within the context of the EOS blockchain for managing sessions.  While this is termed a `stack`, the way this container works is closer to an `std::deque`.  The operations that are valid from the bottom of the stack would only be making the session irrevisible by committing it into the permament datastore.  The exception to this would be when the bottom and the top of the stack are the same in which case you can also perform the operations valid on the top of stack. The bottom of the stack is normally referred to as the `root` session. The operations that are valid from the top of the stack include squashing (combining the top 2 session on the stack), popping the top session which discards the changes in that session (an undo operation) and pushing a new session instance onto the stack.  The top of the stack is normally referred to as the `head` session.