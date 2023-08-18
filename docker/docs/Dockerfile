FROM docker.io/python:3.8-alpine

RUN apk update

RUN apk add build-base
RUN apk add doxygen
RUN apk add graphviz
RUN apk add ttf-freefont
RUN apk add texlive
RUN apk add texmf-dist-latexextra
RUN apk add texlive-dvi
RUN apk add texmf-dist-fontsextra

RUN pip install sphinx
RUN pip install sphinx_rtd_theme
RUN pip install sphinxcontrib-bibtex
RUN pip install doxysphinx

RUN apk add git


