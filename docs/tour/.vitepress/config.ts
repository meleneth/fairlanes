import { defineConfig } from 'vitepress'
import mermaid from 'mermaid'
import markdownItMermaid from 'markdown-it-mermaid-plugin'

export default defineConfig({
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

  markdown: {
    config(md) {
      md.use(markdownItMermaid, mermaid)
    }
  }
})
