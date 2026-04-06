"""
Name: Jamie White
Date Started: 25/11/2025
Brief Project Description: App with GUI to manage book list
GitHub URL: (copy-and-paste your URL only on the next line)
https://github.com/cp1404-students/a2-books-Jamie7479
"""

from kivy.app import App
from kivy.app import Builder
from kivy.uix.button import Button




class BooksToReadApp(App):
    """App class with GUI to manage book list."""

    def __init__(self, **kwargs):
        """Construct main Kivy app."""
        super().__init__(**kwargs)


    def build(self):
        """Build the Kivy GUI."""
        self.title = "Books To Read 2.0"
        self.root = Builder.load_file("app.kv")
        return self.root


if __name__ == '__main__':
    BooksToReadApp().run()