FROM python:3.8

RUN pip install --upgrade pip
RUN pip install sphinx
RUN pip install sphinx_rtd_theme
RUN pip install sphinxcontrib-bibtex
RUN pip install doxysphinx
RUN pip install sphinxcontrib.needs
RUN pip install sphinxcontrib.plantuml
RUN pip install autoapi
RUN pip install sphinx-autoapi
RUN pip install myst_parser
RUN pip install sphinx_copybutton
RUN pip install sphinxcontrib.doxylink
RUN pip install sphinx_design
RUN pip install Sphinx-Substitution-Extensions
RUN pip install sphinx_toolbox
RUN pip install sphinx-theme
RUN pip install sphinx-book-theme
RUN pip install 'clang-format==18.1.0'

RUN apt update
RUN apt install -y build-essential
RUN apt install -y doxygen
RUN apt install -y graphviz
RUN apt install -y fonts-freefont-ttf
RUN apt install -y texlive
RUN apt install -y texlive-latex-extra
RUN apt install -y texlive-fonts-extra
RUN apt install -y libffi-dev
RUN apt install -y ghostscript
RUN apt install -y texlive-extra-utils
RUN apt install -y texlive-font-utils



