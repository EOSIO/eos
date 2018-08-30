/*
* Copyright(c) 2016 dragon jiang<jianlinlong@gmail.com>
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files(the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions :
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/

#ifndef _ZDB_CPP_H_
#define _ZDB_CPP_H_

#define ZDBCPP_BEGIN namespace zdbcpp {
#define ZDBCPP_END   }

#include <zdb/zdb.h>
#include <string>
#include <utility>
#include <stdexcept>

ZDBCPP_BEGIN

//SQLException
class sql_exception : public std::runtime_error
{
public:
    sql_exception(const char* msg = "SQLException")
        : std::runtime_error(msg)
    {}
};

#define except_wrapper(f) TRY { f; } CATCH(SQLException) {throw sql_exception(Exception_frame.message);} END_TRY

struct noncopyable
{
    noncopyable() = default;

    // make it noncopyable
    noncopyable(noncopyable const&) = delete;
    noncopyable& operator=(noncopyable const&) = delete;

    // make it not movable
    noncopyable(noncopyable&&) = delete;
    noncopyable& operator=(noncopyable&&) = delete;
};


class URL: private noncopyable
{
public:
    URL(const std::string& url)
        :URL(url.c_str())
    {}

    URL(const char *url) {
        t_ = URL_new(url);
    }

    ~URL() {
        URL_free(&t_);
    }

    operator URL_T() {
        return t_;
    }

public:
    const char *getProtocol() const {
        except_wrapper(return URL_getProtocol(t_));
    }

    const char *getUser() const {
        except_wrapper(return URL_getUser(t_));
    }

    const char *getPassword() const {
        except_wrapper(return URL_getPassword(t_));
    }

    const char *getHost() const {
        except_wrapper(return URL_getHost(t_));
    }

    int getPort() const {
        except_wrapper(return URL_getPort(t_));
    }

    const char *getPath() const {
        except_wrapper(return URL_getPath(t_));
    }

    const char *getQueryString() const {
        except_wrapper(return URL_getQueryString(t_));
    }

    const char **getParameterNames() const {
        except_wrapper(return URL_getParameterNames(t_));
    }

    const char *getParameter(const char *name) const {
        except_wrapper(return URL_getParameter(t_, name));
    }

    const char *toString() const {
        except_wrapper(return URL_toString(t_));
    }
    
    static char *unescape(char *url) {
        except_wrapper(return URL_unescape(url));
    }

    static char *escape(const char *url) {
        except_wrapper(return URL_escape(url));
    }

private:
    URL_T t_;
};

class ResultSet : private noncopyable
{
public:
    operator ResultSet_T() {
        return t_;
    }

    ResultSet(ResultSet&& r) 
        :t_(r.t_)
    {
        r.t_ = nullptr;
    }

    ResultSet& operator=(ResultSet&& r)
    {
        if (&r != this) {
            t_ = r.t_;
            r.t_ = nullptr;
        }
    }

protected:
    friend class PreparedStatement;
    friend class Connection;

    ResultSet(ResultSet_T t)
        :t_(t)
    {}

public:
    int getColumnCount() {
        except_wrapper( return ResultSet_getColumnCount(t_) );
    }

    const char *getColumnName(int columnIndex) {
        except_wrapper( return ResultSet_getColumnName(t_, columnIndex) );
    }

    long getColumnSize(int columnIndex) {
        except_wrapper( return ResultSet_getColumnSize(t_, columnIndex) );
    }

    int next() {
        except_wrapper( return ResultSet_next(t_) );
    }

    int isnull(int columnIndex) {
        except_wrapper( return ResultSet_isnull(t_, columnIndex) );
    }

    const char *getString(int columnIndex) {
        except_wrapper( return ResultSet_getString(t_, columnIndex) );
    }

    //note: blob field is ok when use mysql backend, but not worked when use oracle backend.
    //is it a bug of libzdb?
    const char *getStringByName(const char *columnName) {
        except_wrapper( return ResultSet_getStringByName(t_, columnName) );
    }

    int getInt(int columnIndex) {
        except_wrapper( return ResultSet_getInt(t_, columnIndex) );
    }

    int getIntByName(const char *columnName) {
        except_wrapper( return ResultSet_getIntByName(t_, columnName) );
    }

    long long getLLong(int columnIndex) {
        except_wrapper( return ResultSet_getLLong(t_, columnIndex) );
    }

    long long getLLongByName(const char *columnName) {
        except_wrapper( return ResultSet_getLLongByName(t_, columnName) );
    }

    double getDouble(int columnIndex) {
        except_wrapper( return ResultSet_getDouble(t_, columnIndex) );
    }

    double getDoubleByName(const char *columnName) {
        except_wrapper( return ResultSet_getDoubleByName(t_, columnName) );
    }

    const void *getBlob(int columnIndex, int *size) {
        except_wrapper( return ResultSet_getBlob(t_, columnIndex, size) );
    }

    const void *getBlobByName(const char *columnName, int *size) {
        except_wrapper( return ResultSet_getBlobByName(t_, columnName, size) );
    }

    time_t getTimestamp(int columnIndex) {
        except_wrapper( return ResultSet_getTimestamp(t_, columnIndex) );
    }

    time_t getTimestampByName(const char *columnName) {
        except_wrapper( return ResultSet_getTimestampByName(t_, columnName) );
    }

    struct tm getDateTime(int columnIndex) {
        except_wrapper( return ResultSet_getDateTime(t_, columnIndex) );
    }

    struct tm getDateTimeByName(const char *columnName) {
        except_wrapper( return ResultSet_getDateTimeByName(t_, columnName) );
    }

private:
    ResultSet_T t_;
};

class PreparedStatement : private noncopyable
{
public:
    operator PreparedStatement_T() {
        return t_;
    }

    PreparedStatement(PreparedStatement&& r)
        :t_(r.t_)
    {
        r.t_ = nullptr;
    }

    PreparedStatement& operator=(PreparedStatement&& r) {
        if (&r != this) {
            t_ = r.t_;
            r.t_ = nullptr;
        }
    }

protected:
    friend class Connection;

    PreparedStatement(PreparedStatement_T t)
        :t_(t)
    {}

public:
    void setString(int parameterIndex, const char *x) {
        except_wrapper( PreparedStatement_setString(t_, parameterIndex, x) );
    }

    void setInt(int parameterIndex, int x) {
        except_wrapper( PreparedStatement_setInt(t_, parameterIndex, x) );
    }

    void setLLong(int parameterIndex, long long x) {
        except_wrapper( PreparedStatement_setLLong(t_, parameterIndex, x) );
    }

    void setDouble(int parameterIndex, double x) {
        except_wrapper( PreparedStatement_setDouble(t_, parameterIndex, x) );
    }

    void setBlob(int parameterIndex, const void *x, int size) {
        except_wrapper( PreparedStatement_setBlob(t_, parameterIndex, x, size) );
    }

    void setTimestamp(int parameterIndex, time_t x) {
        except_wrapper( PreparedStatement_setTimestamp(t_, parameterIndex, x) );
    }

    void execute() {
        except_wrapper( PreparedStatement_execute(t_) );
    }

    ResultSet executeQuery() {
        except_wrapper(
            ResultSet_T r = PreparedStatement_executeQuery(t_);
            return ResultSet(r);
        );
    }

    long long rowsChanged() {
        except_wrapper( return PreparedStatement_rowsChanged(t_) );
    }

    int getParameterCount() {
        except_wrapper( return PreparedStatement_getParameterCount(t_) );
    }

public: //for c++ template to use
    void bind(int parameterIndex, const char *x) {
        this->setString(parameterIndex, x);
    }

    void bind(int parameterIndex, const std::string& x) {
        this->setString(parameterIndex, x.c_str());
    }

    void bind(int parameterIndex, int x) {
        this->setInt(parameterIndex, x);
    }

    void bind(int parameterIndex, long long x) {
        this->setLLong(parameterIndex, x);
    }

    void bind(int parameterIndex, double x) {
        this->setDouble(parameterIndex, x);
    }

    //blob
    void bind(int parameterIndex, const void *x, int size) {
        this->setBlob(parameterIndex, x, size);
    }

    void bind(int parameterIndex, time_t x) {
        this->setTimestamp(parameterIndex, x);
    }

    //bind args. note: this template function do not support to bind blob type
    template <typename ...Args>
    void bindArgs(Args... args) {
        do_bind_args(1, args...);
    }

private:
    template <typename T>
    void do_bind_args(int parameterIndex, T value) {
        bind(parameterIndex, value);
    }

    template <typename First, typename ...Args>
    void do_bind_args(int parameterIndex, First f, Args... args) {
        bind(parameterIndex, f);
        do_bind_args(parameterIndex + 1, args...);
    }

private:
    PreparedStatement_T t_;
};

class Connection : private noncopyable
{
public:
    operator Connection_T() {
        return t_;
    }

    ~Connection() {
        if (t_) {
            close();
        }
    }

    Connection(Connection&& r)
        :t_(r.t_)
        ,rows_changed_(r.rows_changed_)
    {
        r.t_ = nullptr;
        r.rows_changed_ = -1;
    }

    Connection& operator=(Connection&& r)
    {
        if (&r != this) {
            close();
            t_              = r.t_;
            rows_changed_   = r.rows_changed_;
            r.t_            = nullptr;
            r.rows_changed_ = -1;
        }
    }

protected:  // for ConnectionPool
    friend class ConnectionPool;

    Connection(Connection_T C)
        :t_(C)
        ,rows_changed_(-1)
    {}

    void setClosed() {
        t_ = nullptr;
    }

public:
    void setQueryTimeout(int ms) {
        except_wrapper( Connection_setQueryTimeout(t_, ms) );
    }

    int getQueryTimeout() {
        except_wrapper( return Connection_getQueryTimeout(t_) );
    }

    void setMaxRows(int max) {
        except_wrapper( Connection_setMaxRows(t_, max) );
    }

    int getMaxRows() {
        except_wrapper( return Connection_getMaxRows(t_) );
    }

    //not support
    //URL_T Connection_getURL(T C);

    int ping() {
        except_wrapper( return Connection_ping(t_) );
    }

    void clear() {
        except_wrapper( Connection_clear(t_) );
    }

    //after close(), t_ is set to NULL. so this Connection object can not be used again!
    void close() {
        if (t_) {
            except_wrapper( Connection_close(t_) );
            setClosed();
        }
    }

    void beginTransaction() {
        except_wrapper( Connection_beginTransaction(t_) );
    }

    void commit() {
        except_wrapper( Connection_commit(t_) );
    }

    void rollback() {
        except_wrapper( Connection_rollback(t_) );
    }

    long long lastRowId() {
        except_wrapper( return Connection_lastRowId(t_) );
    }

    long long rowsChanged() {
        except_wrapper(
            return (-1 == rows_changed_) ? Connection_rowsChanged(t_) : rows_changed_;
        );
    }

    void execute(const char *sql) {
        rows_changed_ = -1;
        except_wrapper(
            Connection_execute(t_, sql);
        );
    }

    template<typename ...Args>
    void execute(const char *sql, Args... args) {
        PreparedStatement p = this->prepareStatement(sql, args...);
        p.execute();
        rows_changed_ = p.rowsChanged();
    }

    ResultSet executeQuery(const char *sql) {
        except_wrapper(
            ResultSet_T r = Connection_executeQuery(t_, sql);
            return ResultSet(r);
        );
    }

    template<typename ...Args>
    ResultSet executeQuery(const char *sql, Args... args) {
        PreparedStatement p = this->prepareStatement(sql, args...);
        return p.executeQuery();
    }

    PreparedStatement prepareStatement(const char *sql) {
        except_wrapper(
            PreparedStatement_T r = Connection_prepareStatement(t_, sql);
            return PreparedStatement(r);
        );
    }

    template<typename ...Args>
    PreparedStatement prepareStatement(const char *sql, Args... args) {
        except_wrapper(
            PreparedStatement_T p = Connection_prepareStatement(t_, sql);
            PreparedStatement r(p);
            r.bindArgs(args...);
            return std::move(r);
        );
    }

    const char *getLastError() {
        except_wrapper( return Connection_getLastError(t_) );
    }

    static int isSupported(const char *url) {
        except_wrapper( return Connection_isSupported(url) );
    }

private:
    Connection_T t_;
    long long rows_changed_;
};


class ConnectionPool : private noncopyable
{
public:
    ConnectionPool(const std::string& url)
        :ConnectionPool(url.c_str())
    {}

    ConnectionPool(const char* url)
        :url_(url)
    {
        t_ = ConnectionPool_new(url_);
    }

    ~ConnectionPool() {
        ConnectionPool_free(&t_);
    }

    operator ConnectionPool_T() {
        return t_;
    }

public:
    const URL& getURL() { return url_;}

    void setInitialConnections(int connections) {
        except_wrapper( ConnectionPool_setInitialConnections(t_, connections) );
    }

    int getInitialConnections() {
        except_wrapper( return ConnectionPool_getInitialConnections(t_) );
    }

    void setMaxConnections(int maxConnections) {
        except_wrapper( ConnectionPool_setMaxConnections(t_, maxConnections) );
    }

    int getMaxConnections() {
        except_wrapper( return ConnectionPool_getMaxConnections(t_) );
    }

    void setConnectionTimeout(int connectionTimeout) {
        except_wrapper( ConnectionPool_setConnectionTimeout(t_, connectionTimeout) );
    }

    int getConnectionTimeout() {
        except_wrapper( return ConnectionPool_getConnectionTimeout(t_) );
    }

    void setAbortHandler(void(*abortHandler)(const char *error)) {
        except_wrapper( ConnectionPool_setAbortHandler(t_, abortHandler) );
    }

    void setReaper(int sweepInterval) {
        except_wrapper( ConnectionPool_setReaper(t_, sweepInterval) );
    }

    int size() {
        except_wrapper( return ConnectionPool_size(t_) );
    }

    int active() {
        except_wrapper( return ConnectionPool_active(t_) );
    }

    void start() {
        except_wrapper( ConnectionPool_start(t_) );
    }

    void stop() {
        except_wrapper( ConnectionPool_stop(t_) );
    }

    Connection getConnection() {
        except_wrapper(
            Connection_T C = ConnectionPool_getConnection(t_);
            if (NULL == C) {
                throw sql_exception("maxConnection is reached(got null connection)!");
            }
            return Connection(C);
        );
    }

    void returnConnection(Connection& con) {
        except_wrapper(
            con.setClosed();
            ConnectionPool_returnConnection(t_, con);
        );
    }

    int reapConnections() {
        except_wrapper( return ConnectionPool_reapConnections(t_) );
    }

    static const char *version(void) {
        except_wrapper( return ConnectionPool_version() );
    }

private:
    URL url_;
    ConnectionPool_T t_;
};


ZDBCPP_END

#endif
