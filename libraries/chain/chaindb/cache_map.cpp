#include <cyberway/chaindb/cache_map.hpp>
#include <cyberway/chaindb/controller.hpp>

namespace cyberway { namespace chaindb {

    struct cache_item_compare {
        bool operator()(const service_state& l, const service_state& r) const {
            if (l.pk    < r.pk   ) return true;
            if (l.pk    > r.pk   ) return false;

            if (l.table < r.table) return true;
            if (l.table > r.table) return false;

            if (l.code  < r.code ) return true;
            if (l.code  > r.code ) return false;

            return l.scope < r.scope;
        }

        bool operator()(const cache_item& l, const cache_item& r) const {
            return (*this)(l.object().service, r.object().service);
        }

        bool operator()(const cache_item& l, const service_state& r) const {
            return (*this)(l.object().service, r);
        }

        bool operator()(const service_state& l, const cache_item& r) const {
            return (*this)(l, r.object().service);
        }
    }; // struct cache_compare

    bool operator<(const cache_item& l, const cache_item& r) {
        return cache_item_compare()(l, r);
    }

    struct cache_service_key final {
        account_name code;
        table_name table;

        cache_service_key(const object_value& obj)
        : code(obj.service.code), table(obj.service.table) {
        }

        cache_service_key(const table_info& tab)
        : code(tab.code), table(tab.table->name) {
        }

        cache_service_key(cache_service_key&&) = default;
        cache_service_key(const cache_service_key&) = default;

        friend bool operator<(const cache_service_key& l, const cache_service_key& r) {
            if (l.code < r.code) return true;
            if (l.code > r.code) return false;
            return l.table < r.table;
        }
    }; // cache_service_key

    struct cache_service_info final {
        int cache_item_cnt = 0;
        primary_key_t next_pk = unset_primary_key;
        const cache_converter_interface* converter = nullptr; // exist only for interchain tables

        bool empty() const {
            assert(cache_item_cnt >= 0);
            return (converter == nullptr /* info for contacts tables */) && (cache_item_cnt == 0);
        }
    }; // struct cache_service_info

    struct table_cache_map final {
        ~table_cache_map() {
            clear();
        }

        cache_item* find(const service_state& key) {
            auto itr = cache_tree_.find(key, cache_item_compare());
            if (cache_tree_.end() != itr) {
                return &(*itr);
            }

            return nullptr;
        }

        cache_item_ptr emplace(object_value obj) {
            auto& cache_service = get_cache_service(obj);
            auto  cache_ptr     = cache_item_ptr(find(obj.service));

            if (!cache_ptr) {
                cache_ptr = cache_item_ptr(new cache_item(*this, std::move(obj)));

                // self incrementing of ref counter allows to use object w/o an additional storage
                //   self decrementing happens in remove()
                intrusive_ptr_add_ref(cache_ptr.get());


                cache_tree_.insert(*cache_ptr.get());
                cache_service.cache_item_cnt++;
            } else {
                // updating of an exist cache item
                cache_ptr->set_object(std::move(obj));

                auto lru_itr = lru_list_.iterator_to(*cache_ptr.get());
                lru_list_.erase(lru_itr);
            }

            lru_list_.push_back(*cache_ptr.get());

            // emplace happens in three cases:
            //   1. create object for interchain tables ->  object().is_null()
            //   2. loading object from chaindb         -> !object().is_null()
            //   3. restore from undo state             -> !object().is_null()
            if (cache_service.converter && !cache_ptr->object().is_null()) {
                cache_ptr->data = cache_service.converter->convert_variant(*cache_ptr.get(), cache_ptr->object());
            }

            // TODO: cache size should be configurable
            if (lru_list_.size() > 100'000) {
                auto& cache_itm = lru_list_.front();
                cache_itm.mark_deleted();
            }

            return cache_ptr;
        }

        void remove(cache_item& itm) {
            // self decrementing of ref counter, see emplace()
            auto cache_ptr   = cache_item_ptr(&itm, false);
            auto tree_itr    = cache_tree_.iterator_to(itm);
            auto list_itr    = lru_list_.iterator_to(itm);
            auto service_itr = service_tree_.find(itm.object());

            cache_tree_.erase(tree_itr);
            lru_list_.erase(list_itr);

            if (service_tree_.end() == service_itr) {
                return;
            }

            service_itr->second.cache_item_cnt--;
            if (service_itr->second.empty()) {
                service_tree_.erase(service_itr);
            }
        }

