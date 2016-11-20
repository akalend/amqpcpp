/*
 *  AMQPBase.cpp
 *  librabbitmq++
 *
 *  Created by Alexandre Kalendarev on 15.04.10.
 *
 */

#include "AMQPcpp.h"

using namespace std;

AMQPBase::~AMQPBase() {
	this->closeChannel();
}

void AMQPBase::checkReply(amqp_rpc_reply_t * res) {
	checkClosed(res);
	if (res->reply_type != AMQP_RESPONSE_NORMAL )
	throw AMQPException(res);
}

void AMQPBase::checkClosed(amqp_rpc_reply_t * res) {
	if( res->reply_type == AMQP_RESPONSE_SERVER_EXCEPTION && res->reply.id == AMQP_CHANNEL_CLOSE_METHOD )
		opened=0;
}

void AMQPBase::openChannel() {
	amqp_channel_open(*cnn, channelNum);
	amqp_rpc_reply_t res = amqp_get_rpc_reply(*cnn);
	if ( res.reply_type != AMQP_RESPONSE_NORMAL)
		throw AMQPException(&res);
	opened=1;
}

void AMQPBase::closeChannel() {
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

void AMQPBase::setName(string name) {
	this->name=name;
}
