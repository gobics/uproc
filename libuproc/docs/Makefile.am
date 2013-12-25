# must correspond to HTML_OUTPUT in doxyfile.in!
html = libuproc-$(PACKAGE_VERSION)

docpkg = $(html).tar.gz

$(docpkg) : doxygen.stamp
	tar chozf $@ $(html)

doxygen.stamp : doxyfile
	$(DOXYGEN) $(DOXYFLAGS) $<
	echo Timestamp > $@

CLEANFILES = doxywarn.txt doxygen.stamp $(docpkg)

clean-local :
	-rm -rf
	-rm -rf $(docpkg)
	-rm -rf $(html)

install-data-local : $(docpkg)
	@$(NORMAL_INSTALL)
	$(MKDIR_P) $(DESTDIR)$(docdir)
	tar -C $(DESTDIR)$(docdir) -x -f $<

uninstall-local :
	@$(NORMAL_UNINSTALL)
	-rm -rf $(DESTDIR)$(docdir)/$(html)
