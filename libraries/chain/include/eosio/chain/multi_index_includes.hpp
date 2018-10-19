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

#include "multi_index.hpp"

namespace bmi = boost::multi_index;
using bmi::indexed_by;
using bmi::ordered_unique;
using bmi::ordered_non_unique;
using bmi::composite_key;
using bmi::member;
using bmi::const_mem_fun;
using bmi::tag;
using bmi::composite_key_compare;

template<typename A>
struct get_result_type {
    using type = typename A::result_type;
};

template<typename Value, typename... Fields>
struct composite_key2 {
    using result_type = bool;

    using key_type = boost::tuple<typename get_result_type<Fields>::type...>;

    auto operator()(const Value& v) {
        return boost::make_tuple(Fields()(v)...);
        //return key_type();
    }
};

template<typename Tag,typename KeyFromValue, typename CompareType = std::less<typename KeyFromValue::result_type>>
struct ordered_unique2 {
   using tag_type = Tag;
   using key_from_value_type = KeyFromValue;
   using compare_type = CompareType;
   using extractor_type = KeyFromValue;

};

struct by_id {};

namespace eosio { namespace chain {

struct by_parent {};
struct by_owner {};
struct by_name {};
struct by_action_name {};
struct by_permission_name {};

} } // namespace eosio::chain

namespace eosio { namespace chain { namespace resource_limits {

struct by_owner {};

} } } // namespace eosio::chain::resource_limits;

template<typename... Items>
struct indexed_by2 {};

namespace chaindb {

    template<typename Object,typename Allocator,typename Items>
    struct multi_index_container;

    struct object_id_extractor {
        template<typename T>
        uint64_t operator()(const T& o) const {
            return o.id._id;
        }
    
    };

    template<typename Object,typename Allocator,typename... Items>
    struct multi_index_container<Object,Allocator,indexed_by2<Items...>> {
        using impl = multi_index_impl<tag<Object>,PrimaryIndex<tag<by_id>,object_id_extractor>,Object,Allocator,Items...>;
    };

} // namespace chaindb


namespace chainbase {

   template<typename Object, typename IndexSpec>
   struct shared_multi_index_container2 {
       struct node_type {};
       using value_type = Object;
       using allocator_type = chainbase::allocator<value_type>;
       using container_impl = typename chaindb::multi_index_container<Object,chainbase::allocator<value_type>,IndexSpec>::impl;

       container_impl impl;

       using size_type = std::size_t;

       template<typename Tag, typename Extractor>
       struct index {
           
           using index_impl = typename container_impl::template index<Tag,Extractor>;
           struct type {
               using iterator = typename index_impl::const_iterator;
               
               index_impl impl;

               type(const shared_multi_index_container2* midx)
               : impl(&midx->impl)
               {}

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
               std::pair<iterator,iterator> equal_range(const Key& key) const {return impl.equal_range(key);}

               iterator iterator_to(const value_type& value) const {return impl.iterator_to(value);}

               bool empty() const {return impl.begin() == impl.end();}
               size_t size() const {return 0;}
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
           auto inx = impl.template get_index<Tag>();
           return typename index<chaindb::tag<Tag>,typename decltype(inx)::extractor_type>::type(this);
       }


       template<typename Type>
       iterator find(const oid<Type>& k) const {return impl.find(k._id);}

       template<typename Modifier>
       bool modify(iterator position, Modifier mod) {
           impl.modify(position, 0, mod);
           return true;
       }

       iterator erase(iterator position) {return impl.erase(position);}

       template<typename... Args>
       emplace_return_type emplace(Args&&... args) {
           try {
               Object obj(args...);
               auto iter = impl.emplace(0, [&](Object& o) {
                   o = obj;
               });
               return std::make_pair(iter, true);
           } catch (const fc::exception& err) {
               return std::make_pair(end(),false);
           }
       }

       iterator iterator_to(const value_type& x) const {return impl.iterator_to(x);}

       size_type size() const {return 0;}
       iterator end() {return impl.end();}
       const_iterator end() const {return impl.end();}

       allocator_type get_allocator() const {return allocator;}


       template<typename Tag,typename IteratorType>
       auto 
       project(IteratorType it) {
           return get<Tag>().iterator_to(*it);
       }

       template<typename Tag,typename IteratorType>
       auto
       project(IteratorType it) const {
           return get<Tag>().iterator_to(*it);
       }


       explicit shared_multi_index_container2(allocator_type& al) 
       : impl(0, 0, &al), allocator(al)
       {}
   private:
       allocator_type allocator;
   };

} // namespace chainbase
