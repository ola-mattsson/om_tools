
#include "ip_socket.hpp"
#include "file_descriptor.hpp"
#include <sys/file.h>
#include <unistd.h>

#include <iostream>
#include <vector>
#include <array>
#include <string_view>
#include <thread>
#include <optional>
#include <atomic>

void file_socket();
void unix_socket(std::vector<std::string_view> &args);
void inet_socket(std::vector<std::string_view> &args);

int32_t main(int32_t argc, const char **argv) {
    std::vector<std::string_view> args(argv, argv + argc);

    std::cout << "unix domain socket\n";
    unix_socket(args);
    std::cout << "------------\n";

    std::cout << "tcp socket\n";
    inet_socket(args);
    std::cout << "------------\n";

    std::cout << "Sockets done\n";

    file_socket();
}

/**
 * Returning a file_descriptor object does not close it!
 *
 * This shows how the file_descriptor object will be moved out instead of copied. The move constructor
 * will invalidate the original object so it doesn't close the file when this scope closes.
 * @param name file name
 * @param open flags
 * @param mode access of the new file
 * @return the new file_descriptor object
 */
om_tools::file_descriptor open_file(std::string_view name, int32_t flags, int32_t mode) {
    om_tools::file_descriptor file_d(open(name.data(), flags, mode));
    if (!file_d.valid()) {
        perror("open");
        // return a new, invalid
        return {};
    }
    else {
        // moved out since it can't be copied.
        // if the descriptor could be copied the original
        // might be closed when going out of scope
        return file_d;
    }
}

/**
 * This shows how the file_socket will always close automatically if not returned.
 * The file_descriptor doesn't do much more than closing the file when it should,
 * and not when it shouldn't. the file_descriptor is only a typedef of the basic
 * descriptor_base class but will probably change.
 * Calling open_file is ofc pointless here.
 */
void file_socket() {
    auto file_d = open_file("a_file.txt", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (file_d.valid()) {
        std::string_view data = "Something important";
        write(file_d.get(), data.data(), data.size());
    }
}

/**
 * create a client thread, a server thread or both so they can talk to each other
 * @param args a vector of args
 */
void unix_socket(std::vector<std::string_view> &args) {

    std::atomic<bool> server_up = false;
    std::string_view name("unix_socket_example.server");

    auto server_worker = [&server_up, name]() {
        auto server = om_tools::Socket::create_uds_server_socket(name);
        while (server.valid()) {
            server_up = true;
            auto client = server.wait_request();
            if (client.valid()) {
                std::string_view msg("This is the message");
                auto write_result = write(client.get(), msg.data(), msg.size());
                std::cout << "Server sent " << write_result << " bytes\n";
                break;
            }
        }
    };

    auto client_worker = [&server_up, name]() {
        // hack to make sure server thread is up
        while (!server_up) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }

        auto client = om_tools::Socket::create_uds_client_socket(name);

        if (client.valid()) {
            std::array<char, 100> buff{};
            auto rcv_bytes = read(client.get(), &buff[0], buff.size());
            std::cout << "client rcv bytes: " << rcv_bytes << " content: " << buff.data() << '\n';
        }
    };

    enum {
        SERVER = '1',
        CLIENT = '2',
        SERVER_AND_CLIENT = '3'
    };

    using optional_threads = std::pair<std::optional<std::thread>, std::optional<std::thread>>;

    auto [server, client] = [&](auto &&args) -> optional_threads {
        switch (args.size() == 1 ? '3' : args.at(1)[0]) {
            case SERVER:
                return {std::thread(server_worker), std::nullopt};
            case CLIENT:
                return {std::nullopt, std::thread(client_worker)};
            case SERVER_AND_CLIENT:
            default:
                return {std::thread(server_worker), std::thread(client_worker)};
        }
    }(args);

    if (server) server->join();
    if (client) client->join();

    std::cout << "done\n";
}

/**
 * create a client thread, a server thread or both so they can talk to each other
 * @param args a vector of args
 */
void inet_socket(std::vector<std::string_view> &args) {

    std::atomic<bool> server_up = false;

    auto server_worker = [&server_up]() {
        auto server = om_tools::Socket::create_tcp_server_socket("12345");
        while (server.valid()) {
            server_up = true;
            auto client = server.wait_request();
            if (client.valid()) {
                std::string_view msg("This is the message");
                auto write_result = write(client.get(), msg.data(), msg.size());
                std::cout << "Server sent " << write_result << " bytes\n";
                break;
            }
        }
    };

    auto client_worker = [&server_up]() {
        // hack to make sure server thread is up
        while (!server_up) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }

        auto client = om_tools::Socket::create_tcp_client_socket("0.0.0.0", "12345");

        if (client.valid()) {
            std::array<char, 100> buff{};
            auto rcv_bytes = read(client.get(), &buff[0], buff.size());
            std::cout << "client rcv bytes: " << rcv_bytes << " content: " << buff.data() << '\n';
        }
    };

    enum {
        SERVER = '1',
        CLIENT = '2',
        SERVER_AND_CLIENT = '3'
    };

    using optional_threads = std::pair<std::optional<std::thread>, std::optional<std::thread>>;

    auto [server, client] = [&](auto &&args) -> optional_threads {
        switch (args.size() == 1 ? '3' : args.at(1)[0]) {
            case SERVER:
                return {std::thread(server_worker), std::nullopt};
            case CLIENT:
                return {std::nullopt, std::thread(client_worker)};
            case SERVER_AND_CLIENT:
            default:
                return {std::thread(server_worker), std::thread(client_worker)};
        }
    }(args);

    if (server) server->join();
    if (client) client->join();

    std::cout << "done\n";
}
