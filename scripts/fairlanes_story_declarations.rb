# frozen_string_literal: true

# Story facts and choices use stable IDs. `handled: true` is checked against the
# handwritten C++ dispatcher; unhandled choices remain visible in the report.
story_event :visitor_destroyed_village, handled: true
story_event :recruit_angry_fellow, handled: true
story_event :ask_about_visitor
story_event :first_combat_won, handled: true
story_event :two_starter_wins, handled: true
story_event :village_gear_purchased, handled: true
story_event :recruit_second_hero, handled: true

objective :recruit_first_hero,
          title: "Recruit a hero",
          description: "Visit the village pub and speak to the angry fellow.",
          revealed_by: :visitor_destroyed_village,
          completed_by: :recruit_angry_fellow
objective :recruit_second_hero,
          title: "Meet the next volunteer",
          description: "After two victories, return to town and buy Village Guard Hide.",
          revealed_by: :village_gear_purchased,
          completed_by: :recruit_second_hero

dialogue :angry_fellow, location: :pub, npc: "Angry fellow", start: :greeting,
         nodes: [
           { id: :greeting, speaker: "Angry fellow",
             text: "The Visitor tore through our village. Nobody stood against it. I will.",
             choices: [
               { id: :invite, label: "Then fight beside me.", event: :recruit_angry_fellow, next: :joined },
               { id: :ask, label: "What was that thing?", event: :ask_about_visitor, next: :answer },
               { id: :leave, label: "Leave him alone.", next: :exit }
             ] },
           { id: :answer, speaker: "Angry fellow",
             text: "A Visitor. It will come again. We had better be ready next time.",
             choices: [{ id: :return, label: "I understand.", next: :greeting }] },
           { id: :joined, speaker: "Angry fellow",
             text: "Good. Point me at the first fight.", choices: [] }
         ]

dialogue :second_volunteer, location: :village, npc: "Volunteer", start: :greeting,
         nodes: [
           { id: :greeting, speaker: "Volunteer",
             text: "I saw you come back from those fights. Let me join you.",
             choices: [{ id: :accept, label: "Welcome aboard.", event: :recruit_second_hero, next: :joined },
                       { id: :later, label: "Not yet.", next: :exit }] },
           { id: :joined, speaker: "Volunteer", text: "I'll be ready.", choices: [] }
         ]

# This route documents the actions an enabled autoplayer must take. Its steps
# are validated against emitted facts, dialogue choices, and the shop item.
autoplay_route :origin_opening, steps: [
  { wait_for: :visitor_destroyed_village, action: :choose_dialogue,
    target: :angry_fellow, choice: :invite },
  { wait_for: :two_starter_wins, action: :return_to_town },
  { wait_for: :two_starter_wins, action: :buy_gear,
    target: :village_guard_hide },
  { wait_for: :village_gear_purchased, action: :choose_dialogue,
    target: :second_volunteer, choice: :accept }
]