        void remove(const service_state& key) {
            auto ptr = find(key);
            if (ptr) {
                ptr->mark_deleted();
            }
        }

        void set_revision(const service_state& key, const revision_t rev) {
            auto ptr = find(key);
            if (ptr) {
                ptr->set_revision(rev);
            }
        }

        primary_key_t get_next_pk(const table_info& table) {
            auto ptr = find_cache_service(table);
            if (ptr && unset_primary_key != ptr->next_pk) {
                return ptr->next_pk++;
            }
            return unset_primary_key;
        }

        void set_next_pk(const table_info& table, const primary_key_t pk) {
            auto ptr = find_cache_service(table);
            if (ptr) {
                ptr->next_pk = pk;
            }
        }

        void set_cache_converter(const table_info& table, const cache_converter_interface& converter) {
            get_cache_service(table).converter = &converter;
        }

        void clear() {
            cache_tree_.clear();
            while (!lru_list_.empty()) {
                // self decrementing of ref counter, see emplace()
                auto cache_ptr = cache_item_ptr(&lru_list_.front(), false);
                lru_list_.pop_front();
            }

            for (auto itr = service_tree_.begin(), etr = service_tree_.end(); etr != itr; ) {
                itr->second.cache_item_cnt = 0;
                if (itr->second.empty()) {
                    itr = service_tree_.erase(itr);
                } else {
                    ++itr;
                }
            }
        }

    private:
        using cache_tree_type   = boost::intrusive::set<cache_item>;
        using lru_list_type     = boost::intrusive::list<cache_item, boost::intrusive::constant_time_size<true>>;
        using service_tree_type = std::map<cache_service_key, cache_service_info>;

        cache_tree_type   cache_tree_;
        lru_list_type     lru_list_;
        service_tree_type service_tree_;

        template <typename Table>
        cache_service_info& get_cache_service(const Table& table) {
            return service_tree_.emplace(cache_service_key(table), cache_service_info()).first->second;
        }

        template <typename Table>
        cache_service_info* find_cache_service(const Table& table) {
            auto itr = service_tree_.find({table});
            if (service_tree_.end() != itr) {
                return &itr->second;
            }
            return nullptr;
        }
    }; // struct table_cache_map

    void cache_item::mark_deleted() {
        assert(table_cache_map_);
        auto& map = *table_cache_map_;
        table_cache_map_ = nullptr;
        map.remove(*this);
    }

    cache_map::cache_map()
    : impl_(new table_cache_map()) {
    }

    cache_map::~cache_map() = default;

    void cache_map::set_cache_converter(const table_info& table, const cache_converter_interface& converter) const  {
        impl_->set_cache_converter(table, converter);
    }

    cache_item_ptr cache_map::create(const table_info& table) const {
        auto pk = impl_->get_next_pk(table);
        if (BOOST_UNLIKELY(unset_primary_key == pk)) {
            return {};
        }
        return impl_->emplace({{table, pk}, {}});
    }

    void cache_map::set_next_pk(const table_info& table, const primary_key_t pk) const {
        impl_->set_next_pk(table, pk);
    }

    cache_item_ptr cache_map::find(const table_info& table, const primary_key_t pk) const {
        return cache_item_ptr(impl_->find({table, pk}));
    }

    cache_item_ptr cache_map::emplace(const table_info& table, object_value obj) const {
        assert(obj.pk() != unset_primary_key && !obj.is_null());
        return impl_->emplace(std::move(obj));
    }

    void cache_map::remove(const table_info& table, const primary_key_t pk) const {
        assert(pk != unset_primary_key);
        impl_->remove({table, pk});
    }

    void cache_map::set_revision(const object_value& obj, const revision_t rev) const {
        assert(obj.pk() != unset_primary_key && impossible_revision != rev);
        impl_->set_revision(obj.service, rev);
    }

    void cache_map::clear() {
        impl_->clear();
    }

    //-----------

    cache_converter_interface::cache_converter_interface() = default;

    cache_converter_interface::~cache_converter_interface() = default;

} } // namespace cyberway::chaindb
