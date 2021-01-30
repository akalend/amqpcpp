/*
 *  amqpcpp.h
 *  librabbitmq++
 *
 *  Created by Alexandre Kalendarev on 01.03.10.
 *
 */

#ifndef __AMQPCPP
#define __AMQPCPP

#define AMQPPORT 5672
#define AMQPSPORT 5671
#define AMQPHOST "localhost"
#define AMQPVHOST "/"
#define AMQPLOGIN "guest"
#define AMQPPSWD  "guest"

#define AMQPDEBUG ":5673"

#define AMQP_AUTODELETE		1
#define AMQP_DURABLE		2
#define AMQP_PASSIVE		4
#define AMQP_MANDATORY		8
#define AMQP_IMMIDIATE		16
#define AMQP_IFUNUSED		32
#define AMQP_EXCLUSIVE		64
#define AMQP_NOWAIT			128
#define AMQP_NOACK			256
#define AMQP_NOLOCAL		512
#define AMQP_MULTIPLE		1024


#define HEADER_FOOTER_SIZE 8 //  7 bytes up front, then payload, then 1 byte footer
#define FRAME_MAX 131072    // max lenght (size) of frame

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>

#include <stdint.h>

#include "amqp.h"
#include "amqp_framing.h"
#include "amqp_tcp_socket.h"
#include "amqp_ssl_socket.h"

#include <iostream>
#include <vector>
#include <map>
#include <memory>
#include <exception>

#if __cplusplus > 199711L // C++11 or greater
#include <functional>
#endif
//export AMQP;

class AMQPQueue;

enum AMQPEvents_e {
	AMQP_MESSAGE, AMQP_SIGUSR, AMQP_CANCEL, AMQP_CLOSE_CHANNEL
};

enum AMQPProto_e {
	AMQP_proto, AMQPS_proto
};

#define SET_AMQP_PROTO_BY_SSL_USAGE(b) (b ? AMQPS_proto : AMQP_proto)

class AMQPException : public std::exception {
	std::string message;
	int code;
	public:
		explicit AMQPException(std::string message);
		explicit AMQPException(amqp_rpc_reply_t * res);
		
		// creates error message from error code provided by librabbitmq
		AMQPException(std::string action, int error_code);
		
		virtual ~AMQPException() throw() {}
		
		std::string   getMessage() const;
		uint16_t getReplyCode() const;
		
		virtual const char* what() const throw () {
			return message.c_str();
		}
};



class AMQPMessage {

	char * data;
	uint32_t len;
	std::string exchange;
	std::string routing_key;
	uint32_t delivery_tag;
	int message_count;
	std::string consumer_tag;
	AMQPQueue * queue;
	std::map<std::string,std::string> headers;

	public :
		AMQPMessage(AMQPQueue * queue);
		~AMQPMessage();

		void setMessage(const char * data,uint32_t length);
		char * getMessage(uint32_t* length);

		void addHeader(std::string name, amqp_bytes_t * value);
		void addHeader(std::string name, uint64_t * value);
		void addHeader(std::string name, uint8_t * value);
		void addHeader(amqp_bytes_t * name, amqp_bytes_t * value);
		std::string getHeader(std::string name);

		void setConsumerTag( amqp_bytes_t consumer_tag);
		void setConsumerTag( std::string consumer_tag);
		std::string getConsumerTag();

		void setMessageCount(int count);
		int getMessageCount();

		void setExchange(amqp_bytes_t exchange);
		void setExchange(std::string exchange);
		std::string getExchange();

		void setRoutingKey(amqp_bytes_t routing_key);
		void setRoutingKey(std::string routing_key);
		std::string getRoutingKey();

		uint32_t getDeliveryTag();
		void setDeliveryTag(uint32_t delivery_tag);

		AMQPQueue * getQueue();
};


class AMQPBase {
	protected:
		std::string name;
		short parms;
		amqp_connection_state_t * cnn;
		int channelNum;
		AMQPMessage * pmessage;

		short opened;

		void checkReply(amqp_rpc_reply_t * res);
		void checkClosed(amqp_rpc_reply_t * res);
		void openChannel();


	public:
		virtual ~AMQPBase();
		int getChannelNum();
		void setParam(short param);
		std::string getName();
		void closeChannel();
		void reopen();
		void setName(const char * name);
		void setName(std::string name);
};

