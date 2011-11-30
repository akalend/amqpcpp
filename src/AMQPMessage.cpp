/*
 *  AMQPMessage.cpp
 *  librabbitmq++
 *
 *  Created by Alexandre Kalendarev on 15.04.10.
 *
 */

#include "AMQPcpp.h"

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

void AMQPMessage::setMessage(const char * data) {
	if (!data)
		return;

	if (this->data)
		free(this->data);

	this->data = strdup(data);
}

char * AMQPMessage::getMessage() {
	if (this->data)
		return this->data;
	return '\0';
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
	headers.insert( pair<string,string>(name,svalue));
}

void AMQPMessage::addHeader(string name, uint64_t * value) {
	char ivalue[32];
	bzero(ivalue,32);
	sprintf(ivalue,"%lld",(long long int)*value);
	headers.insert(pair<string,string>(name,string(ivalue)));
}

void AMQPMessage::addHeader(string name, uint8_t * value) {
	char ivalue[4];
	bzero(ivalue,4);
	sprintf(ivalue,"%d",*value);
	headers.insert( pair<string,string>(name,string(ivalue)));
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
