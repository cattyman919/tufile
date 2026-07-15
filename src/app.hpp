#pragma once

#include <cstddef>
#include <filesystem>
#include <ftxui/component/app.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <string>
#include <vector>

namespace tufile {

constexpr size_t TOTAL_KEYBINDS = 5;

class App final {
public:
  bool show_hidden_files{false};

  struct Action {
    std::vector<ftxui::Event> keys;
    std::string description;
    std::function<bool(App&)> action;
  };

  static const std::array<Action, TOTAL_KEYBINDS>& get_keybinds();

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

  std::filesystem::path cwd{std::filesystem::current_path()};

  std::vector<std::filesystem::directory_entry> cwd_entries;
  std::vector<std::filesystem::directory_entry> parent_entries;

  std::vector<std::string> cwd_entry_names;
  std::vector<std::string> parent_entry_names;

  int selected_index{0};

  auto get_file_entries(const std::filesystem::path& path)
      -> const std::vector<std::filesystem::directory_entry>;

  auto handle_event(ftxui::Event event) -> bool;

  // update current refresh state
  auto update_path(const std::filesystem::path& path) -> void;

  // update to a new directory path
  // auto next_dir_state(const std::filesystem::path& path) -> void;
  // auto prev_dir_state(const std::filesystem::path& path) -> void;

  /// View
  auto header_view() -> const ftxui::Element;
  auto footer_view() -> const ftxui::Element;
  auto left_pane_view() -> const ftxui::Element;
  auto middle_pane_view() -> const ftxui::Component;
  auto right_pane_view() -> const ftxui::Element;
};
}; // namespace tufile
