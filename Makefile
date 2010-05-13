all: lib example_publish example_consume example_get

lib:
	gcc -c AMQPBase.cpp AMQPException.cpp AMQPMessage.cpp AMQPConnection.cpp AMQPExchange.cpp AMQPQueue.cpp
	ar rcs libamqpcpp.a *o

example_publish:
	g++ -o example_publish  -lamqpcpp -lrabbitmq    -Iamqpcpp -I/usr/local/include -L/usr/local/lib  -L.  example_publish.cpp

example_consume:
	g++ -o example_consume  -lamqpcpp -lrabbitmq    -Iamqpcpp -I/usr/local/include -L/usr/local/lib  -L.  example_consume.cpp

example_get:
	g++ -o example_get  -lamqpcpp -lrabbitmq    -Iamqpcpp -I/usr/local/include -L/usr/local/lib  -L.  example_get.cpp

clean:
	rm *o
	rm *a
	rm example_publish example_consume example_get