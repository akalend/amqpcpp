/*
 *  AMQPQueue.cpp
 *  librabbitmq++
 *
 *  Created by Alexandre Kalendarev on 12.03.10.
 *
 */

#include "AMQPcpp.h"
#include <algorithm> // std::transform
#include <limits>
#include <deque>
#include <assert.h>

using namespace std;

AMQPQueue::AMQPQueue(amqp_connection_state_t * cnn, int channelNum) {
	this->cnn = cnn;
	this->channelNum = channelNum;

	delivery_tag =0;
	openChannel();
}

AMQPQueue::AMQPQueue(amqp_connection_state_t * cnn, int channelNum, string name) {
	this->cnn = cnn;
	this->channelNum = channelNum;
	this->name = name;

	delivery_tag =0;
	openChannel();
}

AMQPQueue::~AMQPQueue() {
}

// Declare command /* 50, 10; 3276810 */
void AMQPQueue::Declare() {
	parms=0;
	sendDeclareCommand();
}

void AMQPQueue::Declare(string name) {
	this->parms=AMQP_AUTODELETE;
	this->name=name;
	sendDeclareCommand();
}

void AMQPQueue::Declare(string name, short parms, const std::vector<KeyValuePair>& arguments) {
	this->parms=parms;
	this->name=name;
	sendDeclareCommand(arguments);
}

// copied from amqp_table.cpp
amqp_table_entry_t amqp_table_construct_utf8_entry(const char *key, const char *value) {
	amqp_table_entry_t ret;
	ret.key = amqp_cstring_bytes(key);
	ret.value.kind = AMQP_FIELD_KIND_UTF8;
	ret.value.value.bytes = amqp_cstring_bytes(value);
	return ret;
}

void AMQPQueue::sendDeclareCommand(const std::vector<KeyValuePair>& arguments) {
	amqp_bytes_t queue_name = amqp_cstring_bytes(name.c_str());

	/*
		amqp_basic_properties_t props;
		props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG;
		props.content_type = amqp_cstring_bytes("text/plain");
	*/
	amqp_table_t args;
	args.num_entries = static_cast<int>(arguments.size());

	std::vector<amqp_table_entry_t> a(arguments.size());
	if (arguments.empty()) {
		args.entries = nullptr;
	}
	else {
		const auto toTableEntry = [](const KeyValuePair& kvp) {
			return amqp_table_construct_utf8_entry(kvp.key.c_str(), kvp.value.c_str());
		};
		std::transform(arguments.begin(), arguments.end(), a.begin(), toTableEntry);
		args.entries = &a[0];
	}

	amqp_boolean_t exclusive =  (parms & AMQP_EXCLUSIVE)	? 1:0;
	amqp_boolean_t passive =    (parms & AMQP_PASSIVE)		? 1:0;
	amqp_boolean_t autodelete = (parms & AMQP_AUTODELETE)	? 1:0;
	amqp_boolean_t durable =    (parms & AMQP_DURABLE)		? 1:0;

	amqp_queue_declare_t s;
		s.ticket = 0;
		s.queue = queue_name;
		s.passive = passive;
		s.durable = durable;
		s.exclusive = exclusive;
		s.auto_delete = autodelete;
		s.nowait = 0;
		s.arguments = args;

	amqp_method_number_t method_ok = AMQP_QUEUE_DECLARE_OK_METHOD;
	amqp_rpc_reply_t res = amqp_simple_rpc(*cnn, channelNum, AMQP_QUEUE_DECLARE_METHOD, &method_ok , &s);

	AMQPBase::checkReply(&res);

	amqp_release_buffers(*cnn);
	char error_message [256];
	memset(error_message,0,256);

	if (res.reply_type == AMQP_RESPONSE_NONE) {
		throw AMQPException("error the QUEUE.DECLARE command, response none");
	}

	if (res.reply.id == AMQP_CHANNEL_CLOSE_METHOD) {
		amqp_channel_close_t * err = (amqp_channel_close_t *) res.reply.decoded;

		int c_id = 	(int) err->class_id;
		sprintf( error_message, "server error %u, message '%s' class=%d method=%u ", err->reply_code, (char*)err->reply_text.bytes, c_id,err->method_id);
		opened=0;

		throw AMQPException(&res);
	} else if (res.reply.id == AMQP_QUEUE_DECLARE_OK_METHOD) {
			amqp_queue_declare_ok_t* data = (amqp_queue_declare_ok_t*) res.reply.decoded;
			count = data->message_count;
	} else {
		sprintf( error_message, "error the Declare command  receive method=%d", res.reply.id);
		throw AMQPException(error_message);
	}
}

