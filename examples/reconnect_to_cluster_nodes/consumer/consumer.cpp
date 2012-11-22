#include <cstdlib>
#include <ctime>
#include <iostream>
#include <sstream>
#include <AMQPcpp.h>
#include "SynchronizedQueue.h"

int ports[3] = {5672, 5673, 5674};

typedef std::string  protocol_t;
SynchronizedQueue<protocol_t> m_q[1];

int onCancel(AMQPMessage * message ) {
    std::cout << "cancel tag="<< message->getDeliveryTag() << std::endl;
    return 0;
}

int i=0;
int onMessage( AMQPMessage * message  ) {
    i++;
    uint32_t j = 0;
    const char * data = message->getMessage(&j);
    ISynchronizedQueue<protocol_t>* pQ = (ISynchronizedQueue<protocol_t>*)m_q;
    pQ->add(data);
    //if (0 == i % 1000 && data)
    //    cout << data << endl;

    //cout << "#" << i << " tag="<< message->getDeliveryTag()
    //     << " content-type:"<< message->getHeader("Content-type") ;
    //cout << " encoding:"<< message->getHeader("Content-encoding")
    //     << " mode="<<message->getHeader("Delivery-mode")<<endl;

    //if (i > 10) {
    //    AMQPQueue * q = message->getQueue();
    //    q->Cancel( message->getConsumerTag() );
    //}
    return 0;
};


int counter = 0;
void handle() {
    ISynchronizedQueue<protocol_t>* pQ = (ISynchronizedQueue<protocol_t>*)m_q;
    while(true)
    {
        boost::this_thread::sleep( boost::posix_time::milliseconds(50) );
 
        protocol_t  protocol;
        while (pQ->get(protocol)) {
            counter++;
            cout << protocol << endl;
            //if (0 == counter % 1000) {
            //    cout << protocol << endl;
            //}
        }
    }
}

int reconnects = 0;
void consume() {
    try {
        reconnects++;
        std::cout << "Connecting:" << reconnects << "..." << std::endl;

        srand((unsigned)time(0));
        std::stringstream ss;
        ss << "guest:guest@localhost:" << ports[rand() % 3];

        AMQP amqp(ss.str());

        AMQPQueue * queue = amqp.createQueue("hello-queue");
        queue->Declare();
        queue->Bind( "hello-exchange", "hola");

        std::cout << "Connected." << std::endl;
        reconnects = 0;

        queue->setConsumerTag("hello-consumer");
        queue->addEvent(AMQP_MESSAGE, onMessage );
        queue->addEvent(AMQP_CANCEL, onCancel );

        //queue->Consume();
        queue->Consume(AMQP_NOACK);

        throw AMQPException("Something happened in Consume." );

    } catch (AMQPException e) {
        std::cout << e.getMessage() << std::endl;
        boost::this_thread::sleep( boost::posix_time::milliseconds(3000) );
        consume();
    }
}


int main() {
    boost::thread_group threads;

    threads.create_thread(handle);
    threads.create_thread(consume);
    
    // Wait for Threads to finish.
    threads.join_all();

    return 0;
}
