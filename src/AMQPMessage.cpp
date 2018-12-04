/*
 *  AMQPMessage.cpp
 *  librabbitmq++
 *
 *  Created by Alexandre Kalendarev on 15.04.10.
 *
 */

#include "AMQPcpp.h"

using namespace std;

AMQPMessage::AMQPMessage( AMQPQueue * queue ) {
	this->queue=queue;
	 message_count=-1;
	 data=NULL;
}

AMQPMessage::~AMQPMessage() {
	if (data) {
		free(data);
	}
}

void AMQPMessage::setMessage(const char * data,uint32_t length) {
	if (!data)
		return;

	if (this->data)
		free(this->data);

	this->data = (char*)malloc(length + 1);
	if (!this->data) {
		throw AMQPException("setMessage: malloc failed");
	}
	memcpy(this->data,data,length);
	this->data[length] = '\0';
	this->len = length;
}

char * AMQPMessage::getMessage(uint32_t* length) {
	if (this->data)
	  {
	    *length = this->len;
	    return this->data;
	  }
	*length = 0;
	return (char*) '\0';
}

string AMQPMessage::getConsumerTag() {
	return this->consumer_tag;
}

void AMQPMessage::setConsumerTag(amqp_bytes_t consumer_tag) {
	this->consumer_tag.assign( (char*)consumer_tag.bytes, consumer_tag.len );
}

void AMQPMessage::setConsumerTag(string consumer_tag) {
	this->consumer_tag=consumer_tag;
}

void AMQPMessage::setDeliveryTag(uint32_t delivery_tag) {
	this->delivery_tag=delivery_tag;
}

uint32_t AMQPMessage::getDeliveryTag() {
	return this->delivery_tag;
}

void AMQPMessage::setMessageCount(int count) {
	this->message_count = count;
}

int AMQPMessage::getMessageCount() {
	return message_count;
}

void AMQPMessage::setExchange(amqp_bytes_t exchange) {
	if (exchange.len)
		this->exchange.assign( (char*)exchange.bytes, exchange.len );
}

void AMQPMessage::setExchange(string exchange) {
	this->exchange = exchange;
}

string AMQPMessage::getExchange() {
	return exchange;
}

void AMQPMessage::setRoutingKey(amqp_bytes_t routing_key) {
	if (routing_key.len)
		this->routing_key.assign( (char*)routing_key.bytes, routing_key.len );
}

void AMQPMessage::setRoutingKey(string routing_key) {
	this->routing_key=routing_key;
}

string AMQPMessage::getRoutingKey() {
	return routing_key;
}

void AMQPMessage::addHeader(string name, amqp_bytes_t * value) {
	string svalue;
	svalue.assign(( const char *) value->bytes, value->len);
	headers[name] = svalue;
	//headers.insert( pair<string,string>(name,svalue));
}

void AMQPMessage::addHeader(string name, uint64_t * value) {
	char ivalue[32];
	memset(ivalue,0,32);
	sprintf(ivalue,"%lu", *value);
	headers[name] = string(ivalue);
	//headers.insert(pair<string,string>(name,string(ivalue)));
}

void AMQPMessage::addHeader(string name, uint8_t * value) {
	char ivalue[4];
	memset(ivalue,0,4);
	sprintf(ivalue,"%d",*value);
	headers[name] = string(ivalue);
	//headers.insert( pair<string,string>(name,string(ivalue)));
}

void AMQPMessage::addHeader(amqp_bytes_t * name, amqp_bytes_t * value) {
	//cout << "name " << name << endl;
	string sname;
	sname.assign((const char *) name->bytes, name->len);
	string svalue;
	svalue.assign((const char *) value->bytes, value->len);
	headers[sname] = string(svalue);
	//headers.insert(pair<string, string>(sname, svalue));
}

string AMQPMessage::getHeader(string name) {
	if (headers.find(name) == headers.end())
		return "";
	else
		return headers[name];
}

AMQPQueue * AMQPMessage::getQueue() {
	return queue;
}
