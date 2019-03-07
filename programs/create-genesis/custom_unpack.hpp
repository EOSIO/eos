#pragma once
// declares types and unpackers before fc/io/raw.hpp included (to be visible by compiler)
#include <string>
#include <vector>
#include <map>

namespace cw {
namespace golos {
struct comment_object;
}

template<int N>
struct gls_shared_str {
    std::string value;
};
struct gls_acc_name {
    std::string value;
};
inline bool operator<(const gls_acc_name& lhs, const gls_acc_name& rhs) {
    return lhs.value < rhs.value;
}
struct gls_asset {
    int64_t value;
    char symbol[8];
};

}

namespace fc {

namespace raw {
template<typename T> void unpack(T&, std::string&);
template<typename T> void unpack(T&, cw::golos::comment_object&);
}

template<typename S> S& operator>>(S&, cw::gls_acc_name&);
template<typename S, int N> S& operator>>(S&, cw::gls_shared_str<N>&);

template<typename S>
inline S& operator>>(S& s, cw::gls_asset& a) {
    s.read((char*)&a, sizeof(a));
    return s;
}


}
