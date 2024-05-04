#include <iostream>
#include <memory>
#include <mutex>
using namespace std;

// 利用 std::call_once() 函数对 类 X 的数据成员实施线程安全的延迟初始化

class connection_info
{};
class connection_handle
{
public:
    void send_data(const data_packet&);
    data_packet receive_data();

};

class ConnectionManager
{
public:
   connection_handle open(const connection_info& ci);
   
};
class data_packet
{};
ConnectionManager connection_manager;

class X
{
private:
    connection_info connection_details;
    connection_handle connection;

    std::once_flag connection_init_flag;  // 类似 std::mutex 既不可复制，也不可移动
    void open_connection()  // 初始化数据
    {
        connection = connection_manager.open(connection_details);
    }
public:
    X(connection_info const& con_details) : connection_details(con_details) 
    {}
    void send_data(data_packet const& data)  // 首次调用时，借助 open_connection 进行数据初始化，该函数需要 this 指针，所以需要传入 this 指针
    {
        std::call_once(connection_init_flag, &X::open_connection, this);
        connection.send_data(data);
    }

    data_packet receive_data()  // 首次调用时，借助 open_connection 进行数据初始化
    {
        std::call_once(connection_init_flag, &X::open_connection, this);
        return connection.receive_data();
    }
};