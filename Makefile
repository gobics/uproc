include config.mk

CPPFLAGS += -Ilibupro/$(INCDIR)

ARCHIVE := libupro/$(ARCHIVENAME)
TARGETS = upro-dna upro-prot upro-orf ecurve-convert ecurve-build ecurve-calib



default : $(TARGETS)


upro-dna : main.c $(ARCHIVE)
	@echo CC $@
	@$(CC) $(CPPFLAGS) $(CFLAGS) -DMAIN_DNA -o $@ $^ $(LIBS)

upro-prot : main.c $(ARCHIVE)
	@echo CC $@
	@$(CC) $(CPPFLAGS) $(CFLAGS) -DMAIN_PROT -o $@ $^ $(LIBS)

upro-orf : orf.c $(ARCHIVE)
	@echo CC $@
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $^ $(LIBS)

ecurve-% : %.c $(ARCHIVE)
	@echo CC $@
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $^ $(LIBS)

$(ARCHIVE) :
	@$(MAKE) -C libupro $(ARCHIVENAME)

clean :
	rm -f $(TARGETS)
	$(MAKE) -C libupro clean

.PHONY : clean $(ARCHIVE)