class AMQPQueue : public AMQPBase  {
	protected:
#if __cplusplus > 199711L // C++11 or greater
                std::map< AMQPEvents_e, std::function<int(AMQPMessage*)> > events;
#else
		std::map< AMQPEvents_e, int(*)( AMQPMessage * ) > events;
#endif
		std::string consumer_tag;
		uint32_t delivery_tag;
		uint32_t count;
	public:
		AMQPQueue(amqp_connection_state_t * cnn, int channelNum);
		AMQPQueue(amqp_connection_state_t * cnn, int channelNum, std::string name);

		void Declare();
		void Declare(std::string name);
		void Declare(std::string name, short parms);

		void Delete();
		void Delete(std::string name);

		void Purge();
		void Purge(std::string name);

		void Bind(std::string exchangeName, std::string key);

		void unBind(std::string exchangeName, std::string key);

		void Get();
		void Get(short param);

		void Consume();
		void Consume(short param);

		void Cancel(amqp_bytes_t consumer_tag);
		void Cancel(std::string consumer_tag);

		void Ack();
		void Ack(uint32_t delivery_tag);

		AMQPMessage * getMessage() {
			return pmessage;
		}

		uint32_t getCount() {
			return count;
		}

		void setConsumerTag(std::string consumer_tag);
		std::string getConsumerTag();

		void addEvent( AMQPEvents_e eventType, int (*event)(AMQPMessage*) );
#if __cplusplus > 199711L // C++11 or greater
                void addEvent( AMQPEvents_e eventType, std::function<int(AMQPMessage*)>& event );
#endif
		virtual ~AMQPQueue();
		
		void Qos(uint32_t prefetch_size, uint16_t prefetch_count, amqp_boolean_t global );
	private:
		void sendDeclareCommand();
		void sendDeleteCommand();
		void sendPurgeCommand();
		void sendBindCommand(const char * exchange, const char * key);
		void sendUnBindCommand(const char * exchange, const char * key);
		void sendGetCommand();
		void sendConsumeCommand();
		void sendCancelCommand();
		void sendAckCommand();
		void setHeaders(amqp_basic_properties_t * p);
};


class AMQPExchange : public AMQPBase {
	std::string type;
	std::map<std::string,std::string> sHeaders;
	std::map<std::string,std::string> sHeadersSpecial;
	std::map<std::string, int> iHeaders;

	public:
		AMQPExchange(amqp_connection_state_t * cnn, int channelNum);
		AMQPExchange(amqp_connection_state_t * cnn, int channelNum, std::string name);
		virtual ~AMQPExchange();

		void Declare();
		void Declare(std::string name);
		void Declare(std::string name, std::string type);
		void Declare(std::string name, std::string type, short parms);

		void Delete();
		void Delete(std::string name);

		void Bind(std::string queueName);
		void Bind(std::string queueName, std::string key);

		void Publish(std::string message, std::string key);
		void Publish(char * data, uint32_t length, std::string key);

		void setHeader(std::string name, int value);
		void setHeader(std::string name, std::string value);
		void setHeader(std::string name, std::string value, bool special);

	private:
		AMQPExchange();
		void checkType();
		void sendDeclareCommand();
		void sendDeleteCommand();
		void sendPublishCommand();

		void sendBindCommand(const char * queueName, const char * key);
		void sendPublishCommand(amqp_bytes_t messageByte, const char * key);
		void sendCommand();
		void checkReply(amqp_rpc_reply_t * res);
		void checkClosed(amqp_rpc_reply_t * res);

};

class AMQP {
	public:
		AMQP();
		AMQP(std::string cnnStr, bool use_ssl_=false,
				std::string cacert_path_="", std::string client_cert_path_="", std::string client_key_path_="",
				bool verify_peer_=false, bool verify_hostname_=false);
		~AMQP();

		AMQPExchange * createExchange();
		AMQPExchange * createExchange(std::string name);

		AMQPQueue * createQueue();
		AMQPQueue * createQueue(std::string name);

		void printConnect();

		void closeChannel();

	private:
		void init(enum AMQPProto_e proto);
		void initDefault(enum AMQPProto_e proto);
		void connect();
		void parseCnnString(std::string cnnString );
		void parseHostPort(std::string hostPortString );
		void parseUserStr(std::string userString );
		void sockConnect();
		void login();



		int port;
		std::string host;
		std::string vhost;
		std::string user;
		std::string password;
		amqp_socket_t *sockfd;
		int channelNumber;

		amqp_connection_state_t cnn;
		AMQPExchange * exchange;

		bool use_ssl;
		enum AMQPProto_e proto;
		std::string cacert_path;
		std::string client_cert_path;
		std::string client_key_path;
		bool verify_peer;
		bool verify_hostname;

		std::vector<AMQPBase*> channels;
};


#endif //__AMQPCPP
