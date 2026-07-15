#include "app.hpp"
#include <cassert>
#include <filesystem>
#include <ftxui/component/app.hpp>
#include <ftxui/component/component.hpp>

#include <ftxui/dom/deprecated.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/screen/string.hpp>

auto tufile::App::run() const -> void {

  auto current_path = std::filesystem::current_path();

  auto file_entries = std::filesystem::directory_iterator{current_path};

  std::vector<std::string> entries{};
  entries.reserve(entries.size());

  for (const auto& entry : file_entries) {
    entries.emplace_back(entry.path().filename());
  }

  // assert(entries.size() > 0 && "Has to have at least one entry");

  int selected = 0;

  // Create a menu component
  auto menu = ftxui::Menu(&entries, &selected);

  auto middle_pane = menu | ftxui::border;

  // Parent component
  auto left_pane =
      ftxui::Renderer([] { return ftxui::text("Left Pane") | ftxui::border; });

  // File / Directory Info
  auto right_pane =
      ftxui::Renderer([] { return ftxui::text("Right Pane") | ftxui::border; });

  auto component =
      ftxui::Container::Horizontal({left_pane, middle_pane, right_pane});

  // auto layout = ftxui::hbox({component, component, component});

  this->terminalApp.FullscreenAlternateScreen().Loop(component);
}
