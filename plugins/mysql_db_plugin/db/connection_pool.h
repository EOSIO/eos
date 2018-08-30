#ifndef CONNECTION_H
#define CONNECTION_H

#include <memory>
#include "zdbcpp.h"

namespace eosio {
class connection_pool
{
    public:
        explicit connection_pool(const std::string& uri);
        ~connection_pool();
        
        zdbcpp::Connection get_connection();
        void release_connection(zdbcpp::Connection& con);

    private:
        std::shared_ptr<zdbcpp::ConnectionPool> m_pool;        
        URL_T uri;
};
}

#endif