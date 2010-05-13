/*
 *  AMQPQueue.cpp
 *  rabbitmq++
 *
 *  Created by Alexandre Kalendarev on 12.03.10.
 *
 */

#include <iostream>
#include <cstring>

#include "amqpcpp.h"

using namespace std;

AMQPQueue::AMQPQueue(amqp_connection_state_t * cnn, int channelNum) {
		this->cnn = cnn;
		this->channelNum = channelNum;

//		std::cout << "AMQPQueue()\n";;				
		consumer_tag.bytes=NULL;
		consumer_tag.len=0;
		delivery_tag =0;	
		pmessage=NULL;
		openChannel();
}	

AMQPQueue::AMQPQueue(amqp_connection_state_t * cnn, int channelNum, string name) {
		this->cnn = cnn;
		this->channelNum = channelNum;
		this->name = name;
		
		consumer_tag.bytes=NULL;
		consumer_tag.len=0;
		delivery_tag =0;
		pmessage=NULL;				
		openChannel();
}

AMQPQueue::~AMQPQueue() {
//	std::cout << "~AMQPQueue()\n";
	this->closeChannel();
	if (pmessage)
		delete pmessage;
}
	
// Declare command /* 50, 10; 3276810 */

void AMQPQueue::Declare() {
	AMQPQueue::parms=0;
	AMQPQueue::sendDeclareCommand();

}

void AMQPQueue::Declare(string name) {
 	AMQPQueue::parms=AMQP_AUTODELETE;
	AMQPQueue::name=name;
	AMQPQueue::sendDeclareCommand();

}

void AMQPQueue::Declare(const char * name) {
	AMQPQueue::parms=AMQP_AUTODELETE;
	AMQPQueue::name=name;
	AMQPQueue::sendDeclareCommand();
}

void AMQPQueue::Declare(string name, short parms) {
 	AMQPQueue::parms=parms;
	AMQPQueue::name=name;
	AMQPQueue::sendDeclareCommand();

}

void AMQPQueue::Declare(const char * name, short parms) {
	AMQPQueue::parms=parms;
	AMQPQueue::name=name;
	AMQPQueue::sendDeclareCommand();
}


void AMQPQueue::sendDeclareCommand() {

	if (!name.size())
		throw AMQPException("the queue must to have the name");
		
	amqp_bytes_t queue_name = amqp_cstring_bytes(name.c_str());

/*
	amqp_basic_properties_t props;
    props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG;
    props.content_type = amqp_cstring_bytes("text/plain");
*/
	amqp_table_t args;
	args.num_entries = 0;
	args.entries = NULL;  
											  
	amqp_boolean_t exclusive =	(parms & AMQP_EXCLUSIVE)	? 1:0;
	amqp_boolean_t passive =	(parms & AMQP_PASSIVE)		? 1:0;
	amqp_boolean_t autodelete = (parms & AMQP_AUTODELETE)	? 1:0;
	amqp_boolean_t durable =	(parms & AMQP_DURABLE)		? 1:0;

	//state, channel, classname, requestname, replyname, structname,

//	cout<< "start Declare\n";
	amqp_rpc_reply_t res = AMQP_SIMPLE_RPC(*cnn, 
				channelNum,
				QUEUE, DECLARE, DECLARE_OK,
				amqp_queue_declare_t,
				0,
				queue_name, passive, durable, exclusive, autodelete, 0, 
				args);
	AMQPBase::checkReply(&res);		
	amqp_release_buffers(*cnn);
	char error_message [256];
	bzero(error_message,256);

	
		if ( res.reply_type == AMQP_RESPONSE_NONE) {
			throw AMQPException("error the QUEUE.DECLARE command, response none");		
		}
		
		if ( res.reply.id == AMQP_CHANNEL_CLOSE_METHOD ) {

			amqp_channel_close_t * err = (amqp_channel_close_t *) res.reply.decoded;
			
			int c_id = 	(int) err->class_id;
			sprintf( error_message, "server error %d, message '%s' class=%d method=%d ", err->reply_code, err->reply_text.bytes, c_id,err->method_id);
//			cout << "****** "<< error_message << endl;
			opened=0;

			throw AMQPException( &res);
		} if (res.reply.id == AMQP_QUEUE_DECLARE_OK_METHOD) {

				amqp_queue_declare_ok_t* data = (amqp_queue_declare_ok_t*) res.reply.decoded;
				count = data->message_count;
				

		} else {
			sprintf( error_message, "error the Declare command  receive method=%d", res.reply.id);
			throw AMQPException(error_message );
		}
			
}

