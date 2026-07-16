#include "app.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <format>
#include <ftxui/component/app.hpp>
#include <ftxui/component/component.hpp>

#include <ftxui/component/event.hpp>
#include <ftxui/dom/deprecated.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/screen/string.hpp>
#include <ranges>
#include <string>
#include <system_error>
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
      App::Action{.keys = {ftxui::Event::Character('h')},
                  .description = "Prev Dir",
                  .action = [](App& app) -> bool {
                    app.terminal_app.PostEvent(
                        ftxui::Event::Special("PreviousDir"));
                    return true;
                  }},
      App::Action{.keys = {ftxui::Event::Character('l')},
                  .description = "Next Dir",
                  .action = [](App& app) -> bool {
                    app.terminal_app.PostEvent(
                        ftxui::Event::Special("NextDir"));
                    return true;
                  }},
      App::Action{.keys = {ftxui::Event::Character('.')},
                  .description = "Toggle hidden",
                  .action = [](App& app) -> bool {
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

auto tufile::App::get_file_entries(const std::filesystem::path& path)
    -> const std::vector<std::filesystem::directory_entry> {

  auto file_entries = std::filesystem::directory_iterator{path};

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
  this->update_path(this->cwd);

  auto middle_pane = this->middle_pane_view();

  auto component =
      ftxui::Renderer(
          middle_pane,
          [&] {
            return ftxui::vbox({
                this->header_view(),
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
          [&](ftxui::Event event) { return this->handle_event(event); });

  this->terminal_app.Loop(component);
}
auto tufile::App::update_path(const std::filesystem::path& path) -> void {
  this->cwd = path;
  this->cwd_entries = this->get_file_entries(path);
  this->parent_entries = this->get_file_entries(path.parent_path());

  auto entries_view =
      [&](std::vector<std::filesystem::directory_entry>& entries) {
        return entries | std::views::transform(
                             [](std::filesystem::directory_entry entry) {
                               if (entry.is_directory()) {
                                 return "D " + entry.path().filename().string();
                               }
                               return entry.path().filename().string();
                             });
      };

  auto cwd_entries_view = entries_view(this->cwd_entries);
  auto parent_entries_view = entries_view(this->parent_entries);

  if (this->selected_index >= this->cwd_entry_names.size()) {
    this->selected_index =
        this->cwd_entry_names.empty() ? -1 : this->cwd_entry_names.size() - 1;
  }

  this->cwd_entry_names.assign(cwd_entries_view.begin(),
                               cwd_entries_view.end());

  this->parent_entry_names.assign(parent_entries_view.begin(),
                                  parent_entries_view.end());
}

auto tufile::App::handle_event(ftxui::Event event) -> bool {
  if (event == ftxui::Event::Special("Rescan")) {
    this->show_hidden_files = !this->show_hidden_files;
    this->update_path(this->cwd);
    return true;

  } else if (event == ftxui::Event::Special("PreviousDir")) {
    this->update_path(cwd.parent_path());
    return true;

  } else if (event == ftxui::Event::Special("NextDir")) {
    if (this->selected_index == -1) {
      return true;
    }

    auto clone_cwd{this->cwd};
    const auto target_path = clone_cwd.append(
        this->cwd_entries[this->selected_index].path().filename().string());

    if (std::filesystem::is_directory(target_path)) {
      this->update_path(target_path);
      return true;
    }

    // Do nothing
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

auto tufile::App::header_view() -> const ftxui::Element {
  return ftxui::text(this->cwd.string());
}

auto tufile::App::footer_view() -> const ftxui::Element {
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

// Parent component
auto tufile::App::left_pane_view() -> const ftxui::Element {
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

auto permission_string(std::filesystem::perms p) -> std::string {
  using std::filesystem::perms;

  std::string result;
  result.reserve(9);

  auto show = [&](char op, perms perm) {
    char permChar = (perms::none == (perm & p) ? '-' : op);
    result.push_back(permChar);
  };

  show('r', perms::owner_read);
  show('w', perms::owner_write);
  show('x', perms::owner_exec);
  show('r', perms::group_read);
  show('w', perms::group_write);
  show('x', perms::group_exec);
  show('r', perms::others_read);
  show('w', perms::others_write);
  show('x', perms::others_exec);

  return result;
}

auto file_type_string(std::filesystem::file_type f_type) -> std::string {

  switch (f_type) {
  case std::filesystem::file_type::regular:
    return "File";
  case std::filesystem::file_type::directory:
    return "Directory";
  case std::filesystem::file_type::block:
    return "Block";
  case std::filesystem::file_type::character:
    return "Character";
  case std::filesystem::file_type::fifo:
    return "FIFO";
  case std::filesystem::file_type::socket:
    return "Socket";
  default:
    return "Unknown";
  }
}

auto tufile::App::right_pane_view() -> const ftxui::Element {

  auto clone_cwd{this->cwd};
  const auto target_path = clone_cwd.append(
      this->cwd_entries[this->selected_index].path().filename().string());

  std::error_code err;
  uintmax_t file_size{std::filesystem::file_size(target_path, err)};
  if (err) {
    file_size = 0;
  }

  auto status{std::filesystem::status(target_path, err)};
  std::string perms_string{permission_string(status.permissions())};
  std::string file_type_status_string{file_type_string(status.type())};
  if (err) {
    perms_string = "Unknown";
    file_type_status_string = "Unknown";
  }

  // File / Directory Info
  auto right_pane =
      ftxui::vbox({
          ftxui::text(std::format("File Type : {}", file_type_status_string)),
          ftxui::text(std::format("File Size : {} Bytes", file_size)),
          ftxui::text(std::format("Permission : {}", perms_string)),
      }) |
      ftxui::border;
  return right_pane;
}

auto tufile::App::middle_pane_view() -> const ftxui::Component {
  auto menu = ftxui::Menu(&this->cwd_entry_names, &this->selected_index);

  return menu;
}
