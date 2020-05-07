#include <functional>
#include <mutex>
#include <set>
#include <thread>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

typedef websocketpp::server<websocketpp::config::asio> server;

using websocketpp::connection_hdl;

class count_server {
public:
    count_server() : m_count(0) {
        m_server.init_asio();
                
        m_server.set_open_handler(bind(&count_server::on_open,this,_1));
        m_server.set_close_handler(bind(&count_server::on_close,this,_1));
    }
    
    void on_open(connection_hdl hdl) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_connections.insert(hdl);
    }
    
    void on_close(connection_hdl hdl) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_connections.erase(hdl);
    }
    
    void count() {
        while (1) {
            sleep(1);
            m_count++;
            
            std::stringstream ss;
            ss << m_count;
            
            std::lock_guard<std::mutex> lock(m_mutex);    
            for (auto it : m_connections) {
                m_server.send(it,ss.str(),websocketpp::frame::opcode::text);
            }
        }
    }
    
    void run(uint16_t port) {
        m_server.listen(port);
        m_server.start_accept();
        m_server.run();
    }
private:
    typedef std::set<connection_hdl,std::owner_less<connection_hdl>> con_list;
    
    int m_count;
    server m_server;
    con_list m_connections;
    std::mutex m_mutex;
};

int main() {
    count_server server;
    std::thread t(std::bind(&count_server::count,&server));
    server.run(9002);
}