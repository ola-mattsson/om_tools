
#include <iostream>
#include <vector>
#include <array>
#include <string_view>
#include <thread>
#include <optional>

#include <descriptors.h>
#include <sys/file.h>

std::atomic<bool> server_up = false;

void file_socket();
void inet_socket(const std::vector<const std::string_view> &args);

int main(int argc, char **argv) {
    const std::vector<const std::string_view> args(argv, argv + argc);

    inet_socket(args);

    file_socket();


    using fd = om_tools::file_descriptor;
    fd a;                       // file_descriptor
//    fd b = a;  // no assign
//    fd b1(a);  // no copy
    auto c = fd();              // file_descriptor move
    auto d = fd(open("", 0));   // file_descriptor move
    auto e(fd(open("", 0)));    // file_descriptor move
    auto f = open("", 0);       // int
    fd g(f);                    // file_descriptor move
}

/**
 * Returning a file_descriptor object does not close it!
 *
 * This shows how the file_descriptor object will be moved out instead of copied. The move constructor
 * will invalidate the original object so it doesn't close the file when this scope closes.
 * @param name file name
 * @param open flags
 * @return the new file_descriptor object
 */
om_tools::file_descriptor open_file(std::string_view name, int32_t flags) {
    om_tools::file_descriptor file_d(open(name.data(), flags));
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
    auto file_d = open_file("a_file.txt", O_WRONLY);
    if (file_d.valid()) {
        std::string_view data = "Something important";
        write(file_d.get(), data.data(), data.size());
    }
}

/**
 * create a client thread, a server thread or both so they can talk to each other
 * @param args a vector of args
 */
void inet_socket(const std::vector<const std::string_view> &args) {

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
