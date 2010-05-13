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

#include <stdint.h>

#include <amqp.h>
#include <amqp_framing.h>

#include <iostream>
#include <vector>
#include <map>
#include <memory>

//export AMQP; 
using namespace std;

enum AMQPEvents_e {
	AMQP_MESSAGE, AMQP_SIGUSR, AMQP_CANCEL, AMQP_CLOSE_CHANNEL
};



class AMQPException {

	string message;
	int code;
	public: 
		AMQPException(const char * message);
		AMQPException( amqp_rpc_reply_t * res );
	
		string getMessage();
		uint16_t    getReplyCode();
};

class AMQPQueue;

class AMQPMessage {
	
	char * data;
	string exchange;
	string routing_key;
	uint32_t delivery_tag;
	int message_count; 
	string consumer_tag;
	AMQPQueue * queue;
	map<string,string> headers;
	 
	public :	
		AMQPMessage(AMQPQueue * queue);		
		~AMQPMessage();
		
		void setMessage(char * data);
		char * getMessage();
		
		void addHeader(char * name, amqp_bytes_t * value);
		void addHeader(char * name, uint64_t * value);	
		void addHeader(char * name, uint8_t * value);			
		string getHeader(string name);
		
		void setConsumerTag( amqp_bytes_t consumer_tag);
		void setConsumerTag( string consumer_tag);
		string getConsumerTag();				
		
		void setMessageCount( int count);
		int getMessageCount();

		void setExchange(amqp_bytes_t exchange);
		void setExchange(char * exchange);
		string getExchange();

		void setRoutingKey(amqp_bytes_t routing_key);
		void setRoutingKey(char * routing_key);
		string getRoutingKey();
		
		uint32_t getDeliveryTag();
		void setDeliveryTag(uint32_t delivery_tag);
		
		AMQPQueue * getQueue();
	
};


class AMQPBase {

	protected:
		string name;
		short parms;
		amqp_connection_state_t * cnn;
		int channelNum;
		AMQPMessage * pmessage;
		
		short opened;
		
		void checkReply(amqp_rpc_reply_t * res);
		void checkClosed(amqp_rpc_reply_t * res);
		void openChannel();		
		
		
	public:	
		~AMQPBase();		
		int getChannelNum();
		void setParam(short param);
		string getName();
		void closeChannel();
		void reopen();
		void setName(const char * name);
		void setName(string name);	
};

class AMQPQueue : public AMQPBase  {
	protected:	
		map< AMQPEvents_e, int(*)( AMQPMessage * ) > events;
		amqp_bytes_t consumer_tag;
		uint32_t delivery_tag;
		uint32_t count;
	public:
		AMQPQueue(amqp_connection_state_t * cnn, int channelNum);
		AMQPQueue(amqp_connection_state_t * cnn, int channelNum, string name);

		void Declare();
		void Declare(string name);
		void Declare(const char * name);
		void Declare(string name, short parms);
		void Declare(const char * name, short parms);

		void Delete();
		void Delete(string name);
		void Delete(const char * name);

		void Purge();
		void Purge(string name);
		void Purge(const char * name);

		void Bind(string exchangeName, const char * key);
		void Bind(const char * exchangeName , string key);
		void Bind(string exchangeName, string key);
		void Bind(const char * queueName , const char * key);

		void unBind(string exchangeName, const char * key);
		void unBind(const char * exchangeName , string key);
		void unBind(string exchangeName, string key);
		void unBind(const char * queueName , const char * key);

		void Get();
		void Get(short param);

		void Consume();
		void Consume(short param);

		void Cancel(amqp_bytes_t consumer_tag);
		void Cancel(const char * consumer_tag);
		void Cancel(string consumer_tag);

		void Ack();
		void Ack(uint32_t delivery_tag);
		
		AMQPMessage * getMessage() {
			return pmessage;
		}
		
		uint32_t getCount() {
			return count;
		}
		
		void setConsumerTag(string consumer_tag);
		void setConsumerTag(const char * consumer_tag);
		amqp_bytes_t getConsumerTag();
		
		void addEvent( AMQPEvents_e eventType, void * event );
				
		~AMQPQueue();

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
	string type;		
	map<string,string> sHeaders;
	map<string, int> iHeaders;
	
	public :
		AMQPExchange(amqp_connection_state_t * cnn, int channelNum);
		AMQPExchange(amqp_connection_state_t * cnn, int channelNum, string name);
		 
		void Declare();
		void Declare(string name);
		void Declare(const char * name);
		void Declare(string name, const char * type);		
		void Declare(const char * name, const char * type);		
		void Declare(string name, const char * type, short parms);		
		void Declare(const char * name, const char * type, short parms);		

		void Delete();
		void Delete(string name);
		void Delete(const char * name);

		void Bind(string queueName);
		void Bind(const char * queueName) ;
		void Bind(string queueName, const char * key);
		void Bind(const char * queueName , string key ) ;
		void Bind(string queueName, string key);
		void Bind(const char * queueName , const char * key ) ;

		void Publish(const char * message, const char * key);
		void Publish(string message, const char * key);
		void Publish(string message, string key);
		void Publish(const char * message, string key);
				
		void setHeader(const char * name, int value);
		void setHeader(const char * name, const char * value);
		void setHeader(const char * name, string value);

	protected:

	private:		
		AMQPExchange();
		void checkType();
		void sendDeclareCommand();
		void sendDeleteCommand();
		void sendPublishCommand();
		
		void sendBindCommand(const char * queueName, const char * key);
		void sendPublishCommand(const char * message, const char * key);
		void sendCommand();
		void checkReply(amqp_rpc_reply_t * res);
		void checkClosed(amqp_rpc_reply_t * res);
			
};

class AMQP {

	int port;
	string host;
	string vhost;
	string user;
	string password;
	int sockfd;
	int channelNumber;
	
	amqp_connection_state_t cnn;
	AMQPExchange * exchange;

	vector<AMQPBase*> channels;
				
	public:
		AMQP();
		AMQP( string cnnStr);
		AMQP(const char* cnnStr);
		~AMQP();
						
		AMQPExchange * createExchange();
		AMQPExchange * createExchange(string name);
		AMQPExchange * createExchange(const char * name);

		AMQPQueue * createQueue();
		AMQPQueue * createQueue(string name);
		AMQPQueue * createQueue(const char * name);

		void printConnect();
		
		void closeChannel();
		
	private:
		//AMQP& operator =(AMQP &ob);
		AMQP( AMQP &ob );
		void init();
		void initDefault();
		void connect();
		void parseCnnString(string cnnString );
		void parseHostPort(string hostPortString );
		void parseUserStr(string userString );
		void sockConnect();
		void login();
		//void chanalConnect();
		
};


#endif