// Delete command  /* 50, 40; 3276840 */
void AMQPQueue::Delete() {
	if (!name.size())
		throw AMQPException("the name of queue not set");
	sendDeleteCommand();
}

void AMQPQueue::Delete(string name) {
	this->name=name;
	sendDeleteCommand();
}

void AMQPQueue::sendDeleteCommand() {
	amqp_bytes_t queue = amqp_cstring_bytes(name.c_str());

	amqp_queue_delete_t s;
		s.ticket = 0;
		s.queue = queue;
		s.if_unused = ( AMQP_IFUNUSED & parms ) ? 1:0;
		s.nowait = ( AMQP_NOWAIT & parms ) ? 1:0;

	amqp_method_number_t method_ok = AMQP_QUEUE_DELETE_OK_METHOD;

	amqp_rpc_reply_t res = amqp_simple_rpc(*cnn, channelNum, AMQP_QUEUE_DELETE_METHOD, &method_ok, &s);

	AMQPBase::checkReply(&res);
}

// Purge command /* 50, 30; 3276830 *
void AMQPQueue::Purge() {
	if (!name.size())
		throw AMQPException("the name of queue not set");
	sendPurgeCommand();
}

void AMQPQueue::Purge(string name) {
	 this->name=name;
	 sendPurgeCommand();
}

void AMQPQueue::sendPurgeCommand() {
	amqp_bytes_t queue = amqp_cstring_bytes(name.c_str());

	amqp_queue_purge_t s;
		s.ticket = 0;
		s.queue = queue;
		s.nowait = ( AMQP_NOWAIT & parms ) ? 1:0;

	amqp_method_number_t method_ok = AMQP_QUEUE_PURGE_OK_METHOD;

	amqp_rpc_reply_t res = amqp_simple_rpc(*cnn, channelNum, AMQP_QUEUE_PURGE_METHOD, &method_ok , &s);

	AMQPBase::checkReply(&res);
}

// Bind command /* 50, 20; 3276820 */
void AMQPQueue::Bind(string name, string key) {
	sendBindCommand(name.c_str(), key.c_str());
}

void AMQPQueue::sendBindCommand(const char * exchange, const char * key) {
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
	amqp_rpc_reply_t res = amqp_simple_rpc(*cnn, channelNum, AMQP_QUEUE_BIND_METHOD, &method_ok, &s);

	AMQPBase::checkReply(&res);
}


// UnBind command /* 50, 50; 3276850 */
void AMQPQueue::unBind(string name, string key) {
	sendUnBindCommand(name.c_str(), key.c_str());
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
    amqp_rpc_reply_t res = amqp_simple_rpc(*cnn, channelNum, AMQP_QUEUE_UNBIND_METHOD, &method_ok, &s);

	AMQPBase::checkReply(&res);
}


// GET method /* 60, 71; 3932231 */
void AMQPQueue::Get() {
	parms=0;
	sendGetCommand();
}

void AMQPQueue::Get(short parms) {
	this->parms=parms;
	sendGetCommand();
}

