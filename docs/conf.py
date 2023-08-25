# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

from pathlib import Path
from sphinx.application import Sphinx
import sys

HERE = Path(__file__).parent.resolve()
sys.path.append(str(HERE))
import refpages

project = "lmno"
copyright = "2023, vector-of-bool"
author = "vector-of-bool"
release = "0.0.0"

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

# extensions = ['sphinx_math_dollar']

templates_path = []
exclude_patterns = ["Thumbs.db", ".DS_Store", "prolog.rst", "ref-in/*.rst"]
nitpicky = True
# cpp_debug_lookup = True
# cpp_debug_show_tree = True

refpages.generate(HERE / "ref-in", HERE / "ref/api")

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = "furo"
html_static_path = ["static"]
highlight_language = "cpp"
primary_domain = "cpp"
default_role = "cpp:expr"
html_theme_options = {
    # 'logo': {
    #     'image_light': 'light.png',
    #     'image_dark': 'dark.png',
    # }
}

rst_prolog = r"""
.. include:: /prolog.rst
"""

mathjax3_config = {
    "chtml": {
        "displayAlign": "left",
    },
    "tex": {
        "inlineMath": [["$", "$"], ["\\(", "\\)"]],
    },
}

from pygments.lexers.apl import APLLexer
from pygments import token


class LMNOLexer(APLLexer):
    name = "lmno"
    aliases = ["lmno"]

    tokens = APLLexer.tokens | {
        "root": [
            (r"\s+", token.Whitespace),
            (r"\(:.*?:\)", token.Comment),
            (r"[\]\[()·;{}$.:]", token.Punctuation),
            (r"[‿˘˜¬¨˙⁼⌜´`/\\∘○⚇⎉⌾⊸⟜φ]", token.Name.Attribute),
            (r"[-+×√÷⍳↕⋄≠=#⌊⌈∨∧⌽~⥊]", token.Operator),
            (r"¯?\d+", token.Number),
            (r"[π∞]", token.Name.Constant),
            (r"[αω∞]", token.Name.Constant),
            (r"[←]", token.Keyword.Declaration),
            (r"[A-Za-z_]\w*", token.Name.Variable),
        ]
    }


from docutils.parsers.rst.directives import tables
from sphinx.highlighting import lexers


class SummaryTable(tables.CSVTable):
    def run(self):
        els = super().run()
        tbl = els[0]
        tbl["classes"] += ["summary-table", "mono-links"]
        return els


lexers["lmno"] = LMNOLexer()


def setup(app: Sphinx):
    app.add_css_file("styles.css")
    app.add_directive("summary-table", SummaryTable)
