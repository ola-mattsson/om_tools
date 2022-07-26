
#include <iostream>
#include <vector>
#include <array>
#include <string_view>
#include <thread>
#include <optional>

#include <descriptors.h>
#include <sys/file.h>

std::atomic<bool> server_up = false;

void inet_socket(int argc, char **argv);
void file_socket();

int main(int argc, char **argv) {
    inet_socket(argc, argv);

    file_socket();
}

om_tools::file_socket open_file(std::string_view name, int32_t flags) {
    om_tools::file_socket file_d(open("a_file.txt", flags));
    if (!file_d.valid()) {
        perror("open");
        // return a new, invalid
        return {};
    }
    else {
        // moved out since it can't be copied.
        // if the descriptor could be copied the original
        // would be closed when going out of scope
        return file_d;
    }
}

void file_socket() {
    auto file_d = open_file("a_file.txt", O_WRONLY);
    std::string_view data = "Something important";
    write(file_d.get(), data.data(), data.size());
}

void inet_socket(int argc, char **argv) {
    const std::vector<const std::string_view> args(argv, argv + argc);

    auto server_worker = []() {
        auto server = om_tools::socket_fd::create_server_socket("12345");
        while (server.valid()) {
            server_up = true;
            auto client = server.wait_request();
            if (client.valid()) {
                auto write_result = write(client.get(), "HEJ", 3);
                std::cout << "Server sent " << write_result << " bytes\n";
                break;
            }
        }
    };

    auto client_worker = []() {
        // hack to make sure server thread is up
        while (!server_up) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }

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
