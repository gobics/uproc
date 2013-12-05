docpkg = $(PACKAGE_TARNAME)-doxy-$(PACKAGE_VERSION).tar.gz

doc_DATA = $(docpkg)

$(docpkg) : doxygen.stamp
	tar chozf $@ html

doxygen.stamp : doxyfile
	$(DOXYGEN) $(DOXYFLAGS) $<
	echo Timestamp > $@

CLEANFILES = doxywarn.txt doxygen.stamp $(docpkg)

clean-local :
	-rm -rf html
