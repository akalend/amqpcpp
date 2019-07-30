CXX      = g++
CFLAGS   = -Wall
CPPFLAGS = $(CFLAGS) -Irabbitmq-c/librabbitmq -I/usr/local/include -Lrabbitmq-c/build/librabbitmq -L/usr/local/lib -Iinclude/

LIBRARIES= rabbitmq ssl crypto
LIBS     = $(addprefix -l,$(LIBRARIES))

LIBNAME  = amqpcpp
LIBFILE  = lib$(LIBNAME).a
LIBSO    = lib$(LIBNAME).so

SOURCES  = src/AMQP.cpp src/AMQPBase.cpp src/AMQPException.cpp src/AMQPMessage.cpp src/AMQPExchange.cpp src/AMQPQueue.cpp
EXFILES  = example_publish.cpp example_consume.cpp example_get.cpp
EXAMPLES = $(EXFILES:.cpp=)
OBJECTS  = $(SOURCES:.cpp=.o)


all: lib $(EXAMPLES)

lib: $(LIBFILE) $(LIBSO)

$(LIBFILE): $(OBJECTS)
	$(AR) rcs $@ $(OBJECTS)

$(LIBSO): $(OBJECTS)
	$(CXX) $(CPPFLAGS) -fPIC -shared $(SOURCES) -o $(LIBSO)
	
$(EXAMPLES): $(addprefix examples/,$(EXFILES)) $(LIBFILE)
	$(CXX) $(CPPFLAGS) -o $@ examples/$@.cpp $(LIBFILE) $(LIBS)

install:
	cp $(LIBSO) /usr/local/lib
	ldconfig
	cp include/* /usr/local/include/
	
uninstall:
	rm /usr/local/lib/$(LIBSO)
	rm /usr/local/include/AMQPcpp.h
	
clean:
	rm -f $(OBJECTS) $(EXAMPLES) $(LIBFILE) $(LIBSO)
