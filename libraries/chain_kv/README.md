*Communication from Bucky in regards to the new Session API.*

The undo mechanism in chain-kv currently has left a lot to be desired in terms of performance. The proposed caching mechanism is aimed to help minimize these overheads, and as an added benefit hopefully minimize any overheads for reads and writes in general.  During my original incarnation of this, I had thought about modeling the CAM system off of a more traditional CPU caching system.  This is still doable and would more than likely be an optimization, but it will more than likely be a micro optimization at the cost of a fair amount of complexity.  But, this is still an avenue that we could go down if the performance is still not good enough.

###Basic Overview
* A CAM (content-addressable memory) system for the caching mechanism.  In this proposal, this will utilize ​std::unordered_map​.
* A concept of an entry, this is used to act as the abstraction of a key-value entry that can be referenced and handle the complexity of RocksDB or the caching mechanism.
* A concept of a session, this will be more in line with the definition of a session in chainbase.  A session can be created from a chain-kv context or a different session (to allow for nested sessions).
* This system is optimizing for the case that the undo will occur, undos will only update afree list and possibly an mmap call (rarely).
* Reads and writes should generally be O(1) for the client code.

###Caching System

**class key_slice**   
A fixed size slice to be used for hashing.  The working idea is 8 bytes to optimize for a uint64_t.  This is also to limit the ability of gaming that can occur with massive keys.

**class entry**
RAII for handling the lifetime of the entry for write backs and reading.   

```cpp
entry(std::string_view k); // ctor
key_slice get_slice() const;
std::string key. /* this is the key of entry */ 
void* data. /* handle in memory of the held data */   
std::size_t data_size.
```

**class kv_cache**
```cpp
entry& get(std::string_view key);    
std::unordered_map<key_slice, list<entry>> mapping.
```

**struct null_parent**

**class session**
```cpp
session(chain_kv& db); // ctor   
session(session& db);   // ctor   
std::variant<null_parent*, chain_kv*, session*> parent;   
std::optional<session*> child;   
kv_cache cache.
```

**session::session(chain_kv&)​** 
This will create a cache that is backed by the main RocksDB instance.  This allows for gets and write backs to be against Rocks.

**session::session(session&)**
This will create a cache that is backed by another cache.  This means that all gets will go through the parent session before getting the value from Rocks and writebacks will only writeback to the parent session.  This can be achieved by iterating the cache and doing something like parent->set(entry)(actually something like std::visit yada yada).  Basically, set the old entry to the new entry.

**session::~session(session&)** 
This will handle the write back and either batch write back to Rocks or handle the write back to the parent session.  If it’s parent is a ​null_parent​ then no write back should take place, this session should be ignored (undo).

**kv_cache**
Because we are only using a partial key for the mapping, we will have to have a list of entries (this should be an std::vector, in practice, as it shouldn’t ever decrease or have arbitrary removals).  This does mean that the worst case lookup is O(N).  If we are okay with any concerns of large keys, then we can do away with the concept of the ​key_slice​ and this lookup will be O(1).

**Session**
Limitation of this design for simplicity, is that each session can have optionally only 1 child.  During the write back phase this will update it’s child with either null_parent (i.e. this leg is now dead, or with chain_kv).  This means that evaluation of those don’t have to occur until they areactually needed to be evaluated and that a session can be committed without having to do a lotof work on sessions that are building on it.

**Memory Management**
To limit the issues with fragmentation and excessive kernel mode calls, explicit memory management can be done.

**Session allocator**
This should be a chunk based allocator that premaps N pages for use.  This should pre alloc per M page to keep kernel mode calls to a minimum without over allocation occurring.  So, mmap(Npages, prot_none), then mmap (M pages, prot_read|prot_write).  From here we can allocate per Z chunks purely in user-land.  This can be used to allocate for each ​entry​ “data” buffer that is needed.   When we make a call to retrieve from RocksDB, we can have a temporary buffer to retrieve or allocate for this once for the ​entry.  This also has the benefit of providing strong locality of the key value sessions.  This should make this system (hopefully) pretty cache friendly and reads and writes will already be in CPU cache.  Deallocation is done by simply updating the free list for the chunk allocator and re-mmapping only when it dips below some threshold so as to not have bouncing issues (effectively have some type of hysteresis for this phase). Or we can defer re-mmapping to free the pages if we check to see if the system memory contention is getting high.

