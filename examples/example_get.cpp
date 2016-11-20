#include "AMQPcpp.h"

using namespace std;

int main () {



	try {
//		AMQP amqp("123123:akalend@localhost/private");
		AMQP amqp("123123:akalend@localhost:5673/private");		

		AMQPQueue * qu2 = amqp.createQueue("q2");
		qu2->Declare();		
		
		
		while (  1 ) {
			qu2->Get(AMQP_NOACK);

			AMQPMessage * m= qu2->getMessage();
			
			cout << "count: "<<  m->getMessageCount() << endl;											 
			if (m->getMessageCount() > -1) {
			uint32_t j = 0;
			cout << "message\n"<< m->getMessage(&j) << "\nmessage key: "<<  m->getRoutingKey() << endl;
			cout << "exchange: "<<  m->getExchange() << endl;											
			cout << "Content-type: "<< m->getHeader("Content-type") << endl;	
			cout << "Content-encoding: "<< m->getHeader("Content-encoding") << endl;	
			} else 
				break;				
						
							
		}	
	} catch (AMQPException e) {
		std::cout << e.getMessage() << std::endl;
	}

	return 0;					

}
