#include <string_view>

#include <boost/pool/pool.hpp>

#include <b1/session/bytes.hpp>

using namespace b1::session;

const bytes bytes::invalid{};

bytes::bytes(const bytes& b)
: m_memory_allocator_address{b.m_memory_allocator_address},
  m_use_count_address{b.m_use_count_address},
  m_length{b.m_use_count_address},
  m_data{b.m_data}
{
    ++(*m_use_count_address);
}

bytes::bytes(bytes&& b)
: m_memory_allocator_address{std::move(b.m_memory_allocator_address)},
  m_use_count_address{std::move(b.m_use_count_address)},
  m_length{std::move(b.m_use_count_address)},
  m_data{std::move(b.m_data)}
{
    b.m_use_count_address = nullptr;
    b.m_length = nullptr;
    b.m_data = nullptr;
}

bytes::~bytes()
{
    if (!m_data || !m_length || !m_use_count_address)
    {
        return;
    }
    
    if (--(*m_use_count_address) != 0)
    {
        return;
    }
    
    // TODO:  bytes shouldn't need to know the memory allocator in use.  Need a generic interface to cast to.
    // The memory pool must remain in scope for the lifetime of this instance.
    auto* free_function = reinterpret_cast<free_function_type*>(*m_memory_allocator_address);
    (*free_function)(m_data, 3 * sizeof(size_t) + *m_length);
}

auto bytes::operator=(const bytes& b) -> bytes&
{
    if (this == &b)
    {
        return *this;
    }
    
    m_memory_allocator_address = b.m_memory_allocator_address;
    m_use_count_address = b.m_use_count_address;
    m_length = b.m_length;
    m_data = b.m_data;
    
    ++(*m_use_count_address);
    
    return *this;
}

auto bytes::operator=(bytes&& b) -> bytes&
{
    if (this == &b)
    {
        return *this;
    }
    
    m_memory_allocator_address = b.m_memory_allocator_address;
    m_use_count_address = b.m_use_count_address;
    m_length = b.m_length;
    m_data = b.m_data;
    
    b.m_memory_allocator_address = nullptr;
    b.m_use_count_address = nullptr;
    b.m_length = nullptr;
    b.m_data = nullptr;
    
    return *this;
}

auto bytes::data() const -> const void * const
{
    return m_data;
}

auto bytes::length() const -> size_t
{
    if (!m_length)
    {
        return 0;
    }
    
    return *m_length;
}

auto bytes::operator==(const bytes& other) const -> bool
{
    if (m_length && other.m_length && m_data && other.m_data)
    {
        if (*m_length != *other.m_length)
        {
            return false;
        }
        
        return memcmp(m_data, other.m_data, *m_length) == 0 ? true : false;
    }
    
    return false;
}

auto bytes::operator!=(const bytes& other) const -> bool
{
    return !(*this == other);
}
