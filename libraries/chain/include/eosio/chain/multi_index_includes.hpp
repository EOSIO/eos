/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/ordered_index.hpp>

#include <cyberway/chaindb/multi_index.hpp>

namespace bmi = boost::multi_index;
using bmi::indexed_by;
using bmi::ordered_unique;
using bmi::ordered_non_unique;
using bmi::composite_key;
using bmi::member;
using bmi::const_mem_fun;
using bmi::tag;
using bmi::composite_key_compare;

struct by_id {};

namespace eosio { namespace chain {

    struct by_parent {};
    struct by_owner {};
    struct by_name {};
    struct by_action_name {};
    struct by_permission_name {};
    struct by_trx_id {};
    struct by_expiration {};
    struct by_delay {};
    struct by_status {};
    struct by_sender_id {};

} } // namespace eosio::chain

namespace eosio { namespace chain { namespace resource_limits {

    struct by_owner {};

} } } // namespace eosio::chain::resource_limits;

namespace cyberway { namespace chaindb {
    template<typename A>
    struct get_result_type {
        using type = typename A::result_type;
    }; // struct get_result_type

    template<typename Tag, typename KeyFromValue, typename CompareType = std::less<typename KeyFromValue::result_type>>
    struct ordered_unique {
        using tag_type = Tag;
        using key_from_value_type = KeyFromValue;
        using compare_type = CompareType;
        using extractor_type = KeyFromValue;
    }; // struct ordered_unique

    template<typename Value, typename... Fields>
    struct composite_key {
        using result_type = bool;

        using key_type = boost::tuple<typename get_result_type<Fields>::type...>;

        auto operator()(const Value& v) {
            return boost::make_tuple(Fields()(v)...);
            //return key_type();
        }
    }; // struct composite_key

    template<typename... Items>
    struct indexed_by {};

    template<typename Object, typename Allocator, typename Items>
    struct multi_index_container;

    struct object_id_extractor {
        template<typename T>
        uint64_t operator()(const T& o) const {
            return o.id._id;
        }
    }; // struct object_id_extractor

    template<typename Object, typename Allocator, typename... Items>
    struct multi_index_container<Object, Allocator, indexed_by<Items...>> {
        using impl = multi_index<tag<Object>, PrimaryIndex<tag<by_id>, object_id_extractor>, Object, Allocator, Items...>;
    };

   template<typename Object, typename IndexSpec>
   struct shared_multi_index_container {
       struct node_type {};
       using value_type = Object;
       using allocator_type = chainbase::allocator<value_type>;
       using container_impl = typename multi_index_container<Object, chainbase::allocator<value_type>, IndexSpec>::impl;

       container_impl impl;

       using size_type = std::size_t;

       template<typename Tag, typename Extractor>
       struct index {
           using index_impl = typename container_impl::template index<Tag, Extractor>;
           struct type {
               using iterator = typename index_impl::const_iterator;
               
               index_impl impl;

               type(const shared_multi_index_container* midx)
               : impl(&midx->impl)
               { }

               iterator begin() const {return impl.begin();}
               iterator end() const {return impl.end();}

               iterator rbegin() const {return impl.rbegin();}
               iterator rend() const {return impl.rend();}

               template<typename Key>
               iterator find(const Key& key) const {return impl.find(key);}

               template<typename Key>
               iterator lower_bound(const Key& key) const {return impl.lower_bound(key);}

               template<typename Key>
               iterator upper_bound(const Key& key) const {return impl.upper_bound(key);}

               template<typename Key>
               std::pair<iterator, iterator> equal_range(const Key& key) const {return impl.equal_range(key);}

               iterator iterator_to(const value_type& value) const {return impl.iterator_to(value);}

               bool empty() const {return impl.begin() == impl.end();}
               size_t size() const {return 0;} // TODO: ?
           };
       };

       using iterator = typename container_impl::const_iterator;
       using const_iterator = iterator;
       using emplace_return_type = typename std::pair<iterator,bool>;

       uint64_t available_primary_key() const {
           return impl.available_primary_key();
       }

       template<typename Tag>
       auto get() const {
           auto idx = impl.template get_index<Tag>();
           return typename index<cyberway::chaindb::tag<Tag>, typename decltype(idx)::extractor_type>::type(this);
       }

       template<typename Type>
       iterator find(const chainbase::oid<Type>& k) const { return impl.find(k._id); }

       template<typename Modifier>
       bool modify(iterator position, Modifier&& mod) {
           impl.modify(position, std::forward<Modifier>(mod));
           return true;
       }

       iterator erase(iterator position) { return impl.erase(position); }

       template<typename Constructor, typename Allocator>
       emplace_return_type emplace(Constructor&& constructor, Allocator&&) {
           try {
               auto iter = impl.emplace(std::forward<Constructor>(constructor));
               return std::make_pair(std::move(iter), true);
           } catch (const fc::exception& err) {
               return std::make_pair(end(), false);
           }
       }

       emplace_return_type emplace(value_type) {
           return std::make_pair(end(), false);
       }

       iterator iterator_to(const value_type& x) const {
           return impl.iterator_to(x);
       }

       size_type size() const { return 0; } // TODO: ?
       iterator begin() {return impl.begin(); }
       const_iterator begin() const { return impl.begin(); }
       iterator end()  {return impl.end(); }
       const_iterator end() const { return impl.end(); }

       allocator_type get_allocator() const { return allocator; }

       template<typename Tag, typename IteratorType>
       auto project(IteratorType it) {
           return get<Tag>().iterator_to(*it);
       }

       template<typename Tag, typename IteratorType>
       auto project(IteratorType it) const {
           return get<Tag>().iterator_to(*it);
       }

       explicit shared_multi_index_container(allocator_type& al, chaindb_controller& controller)
       : impl(&al, controller), allocator(al)
       { }

   private:
       allocator_type allocator;
   };

} } // namespace cyberway::chaindb
