#include <string>
#include <vector>

#include "Application.hpp"

int main(int argc, char* argv[]) {
    auto arguments = std::vector<std::string>();
    if (argc > 1) {
        arguments.assign(argv + 1, argv + argc);
    }

    auto application = Bloom::Application();
    return application.run(arguments);
}
