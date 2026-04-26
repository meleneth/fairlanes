import { defineConfig } from 'vitepress'
import { withMermaid } from 'vitepress-plugin-mermaid'

export default withMermaid(
  defineConfig({
    title: 'Fairlanes Tour',
    description: 'Guided tour of the Fairlanes codebase',
    base: '/fairlanes/',

    themeConfig: {
      nav: [
        { text: 'Tour Home', link: '/' }
      ],

      sidebar: [
        {
          text: 'Guided Tour',
          items: [
            { text: 'Overview', link: '/' },
            { text: 'Glossary', link: '/glossary' },
            { text: 'Architecture', link: '/architecture' },
            { text: 'Development', link: '/development' },
            { text: 'Event Flow', link: '/event-flow' }
          ]
        },
        {
          text: 'Fairlanes',
          items: [
            { text: 'Context', link: '/fairlanes/context' },
            { text: 'Grand Central', link: '/fairlanes/grand_central' },
            { text: 'Systems', link: '/fairlanes/systems' }
          ]
        },
        {
          text: 'Widgets',
          items: [
            { text: 'Fancy Log', link: '/fairlanes/widgets/fancy_log' },
            { text: 'Account Battle View', link: '/fairlanes/widgets/account_battle_view' },
            { text: 'Combatant', link: '/fairlanes/widgets/combatant' }
          ]
        },
        {
          text: 'Seerin',
          items: [
            { text: 'Combat Engine', link: '/seerin/combat_engine' }
          ]
        },
        {
          text: 'Foundations',
          items: [
            { text: 'Foundations Index', link: '/foundations/' },
            { text: 'RAII', link: '/foundations/raii' },
            { text: 'ECS', link: '/foundations/ecs' },
            { text: 'Event-Driven Architecture', link: '/foundations/event-driven' },
            { text: 'FSM', link: '/foundations/fsm' },
            { text: 'Unit Testing', link: '/foundations/unit-testing' },
            { text: 'Lambdas', link: '/foundations/lambdas' }
          ]
        },
        {
          text: 'Libraries',
          items: [
            { text: 'boost_ext::sml', link: '/libraries/boost_ext_sml' },
            { text: 'Catch2', link: '/libraries/catch2' },
            { text: 'EnTT', link: '/libraries/entt' },
            { text: 'eventpp', link: '/libraries/eventpp' }
          ]
        }
      ],

      outline: 'deep'
    },

    mermaid: {
      // optional mermaid config goes here
    }
  })
)
