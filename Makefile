PREFIX=/usr/local
.PHONY: all lib clean install uninstall
CXX      = g++
CFLAGS   = -Wall
HEADERS = $(shell echo include/*.h)
CPPFLAGS = $(CFLAGS) -I/usr/local/include -Iinclude -L/usr/local/lib

LIBRARIES= rabbitmq ssl crypto
LIBS     = $(addprefix -l,$(LIBRARIES))

LIBNAME  = amqpcpp
LIBFILE  = lib$(LIBNAME).a

SOURCES  = $(shell echo src/*.cpp)
EXFILES  = $(shell echo examples/*.cpp)
EXAMPLES = $(EXFILES:.cpp=)
OBJECTS  = $(SOURCES:.cpp=.o)


all: lib $(EXAMPLES)

lib: $(LIBFILE)

$(LIBFILE): $(OBJECTS)
	$(AR) rcs $@ $(OBJECTS)

$(EXAMPLES): $(EXFILES) $(LIBFILE)
	$(CXX) $(CPPFLAGS) -o $@ $@.cpp  $(LIBFILE) $(LIBS)

clean:
	rm -f $(OBJECTS) $(EXAMPLES) $(LIBFILE)

install:
	install ./$(LIBFILE) $(PREFIX)/lib
	cp -v include/*.h $(PREFIX)/include

uninstall:
	rm -f $(PREFIX)/lib/$(LIBFILE)
