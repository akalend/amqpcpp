#include "amqpcpp.h"
#include <iostream>

int i=0;

void onCancel(AMQPMessage * message ) {
	cout << "cancel tag="<< message->getDeliveryTag() << endl;
}

int  onMessage( AMQPMessage * message  ) {

	char * data = message->getMessage();
	if (data)
		  cout << data << endl;
	
	i++;
	
	cout << "#" << i << " tag="<< message->getDeliveryTag() << " content-type:"<< message->getHeader("Content-type") ;
	cout << " encoding:"<< message->getHeader("Content-encoding")<< " mode="<<message->getHeader("Delivery-mode")<<endl;
	
	if (i > 10) {	
		AMQPQueue * q = message->getQueue();
		q->Cancel( message->getConsumerTag() );
	}	
	return 0;
};


int main () {

	
	try {
//		AMQP amqp("123123:akalend@localhost/private");
		
		AMQP amqp("123123:akalend@localhost:5673/private");
		
		AMQPQueue * qu2 = amqp.createQueue("q2");
		
		qu2->Declare();
		qu2->Bind( "e", "");		

		qu2->setConsumerTag("tag_123");
		qu2->addEvent(AMQP_MESSAGE, (void*) onMessage );
		qu2->addEvent(AMQP_CANCEL, (void*) onCancel );	

		qu2->Consume(AMQP_NOACK);//
			
										
	} catch (AMQPException e) {
		std::cout << e.getMessage() << std::endl;
	}

	return 0;					

}

