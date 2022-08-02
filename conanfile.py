from conans import ConanFile, CMake


class AppOlaToolsConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake_find_package"
    default_options = {"boost:header_only": True,
                       "fmt:header_only": True,
                       "libcurl:shared": False
                       }

    def requirements(self):
        self.requires("boost/1.76.0")
        # self.requires("gtest/cci.20210126")
        self.requires("gtest/cci.20210126")
        self.requires("zlib/1.2.12")
        self.requires("libpq/11.11")
        self.requires("libcurl/7.75.0")
        self.requires("hiredis/1.0.2")
        # redis async, pubsub needs libevent
        self.requires("libevent/2.1.12")
        self.requires("fmt/9.0.0")
        self.requires("openssl/1.1.1o")

    # def build_requirements(self):
    #     self.requires("cmake/3.21.4")

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