void AMQPQueue::sendGetCommand() {
	amqp_bytes_t queueByte = amqp_cstring_bytes(name.c_str());

	amqp_basic_get_t s;
		s.ticket = 0;
		s.queue = queueByte;
		s.no_ack = ( AMQP_NOACK & parms ) ? 1:0;

	amqp_method_number_t replies[] = {
		AMQP_BASIC_GET_OK_METHOD,
		AMQP_BASIC_GET_EMPTY_METHOD,
		AMQP_CONNECTION_CLOSE_METHOD,
		0
	};

	amqp_rpc_reply_t res = amqp_simple_rpc(*cnn, channelNum, AMQP_BASIC_GET_METHOD, replies, &s);

	// 3932232 GET_EMPTY
	// 3932231 GET_OK
	// 1310760 CHANNEL_CLOSE

	char error_message[256];
	amqp_frame_t frame;

	amqp_release_buffers(*cnn);

	pmessage = std::make_unique<AMQPMessage>(this);

	if ( res.reply_type == AMQP_RESPONSE_NONE) {
		throw AMQPException("error the Get command, response none");
	}

	if ( res.reply_type == AMQP_RESPONSE_LIBRARY_EXCEPTION) {
		throw AMQPException(&res);
	}

	if ( res.reply.id == AMQP_CHANNEL_CLOSE_METHOD ) {
		amqp_channel_close_t * err = (amqp_channel_close_t *) res.reply.decoded;

		sprintf( error_message, "server error %u, message '%s' class=%d method=%u ", err->reply_code, (char*)err->reply_text.bytes, (int)err->class_id, err->method_id);
		opened=0;

		throw AMQPException( &res);
	} else if (res.reply.id == AMQP_BASIC_GET_EMPTY_METHOD) {
		pmessage->setMessageCount(-1);
		return;
	} else if (res.reply.id == AMQP_BASIC_GET_OK_METHOD) {
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
	std::deque<std::vector<char>> buffered_frames;
	size_t total_length = 0;

	while (1){ //receive frames...
		amqp_maybe_release_buffers(*cnn);
		result = amqp_simple_wait_frame(*cnn, &frame);
		if (result < 0)
			throw AMQPException(" read frame error");

		if (frame.frame_type == AMQP_FRAME_HEADER){
			amqp_basic_properties_t * p = (amqp_basic_properties_t *) frame.payload.properties.decoded;
			this->setHeaders(p);
			continue;
		}

		if (frame.frame_type == AMQP_FRAME_BODY){
			size_t frame_len = frame.payload.body_fragment.len;

			buffered_frames.emplace_back(frame_len);

			if (frame_len > 0) {
				auto& buffered_frame = buffered_frames.back();
				memcpy(&buffered_frame[0], frame.payload.body_fragment.bytes, frame_len);
				total_length += frame_len;
			}

			if (frame_len < FRAME_MAX - HEADER_FOOTER_SIZE)
				break;

			continue;
		}
	}

	if (!buffered_frames.empty()) {
		std::vector<char> complete_frame(total_length + 1);
		size_t i = 0;
		for (const auto& buffered_frame : buffered_frames) {
			if (buffered_frame.size() > 0) {
				memcpy(&complete_frame[i], &buffered_frame[0], buffered_frame.size());
				i += buffered_frame.size();
			}
		}
		assert(i == total_length);
		complete_frame[i] = '\0';

		pmessage->setMessage(complete_frame.data(), total_length);
	}
	amqp_release_buffers(*cnn);
}

void AMQPQueue::addEvent( AMQPEvents_e eventType, int (*event)(AMQPMessage*)) {
#if __cplusplus > 199711L || (defined(_MSC_VER) && _MSC_VER >= 1800) // C++11 or greater
	std::function<int(AMQPMessage*)> callback = &(*event);
	addEvent(eventType, callback);
#else
	if (events.find(eventType) != events.end())
		throw AMQPException("event already added");
	events[eventType] = reinterpret_cast< int(*)( AMQPMessage * ) > (event);
#endif
}

#if __cplusplus > 199711L || (defined(_MSC_VER) && _MSC_VER >= 1800) // C++11 or greater
void AMQPQueue::addEvent( AMQPEvents_e eventType, std::function<int(AMQPMessage*)>& event) {
	if (events.find(eventType) != events.end())
		throw AMQPException("the event already added");
	events[eventType] = event;
}
#endif

void AMQPQueue::Consume() {
	parms=0;
	sendConsumeCommand();
}

void AMQPQueue::Consume(short parms) {
	this->parms=parms;
	sendConsumeCommand();
}

void AMQPQueue::setConsumerTag(string consumer_tag) {
	this->consumer_tag = consumer_tag;
}

void AMQPQueue::sendConsumeCommand() {
//	amqp_basic_consume_ok_t *consume_ok;
	amqp_bytes_t queueByte = amqp_cstring_bytes(name.c_str());

	char error_message[256];

	/*
		amqp_basic_consume_ok_t * res = amqp_basic_consume( *cnn, channelNum,
			queueByte, consumer_tag,
			0, //amqp_boolean_t no_local,
			1, // amqp_boolean_t no_ack,
			0  //amqp_boolean_t exclusive
		); 
	*/

	amqp_method_number_t replies[] = {
		AMQP_BASIC_CONSUME_OK_METHOD,
		AMQP_CONNECTION_CLOSE_METHOD,
		AMQP_BASIC_CANCEL_OK_METHOD,
		0
	};

	amqp_basic_consume_t s;
	memset(&s,0,sizeof(amqp_basic_consume_t));
		s.ticket = channelNum;
		s.queue = queueByte;
		s.consumer_tag = amqp_cstring_bytes(consumer_tag.c_str());
		s.no_local = ( AMQP_NOLOCAL & parms ) ? 1:0;
		s.no_ack = ( AMQP_NOACK & parms ) ? 1:0;
		s.exclusive = ( AMQP_EXCLUSIVE & parms ) ? 1:0;
		//add by chenyujian 20120731
		//arguments should be initialized
		s.arguments = amqp_empty_table;

	amqp_rpc_reply_t res = amqp_simple_rpc(*cnn, channelNum, AMQP_BASIC_CONSUME_METHOD, replies, &s);

	if ( res.reply_type == AMQP_RESPONSE_NONE) {
		throw AMQPException("error the Consume command, response none");
	} else if ( res.reply.id == AMQP_CHANNEL_CLOSE_METHOD ) {
		amqp_channel_close_t * err = (amqp_channel_close_t *) res.reply.decoded;
		sprintf( error_message, "server error %u, message '%s' class=%d method=%u ", err->reply_code, (char*)err->reply_text.bytes, (int)err->class_id, err->method_id);
		opened=0;

		throw AMQPException(error_message);
	} else if (res.reply.id == AMQP_BASIC_CANCEL_OK_METHOD) {
		return;//cancel ok
	}
//		else if (res.reply.id == AMQP_BASIC_CONSUME_OK_METHOD) {
//		consume_ok = (amqp_basic_consume_ok_t*) res.reply.decoded;
//		//printf("****** consume Ok c_tag=%s", consume_ok->consumer_tag.bytes );
//	}
	pmessage = std::make_unique<AMQPMessage>(this);

	amqp_frame_t frame;
	char * buf=NULL, *pbuf = NULL;

	size_t body_received;
	size_t body_target;

	while(1) {
		amqp_maybe_release_buffers(*cnn);
		int result = amqp_simple_wait_frame(*cnn, &frame);
		//modified by chenyujian 20120731
		//if (result <= 0) return;
		//according to definition of the amqp_simple_wait_frame
		//result = 0 means success	
		if (result < 0) {
			throw AMQPException("amqp_simple_wait_frame", result);
		}
		//printf("frame method.id=%d  frame.frame_type=%d\n",frame.payload.method.id, frame.frame_type);

		if (frame.frame_type != AMQP_FRAME_METHOD){
			//cout << "continue  method.id="<< frame.payload.method.id << " ";
			//cout << "frame.frame_type="<< frame.frame_type << endl;
			continue;
		}

		if (frame.payload.method.id == AMQP_BASIC_CANCEL_OK_METHOD){
			//cout << "CANCEL OK method.id="<< frame.payload.method.id << endl;
			if ( events.find(AMQP_CANCEL) != events.end() ) {
#if __cplusplus > 199711L || (defined(_MSC_VER) && _MSC_VER >= 1800) // C++11 or greater
				events[AMQP_CANCEL](pmessage.get());
#else
				(*events[AMQP_CANCEL])(pmessage);
#endif
			}
			break;
		}

		if (frame.payload.method.id != AMQP_BASIC_DELIVER_METHOD){
			//cout << "NON BASIC_DELIVER_METHOD continue  method.id="<< frame.payload.method.id << endl;
			continue;
		}

		amqp_basic_deliver_t * delivery = (amqp_basic_deliver_t*) frame.payload.method.decoded;

		delivery_tag = delivery->delivery_tag;
		pmessage->setConsumerTag(delivery->consumer_tag);
		pmessage->setDeliveryTag(delivery->delivery_tag);

		pmessage->setExchange(delivery->exchange);
		pmessage->setRoutingKey(delivery->routing_key);

		result = amqp_simple_wait_frame(*cnn, &frame);
		//modified by chenyujian 20120731
		//if (result <= 0) {
		if (result < 0) {	
			throw AMQPException("The returned read frame is invalid");
			return;
		}

		if (frame.frame_type != AMQP_FRAME_HEADER) {
			throw AMQPException("The returned frame type is invalid");
		}

		amqp_basic_properties_t * p = (amqp_basic_properties_t *) frame.payload.properties.decoded;

		this->setHeaders(p);

		if (frame.payload.properties.body_size >= std::numeric_limits<size_t>::max()) {
			throw AMQPException("Frame size " + std::to_string(frame.payload.properties.body_size) + " exceeds maximum supported by the platform");
		}

		body_target = static_cast<size_t>(frame.payload.properties.body_size);
		body_received = 0;

		buf = (char*) malloc(body_target+1);
		*(buf+body_target)='\0';
		pbuf = buf;

		while (body_received < body_target) {
			result = amqp_simple_wait_frame(*cnn, &frame);
			//modified by chenyujian 20120731
			//if (result <= 0) break;
			if (result < 0) break;
			//printf("frame.frame_type=%d\n", frame.frame_type);

			if (frame.frame_type != AMQP_FRAME_BODY) {
				throw AMQPException("The returned frame has no body");
			}

			body_received += frame.payload.body_fragment.len;

			memcpy( pbuf, frame.payload.body_fragment.bytes, frame.payload.body_fragment.len);
			pbuf += frame.payload.body_fragment.len;
		}

		pmessage->setMessage(buf,body_received);
		free(buf);

		if ( events.find(AMQP_MESSAGE) != events.end() ) {
#if __cplusplus > 199711L || (defined(_MSC_VER) && _MSC_VER >= 1800) // C++11 or greater
			int res = events[AMQP_MESSAGE](pmessage.get());
#else
			int res = (int)(*events[AMQP_MESSAGE])(pmessage);
#endif 
			
			//cout << "res="<<res<<endl;
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

	if (p->_flags & AMQP_BASIC_HEADERS_FLAG) {
		int max = (p->headers).num_entries;
		int i = 0;
		for (i = 0; i < max; i++) {
			amqp_bytes_t keyBytes = (p->headers).entries[i].key;
			amqp_bytes_t valueBytes = (p->headers).entries[i].value.value.bytes;
			pmessage->addHeader(&keyBytes, &valueBytes);
		}
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
	this->consumer_tag = consumer_tag;
	sendCancelCommand();
}

void AMQPQueue::Cancel(amqp_bytes_t consumer_tag){
	this->consumer_tag = string((char*)consumer_tag.bytes, consumer_tag.len);
	sendCancelCommand();
}

void AMQPQueue::sendCancelCommand(){
	amqp_basic_cancel_t s;
		s.consumer_tag= amqp_cstring_bytes(consumer_tag.c_str());
		s.nowait=( AMQP_NOWAIT & parms ) ? 1:0;

	amqp_send_method(*cnn, channelNum, AMQP_BASIC_CANCEL_METHOD, &s);
}

std::string AMQPQueue::getConsumerTag() {
	return consumer_tag;
}

void AMQPQueue::Ack() {
	if (!delivery_tag)
		throw AMQPException("the delivery tag not set");

	sendAckCommand();
}

void AMQPQueue::Ack(uint64_t delivery_tag) {
	this->delivery_tag=delivery_tag;

	sendAckCommand();
}

void AMQPQueue::sendAckCommand() {
	amqp_basic_ack_t s;
	s.delivery_tag=delivery_tag;
	s.multiple = ( AMQP_MULTIPLE & parms ) ? 1:0;

	amqp_send_method(*cnn, channelNum, AMQP_BASIC_ACK_METHOD, &s);
}

void AMQPQueue::Reject(bool requeue) {
	if (!delivery_tag)
		throw AMQPException("the delivery tag not set");

	sendRejectCommand(requeue);
}

void AMQPQueue::Reject(uint64_t delivery_tag, bool requeue) {
	this->delivery_tag = delivery_tag;

	sendRejectCommand(requeue);
}

void AMQPQueue::sendRejectCommand(bool requeue) {
	amqp_basic_reject_t s;
	s.delivery_tag = delivery_tag;
	s.requeue = requeue ? 1 : 0;

	amqp_send_method(*cnn, channelNum, AMQP_BASIC_REJECT_METHOD, &s);
}

void AMQPQueue::Qos(
        uint32_t prefetch_size,
        uint16_t prefetch_count,
        amqp_boolean_t global )
{
    amqp_method_number_t method_ok = AMQP_BASIC_QOS_OK_METHOD;

    amqp_basic_qos_t req;
    req.prefetch_size = prefetch_size;
    req.prefetch_count = prefetch_count;
    req.global = global;

    amqp_rpc_reply_t res = amqp_simple_rpc( *cnn, channelNum,
                               AMQP_BASIC_QOS_METHOD, &method_ok, &req);

    if (res.reply.id != AMQP_BASIC_QOS_OK_METHOD) {
        throw AMQPException("AMQPQueue::Qos Fail.");
    }
}
