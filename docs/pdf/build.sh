#!/bin/bash
set -e
MAIN_FILE="Chasing_the_Moment_of_Impact"

echo "==================================================="
echo " Starting Compilation for $MAIN_FILE.tex"
echo "==================================================="

echo "[1/5] Cleaning up old files..."
rm -f $MAIN_FILE.aux $MAIN_FILE.bbl $MAIN_FILE.blg $MAIN_FILE.log $MAIN_FILE.out $MAIN_FILE.toc
echo "[2/5] Running XeLaTeX (Pass 1)..."
xelatex -interaction=nonstopmode $MAIN_FILE.tex
echo "[3/5] Running BibTeX..."
bibtex $MAIN_FILE
echo "[4/5] Running XeLaTeX (Pass 2)..."
xelatex -interaction=nonstopmode $MAIN_FILE.tex
echo "[5/5] Running XeLaTeX (Pass 3)..."
xelatex -interaction=nonstopmode $MAIN_FILE.tex

echo "==================================================="
echo " Done! PDF generated at: $MAIN_FILE.pdf"
echo "==================================================="