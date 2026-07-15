#include "app.hpp"
#include <array>
#include <cassert>
#include <cstddef>
#include <filesystem>
#include <ftxui/component/app.hpp>
#include <ftxui/component/component.hpp>

#include <ftxui/component/event.hpp>
#include <ftxui/dom/deprecated.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/screen/string.hpp>
#include <iostream>
#include <vector>

const std::array<tufile::App::Action, 2> tufile::App::keybinds{
    App::Action{.keys = {"q"},
                .description = "Quit",
                .action = [](App& app) -> bool {
                  // Success! App is complete, so we can access private members.
                  app.terminal_app.Exit();
                  return true;
                }},
    App::Action{.keys = {"Up", "Down", "j", "k"},
                .description = "Navigate",
                // Removed 'app' name to fix unused parameter warning
                .action = [](App&) -> bool { return false; }}};

auto tufile::App::run() -> void {

  auto current_path = std::filesystem::current_path();

  auto file_entries = std::filesystem::directory_iterator{current_path};

  std::vector<std::string> entries;
  entries.reserve(entries.size());

  for (const auto& entry : file_entries) {
    const auto& filename = entry.path().filename().string();

    if (filename.starts_with('.') && !this->show_hidden_files) {
      continue;
    }
    entries.emplace_back(filename);
  }

  // assert(entries.size() > 0 && "Has to have at least one entry");

  int selected = 0;

  // Create a menu component
  auto menu = ftxui::Menu(&entries, &selected);

  // Parent component
  auto left_pane = ftxui::text("Left Pane") | ftxui::border;

  auto middle_pane = menu | ftxui::border;

  // Footer for helper keys
  // auto footer = ftxui::text("Up/Down/j/k: Navigate");

  auto footer = [&] {
    std::vector<ftxui::Element> elements;
    elements.reserve(50);

    for (size_t index{0}; const auto& action : tufile::App::keybinds) {
      for (size_t index{0}; const auto& key : action.keys) {
        elements.emplace_back(ftxui::text(key) |
                              ftxui::color(ftxui::Color::Blue));

        if (index != action.keys.size() - 1) {
          elements.emplace_back(ftxui::text("/") |
                                ftxui::color(ftxui::Color::Grey70));
        }
        index++;
      }
      elements.emplace_back(ftxui::text(": ") |
                            ftxui::color(ftxui::Color::Grey70));
      elements.emplace_back(ftxui::text(action.description) |
                            ftxui::color(ftxui::Color::Plum1));

      if (index != tufile::App::keybinds.size() - 1) {
        elements.emplace_back(ftxui::text(" | ") |
                              ftxui::color(ftxui::Color::Grey70));
      }

      index++;
    }
    return ftxui::hbox(elements);
  }();

  // File / Directory Info
  auto right_pane = ftxui::text("Right Pane") | ftxui::border;

  auto component =
      ftxui::Renderer(
          middle_pane,
          [&] {
            return ftxui::vbox({
                ftxui::hbox({
                    left_pane | ftxui::flex_grow_factor(1),
                    middle_pane->Render() | ftxui::flex_grow_factor(4),
                    right_pane | ftxui::flex_grow_factor(3),
                }) | ftxui::flex_grow,
                footer | ftxui::center,
            });
          }) |
      ftxui::CatchEvent(
          [this](ftxui::Event event) { return this->handle_key_event(event); });

  // auto layout = ftxui::hbox({component, component, component});

  this->terminal_app.Loop(component);
}

auto tufile::App::handle_key_event(ftxui::Event event) -> bool {
  if (event == ftxui::Event::Character('q')) {
    this->terminal_app.Exit();
    return true; // swallowed, menu never sees 'q'
  }

  if (event == ftxui::Event::Character('.')) {
    this->show_hidden_files = true;
    return true;
  }

  return false; // everything else passes through to menu
}