**Entry**
To set new values, it will either use the same buffer if small enough, or it will allocate a new set of chunks and free the old set (could defer this freeing if needed).  

**Iteration concerns**
If we handle all new “sets” to the cache as write through and do both an update to the cachea nd a RocksDB put at the same time, then iteration can occur as normal and we only need to check the key from the iterator for materializing the value from either Rocks or the cache (which should already be built into this system anyhow).  If we believe that will be too much of an overhead, then more thought needs to be made to ensure consistency between sessions and iterations.

**Examples of flow**
*Create a fork*
session fork_session(db); // chain_kv instance

*Create a block*
​session block_session(fork_session); // base off of the fork session running.

*Execute a transaction*
​session transaction_session( block_session );

*Execute an action*
​auto& e = transaction_session.get(“foo”);   
transaction_session.set(e, “something new”);

*If the action or the transaction fails*   
Set the transaction_session parent to null_parent and let go out of scope.

*If the action and the transaction succeeds* 
​Let the transaction_session go out of scope and it will write it’s changes back to the block_session.

*If the block fails*
Set the block_session parent to null_parent and let it go out of scope.

*If the block succeeds*   
Let the session go out of scope and it will write it’s changes back to the fork_session.

###Iterator Cache Discussion

```cpp
template <bool IsConst>
    class session_iterator_base {
       using difference_t = long;
       using value_t      = std::conditional_t<IsConst, const key_value, key_value>;
       using pointer_t    = key_value*;
       using reference_t  = key_value&;
       using iterator_cat = std::bidirectional_iterator_tag;
       using session_t    = session<persistent_data_store, cache_data_store>;
       public:
        session_iterator(iterator_state is, session<
        session_iterator(const session_iterator&) = default;
        session_iterator(session_iterator&&) = default;
        session_iterator& operator=(const session_iterator&) = default;
        session_iterator& operator=(session_iterator&&) = default;
        session_iterator& operator++() {
           if (is_next_in_cache())
              increment(true);
           else
               increment(false);
           return *this;
        }
        session_iterator operator++(int) {
            auto ret = *this;
            ++(*this);
            return ret;
        }
        session_iterator& operator--() {
        }
        session_iterator operator--(int) {
        }
        value_t& operator*() {
        }
        const value_t& operator*() const {
        }
        value_t& operator->() {
        }
        const value_t& operator->() const {
        }
        bool operator==(const session_iterator& other) const {
        }
        bool operator!=(const session_iterator& other) const {
        }
       protected:
        session_iterator() = default;
        void increment(bool in_cache) {
             if (in_cache)
                std::visit(overloaded { [](cache_iter& i) { ++i; },
                                                      [&](parent_iter& i) { iterator_state = active_session->cache_find(i.key()) }}, iterator_state);
             else
               std::visit(overloaded {  [](db_iter& i) { ++i; },
                                                     [&](cache_iter& i) { iterator_state = active_session->db_find(i.key()) }}, iterator_state);
        }
       // same thing for decrement but you know decrement it
        bool is_next_in_cache() const {
           return active_session->iterator_cache[std::visit([](const auto& i) { return i.key(); }, iterator_state)].first;
        }
        bool is_prev_in_cache() const {
           return active_session->iterator_cache[std::visit([](const auto& i) { return i.key(); }, iterator_state)].second;
        }
        const reference_t read(const bytes& key) const {
            return std::visit ....;
        }
       private:
        iterator_state state;
        session_t*     active_session;
    };
```

