#pragma once

#include <ftxui/component/app.hpp>
#include <string>
#include <vector>

namespace tufile {

class App final {
public:
  App() {}
  // Remove copy constructor
  App(const App&) = delete;
  // Remove copy assignment
  App& operator=(const App&) = delete;
  // Remove copy assignment (r-value)
  App& operator=(App&&) = delete;

  auto run() -> void;

private:
  inline static ftxui::App terminal_app{
      ftxui::App::FullscreenAlternateScreen()};

  bool show_hidden_files{false};

  auto handle_key_event(ftxui::Event event) -> bool;

  // 1. Define Action inside App
  struct Action {
    std::vector<std::string> keys;
    std::string description;
    std::function<bool(App&)> action;
  };

  static const std::array<Action, 2> keybinds;
};
}; // namespace tufile
