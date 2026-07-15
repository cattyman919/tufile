#include "app.hpp"
#include <algorithm>
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
#include <ranges>
#include <vector>

const std::array<tufile::App::Action, tufile::TOTAL_KEYBINDS>&
tufile::App::get_keybinds() {
  static const std::array<tufile::App::Action, tufile::TOTAL_KEYBINDS> binds{
      App::Action{.keys = {ftxui::Event::Character('q')},
                  .description = "Quit",
                  .action = [](App& app) -> bool {
                    app.terminal_app.Exit();
                    return true;
                  }},
      App::Action{.keys = {ftxui::Event::Character('.')},
                  .description = "Toggle hidden",
                  .action = [](App& app) -> bool {
                    app.show_hidden_files = !app.show_hidden_files;
                    app.terminal_app.PostEvent(ftxui::Event::Special("Rescan"));
                    return true;
                  }},
      App::Action{.keys =
                      {
                          ftxui::Event::Character('j'),
                          ftxui::Event::Character('k'),
                          ftxui::Event::ArrowDown,
                          ftxui::Event::ArrowUp,
                      },
                  .description = "Navigate",
                  .action = [](App&) -> bool { return false; }}};

  return binds;
}
auto tufile::App::get_parent_file_entries()
    -> std::vector<std::filesystem::directory_entry> {
  auto file_entries =
      std::filesystem::directory_iterator{this->cwd.parent_path()};

  std::vector<std::filesystem::directory_entry> entries;
  for (const auto& entry : file_entries) {
    const auto& filename = entry.path().filename().string();
    if (filename.starts_with('.') && !this->show_hidden_files) {
      continue;
    }
    entries.emplace_back(entry);
  }

  return entries;
}

auto tufile::App::get_cwd_file_entries()
    -> std::vector<std::filesystem::directory_entry> {
  auto file_entries = std::filesystem::directory_iterator{this->cwd};

  std::vector<std::filesystem::directory_entry> entries;
  for (const auto& entry : file_entries) {
    const auto& filename = entry.path().filename().string();
    if (filename.starts_with('.') && !this->show_hidden_files) {
      continue;
    }
    entries.emplace_back(entry);
  }

  return entries;
}

auto tufile::App::run() -> void {
  this->cwd_entries = this->get_cwd_file_entries();
  this->parent_entries = this->get_parent_file_entries();

  auto middle_pane = this->middle_pane_view();

  auto component =
      ftxui::Renderer(
          middle_pane,
          [&] {
            return ftxui::vbox({
                ftxui::hbox({
                    this->left_pane_view() | ftxui::flex_grow_factor(1),
                    ftxui::window(ftxui::text(this->cwd.filename().string()),
                                  middle_pane->Render()) |
                        ftxui::flex_grow_factor(4),
                    this->right_pane_view() | ftxui::flex_grow_factor(3),
                }) | ftxui::flex_grow,
                this->footer_view() | ftxui::center,
            });
          }) |
      ftxui::CatchEvent(
          [this](ftxui::Event event) { return this->handle_event(event); });

  // auto layout = ftxui::hbox({component, component, component});

  this->terminal_app.Loop(component);
}

auto tufile::App::handle_event(ftxui::Event event) -> bool {
  if (event == ftxui::Event::Special("Rescan")) {
    this->cwd_entries = this->get_cwd_file_entries();
    this->parent_entries = this->get_parent_file_entries();
    return true;
  }

  for (const auto& keybind : tufile::App::get_keybinds()) {
    auto it = std::ranges::find(keybind.keys, event);
    if (it != keybind.keys.end()) {
      return keybind.action(*this);
    }
  }

  return false; // everything else passes through to menu
}

auto tufile::App::footer_view() -> ftxui::Element {
  std::vector<ftxui::Element> elements;
  elements.reserve(50);

  // Helper to format events into clean UI strings
  auto format_key = [](const ftxui::Event& ev) -> std::string {
    if (ev == ftxui::Event::ArrowUp)
      return "Up";
    if (ev == ftxui::Event::ArrowDown)
      return "Down";
    if (ev == ftxui::Event::ArrowLeft)
      return "Left";
    if (ev == ftxui::Event::ArrowRight)
      return "Right";
    if (ev == ftxui::Event::Return)
      return "Enter";
    if (ev == ftxui::Event::Escape)
      return "Esc";
    if (ev == ftxui::Event::Tab)
      return "Tab";

    // Fallback for standard characters ('q', 'j', 'k', etc.)
    return ev.character();
  };

  size_t action_idx = 0;
  for (const auto& action : tufile::App::get_keybinds()) {

    size_t key_idx = 0;
    for (const auto& key : action.keys) {
      // Use our helper to get a nice string
      elements.emplace_back(ftxui::text(format_key(key)) |
                            ftxui::color(ftxui::Color::Blue));

      if (key_idx != action.keys.size() - 1) {
        elements.emplace_back(ftxui::text("/") |
                              ftxui::color(ftxui::Color::Grey70));
      }
      key_idx++;
    }

    elements.emplace_back(ftxui::text(": ") |
                          ftxui::color(ftxui::Color::Grey70));
    elements.emplace_back(ftxui::text(action.description) |
                          ftxui::color(ftxui::Color::Plum1));

    if (action_idx != tufile::App::get_keybinds().size() - 1) {
      elements.emplace_back(ftxui::text(" | ") |
                            ftxui::color(ftxui::Color::Grey70));
    }
    action_idx++;
  }

  return ftxui::hbox(elements);
}

auto tufile::App::left_pane_view() -> ftxui::Element {
  auto parent_entries_view =
      this->parent_entries |
      std::views::transform([](std::filesystem::directory_entry entry) {
        return entry.path().filename().string();
      });

  this->parent_entry_names.assign(parent_entries_view.begin(),
                                  parent_entries_view.end());

  std::vector<ftxui::Element> elements{parent_entries.size()};

  size_t index{0};
  for (const auto& entry : this->parent_entries) {
    auto filename = entry.path().filename().string();
    elements[index] = ftxui::text(filename);
    index++;
  }

  // Parent component
  auto left_pane = ftxui::vbox(elements) | ftxui::border;
  return left_pane;
}

auto tufile::App::right_pane_view() -> ftxui::Element {
  // File / Directory Info
  auto right_pane = ftxui::text("Right Pane") | ftxui::border;
  return right_pane;
}

auto tufile::App::middle_pane_view() -> ftxui::Component {
  auto entries_view =
      this->cwd_entries |
      std::views::transform([](std::filesystem::directory_entry entry) {
        return entry.path().filename().string();
      });

  this->cwd_entry_names.assign(entries_view.begin(), entries_view.end());

  auto menu = ftxui::Menu(&this->cwd_entry_names, &this->selected_index);

  return menu;
}
