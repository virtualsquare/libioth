PANDOC=pandoc
PANDOCOK := $(shell command -v ${PANDOC} 2> /dev/null)

none:

% : %.md
ifdef PANDOCOK
# copy copyright notice
		grep "^\.\\\\\"" $< > $@ || true
# run pandoc
		$(eval SECTION := $(subst .,,$(suffix $@)))
		$(eval BASENAME := $(basename $@))
		$(eval TITLE := $(shell echo "${BASENAME}\(${SECTION}\)" | tr [:lower:] [:upper:]))
		$(eval HEADER := "$(shell man ${SECTION} intro | head -1 | sed -e 's/^[^[:blank:]]*[[:blank:]]*//' -e 's/[[:blank:]]*[^[:blank:]]*$$//' )")
		$(PANDOC) -standalone -M title=${TITLE} -M section=${SECTION} -M header=${HEADER} -M footer=${PANDOC_ORG} -M "date=`date +\"%B %Y\"`" --to man $< >> $@
# workaround for missing font warnings
		sed -i -e '/^\.ie/,/^\.\\}/d' $@ || true
		sed -i -e 's/\\f\[V\]/\\f\[CB\]/g' $@ || true
		sed -i -e 's/\\f\[CR\]/\\f\[CB\]/g' $@ || true
else
		echo "${PANDOC} is not available. Manpage $@ cannot be updated" >/dev/stderr >&2
endif
