/*
 *  AMQPBase.cpp
 *  rabbitmq++
 *
 *  Created by Alexandre Kalendarev on 15.04.10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include <iostream>
#include <string>

#include "amqpcpp.h"

using namespace std;

#ifndef __AMQPBASE
#define __AMQPBASE

AMQPBase::~AMQPBase() {
//			std::cout << "~AMQPBase() channel="<<channelNum<<endl;;		

	/*if (message) {
		delete message;
	}*/

	this->closeChannel();
}	

void AMQPBase::checkReply(amqp_rpc_reply_t * res) {
	checkClosed(res);
	if (res->reply_type != AMQP_RESPONSE_NORMAL ) 
	throw AMQPException::AMQPException(res);
}

void AMQPBase::checkClosed(amqp_rpc_reply_t * res) {
	if ( res->reply_type == AMQP_RESPONSE_SERVER_EXCEPTION && 
			res->reply.id == AMQP_CHANNEL_CLOSE_METHOD)
			opened=0;
}

void AMQPBase::openChannel() {

	opened=1;	
//			std::cout << "openChannel num="<< channelNum << std::endl;

	amqp_channel_open(*cnn, channelNum);
	amqp_rpc_reply_t res = amqp_get_rpc_reply(*cnn);
	if ( res.reply_type != AMQP_RESPONSE_NORMAL) 
			throw AMQPException::AMQPException(&res);
}

void AMQPBase::closeChannel() {
//	std::cout << "closeChannel num="<< channelNum << std::endl;
	if (opened)
		amqp_channel_close(*cnn, channelNum, AMQP_REPLY_SUCCESS);		
}

void AMQPBase::reopen() {

	if (opened) return;
	AMQPBase::openChannel();	
}


int AMQPBase::getChannelNum() {
	return channelNum;
}


void AMQPBase::setParam(short param) {
	this->parms=param;
}

string AMQPBase::getName() {
	if (!name.size())
		name="";	
	return name;
}

void AMQPBase::setName(const char * name) {
	this->name=name;
}

void AMQPBase::setName(string name) {
	this->name=name;
}			


#endif