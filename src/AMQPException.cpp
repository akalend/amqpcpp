/*
 *  AMQPException.cpp
 *  librabbitmq++
 *
 *  Created by Alexandre Kalendarev on 10.03.10.
 *
 */

#include "AMQPcpp.h"

using namespace std;

AMQPException::AMQPException(string message) {
	this->message= message;
}

AMQPException::AMQPException(string action, int error_code)
{
	this->message = action + " : " + amqp_error_string2(error_code);
}

AMQPException::AMQPException( amqp_rpc_reply_t * res) {
	if( res->reply_type == AMQP_RESPONSE_LIBRARY_EXCEPTION) {
		this->message = res->library_error ? strerror(res->library_error) : "end-of-stream";
	}

	if( res->reply_type == AMQP_RESPONSE_SERVER_EXCEPTION) {
		char buf[512];
		memset(buf,0,512);
		this->code = 0;

		if(res->reply.id == AMQP_CONNECTION_CLOSE_METHOD) {
			amqp_connection_close_t *m = (amqp_connection_close_t *) res->reply.decoded;
			this->code = m->reply_code;

			sprintf(buf, "server connection error %d, message: %.*s",
				m->reply_code,
				(int) m->reply_text.len,
				(char *) m->reply_text.bytes
			);
		} else if(res->reply.id == AMQP_CHANNEL_CLOSE_METHOD) {
			amqp_channel_close_t *n = (amqp_channel_close_t *) res->reply.decoded;
			this->code = n->reply_code;

			sprintf(buf, "server channel error %d, message: %.*s class=%d method=%d",
				n->reply_code,
				(int) n->reply_text.len,
				(char *) n->reply_text.bytes,
				(int) n->class_id,
				n->method_id
			);
		} else {
			sprintf(buf, "unknown server error, method id 0x%08X", res->reply.id);
		}
		this->message=buf;
	}
}

uint16_t AMQPException::getReplyCode() const {
	return code;
}

string AMQPException::getMessage() const {
	return message;
}
