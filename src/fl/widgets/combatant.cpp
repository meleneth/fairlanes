#include "combatant.hpp"

#include "fill_bar.hpp"

#include "fl/ecs/components/atb_charge.hpp"
#include "fl/ecs/components/closet.hpp"
#include "fl/ecs/components/dire_bleed.hpp"
#include "fl/ecs/components/freeze.hpp"
#include "fl/ecs/components/hp_bar_color_override.hpp"
#include "fl/ecs/components/party_member.hpp"
#include "fl/ecs/components/poison.hpp"
#include "fl/ecs/components/skill_slots.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/components/track_xp.hpp"
#include "fl/ecs/components/visual_effects.hpp"
#include "fl/ecs/systems/combatant_status_visuals.hpp"
#include "fl/lospec500.hpp"
#include "fl/skills/skill.hpp"
#include "fl/widgets/effects/decal.hpp"

#include <array>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <ftxui/dom/node.hpp>
#include <ftxui/screen/screen.hpp>

namespace fl::widgets {
namespace {

class AttackDecalNode : public ftxui::Node {
public:
  AttackDecalNode(ftxui::Element child,
                  std::vector<fl::ecs::components::DecalEffect> effects)
      : ftxui::Node(ftxui::Elements{std::move(child)}),
        effects_(std::move(effects)) {}

  void ComputeRequirement() override {
    children_[0]->ComputeRequirement();
    requirement_ = children_[0]->requirement();
  }

  void SetBox(ftxui::Box box) override {
    box_ = box;
    children_[0]->SetBox(box);

    const int width = box_.x_max - box_.x_min + 1;
    const int combatant_height = box_.y_max - box_.y_min + 1;
    if (width <= 0 || combatant_height <= 0) {
      prepared_.clear();
      prepared_width_ = 0;
      prepared_height_ = 0;
      return;
    }

    if (prepared_width_ == width && prepared_height_ == combatant_height &&
        prepared_.size() == effects_.size()) {
      return;
    }

    prepared_width_ = width;
    prepared_height_ = combatant_height;
    prepared_.clear();
    prepared_.reserve(effects_.size());

    for (const auto &effect : effects_) {
      const int decal_height = combatant_height + effect.extra_height;
      auto animation = fl::widgets::effects::make_decal_animation(
          effect.animation_kind, width, decal_height, effect.config);
      if (animation) {
        prepared_.push_back(PreparedEffect{effect, std::move(animation)});
      }
    }
  }

  void Render(ftxui::Screen &screen) override {
    children_[0]->Render(screen);

    const int width = box_.x_max - box_.x_min + 1;
    const int combatant_height = box_.y_max - box_.y_min + 1;
    if (width <= 0 || combatant_height <= 0) {
      return;
    }

    const auto now = fl::ecs::components::DecalEffect::Clock::now();
    for (const auto &prepared : prepared_) {
      auto frame = prepared.animation->render(prepared.effect.progress_at(now));
      const int decal_y_min = box_.y_max - frame.height + 1;
      for (int y = 0; y < frame.height; ++y) {
        const int screen_y = decal_y_min + y;
        if (screen_y < 0 || screen_y >= screen.dimy()) {
          continue;
        }
        for (int x = 0; x < frame.width; ++x) {
          const int screen_x = box_.x_min + x;
          if (screen_x < 0 || screen_x >= screen.dimx()) {
            continue;
          }

          const auto &cell = frame.at(x, y);
          if (!cell.active()) {
            continue;
          }

          auto &pixel = screen.PixelAt(screen_x, screen_y);
          const bool had_text = pixel.character != " ";
          if (cell.bg) {
            pixel.background_color = *cell.bg;
          }
          if (cell.fg) {
            pixel.foreground_color = *cell.fg;
          }
          if (cell.glyph != ' ') {
            pixel.character = std::string(1, cell.glyph);
          } else if (had_text && cell.bg) {
            pixel.foreground_color = fl::lospec500::color_at(41);
          }
        }
      }
    }
  }

private:
  struct PreparedEffect {
    fl::ecs::components::DecalEffect effect;
    std::shared_ptr<const fl::widgets::effects::DecalAnimation> animation;
  };

