#pragma once

#include <ftxui/component/app.hpp>

namespace tufile {

class App final {
public:
  App() {}
  // Remove copy constructor
  App(const App&) = delete;
  // Remove copy assignment
  App& operator=(const App&) = delete;
  // Remove copy assignment (r-value)
  App& operator=(const App&&) = delete;

  auto run() const -> void;

private:
  ftxui::App terminalApp = ftxui::App::TerminalOutput();
  ;
};

} // namespace tufile