// Delete command  /* 50, 40; 3276840 */

void AMQPQueue::Delete() {
	
	if (!name.size())
			throw AMQPException::AMQPException("the name of queue not set");
	
	AMQPQueue::sendDeleteCommand();
}

void AMQPQueue::Delete(const char * name) {
	 AMQPQueue::name=name;
	 AMQPQueue::sendDeleteCommand();
}

void AMQPQueue::Delete(string name) {
	 AMQPQueue::name=name;
	 AMQPQueue::sendDeleteCommand();
}


void AMQPQueue::sendDeleteCommand(){
	 
	amqp_bytes_t queue = amqp_cstring_bytes(name.c_str());

	amqp_queue_delete_t s;
		s.ticket = 0;
		s.queue = queue;
		s.if_unused =	( AMQP_IFUNUSED & parms ) ? 1:0;
		s.nowait =		( AMQP_NOWAIT & parms ) ? 1:0;

	amqp_method_number_t method_ok = AMQP_QUEUE_DELETE_OK_METHOD; 
			
	amqp_rpc_reply_t res = amqp_simple_rpc(*cnn, channelNum, 
			AMQP_QUEUE_DELETE_METHOD, 
			&method_ok , &s);
	
	AMQPBase::checkReply(&res);			
}


// Purge command /* 50, 30; 3276830 */


void AMQPQueue::Purge() {
	
	if (!name.size())
			throw AMQPException::AMQPException("the name of queue not set");
	
	AMQPQueue::sendPurgeCommand();
}

void AMQPQueue::Purge(const char * name) {
	 AMQPQueue::name=name;
	 AMQPQueue::sendPurgeCommand();
}

void AMQPQueue::Purge(string name) {
	 AMQPQueue::name=name;
	 AMQPQueue::sendPurgeCommand();
}


void AMQPQueue::sendPurgeCommand(){
	 
	amqp_bytes_t queue = amqp_cstring_bytes(name.c_str());

	amqp_queue_delete_t s;
		s.ticket = 0;
		s.queue = queue;
		s.nowait = ( AMQP_NOWAIT & parms ) ? 1:0;

	amqp_method_number_t method_ok = AMQP_QUEUE_PURGE_OK_METHOD; 
			
	amqp_rpc_reply_t res = amqp_simple_rpc(*cnn, channelNum, 
			AMQP_QUEUE_PURGE_METHOD, 
			&method_ok , &s);
	
	AMQPBase::checkReply(&res);		
}


// Bind command /* 50, 20; 3276820 */

void AMQPQueue::Bind(const char * name, string key) {
	AMQPQueue::sendBindCommand(name, key.c_str() );
}

void AMQPQueue::Bind(const char * name, const char * key) {
	AMQPQueue::sendBindCommand(name, key);
}

void AMQPQueue::Bind(string name, const char * key) {
	AMQPQueue::sendBindCommand(name.c_str(), key);
}

void AMQPQueue::Bind(string name, string key) {
	AMQPQueue::sendBindCommand(name.c_str(), key.c_str());
}

void AMQPQueue::sendBindCommand(const char * exchange, const char * key){
	 
	amqp_bytes_t queueByte = amqp_cstring_bytes(name.c_str());
	amqp_bytes_t exchangeByte = amqp_cstring_bytes(exchange);
	amqp_bytes_t keyByte = amqp_cstring_bytes(key);
	
    amqp_queue_bind_t s;
		s.ticket = 0;
		s.queue = queueByte;
		s.exchange = exchangeByte;
		s.routing_key = keyByte;
		s.nowait = ( AMQP_NOWAIT & parms ) ? 1:0;
		s.arguments.num_entries = 0;
		s.arguments.entries = NULL;

	
	amqp_method_number_t method_ok = AMQP_QUEUE_BIND_OK_METHOD;
    amqp_rpc_reply_t res = amqp_simple_rpc( *cnn,
					channelNum,
					AMQP_QUEUE_BIND_METHOD,
					&method_ok, &s);

	AMQPBase::checkReply(&res);
	
}


// UnBind command /* 50, 50; 3276850 */

void AMQPQueue::unBind(const char * name, string key) {
	AMQPQueue::sendUnBindCommand(name, key.c_str() );
}

