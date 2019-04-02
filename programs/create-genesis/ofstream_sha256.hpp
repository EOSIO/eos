#pragma once
#include <fc/crypto/sha256.hpp>
#include <boost/filesystem/fstream.hpp>


namespace cyberway { namespace genesis {

namespace bfs = boost::filesystem;


class ofstream_sha256: public bfs::ofstream {
public:
    ofstream_sha256(): bfs::ofstream() {
    }
    ofstream_sha256(const bfs::path& p): bfs::ofstream(p, std::ios_base::binary) {
        bfs::ofstream::exceptions(std::ofstream::failbit | std::ofstream::badbit);
    }
    ~ofstream_sha256() {
    }

    template<typename T>
    void write(const T& x) {
        write((const char*)&x, sizeof(T));
    }
    void write(const char* p, uint32_t l) {
        _e.write(p, l);
        bfs::ofstream::write(p, l);
    }
    fc::sha256 hash() {
        return _e.result();
    }

private:
    fc::sha256::encoder _e;
};


}} // cyberway::genesis
