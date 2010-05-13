/*
 *  AMQPException.cpp
 *  rabbitmq++
 *
 *  Created by Alexandre Kalendarev on 10.03.10.
 *
 */

#include <iostream>
#include <string>

#include "amqpcpp.h"

using namespace std;

AMQPException::AMQPException(const char * message) {
		this->message= message;
}


AMQPException::AMQPException( amqp_rpc_reply_t * res) {

	if ( res->reply_type == AMQP_RESPONSE_LIBRARY_EXCEPTION) {	

		this->message = res->library_errno ? strerror(res->library_errno) : "end-of-stream";	
	} 
	
	if ( res->reply_type == AMQP_RESPONSE_SERVER_EXCEPTION) {	

		char buf[512];
		bzero(buf,512);
		this->code = 0;

		switch (res->reply.id) {
			case AMQP_CONNECTION_CLOSE_METHOD: 
			{
				amqp_connection_close_t *m = (amqp_connection_close_t *) res->reply.decoded;
				this->code = m->reply_code;
				
				sprintf(buf, "server connection error %d, message: %.*s",
				m->reply_code,
				(int) m->reply_text.len, (char *) m->reply_text.bytes);
				break;
			}
			case AMQP_CHANNEL_CLOSE_METHOD: 
			{
			  amqp_channel_close_t *m = (amqp_channel_close_t *) res->reply.decoded;
			  this->code = m->reply_code;
				
			  int c_id = (int) m->class_id;
			  sprintf(buf, "server channel error %d, message: %.*s class=%d method=%d",
				  m->reply_code,
				  (int) m->reply_text.len, (char *) m->reply_text.bytes, c_id,m->method_id);
			  break;
			}
			default:
			  sprintf(buf, "unknown server error, method id 0x%08X", res->reply.id);
			  break;
		} // end switch
		
//			cout << "message = "<< buf<< endl;;
		this->message=buf;
	} 

}

uint16_t  AMQPException::getReplyCode() {
	return code;
}

string AMQPException::getMessage()
{
		return message;
}