void AMQPQueue::unBind(const char * name, const char * key) {
	AMQPQueue::sendUnBindCommand(name, key);
}

void AMQPQueue::unBind(string name, const char * key) {
	AMQPQueue::sendUnBindCommand(name.c_str(), key);
}

void AMQPQueue::unBind(string name, string key) {
	AMQPQueue::sendUnBindCommand(name.c_str(), key.c_str());
}

void AMQPQueue::sendUnBindCommand(const char * exchange, const char * key){
	 
	amqp_bytes_t queueByte = amqp_cstring_bytes(name.c_str());
	amqp_bytes_t exchangeByte = amqp_cstring_bytes(exchange);
	amqp_bytes_t keyByte = amqp_cstring_bytes(key);
	
    amqp_queue_bind_t s;
		s.ticket = 0;
		s.queue = queueByte;
		s.exchange = exchangeByte;
		s.routing_key = keyByte;
		s.nowait = ( AMQP_NOWAIT & parms ) ? 1:0;
		s.arguments.num_entries = 0;
		s.arguments.entries = NULL;

	
	amqp_method_number_t method_ok = AMQP_QUEUE_UNBIND_OK_METHOD;
    amqp_rpc_reply_t res = amqp_simple_rpc( *cnn,
					channelNum,
					AMQP_QUEUE_UNBIND_METHOD,
					&method_ok, &s);

	AMQPBase::checkReply(&res);
	
}


// GET method /* 60, 71; 3932231 */

void AMQPQueue::Get() {
	AMQPQueue::parms=0;
	AMQPQueue::sendGetCommand();
}

void AMQPQueue::Get(short parms) {
	AMQPQueue::parms=parms;
	AMQPQueue::sendGetCommand();
}

void AMQPQueue::sendGetCommand() {

	amqp_bytes_t queueByte = amqp_cstring_bytes(name.c_str());
	
    amqp_basic_get_t s;	
		s.ticket = 0;
		s.queue = queueByte;
		s.no_ack = ( AMQP_NOACK & parms ) ? 1:0;

  amqp_method_number_t replies[] = { AMQP_BASIC_GET_OK_METHOD,
				     AMQP_BASIC_GET_EMPTY_METHOD,
					 AMQP_CONNECTION_CLOSE_METHOD,
				     0 };
					 



 
	 amqp_rpc_reply_t res = 
     AMQP_MULTIPLE_RESPONSE_RPC( *cnn, 
					channelNum, 
					BASIC,GET,
					replies,
			        amqp_basic_get_t,
			        channelNum,
					queueByte,
					( AMQP_NOACK & parms ) ? 1:0);

	// 3932232 GET_EMPTY
	// 3932231 GET_OK
	// 1310760 CHANNEL_CLOSE

		char error_message [256];
		amqp_frame_t frame;
	

	amqp_release_buffers(*cnn);
	
	
	if (pmessage)
		delete(pmessage);
		
	pmessage = new AMQPMessage(this);
	
		if ( res.reply_type == AMQP_RESPONSE_NONE) {
			throw AMQPException("error the Get command, response none");		
		}
		
		if ( res.reply.id == AMQP_CHANNEL_CLOSE_METHOD ) {
			

			amqp_channel_close_t * err = (amqp_channel_close_t *) res.reply.decoded;
			
			int c_id = 	(int) err->class_id;
			sprintf( error_message, "server error %d, message '%s' class=%d method=%d ", err->reply_code, err->reply_text.bytes, c_id,err->method_id);
//			cout << "****** "<< error_message << endl;
			opened=0;

			throw AMQPException( &res);
		} if (res.reply.id == AMQP_BASIC_GET_EMPTY_METHOD) {
			pmessage->setMessageCount(-1);
			return;		
		} if (res.reply.id == AMQP_BASIC_GET_OK_METHOD) {

				amqp_basic_get_ok_t* data = (amqp_basic_get_ok_t*) res.reply.decoded;
				
				delivery_tag = data->delivery_tag;
				pmessage->setDeliveryTag(data->delivery_tag);

				amqp_bytes_t exName = data->exchange;			
				
				char * dst = (char *) malloc(exName.len+1);
				strncpy(dst, (const char *)exName.bytes, exName.len  );
				*(dst+exName.len) = '\0';
				pmessage->setExchange(dst);	
				free(dst);
				
				amqp_bytes_t routingKey = data->routing_key;			
				
				dst = (char *) malloc(routingKey.len+1);
				strncpy(dst, (const char *)routingKey.bytes, routingKey.len);
				*(dst+routingKey.len) = '\0';
				pmessage->setRoutingKey(dst);	
				free(dst);
								
				pmessage->setMessageCount(data->message_count);	
				

		} else {
			sprintf( error_message, "error the Get command  receive method=%d", res.reply.id);
			throw AMQPException(error_message );
		}
	
	int result;
	size_t len=0;
	char * tmp = NULL;
	char * old_tmp = NULL;
		
	while (1){ // receive  frames: 
		amqp_maybe_release_buffers(*cnn);
		result = amqp_simple_wait_frame(*cnn, &frame);
		if (result <= 0) 
					throw AMQPException(" read frame error");

		if (frame.frame_type == AMQP_FRAME_HEADER){
			//cout << "----  AMQP_FRAME_HEADER   --------\n";
				amqp_basic_properties_t * p = (amqp_basic_properties_t *) frame.payload.properties.decoded;		
				AMQPQueue::setHeaders(p);
				continue;
		}	   

		if (frame.frame_type == AMQP_FRAME_BODY){
		
//			cout << "----  AMQP_FRAME_BODY   --------\n";
			
			uint32_t frame_len = frame.payload.body_fragment.len;	
			
			size_t old_len = len;
			len += frame_len;
			
			if ( tmp ) {
				old_tmp = tmp;
//				cout << "msg len ="<< len << endl;					
				tmp = (char*) malloc(len+1);
				if (!tmp) {
						throw AMQPException("the can't allocae object");
				}
					
				memcpy( tmp, old_tmp, old_len );
				free(old_tmp);
				memcpy(tmp + old_len,frame.payload.body_fragment.bytes, frame_len);
				*(tmp+frame_len+old_len) = '\0';
			
			} else { // the first allocate
				tmp = (char*) malloc(frame_len+1);
				if (!tmp) {
					throw AMQPException("the can't reallocae object");
				}
				strncpy(tmp, (char*) frame.payload.body_fragment.bytes, frame_len);
				*(tmp+frame_len) = '\0';
			}

			if ( frame_len < FRAME_MAX - HEADER_FOOTER_SIZE) 
					break;

			continue;
		}	   
		
	} // end while

	
	if (tmp) {
		pmessage->setMessage(tmp);	
		free(tmp);
	}
	amqp_release_buffers(*cnn);
//	cout << "end message\n";
}