  std::vector<fl::ecs::components::DecalEffect> effects_;
  std::vector<PreparedEffect> prepared_;
  int prepared_width_ = 0;
  int prepared_height_ = 0;
};

class UnderlayDecalNode : public ftxui::Node {
public:
  UnderlayDecalNode(ftxui::Element child,
                    std::vector<fl::ecs::components::DecalEffect> effects)
      : ftxui::Node(ftxui::Elements{std::move(child)}),
        effects_(std::move(effects)) {}

  void ComputeRequirement() override {
    children_[0]->ComputeRequirement();
    requirement_ = children_[0]->requirement();
  }

  void SetBox(ftxui::Box box) override {
    box_ = box;
    children_[0]->SetBox(box);

    const int width = box_.x_max - box_.x_min + 1;
    const int combatant_height = box_.y_max - box_.y_min + 1;
    if (width <= 0 || combatant_height <= 0) {
      prepared_.clear();
      prepared_width_ = 0;
      prepared_height_ = 0;
      return;
    }

    if (prepared_width_ == width && prepared_height_ == combatant_height &&
        prepared_.size() == effects_.size()) {
      return;
    }

    prepared_width_ = width;
    prepared_height_ = combatant_height;
    prepared_.clear();
    prepared_.reserve(effects_.size());

    for (const auto &effect : effects_) {
      const int decal_height = combatant_height + effect.extra_height;
      auto animation = fl::widgets::effects::make_decal_animation(
          effect.animation_kind, width, decal_height, effect.config);
      if (animation) {
        prepared_.push_back(PreparedEffect{effect, std::move(animation)});
      }
    }
  }

  void Render(ftxui::Screen &screen) override {
    children_[0]->Render(screen);

    const int width = box_.x_max - box_.x_min + 1;
    const int combatant_height = box_.y_max - box_.y_min + 1;
    if (width <= 0 || combatant_height <= 0) {
      return;
    }

    std::vector<ftxui::Pixel> text_pixels;
    text_pixels.reserve(static_cast<std::size_t>(width * combatant_height));
    for (int y = box_.y_min; y <= box_.y_max; ++y) {
      for (int x = box_.x_min; x <= box_.x_max; ++x) {
        if (x < 0 || y < 0 || x >= screen.dimx() || y >= screen.dimy()) {
          text_pixels.push_back(ftxui::Pixel{});
          continue;
        }
        text_pixels.push_back(screen.PixelAt(x, y));
      }
    }

    const auto now = fl::ecs::components::DecalEffect::Clock::now();
    for (const auto &prepared : prepared_) {
      auto frame =
          prepared.animation->render(prepared.effect.loop_progress_at(now));
      const int decal_y_min = box_.y_max - frame.height + 1;
      for (int y = 0; y < frame.height; ++y) {
        const int screen_y = decal_y_min + y;
        if (screen_y < 0 || screen_y >= screen.dimy()) {
          continue;
        }
        for (int x = 0; x < frame.width; ++x) {
          const int screen_x = box_.x_min + x;
          if (screen_x < 0 || screen_x >= screen.dimx()) {
            continue;
          }

          const auto &cell = frame.at(x, y);
          if (!cell.active()) {
            continue;
          }

          auto &pixel = screen.PixelAt(screen_x, screen_y);
          if (cell.bg) {
            pixel.background_color = *cell.bg;
          }
          if (cell.fg) {
            pixel.foreground_color = *cell.fg;
          }
          if (cell.glyph != ' ') {
            pixel.character = std::string(1, cell.glyph);
          }
        }
      }
    }

    std::size_t index = 0;
    for (int y = box_.y_min; y <= box_.y_max; ++y) {
      for (int x = box_.x_min; x <= box_.x_max; ++x) {
        const auto saved = text_pixels[index++];
        if (x < 0 || y < 0 || x >= screen.dimx() || y >= screen.dimy()) {
          continue;
        }
        if (saved.character != " ") {
          screen.PixelAt(x, y) = saved;
        }
      }
    }
  }

private:
  struct PreparedEffect {
    fl::ecs::components::DecalEffect effect;
    std::shared_ptr<const fl::widgets::effects::DecalAnimation> animation;
  };