I wanted to try to fit my quick idea into your code as it is, and I'm just out of time.  So, I'm just going to describe what this could look like.
So, session_iterator now holds a pointer to the active session and an std::variant<cache_iterator, rocks_iterator>.  In session we now have a iterator_cache which is just an std::unordered_map<bytes, std::pair<bool, bool>>.  This cache will state whether the next value is in the cache or not {first -> next is there, second -> previous is there}.
When we read we update this cache, or if we want to when we iterate we "read through" the values and prime the cache as we move along.  This allows for cascading the nested sessions without any additional logic (instead of db_iter it is the parent session iter).  We can still collapse the state down if needed, but it isn't required at all with this.  There is the overhead of too many bounces, but the hope is that much like L1 to L2 that it will be there and we won't have to dig all the way to DB too often. You shouldn't need to predict what is in what cache.  Also, we need to have some solution that will not require just one cache at the end as in your last solution.  This will end up forcing us to redesign again in a few months when we start to parallelize these things.  I think one of the biggest things that you are missing with this design, is that is an inherently recursive problem.  By this I mean that the state is minimized at each session.  I think you keep trying to view this as an imperative problem or prescribing an imperative solution to it.  At each session we only have the question is this key in cache or not.  We can add some additional information to hold which is the lower bound key if we need (I had left that out as I thought for the most part it would be an implementation detail of whether and where to call back to get the "lower" key and where or how we can represent that information.
But, if you would like a possible design for that.  What I had had in mind was we could have a keys cache kind of thing in the session that caches the bytes for the lower bytes.  In reality you only need to know either the lowest or the highest, as it looks like we are still going with circular iterators.  If you look at the lowest's previous and it's in cache then that it the "highest", else you need to get it from the parent or you can just cache both, it's not a big deal at that point.  Back to the recursive nature of this solution.  As mentioned above the only state question at any point P (any possible session), you only care about is the entry with the key X in the cache C.  If it is not then we increment the parent iterator (session_iterator, this is the recursive part), we are going to not care how the parent iterator is working, just that it will point to the key/value that is logically next or previous.
So the states are:
     We either have the next/previous in cache :
            We are in a cache_iterator state -> simply increment that iterator
            We are in a parent_iterator state -> replace the held iterator to the found next cached iterator
     Else :
          We are in a cache_iterator state -> replace the held iterator with the found next parent iterator
          We are in a parent_iterator state -> simply increment that iterator
The session_iterator/cache_iterator and rocksdb_iterator should have the same lexical interface for this to work.
But, at point P in this:  <session name>(<session parent>)[<held values>]
  base_session(rocks)[1,2,3,4,5,6] -> session_0(base_session)[1,2,3] -> session_1(session_0)[3,6] -> session_2(session_1)[3,4,5]
let's say that point P in this case is session_2.
Then,
Let's say we are iterating with it = 1;
Then our iterator would be session_1 -> session_iterator::parent_iterator -> session_0-> session_iterator::cache_iterator -> 1.
We do an it++, we are still using the parent iterator and it's the same has last time except it's now pointing to 2.
We then do another it++, we now set our held iterator to the cache_iterator -> 3.
Let's say that session_0 removes [1] (before the creation of sessions 1 and 2) and we need to find the lower bound in session_2.  So we have the say get lower from the parent (if we don't already have it saved), we then go back to session_1, which hopefully has saved what the lower is then gives you that key, if not it will go to session_0, which has modified the lower so, we are guaranteed that lower will be given to it.  If session_2 adds a value that would be less than the lower or higher than the highest, we simply update the cached lowest and highest keys (some of the other solutions for caching that value would need to bounce around).
This means that like L1 -> L2 -> L3 cache, we are assuming (speculating) that the cached values for these things will be in close levels and hopefully not required full run through all the way to the base parent.  This is where we need to do profiling after this implementation is done to determine if we should "prefetch" during the iteration or not.  Also, like system cache, we don't care when we look backwards as to where the information is coming from we just know that it will be handled.