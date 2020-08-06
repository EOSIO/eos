#include <b1/session/bytes.hpp>
#include <b1/session/key_value.hpp>

using namespace b1::session;

const key_value key_value::invalid{};

auto key_value::key() const -> const bytes&
{
    return m_key;
}

auto key_value::value() const -> const bytes&
{
    return m_value;
}

auto key_value::operator==(const key_value& other) const -> bool
{
    return m_key == other.m_key && m_value == other.m_value;
}

auto key_value::operator!=(const key_value& other) const -> bool
{
    return !(*this == other);
}
