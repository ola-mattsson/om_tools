
#include <iostream>
#include <vector>
#include <array>
#include <string_view>
#include <thread>
#include <optional>

#include <descriptors.h>


int main(int argc, char **argv) {
    const std::vector<const std::string_view> args(argv, argv + argc);

    auto server_worker = []() {
        auto server = om_tools::socket_fd::create_server_socket(12345);
        while (server.valid()) {
            auto client = server.wait_request();
            if (client.valid()) {
                auto write_result = write(client.get(), "HEJ", 3);
                std::cout << "Server sent " << write_result << " bytes\n";
                break;
            }
        }
    };

    auto client_worker = []() {
        auto client = om_tools::socket_fd::create_client_socket("127.0.0.1", "12345");

        if (client.valid()) {
            std::array<char, 10> buff{};
            auto rcv_bytes = read(client.get(), &buff[0], 10);
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
        switch (args.empty() ? 3 : args.at(1)[0]) {
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
