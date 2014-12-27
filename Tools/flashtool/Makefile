

UI_FILES += $(wildcard *.ui)
UI_FILES += $(wildcard DockWindows/*.ui)


UI_TARGETS = $(addsuffix _ui.py, $(basename $(UI_FILES)))


all: $(UI_TARGETS)

clean:
	@echo
	@echo Cleaning project:
	rm -f $(UI_TARGETS)
	rm -f *.pyc
	rm -f DockWindows/*.pyc


# Define programs and commands
#
PYUIC = /cygdrive/c/Python27/Lib/site-packages/PyQt4/pyuic4.bat


# Convert Qt .ui files to python.  
# - Filter out "Created: <date>" to prevent git merge conflicts
# - Rewrite Image paths
#
%_ui.py: %.ui
	@echo
	@echo Converting ui file: $@
	$(PYUIC) -x $< | \
	    grep -v "^# *Created:" | \
	    grep -v "^# *by:" | \
	    sed "s/\.\.\/Images/Images/g" > $@

