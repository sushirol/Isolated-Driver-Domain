
del %1.pdf %1.aux %1.bbl %1.blg %1.log %1.out %1.dvi
latex %1
bibtex %1
latex  %1
latex  %1
::check  %1
del %1.pdf  %1.bbl %1.blg %1.log %1.out
dvipdfm %1.dvi
::dvips %1.dvi
::ps2pdf.exe %1.ps
::check %1
del %1.aux %1.dvi
    