void AMQPQueue::addEvent( AMQPEvents_e eventType, void * event ) {
	if (events.find(eventType) != events.end())
		throw AMQPException("the event alredy added");
		
	events[eventType]= reinterpret_cast<   int(*)( AMQPMessage * )  > (event);
}


void AMQPQueue::Consume() {
	AMQPQueue::parms=0;
	AMQPQueue::sendConsumeCommand();
}

void AMQPQueue::Consume(short parms) {
	AMQPQueue::parms=parms;
	AMQPQueue::sendConsumeCommand();
}


void AMQPQueue::setConsumerTag(string consumer_tag) {
	this->consumer_tag = amqp_cstring_bytes(consumer_tag.c_str());
}
void AMQPQueue::setConsumerTag(const char * consumer_tag) {
	this->consumer_tag = amqp_cstring_bytes(consumer_tag);
}


void AMQPQueue::sendConsumeCommand() {

	amqp_basic_consume_ok_t *consume_ok;
	amqp_bytes_t queueByte = amqp_cstring_bytes(name.c_str());

	char error_message[256];
	 /* 
	amqp_basic_consume_ok_t * res = 
			amqp_basic_consume( *cnn, 
						channelNum, 
					   queueByte,
					   consumer_tag,
					    0, //amqp_boolean_t no_local,
					    1, // amqp_boolean_t no_ack,
					    0 ); //amqp_boolean_t exclusive
	*/
	
	  amqp_method_number_t replies[] = { AMQP_BASIC_CONSUME_OK_METHOD,
					AMQP_CONNECTION_CLOSE_METHOD,
					AMQP_BASIC_CANCEL_OK_METHOD,
					0 };
					 
	 amqp_rpc_reply_t res = 
     AMQP_MULTIPLE_RESPONSE_RPC( *cnn, 
					channelNum, 
					BASIC,CONSUME,
					replies,
			        amqp_basic_consume_t,
			        channelNum,
					queueByte,
					consumer_tag, 
					( AMQP_NOLOCAL & parms ) ? 1:0, //no_local,
					( AMQP_NOACK & parms ) ? 1:0, //no_ack, 
					( AMQP_EXCLUSIVE & parms ) ? 1:0);
	
	
		if ( res.reply_type == AMQP_RESPONSE_NONE) {
			throw AMQPException("error the Consume command, response none");		
		}
		
		if ( res.reply.id == AMQP_CHANNEL_CLOSE_METHOD ) {
			
			amqp_channel_close_t * err = (amqp_channel_close_t *) res.reply.decoded;
			
			int c_id = 	(int) err->class_id;
			sprintf( error_message, "server error %d, message '%s' class=%d method=%d ", err->reply_code, err->reply_text.bytes, c_id,err->method_id);
			opened=0;

			throw AMQPException(error_message);
		} if (res.reply.id == AMQP_BASIC_CANCEL_OK_METHOD) {
			//printf("****** cancel Ok \n");
			return;
		} if (res.reply.id == AMQP_BASIC_CONSUME_OK_METHOD) {					
			consume_ok = (amqp_basic_consume_ok_t*) res.reply.decoded;
			//printf("****** consume Ok c_tag=%s", consume_ok->consumer_tag.bytes );

		}
	
	
	auto_ptr<AMQPMessage> message ( new AMQPMessage(this) );
	pmessage = message.get();
	
	amqp_frame_t frame;
	char * buf=NULL, *pbuf = NULL; 
																
	size_t body_received;
	size_t body_target;
		
	while(1) {

	   amqp_maybe_release_buffers(*cnn);
	   int result = amqp_simple_wait_frame(*cnn, &frame);
		if (result <= 0) return;

		//printf("frame method.id=%d  frame.frame_type=%d\n",frame.payload.method.id, frame.frame_type);		
					
		if (frame.frame_type != AMQP_FRAME_METHOD){
//			cout << "continue  method.id="<< frame.payload.method.id << " ";
//			cout << "frame.frame_type="<< frame.frame_type << endl;
			continue;
		}
	   
		if (frame.payload.method.id == AMQP_BASIC_CANCEL_OK_METHOD){
//			cout << "CANCEL OK method.id="<< frame.payload.method.id << endl;					
			if ( events.find(AMQP_CANCEL) != events.end() ) {
				(*events[AMQP_CANCEL])(pmessage);
			}
			break;
		}


		if (frame.payload.method.id != AMQP_BASIC_DELIVER_METHOD){
//			cout << "NON BASIC_DELIVER_METHOD continue  method.id="<< frame.payload.method.id << endl;					
			continue;
		}
	   
		amqp_basic_deliver_t * delivery = (amqp_basic_deliver_t*) frame.payload.method.decoded;
		
		delivery_tag = delivery->delivery_tag;
		pmessage->setConsumerTag(delivery->consumer_tag);
		pmessage->setDeliveryTag(delivery->delivery_tag);

		pmessage->setExchange(delivery->exchange);
		pmessage->setRoutingKey(delivery->routing_key);
			
					
		result = amqp_simple_wait_frame(*cnn, &frame);
		if (result <= 0) {
			throw AMQPException("The returned read frame is invalid");
			return;
		}
		
	// redelivered=false,exchange=e,routing-key=		
		
		if (frame.frame_type != AMQP_FRAME_HEADER) {
			throw AMQPException("The returned frame type is invalid");
            return;
		}
	
		amqp_basic_properties_t * p = (amqp_basic_properties_t *) frame.payload.properties.decoded;
		
		AMQPQueue::setHeaders(p);

		body_target = frame.payload.properties.body_size;
		body_received = 0;
	   
	    buf = (char*) malloc(body_target+1);
		*(buf+body_target)='\0';
	    pbuf = buf;
		
		while (body_received < body_target) {
		  result = amqp_simple_wait_frame(*cnn, &frame);
		  if (result <= 0) break;

			//printf("frame.frame_type=%d\n", frame.frame_type);
	
	
		  if (frame.frame_type != AMQP_FRAME_BODY) {
				throw AMQPException("The returned frame has no body");
              return;
		  }
		  		  
		  body_received += frame.payload.body_fragment.len;
		  
		  memcpy( pbuf, frame.payload.body_fragment.bytes, frame.payload.body_fragment.len);
		  pbuf += frame.payload.body_fragment.len;

		} // end while 
	   
		pmessage->setMessage(buf);	   
		free(buf);
		
		if ( events.find(AMQP_MESSAGE) != events.end() ) {
			int res = (int)(*events[AMQP_MESSAGE])(pmessage);
//			cout << "res="<<res<<endl;
			if (res) break;
		}
	}
	
}

