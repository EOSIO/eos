/*
 * Copyright (c) 2017, Respective Authors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/tag.hpp>

#include <fc/io/raw.hpp>
#include <fc/io/raw_variant.hpp>
#include <fc/log/logger.hpp>
#include <fc/io/json.hpp>

#include <eos/net/peer_database.hpp>



namespace eos { namespace net {
  namespace detail
  {
    using namespace boost::multi_index;

    class peer_database_impl
    {
    public:
      struct last_seen_time_index {};
      struct endpoint_index {};
      typedef boost::multi_index_container<potential_peer_record, 
                                           indexed_by<ordered_non_unique<tag<last_seen_time_index>, 
                                                                         member<potential_peer_record, 
                                                                                fc::time_point_sec, 
                                                                                &potential_peer_record::last_seen_time> >,
                                                      hashed_unique<tag<endpoint_index>, 
                                                                    member<potential_peer_record, 
                                                                           fc::ip::endpoint, 
                                                                           &potential_peer_record::endpoint>, 
                                                                    std::hash<fc::ip::endpoint> > > > potential_peer_set;

    private:
      potential_peer_set     _potential_peer_set;
      fc::path _peer_database_filename;

    public:
      void open(const fc::path& databaseFilename);
      void close();
      void clear();
      void erase(const fc::ip::endpoint& endpointToErase);
      void update_entry(const potential_peer_record& updatedRecord);
      potential_peer_record lookup_or_create_entry_for_endpoint(const fc::ip::endpoint& endpointToLookup);
      fc::optional<potential_peer_record> lookup_entry_for_endpoint(const fc::ip::endpoint& endpointToLookup);

      peer_database::iterator begin() const;
      peer_database::iterator end() const;
      size_t size() const;
    };

    class peer_database_iterator_impl
    {
    public:
      typedef peer_database_impl::potential_peer_set::index<peer_database_impl::last_seen_time_index>::type::iterator last_seen_time_index_iterator;
      last_seen_time_index_iterator _iterator;
      peer_database_iterator_impl(const last_seen_time_index_iterator& iterator) :
        _iterator(iterator)
      {}
    };
    peer_database_iterator::peer_database_iterator( const peer_database_iterator& c ) :
      boost::iterator_facade<peer_database_iterator, const potential_peer_record, boost::forward_traversal_tag>(c){}

    void peer_database_impl::open(const fc::path& peer_database_filename)
    {
      _peer_database_filename = peer_database_filename;
      if (fc::exists(_peer_database_filename))
      {
        try
        {
          std::vector<potential_peer_record> peer_records = fc::json::from_file(_peer_database_filename).as<std::vector<potential_peer_record> >();
          std::copy(peer_records.begin(), peer_records.end(), std::inserter(_potential_peer_set, _potential_peer_set.end()));
#define MAXIMUM_PEERDB_SIZE 1000
          if (_potential_peer_set.size() > MAXIMUM_PEERDB_SIZE)
          {
            // prune database to a reasonable size
            auto iter = _potential_peer_set.begin();
            std::advance(iter, MAXIMUM_PEERDB_SIZE);
            _potential_peer_set.erase(iter, _potential_peer_set.end());
          }
        }
        catch (const fc::exception& e)
        {
          elog("error opening peer database file ${peer_database_filename}, starting with a clean database", 
               ("peer_database_filename", _peer_database_filename));
        }
      }
    }

    void peer_database_impl::close()
    {
      std::vector<potential_peer_record> peer_records;
      peer_records.reserve(_potential_peer_set.size());
      std::copy(_potential_peer_set.begin(), _potential_peer_set.end(), std::back_inserter(peer_records));

      try
      {
        fc::path peer_database_filename_dir = _peer_database_filename.parent_path();
        if (!fc::exists(peer_database_filename_dir))
          fc::create_directories(peer_database_filename_dir);
        fc::json::save_to_file(peer_records, _peer_database_filename);
      }
      catch (const fc::exception& e)
      {
        elog("error saving peer database to file ${peer_database_filename}", 
             ("peer_database_filename", _peer_database_filename));
      }
      _potential_peer_set.clear();
    }

    void peer_database_impl::clear()
    {
      _potential_peer_set.clear();
    }

    void peer_database_impl::erase(const fc::ip::endpoint& endpointToErase)
    {
      auto iter = _potential_peer_set.get<endpoint_index>().find(endpointToErase);
      if (iter != _potential_peer_set.get<endpoint_index>().end())
        _potential_peer_set.get<endpoint_index>().erase(iter);
    }

    void peer_database_impl::update_entry(const potential_peer_record& updatedRecord)
    {
      auto iter = _potential_peer_set.get<endpoint_index>().find(updatedRecord.endpoint);
      if (iter != _potential_peer_set.get<endpoint_index>().end())
        _potential_peer_set.get<endpoint_index>().modify(iter, [&updatedRecord](potential_peer_record& record) { record = updatedRecord; });
      else
        _potential_peer_set.get<endpoint_index>().insert(updatedRecord);
    }

    potential_peer_record peer_database_impl::lookup_or_create_entry_for_endpoint(const fc::ip::endpoint& endpointToLookup)
    {
      auto iter = _potential_peer_set.get<endpoint_index>().find(endpointToLookup);
      if (iter != _potential_peer_set.get<endpoint_index>().end())
        return *iter;
      return potential_peer_record(endpointToLookup);
    }

    fc::optional<potential_peer_record> peer_database_impl::lookup_entry_for_endpoint(const fc::ip::endpoint& endpointToLookup)
    {
      auto iter = _potential_peer_set.get<endpoint_index>().find(endpointToLookup);
      if (iter != _potential_peer_set.get<endpoint_index>().end())
        return *iter;
      return fc::optional<potential_peer_record>();
    }

    peer_database::iterator peer_database_impl::begin() const
    {
      return peer_database::iterator(new peer_database_iterator_impl(_potential_peer_set.get<last_seen_time_index>().begin()));
    }

    peer_database::iterator peer_database_impl::end() const
    {
      return peer_database::iterator(new peer_database_iterator_impl(_potential_peer_set.get<last_seen_time_index>().end()));
    }

    size_t peer_database_impl::size() const
    {
      return _potential_peer_set.size();
    }

    peer_database_iterator::peer_database_iterator()
    {
    }

    peer_database_iterator::~peer_database_iterator()
    {
    }

    peer_database_iterator::peer_database_iterator(peer_database_iterator_impl* impl) :
      my(impl)
    {
    }

    void peer_database_iterator::increment()
    {
      ++my->_iterator;
    }

    bool peer_database_iterator::equal(const peer_database_iterator& other) const
    {
      return my->_iterator == other.my->_iterator;
    }

    const potential_peer_record& peer_database_iterator::dereference() const
    {
      return *my->_iterator;
    }

  } // end namespace detail

  peer_database::peer_database() :
    my(new detail::peer_database_impl)
  {
  }

  peer_database::~peer_database()
  {}

  void peer_database::open(const fc::path& databaseFilename)
  {
    my->open(databaseFilename);
  }

  void peer_database::close()
  {
    my->close();
  }

  void peer_database::clear()
  {
    my->clear();
  }

  void peer_database::erase(const fc::ip::endpoint& endpointToErase)
  {
    my->erase(endpointToErase);
  }

  void peer_database::update_entry(const potential_peer_record& updatedRecord)
  {
    my->update_entry(updatedRecord);
  }

  potential_peer_record peer_database::lookup_or_create_entry_for_endpoint(const fc::ip::endpoint& endpointToLookup)
  {
    return my->lookup_or_create_entry_for_endpoint(endpointToLookup);
  }

  fc::optional<potential_peer_record> peer_database::lookup_entry_for_endpoint(const fc::ip::endpoint& endpoint_to_lookup)
  {
    return my->lookup_entry_for_endpoint(endpoint_to_lookup);
  }

  peer_database::iterator peer_database::begin() const
  {
    return my->begin();
  }

  peer_database::iterator peer_database::end() const
  {
    return my->end();
  }

  size_t peer_database::size() const
  {
    return my->size();
  }

} } // end namespace eos::net
