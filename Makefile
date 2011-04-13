all: lib example_publish example_consume example_get

lib:
	gcc -c AMQPBase.cpp AMQPException.cpp AMQPMessage.cpp AMQPConnection.cpp AMQPExchange.cpp AMQPQueue.cpp
	ar rcs libamqpcpp.a *o

example_publish:
	g++ -o example_publish example_publish.cpp -lamqpcpp -lrabbitmq -Iamqpcpp -I/usr/local/include -L/usr/local/lib -L.  

example_consume:
	g++ -o example_consume example_consume.cpp -lamqpcpp -lrabbitmq -Iamqpcpp -I/usr/local/include -L/usr/local/lib -L.  

example_get:
	g++ -o example_get     example_get.cpp     -lamqpcpp -lrabbitmq -Iamqpcpp -I/usr/local/include -L/usr/local/lib -L.  

clean:
	rm *o
	rm *a
	rm example_publish example_consume example_get