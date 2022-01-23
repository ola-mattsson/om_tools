
from conans import ConanFile, CMake


class AppOlaToolsConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake_find_package"
    default_options = {"boost:header_only": True}

    def requirements(self):
        self.requires("boost/1.76.0")
        self.requires("zlib/1.2.11")
        self.requires("libpq/11.11")
        self.requires("libpqxx/7.7.0")
        self.requires("libcurl/7.80.0")
        self.requires("gtest/cci.20210126")

    # def build_requirements(self):
    #     self.requires("cmake/3.21.4")

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
