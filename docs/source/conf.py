# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#
import os
import sys
import shutil
sys.path.insert(0, os.path.abspath('.'))

import subprocess as sp

# -- Project information -----------------------------------------------------
project = 'HiCR'
copyright = 'Huawei Technologies Switzerland AG'
author = 'Sergio Martin'

# -- General configuration ---------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
   "sphinxcontrib.needs",
	"sphinxcontrib.plantuml",
	"sphinx.ext.mathjax",
	"sphinx.ext.ifconfig",
	"sphinx.ext.autodoc",
	"autoapi.extension",
	"sphinx.ext.viewcode",
	"myst_parser",
	"matplotlib.sphinxext.plot_directive",
	"sphinx.ext.duration",
	"sphinx.ext.napoleon",
	"sphinx.ext.graphviz",
	"sphinx.ext.todo",
	"sphinx_copybutton",
	"sphinxcontrib.doxylink",
	"sphinx.ext.inheritance_diagram",
	"sphinx_design",
]

# Plantuml
plantuml = "java -Djava.awt.headless=true -jar /usr/share/plantuml/plantuml.jar"
plantuml_output_format = "svg"

# Autoapi
autoapi_dirs = ["."]
autoapi_root = "auto_api"
autoapi_options = ["members", "undoc-members", "show-inheritance", "show-inheritance-diagram", "show-module-summary"]
autoapi_keep_files = True
autoapi_add_toctree_entry = False
autodoc_typehints = "signature"

# Myst
myst_enable_extensions = ["colon_fence"]
myst_heading_anchors = 4

# The suffix(es) of source filenames.
# You can specify multiple suffix as a list of string:
source_suffix = '.rst'

# The master toctree document.
master_doc = 'index'

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = []

# The name of the Pygments (syntax highlighting) style to use.
pygments_style = 'sphinx'

# Tell sphinx what the primary language being documented is
primary_domain = 'cpp'

# Tell sphinx what the pygments highlight language should be
highlight_language = 'cpp'

# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
html_theme = 'sphinx_rtd_theme'
html_options = {             "show_nav_level": 1,
            "collapse_navigation": True,
            "github_url": "https://gitlab.huaweirc.ch/zrc-von-neumann-lab/runtime-system-innovations/hicr",
            "repository_url": "https://gitlab.huaweirc.ch/zrc-von-neumann-lab/runtime-system-innovations/hicr",
            "logo_only": False, }

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = [ ]

# If false, no module index is generated.
html_domain_indices = True

# If false, no index is generated.
html_use_index = True

# If true, the index is split into individual pages for each letter.
html_split_index = False

# If true, links to the reST sources are added to the pages.
html_show_sourcelink = False

# If true, "Created using Sphinx" is shown in the HTML footer. Default is True.
html_show_sphinx = False

# If true, "(C) Copyright ..." is shown in the HTML footer. Default is True.
html_show_copyright = True

# Doxylink (note that the second parameter of the tuple indicates a path relative to
# the sphinx output home)
doxygen_root = "source/doxygen"
doxylink = {
    "HiCR": (
        f"{doxygen_root}/html/tagfile.xml",
        f"{doxygen_root}/html",
    ),
}
