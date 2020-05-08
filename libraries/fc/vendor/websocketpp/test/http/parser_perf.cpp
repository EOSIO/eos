/*
 * Copyright (c) 2011, Peter Thorson. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the WebSocket++ Project nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL PETER THORSON BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <websocketpp/http/parser.hpp>

#include <chrono>

class scoped_timer {
public:
    scoped_timer(std::string i) : m_id(i),m_start(std::chrono::steady_clock::now()) {
        std::cout << "Clock " << i << ": ";
    }
    ~scoped_timer() {
        std::chrono::nanoseconds time_taken = std::chrono::steady_clock::now()-m_start;

        //nanoseconds_per_test

        //tests_per_second

        //1000000000.0/(double(time_taken.count())/1000.0)

        std::cout << 1000000000.0/(double(time_taken.count())/1000.0) << std::endl;

        //std::cout << (1.0/double(time_taken.count())) * double(1000000000*1000) << std::endl;
    }

private:
    std::string m_id;
    std::chrono::steady_clock::time_point m_start;
};

int main() {
    std::string raw = "GET / HTTP/1.1\r\nHost: www.example.com\r\n\r\n";

    std::string firefox = "GET / HTTP/1.1\r\nHost: localhost:5000\r\nUser-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10.7; rv:10.0) Gecko/20100101 Firefox/10.0\r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\nAccept-Language: en-us,en;q=0.5\r\nAccept-Encoding: gzip, deflate\r\nConnection: keep-alive, Upgrade\r\nSec-WebSocket-Version: 8\r\nSec-WebSocket-Origin: http://zaphoyd.com\r\nSec-WebSocket-Key: pFik//FxwFk0riN4ZiPFjQ==\r\nPragma: no-cache\r\nCache-Control: no-cache\r\nUpgrade: websocket\r\n\r\n";

    std::string firefox1 = "GET / HTTP/1.1\r\nHost: localhost:5000\r\nUser-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10.7; rv:10.0) Gecko/20100101 Firefox/10.0\r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\nAccept-Language: en-us,en;q=0.5\r\n";

    std::string firefox2 = "Accept-Encoding: gzip, deflate\r\nConnection: keep-alive, Upgrade\r\nSec-WebSocket-Version: 8\r\nSec-WebSocket-Origin: http://zaphoyd.com\r\nSec-WebSocket-Key: pFik//FxwFk0riN4ZiPFjQ==\r\nPragma: no-cache\r\nCache-Control: no-cache\r\nUpgrade: websocket\r\n\r\n";

    {
        scoped_timer timer("Simplest 1 chop");
        for (int i = 0; i < 1000; i++) {
            websocketpp::http::parser::request r;

            try {
                r.consume(raw.c_str(),raw.size());
            } catch (...) {
                std::cout << "exception" << std::endl;
            }

            if (!r.ready()) {
                std::cout << "error" << std::endl;
                break;
            }
        }
    }

    {
        scoped_timer timer("FireFox, 1 chop, consume old");
        for (int i = 0; i < 1000; i++) {
            websocketpp::http::parser::request r;

            try {
                r.consume2(firefox.c_str(),firefox.size());
            } catch (...) {
                std::cout << "exception" << std::endl;
            }

            if (!r.ready()) {
                std::cout << "error" << std::endl;
                break;
            }
        }
    }

    {
        scoped_timer timer("FireFox, 1 chop");
        for (int i = 0; i < 1000; i++) {
            websocketpp::http::parser::request r;

            try {
                r.consume(firefox.c_str(),firefox.size());
            } catch (...) {
                std::cout << "exception" << std::endl;
            }

            if (!r.ready()) {
                std::cout << "error" << std::endl;
                break;
            }
        }
    }



    {
        scoped_timer timer("FireFox, 2 chop");
        for (int i = 0; i < 1000; i++) {
            websocketpp::http::parser::request r;

            try {
                r.consume(firefox1.c_str(),firefox1.size());
                r.consume(firefox2.c_str(),firefox2.size());
            } catch (...) {
                std::cout << "exception" << std::endl;
            }

            if (!r.ready()) {
                std::cout << "error" << std::endl;
                break;
            }
        }
    }

    return 0;
}
