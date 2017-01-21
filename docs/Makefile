SRC=qctools.md
PDF=$(SRC:.md=.pdf)
HTML=$(SRC:.md=.html)

$(info PDF and HTML rendering has been tested with pandoc version 1.13.2.1, some older versions are known to produce very poor output, please ensure your pandoc is recent enough.)

all: qctools.html qctools.pdf
	
qctools.html: qctools.md
	pandoc -o "$@" "$<"

qctools.pdf:  qctools.md
	pandoc -o "$@" "$<" --latex-engine=xelatex

clean:
	rm -f qctools.pdf qctools.html