void AMQPQueue::setHeaders(amqp_basic_properties_t * p) {
		if (pmessage == NULL) 
			return;
			
		if (p->_flags & AMQP_BASIC_CONTENT_TYPE_FLAG) {			
			pmessage->addHeader("Content-type", &p->content_type );
		}

		if (p->_flags & AMQP_BASIC_CONTENT_ENCODING_FLAG) {
			pmessage->addHeader("Content-encoding", &p->content_encoding );
		}

	
		if (p->_flags & AMQP_BASIC_DELIVERY_MODE_FLAG) {
			pmessage->addHeader("Delivery-mode", &p->delivery_mode );
		}
			
		if (p->_flags & AMQP_BASIC_MESSAGE_ID_FLAG) {
			pmessage->addHeader("message_id", &p->message_id );
		}
		
		if (p->_flags & AMQP_BASIC_USER_ID_FLAG) {
			pmessage->addHeader("user_id", &p->user_id );
		}

		if (p->_flags & AMQP_BASIC_APP_ID_FLAG) {
			pmessage->addHeader("app_id", &p->app_id );
		}

		if (p->_flags & AMQP_BASIC_CLUSTER_ID_FLAG) {
			pmessage->addHeader("cluster_id", &p->cluster_id );
		}
	
		if (p->_flags & AMQP_BASIC_CORRELATION_ID_FLAG) {
			pmessage->addHeader("correlation_id", &p->correlation_id );
		}

		if (p->_flags & AMQP_BASIC_PRIORITY_FLAG) {
			pmessage->addHeader("priority", &p->priority );
		}

		if (p->_flags & AMQP_BASIC_TIMESTAMP_FLAG) {
			pmessage->addHeader("timestamp", &p->timestamp );
		}

		if (p->_flags & AMQP_BASIC_EXPIRATION_FLAG) {
			pmessage->addHeader("Expiration", &p->expiration );
		}
		
		if (p->_flags & AMQP_BASIC_TYPE_FLAG) {
			pmessage->addHeader("type", &p->type);
		}
		
		if (p->_flags & AMQP_BASIC_REPLY_TO_FLAG) {
			pmessage->addHeader("Reply-to", &p->reply_to);
		}

}

