        #pragma once

        #include <boost/sml.hpp>
        #include <ftxui/component/component.hpp>
        #include <ftxui/component/event.hpp>
        #include <ftxui/dom/elements.hpp>
        #include <cstddef>
        #include <memory>
#include <utility>
        #include <vector>

        namespace fl::generated::ftxer {

        enum class ScreenId { Raid, Party, AccountBattle, PartyBattle, EffectGallery, Libram, Bestiary, Story };
        enum class ExternalAction { CollapseHelp, ExpandHelp, NextAccount, NextParty, OverdriveDown, OverdriveUp, PreviousAccount, PreviousParty, ReturnToTown, ShowObjectives, ShowVillage, ToggleActiveScreen, ToggleAutoplay, ToggleHelp, TogglePartyBattle, BackOrQuit };

        class ExternalUiAdapter {
         public:
          virtual ~ExternalUiAdapter() = default;
          virtual ftxui::Component component() = 0;
          virtual ftxui::Element render() = 0;
          virtual bool on_event(const ftxui::Event& event) = 0;
          virtual ScreenId current_screen() const = 0;
          virtual bool show_screen(ScreenId screen) = 0;
          virtual bool invoke(ExternalAction action) = 0;
          virtual bool console_open() const = 0;
          virtual void toggle_console() = 0;
          virtual void focus_console() = 0;
          virtual void restore_focus() = 0;
          virtual bool quit_requested() const = 0;
          virtual bool sandbox_requested() const = 0;
        };

        struct RaidState {};
struct PartyState {};
struct AccountBattleState {};
struct PartyBattleState {};
struct EffectGalleryState {};
struct LibramState {};
struct BestiaryState {};
struct StoryState {};
        struct GoToRaid {};
struct GoToParty {};
struct GoToAccountBattle {};
struct GoToPartyBattle {};
struct GoToEffectGallery {};
struct GoToLibram {};
struct GoToBestiary {};
struct GoToStory {};

        struct NavigationStateMachine {
          auto operator()() const {
            using namespace boost::sml;
            return make_transition_table(
               state<RaidState> + event<GoToRaid> = state<RaidState>,
       state<RaidState> + event<GoToParty> = state<PartyState>,
       state<RaidState> + event<GoToAccountBattle> = state<AccountBattleState>,
       state<RaidState> + event<GoToPartyBattle> = state<PartyBattleState>,
       state<RaidState> + event<GoToEffectGallery> = state<EffectGalleryState>,
       state<RaidState> + event<GoToLibram> = state<LibramState>,
       state<RaidState> + event<GoToBestiary> = state<BestiaryState>,
       state<RaidState> + event<GoToStory> = state<StoryState>,
       state<PartyState> + event<GoToRaid> = state<RaidState>,
       state<PartyState> + event<GoToParty> = state<PartyState>,
       state<PartyState> + event<GoToAccountBattle> = state<AccountBattleState>,
       state<PartyState> + event<GoToPartyBattle> = state<PartyBattleState>,
       state<PartyState> + event<GoToEffectGallery> = state<EffectGalleryState>,
       state<PartyState> + event<GoToLibram> = state<LibramState>,
       state<PartyState> + event<GoToBestiary> = state<BestiaryState>,
       state<PartyState> + event<GoToStory> = state<StoryState>,
       state<AccountBattleState> + event<GoToRaid> = state<RaidState>,
       state<AccountBattleState> + event<GoToParty> = state<PartyState>,
       state<AccountBattleState> + event<GoToAccountBattle> = state<AccountBattleState>,
       state<AccountBattleState> + event<GoToPartyBattle> = state<PartyBattleState>,
       state<AccountBattleState> + event<GoToEffectGallery> = state<EffectGalleryState>,
       state<AccountBattleState> + event<GoToLibram> = state<LibramState>,
       state<AccountBattleState> + event<GoToBestiary> = state<BestiaryState>,
       state<AccountBattleState> + event<GoToStory> = state<StoryState>,
      *state<PartyBattleState> + event<GoToRaid> = state<RaidState>,
       state<PartyBattleState> + event<GoToParty> = state<PartyState>,
       state<PartyBattleState> + event<GoToAccountBattle> = state<AccountBattleState>,
       state<PartyBattleState> + event<GoToPartyBattle> = state<PartyBattleState>,
       state<PartyBattleState> + event<GoToEffectGallery> = state<EffectGalleryState>,
       state<PartyBattleState> + event<GoToLibram> = state<LibramState>,
       state<PartyBattleState> + event<GoToBestiary> = state<BestiaryState>,
       state<PartyBattleState> + event<GoToStory> = state<StoryState>,
       state<EffectGalleryState> + event<GoToRaid> = state<RaidState>,
       state<EffectGalleryState> + event<GoToParty> = state<PartyState>,
       state<EffectGalleryState> + event<GoToAccountBattle> = state<AccountBattleState>,
       state<EffectGalleryState> + event<GoToPartyBattle> = state<PartyBattleState>,
       state<EffectGalleryState> + event<GoToEffectGallery> = state<EffectGalleryState>,
       state<EffectGalleryState> + event<GoToLibram> = state<LibramState>,
       state<EffectGalleryState> + event<GoToBestiary> = state<BestiaryState>,
       state<EffectGalleryState> + event<GoToStory> = state<StoryState>,
       state<LibramState> + event<GoToRaid> = state<RaidState>,
       state<LibramState> + event<GoToParty> = state<PartyState>,
       state<LibramState> + event<GoToAccountBattle> = state<AccountBattleState>,
       state<LibramState> + event<GoToPartyBattle> = state<PartyBattleState>,
       state<LibramState> + event<GoToEffectGallery> = state<EffectGalleryState>,
       state<LibramState> + event<GoToLibram> = state<LibramState>,
       state<LibramState> + event<GoToBestiary> = state<BestiaryState>,
       state<LibramState> + event<GoToStory> = state<StoryState>,
       state<BestiaryState> + event<GoToRaid> = state<RaidState>,
       state<BestiaryState> + event<GoToParty> = state<PartyState>,
       state<BestiaryState> + event<GoToAccountBattle> = state<AccountBattleState>,
       state<BestiaryState> + event<GoToPartyBattle> = state<PartyBattleState>,
       state<BestiaryState> + event<GoToEffectGallery> = state<EffectGalleryState>,
       state<BestiaryState> + event<GoToLibram> = state<LibramState>,
       state<BestiaryState> + event<GoToBestiary> = state<BestiaryState>,
       state<BestiaryState> + event<GoToStory> = state<StoryState>,
       state<StoryState> + event<GoToRaid> = state<RaidState>,
       state<StoryState> + event<GoToParty> = state<PartyState>,
       state<StoryState> + event<GoToAccountBattle> = state<AccountBattleState>,
       state<StoryState> + event<GoToPartyBattle> = state<PartyBattleState>,
       state<StoryState> + event<GoToEffectGallery> = state<EffectGalleryState>,
       state<StoryState> + event<GoToLibram> = state<LibramState>,
       state<StoryState> + event<GoToBestiary> = state<BestiaryState>,
       state<StoryState> + event<GoToStory> = state<StoryState>
            );
          }
        };

        class CompositionRoot : public ftxui::ComponentBase {
         public:
          explicit CompositionRoot(std::unique_ptr<ExternalUiAdapter> adapter);
          bool OnEvent(ftxui::Event event) override;
          ftxui::Element Render() override;
          ftxui::Component ActiveChild() override;
          ScreenId current_screen() const { return current_screen_; }
          std::size_t history_size() const { return history_.size(); }
          bool console_open() const { return adapter_->console_open(); }
          bool quit_requested() const { return adapter_->quit_requested(); }
          bool sandbox_requested() const { return adapter_->sandbox_requested(); }

         private:
          bool navigate(ScreenId destination, bool remember = true);
          bool back();
          bool invoke(ExternalAction action);
          void synchronize(bool remember);
          void dispatch(ScreenId destination);

          std::unique_ptr<ExternalUiAdapter> adapter_;
          ftxui::Component external_component_;
          std::unique_ptr<boost::sml::sm<NavigationStateMachine>> navigation_;
          ScreenId current_screen_{ScreenId::PartyBattle};
          std::vector<ScreenId> history_;
        };

        }  // namespace fl::generated::ftxer
