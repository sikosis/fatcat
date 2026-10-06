NAME := fatcat
CLI := fatcat-cli
CXX ?= g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -O2 -Isrc
APP_SOURCES := src/main.cpp src/FatCatApp.cpp src/MainWindow.cpp src/BreakWindow.cpp \
	src/CatView.cpp src/Session.cpp src/Preferences.cpp src/DeskbarView.cpp
APP_OBJECTS := $(APP_SOURCES:.cpp=.o)
DESKBAR_OBJECT := src/DeskbarView.o
LIBS := -lbe -ltranslation

.PHONY: all clean install uninstall check test

all: $(NAME) $(CLI) FatCatDeskbar.so

$(NAME): $(APP_OBJECTS) FatCat.rsrc
	$(CXX) -o $@ $(APP_OBJECTS) $(LIBS)
	xres -o $@ FatCat.rsrc
	mimeset -f $@

FatCatDeskbar.so: $(DESKBAR_OBJECT) FatCatDeskbar.rsrc
	$(CXX) -shared -o $@ $< -lbe
	xres -o $@ FatCatDeskbar.rsrc
	mimeset -f $@

$(CLI): src/control.cpp src/Messages.h FatCatControl.rsrc
	$(CXX) $(CXXFLAGS) -o $@ src/control.cpp -lbe
	xres -o $@ FatCatControl.rsrc
	mimeset -f $@

FatCat.rsrc: FatCat.rdef artwork/fatcat-icon.hvif
	rc -o $@ $<

FatCatDeskbar.rsrc: FatCatDeskbar.rdef artwork/fatcat-icon.hvif
	rc -o $@ $<

FatCatControl.rsrc: FatCatControl.rdef
	rc -o $@ $<

src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -fPIC -c -o $@ $<

check:
	$(CXX) $(CXXFLAGS) -fsyntax-only $(APP_SOURCES)
	$(CXX) $(CXXFLAGS) -fsyntax-only src/DeskbarView.cpp
	$(CXX) $(CXXFLAGS) -fsyntax-only src/control.cpp

test: tests/session_test
	./tests/session_test

tests/session_test: tests/session_test.cpp src/Session.cpp src/Session.h
	$(CXX) $(CXXFLAGS) -o $@ tests/session_test.cpp src/Session.cpp -lbe

install: all
	sh scripts/install.sh

uninstall:
	sh scripts/uninstall.sh

clean:
	rm -f $(APP_OBJECTS) $(DESKBAR_OBJECT) $(NAME) $(CLI) FatCatDeskbar.so *.rsrc tests/session_test
