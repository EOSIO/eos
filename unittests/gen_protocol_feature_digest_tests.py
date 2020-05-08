#!/usr/bin/env python3

import re
import hashlib
import sys

pattern = re.compile(r"\*\n(Builtin protocol feature: (.*)(?:\*[^/]|[^\*])*)\*/")

def main():
    with open(sys.argv[1], "r") as f:
        contents = f.read()
        print('#include <eosio/chain/protocol_feature_manager.hpp>')
        print('#include <map>')

        print('#include <boost/test/unit_test.hpp>')

        print('using namespace eosio::chain;')

        print('BOOST_AUTO_TEST_CASE(protocol_feature_digest_tests) {')
        print('   std::map<std::string, std::string> digests;')
        for match in re.finditer(pattern, contents):
            print('   digests.emplace("%s", "%s");' % (match.group(2),  hashlib.sha256(match.group(1).encode('utf8')).hexdigest()))
        print('   for(const auto& [id, spec] : builtin_protocol_feature_codenames) {')
        print('      BOOST_TEST(digests[spec.codename] == fc::variant(spec.description_digest).as<std::string>());')
        print('   }')
        print('}')

if __name__ == "__main__":
    main()
