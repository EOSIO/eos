// extern "C" {
// #include "Config.h"
// }
#include <assert.h>
#include <fc/log/logger.hpp>
#include "connection_pool.h"

using namespace zdbcpp;

#define BSIZE 2048

#if HAVE_STRUCT_TM_TM_GMTOFF
#define TM_GMTOFF tm_gmtoff
#else
#define TM_GMTOFF tm_wday
#endif

static void TabortHandler(const char *error) {
        fprintf(stdout, "Error: %s\n", error);
        exit(1);
}

namespace eosio {

    connection_pool::connection_pool(const std::string& uri){
        m_pool = std::make_shared<zdbcpp::ConnectionPool>(uri);
        assert(m_pool);
        m_pool->start();
    }

    connection_pool::~connection_pool()
    {
        assert(m_pool);
        m_pool->stop();
        URL_free(&uri);
    }

    Connection connection_pool::get_connection() {
        assert(m_pool);

        return m_pool->getConnection();
    }

    void connection_pool::release_connection(zdbcpp::Connection& con) {
        assert(m_pool);
        assert(con);
        // m_pool->returnConnection(con);
    }
}