void AMQPQueue::Cancel(string consumer_tag){
	this->consumer_tag = amqp_cstring_bytes(consumer_tag.c_str());
	AMQPQueue::sendCancelCommand();
}

void AMQPQueue::Cancel(const char * consumer_tag){
	this->consumer_tag = amqp_cstring_bytes(consumer_tag);
	AMQPQueue::sendCancelCommand();
}

void AMQPQueue::Cancel(amqp_bytes_t consumer_tag){
	this->consumer_tag.len = consumer_tag.len;
	this->consumer_tag.bytes = consumer_tag.bytes;	
	AMQPQueue::sendCancelCommand();
}



void AMQPQueue::sendCancelCommand(){
	amqp_basic_cancel_t s;
	s.consumer_tag=consumer_tag;
	s.nowait=( AMQP_NOWAIT & parms ) ? 1:0;

	amqp_send_method( *cnn,
			channelNum,
			AMQP_BASIC_CANCEL_METHOD,
			&s);	
}

amqp_bytes_t AMQPQueue::getConsumerTag() {
	return consumer_tag;
}

void AMQPQueue::Ack() {
	if (!delivery_tag)
		throw AMQPException("the delivery tag not set");

	sendAckCommand();
}

void AMQPQueue::Ack(uint32_t delivery_tag) {
	this->delivery_tag=delivery_tag;
	
	sendAckCommand();
}

void AMQPQueue::sendAckCommand() {

	amqp_basic_ack_t s;
	s.delivery_tag=delivery_tag;
	s.multiple = ( AMQP_MULTIPLE & parms ) ? 1:0;

	amqp_send_method( *cnn,
				channelNum,
				AMQP_BASIC_ACK_METHOD,
				&s);

}