  std::vector<fl::ecs::components::DecalEffect> effects_;
  std::vector<PreparedEffect> prepared_;
  int prepared_width_ = 0;
  int prepared_height_ = 0;
};

constexpr int kSkillRowsVisibleHeight = 9;
constexpr int kSkillRowsCount = fl::ecs::components::Closet::kSkillSlotCount;
constexpr int kDebuffRowsVisibleHeight = 9;
constexpr int kDebuffRowsVisibleWidth = 42;

class SkillRowsNode : public ftxui::Node {
public:
  SkillRowsNode(ftxui::Element child,
                std::array<std::string, kSkillRowsCount> skills)
      : ftxui::Node(ftxui::Elements{std::move(child)}),
        skills_(std::move(skills)) {}

  void ComputeRequirement() override {
    children_[0]->ComputeRequirement();
    requirement_ = children_[0]->requirement();
  }

  void SetBox(ftxui::Box box) override {
    box_ = box;
    children_[0]->SetBox(box);
  }

  void Render(ftxui::Screen &screen) override {
    children_[0]->Render(screen);

    const int width = box_.x_max - box_.x_min + 1;
    const int height = box_.y_max - box_.y_min + 1;
    if (width <= 4 || height < kSkillRowsVisibleHeight) {
      return;
    }

    const int y_start = box_.y_min + 3;
    const int x_start = box_.x_min + 2;
    const int max_width = std::max(0, width - 4);
    for (int row = 0; row < kSkillRowsCount; ++row) {
      const int y = y_start + row;
      if (y < 0 || y >= screen.dimy() || y > box_.y_max - 1) {
        continue;
      }

      const auto &line = skills_[static_cast<std::size_t>(row)];
      const int count = std::min(max_width, static_cast<int>(line.size()));
      for (int x = 0; x < count; ++x) {
        const int screen_x = x_start + x;
        if (screen_x < 0 || screen_x >= screen.dimx() ||
            screen_x > box_.x_max - 1) {
          continue;
        }
        auto &pixel = screen.PixelAt(screen_x, y);
        pixel.character = std::string(1, line[static_cast<std::size_t>(x)]);
        pixel.foreground_color = fl::lospec500::color_at(32);
      }
    }
  }

private:
  std::array<std::string, kSkillRowsCount> skills_;
};

std::array<std::string, kSkillRowsCount> skill_rows_for(entt::registry &reg,
                                                        entt::entity entity) {
  std::array<std::string, kSkillRowsCount> rows;
  rows.fill("--");

  if (const auto *member =
          reg.try_get<fl::ecs::components::PartyMember>(entity)) {
    const auto &skill_slots = member->closet().skill_slots;
    for (std::size_t i = 0; i < rows.size(); ++i) {
      if (skill_slots[i].has_value()) {
        rows[i] = fl::skills::display_name(*skill_slots[i]);
      }
    }
    return rows;
  }

  const auto *slots = reg.try_get<fl::ecs::components::SkillSlots>(entity);
  if (slots == nullptr) {
    return rows;
  }

  for (std::size_t i = 0; i < rows.size(); ++i) {
    if (slots->slots[i].has_value()) {
      rows[i] = fl::skills::display_name(*slots->slots[i]);
    }
  }

  return rows;
}

struct DebuffLabel {
  std::string text;
  ftxui::Color color;
};

class DebuffRowsNode : public ftxui::Node {
public:
  DebuffRowsNode(ftxui::Element child, std::vector<DebuffLabel> debuffs)
      : ftxui::Node(ftxui::Elements{std::move(child)}),
        debuffs_(std::move(debuffs)) {}

