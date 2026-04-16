import { defineConfig } from 'vitepress'
import { withMermaid } from 'vitepress-plugin-mermaid'

export default withMermaid(
  defineConfig({
    title: 'Fairlanes Tour',
    description: 'Guided tour of the Fairlanes codebase',
    base: '/fairlanes/',

    themeConfig: {
      nav: [
        { text: 'Tour Home', link: '/' },
        { text: 'Architecture', link: '/architecture' },
        { text: 'Event Flow', link: '/event-flow' }
      ],
      sidebar: [
        {
          text: 'Guided Tour',
          items: [
            { text: 'Overview', link: '/' },
            { text: 'Architecture', link: '/architecture' },
            { text: 'Event Flow', link: '/event-flow' }
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
