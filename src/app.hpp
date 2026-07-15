#pragma once

#include <ftxui/component/app.hpp>
#include <string>
#include <tuple>
#include <vector>

namespace tufile {

using Keys = std::vector<std::string>;
using Action = std::string;
using HelperKey = std::tuple<Keys, Action>;

const std::array<HelperKey, 1> HELPER_KEYS{
    HelperKey{{"Up", "Down", "j", "k"}, "Navigate"}};

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
};

} // namespace tufile