  void ComputeRequirement() override {
    children_[0]->ComputeRequirement();
    requirement_ = children_[0]->requirement();
  }

  void SetBox(ftxui::Box box) override {
    box_ = box;
    children_[0]->SetBox(box);
  }

  void Render(ftxui::Screen &screen) override {
    children_[0]->Render(screen);

    const int width = box_.x_max - box_.x_min + 1;
    const int height = box_.y_max - box_.y_min + 1;
    if (debuffs_.empty() || width < kDebuffRowsVisibleWidth ||
        height < kDebuffRowsVisibleHeight) {
      return;
    }

    std::size_t max_label_width = 0;
    for (const auto &debuff : debuffs_) {
      max_label_width = std::max(max_label_width, debuff.text.size());
    }

    const int x_start = std::max(
        box_.x_min + 2, box_.x_max - static_cast<int>(max_label_width));
    const int max_width = std::max(0, box_.x_max - x_start);
    const int y_start = box_.y_min + 3;
    for (std::size_t row = 0; row < debuffs_.size(); ++row) {
      const int y = y_start + static_cast<int>(row);
      if (y < 0 || y >= screen.dimy() || y > box_.y_max - 1) {
        continue;
      }

      const auto &debuff = debuffs_[row];
      const int count =
          std::min(max_width, static_cast<int>(debuff.text.size()));
      for (int x = 0; x < count; ++x) {
        const int screen_x = x_start + x;
        if (screen_x < 0 || screen_x >= screen.dimx() ||
            screen_x > box_.x_max - 1) {
          continue;
        }
        auto &pixel = screen.PixelAt(screen_x, y);
        pixel.character =
            std::string(1, debuff.text[static_cast<std::size_t>(x)]);
        pixel.foreground_color = debuff.color;
      }
    }
  }

private:
  std::vector<DebuffLabel> debuffs_;
};

std::vector<DebuffLabel> debuff_rows_for(entt::registry &reg,
                                         entt::entity entity) {
  std::vector<DebuffLabel> debuffs;
  const auto visuals = fl::ecs::systems::combatant_status_visuals_for(
      reg, entity, fl::ecs::systems::CombatantVisualLayer::StatusText,
      fl::ecs::systems::CombatantVisualRegion::StatusRows);
  debuffs.reserve(visuals.size());
  for (const auto &visual : visuals) {
    if (!visual.label.empty()) {
      debuffs.push_back(DebuffLabel{
          visual.label, visual.color.value_or(fl::lospec500::color_at(32))});
    }
  }
  return debuffs;
}

fl::ecs::components::CombatantUnderlayDecals
underlay_decals_for(entt::registry &reg, entt::entity entity) {
  fl::ecs::components::CombatantUnderlayDecals decals;
  const auto visuals = fl::ecs::systems::combatant_status_visuals(reg, entity);
  for (const auto &visual : visuals) {
    if (visual.layer == fl::ecs::systems::CombatantVisualLayer::Underlay &&
        visual.decal.has_value()) {
      decals.effects.push_back(*visual.decal);
    }
  }
  return decals;
}

ftxui::Element skill_rows(ftxui::Element child,
                          std::array<std::string, kSkillRowsCount> rows) {
  return std::make_shared<SkillRowsNode>(std::move(child), std::move(rows));
}

ftxui::Element debuff_rows(ftxui::Element child,
                           std::vector<DebuffLabel> debuffs) {
  return std::make_shared<DebuffRowsNode>(std::move(child), std::move(debuffs));
}

ftxui::Element
attack_decal(ftxui::Element child,
             const fl::ecs::components::CombatantDecals &decals) {
  return std::make_shared<AttackDecalNode>(std::move(child), decals.effects);
}

ftxui::Element
underlay_decal(ftxui::Element child,
               const fl::ecs::components::CombatantUnderlayDecals &decals) {
  return std::make_shared<UnderlayDecalNode>(std::move(child), decals.effects);
}

} // namespace

Combatant::Combatant(entt::registry &reg_, entt::entity entity_,
                     bool render_uwu)
    : reg(reg_), entity(entity_), render_uwu_(render_uwu) {}

ftxui::Element Combatant::Render() {
  using namespace fl::ecs::components;
  if (!reg.valid(entity)) {
    return ftxui::window(ftxui::text("[invalid]"),
                         ftxui::text("entity not valid")) |
           ftxui::xflex;
  }

  auto *the_stats = reg.try_get<Stats>(entity);
  if (!the_stats) {
    return ftxui::window(ftxui::text("[no Stats]"),
                         ftxui::text("missing Stats")) |
           ftxui::xflex;
  }

  auto &stats = reg.get<Stats>(entity);
  auto &level = reg.get<TrackXP>(entity);

  constexpr int bar_width = 20;
  const float hp_fill =
      stats.max_hp_ > 0
          ? static_cast<float>(stats.hp_) / static_cast<float>(stats.max_hp_)
          : 0.0f;
  ftxui::Element hp_line = labeled_fill_bar(
      "HP", clamp_fill(hp_fill), bar_width,
      std::to_string(stats.hp_) + "/" + std::to_string(stats.max_hp_));
  if (auto *hp_color = reg.try_get<HPBarColorOverride>(entity);
      hp_color != nullptr && stats.is_alive()) {
    hp_line = hp_line | ftxui::color(hp_color->color);
  }

  // Build the uWu (ATB charge) bar if requested
  std::vector<ftxui::Element> content_lines;
  content_lines.push_back(std::move(hp_line));

  if (render_uwu_) {
    auto *atb = reg.try_get<fl::ecs::components::AtbCharge>(entity);
    if (atb != nullptr) {
      const float atb_fill = atb->max_charge > 0
                                 ? static_cast<float>(atb->charge) /
                                       static_cast<float>(atb->max_charge)
                                 : 0.0f;
      content_lines.push_back(labeled_fill_bar(
          "ATB", clamp_fill(atb_fill), bar_width,
          std::to_string(atb->charge) + "/" + std::to_string(atb->max_charge)));
    }
  }

  // Top border labels
  // clang-format off
  ftxui::Element border = ftxui::window(
    ftxui::hbox(ftxui::Elements {
      ftxui::text(stats.name_) | ftxui::color(ftxui::Color::BlueLight),
      ftxui::filler(),
      ftxui::text("[") | ftxui::bgcolor(fl::lospec500::color_at(1)) | ftxui::color(fl::lospec500::color_at(32)),
     
      ftxui::text(std::to_string(level.level_)) | ftxui::bgcolor(fl::lospec500::color_at(9)) | ftxui::color(fl::lospec500::color_at(15)),
      ftxui::text("]") | ftxui::bgcolor(fl::lospec500::color_at(1)) | ftxui::color(fl::lospec500::color_at(32))
    }),
    ftxui::vbox(std::move(content_lines))
  );
  // clang-format on

  if (auto *co = reg.try_get<ResolvedColorOverride>(entity)) {
    border = border | ftxui::color(co->color);
  }

  if (auto *bg = reg.try_get<ResolvedBackgroundColorOverride>(entity)) {
    border = border | ftxui::bgcolor(bg->color);
  }

  border = skill_rows(std::move(border), skill_rows_for(reg, entity));
  border = debuff_rows(std::move(border), debuff_rows_for(reg, entity));

  auto derived_underlays = underlay_decals_for(reg, entity);
  if (!derived_underlays.effects.empty()) {
    border = underlay_decal(std::move(border), derived_underlays);
  }

  if (auto *decals = reg.try_get<CombatantDecals>(entity)) {
    border = attack_decal(std::move(border), *decals);
  }

  // <-- key: allow the whole Combatant box to flex horizontally
  return border | ftxui::xflex;
}

} // namespace fl::widgets
