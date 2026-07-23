#pragma once

#include <string>

namespace CLI {
class App;
}

namespace taskpad {

struct GlobalOptions {
  std::string tasksDir;
};

int runCLI(int argc, char** argv);

} // namespace taskpad
