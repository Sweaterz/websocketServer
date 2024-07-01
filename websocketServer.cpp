// #include <websocketpp/config/asio_no_tls.hpp>
// #include <websocketpp/server.hpp>

// #include <iostream>
// #include <thread>
// #include <atomic>

// typedef websocketpp::server<websocketpp::config::asio> server;

// using websocketpp::connection_hdl;

// class websocket_server {
// public:
//     websocket_server() : sendFlag(false) {
//         // 初始化WebSocket服务器
//         ws_server.init_asio();

//         // 设置WebSocket事件处理函数
//         ws_server.set_open_handler(std::bind(&websocket_server::on_open, this, std::placeholders::_1));
//         ws_server.set_close_handler(std::bind(&websocket_server::on_close, this, std::placeholders::_1));
//         ws_server.set_message_handler(std::bind(&websocket_server::on_message, this, std::placeholders::_1, std::placeholders::_2));
//     }

//     void run(uint16_t port) {
//         // 启动服务器监听指定的端口
//         ws_server.listen(port);
//         ws_server.start_accept();

//         // 启动IO服务
//         ws_server.run();
//     }

// private:
//     void on_open(connection_hdl hdl) {
//         m_hdl = hdl;

//         // 启动一个线程来不断发送消息给客户端
//         sendFlag = true;
//         m_send_thread = std::thread(&websocket_server::send_message, this);
//     }

//     void on_close(connection_hdl hdl) {
//         std::cout << "Connection closed" << std::endl;

//         // 关闭发送消息线程
//         sendFlag = false;
//         if (m_send_thread.joinable()) {
//             try {
//                 m_send_thread.join();
//             } catch (const std::exception &e) {
//                 std::cerr << "Exception caught during thread join: " << e.what() << std::endl;
//             }
//         }
//     }

//     void on_message(connection_hdl hdl, server::message_ptr msg) {
//         std::cout << "Message received: " << msg->get_payload() << std::endl;
//     }

//     void send_message() {
//         try {
//             while (sendFlag) {
//                 // 发送信息给客户端
//                 std::string message = "Hello from C++ server:" + std::to_string(std::rand());
//                 ws_server.send(m_hdl, message, websocketpp::frame::opcode::text);
//                 std::this_thread::sleep_for(std::chrono::seconds(1)); // 等待1秒后再次发送信息
//             }
//         } catch (const websocketpp::exception &e) {
//             std::cerr << "Exception caught during message send: " << e.what() << std::endl;
//         } catch (const std::exception &e) {
//             std::cerr << "Exception caught during message send: " << e.what() << std::endl;
//         }
//     }

//     server ws_server;
//     connection_hdl m_hdl;
//     std::thread m_send_thread;
//     std::atomic<bool> sendFlag;
// };

// int main() {
//     websocket_server ws_server;

//     // 假设服务器监听9002端口
//     ws_server.run(9002);

//     return 0;
// }

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/common/thread.hpp>

#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>

typedef websocketpp::server<websocketpp::config::asio> server;

class websocket_server {
public:
    websocket_server() : sendFlag(false), client_connected(false) {
        // 初始化WebSocket服务器
        ws_server.init_asio();

        // 设置WebSocket事件处理函数

        ws_server.set_open_handler(websocketpp::lib::bind(&websocket_server::on_open, this, websocketpp::lib::placeholders::_1));
        ws_server.set_close_handler(websocketpp::lib::bind(&websocket_server::on_close, this, websocketpp::lib::placeholders::_1));
        ws_server.set_message_handler(websocketpp::lib::bind(&websocket_server::on_message, this, websocketpp::lib::placeholders::_1, websocketpp::lib::placeholders::_2));
    }

    void run(uint16_t port) {
        // 启动服务器监听指定的端口
        ws_server.listen(port);
        ws_server.start_accept();

        // 启动IO服务
        ws_server.run();
    }

private:
    void on_open(websocketpp::connection_hdl hdl) {
        std::lock_guard<std::mutex> guard(connection_lock);
        
        // 检查是否已经有一个客户端连接
        if (client_connected) {
            std::cout << "A client is already connected. New connection will be terminated." << std::endl;
            ws_server.close(hdl, websocketpp::close::status::normal, "Only one client is allowed");
            return;
        }
        
        client_connected = true;
        m_hdl = hdl;

        // 启动一个线程来不断发送消息给客户端
        sendFlag = true;
        m_send_thread = std::thread(&websocket_server::send_message, this);
    }

    void on_close(websocketpp::connection_hdl hdl) {
        std::lock_guard<std::mutex> guard(connection_lock);
        
        std::cout << "Connection closed" << std::endl;
        client_connected = false;
        
        // 关闭发送消息线程
        sendFlag = false;
        if (m_send_thread.joinable()) {
            try {
                m_send_thread.join();
            } catch (const std::system_error &e) {
                std::cerr << "System error caught during thread join: " << e.what() << std::endl;
            } catch (const std::exception &e) {
                std::cerr << "Exception caught during thread join: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "Unknown exception caught during thread join." << std::endl;
            }
        }
    }

    void on_message(websocketpp::connection_hdl hdl, server::message_ptr msg) {
        std::cout << "Message received: " << msg->get_payload() << std::endl;
    }

    void send_message() {
        try {
            while (sendFlag) {
                // 发送信息给客户端
                std::string message = "Hello from C++ server:" + std::to_string(std::rand());
                ws_server.send(m_hdl, message, websocketpp::frame::opcode::text);
                std::this_thread::sleep_for(std::chrono::seconds(1)); // 等待1秒后再次发送信息
            }
        } catch (const websocketpp::exception &e) {
            std::cerr << "WebSocket exception caught during message send: " << e.what() << std::endl;
        } catch (const std::exception &e) {
            std::cerr << "Exception caught during message send: " << e.what() << std::endl;
        }
    }

    server ws_server;
    websocketpp::connection_hdl m_hdl;
    std::thread m_send_thread;
    std::atomic<bool> sendFlag;
    std::atomic<bool> client_connected;
    std::mutex connection_lock;
};

int main() {
    websocket_server ws_server;

    // 假设服务器监听9002端口
    ws_server.run(9002);

    return 0;
}
