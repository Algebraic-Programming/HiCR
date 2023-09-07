FROM docker.io/python:3.11-alpine

RUN apk update

RUN apk add build-base
RUN apk add doxygen
RUN apk add graphviz
RUN apk add ttf-freefont
RUN apk add texlive
RUN apk add texmf-dist-latexextra
RUN apk add texlive-dvi
RUN apk add texmf-dist-fontsextra
RUN apk add libffi-dev

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

RUN apk add git
RUN apk add bash
RUN apk add openssh-client
RUN apk add clang-extra-tools

