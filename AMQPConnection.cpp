/*
 *  AMQPConnection.cpp
 *  librabbitmq++
 *
 *  Created by Alexandre Kalendarev on 01.03.10.
 *
 */

#include <iostream>
#include <string>

#include "amqpcpp.h"

using namespace std;

void AMQP::initDefault() {
	host = string(AMQPHOST);
	port = AMQPPORT;
	vhost = string(AMQPVHOST);
	user = string(AMQPLOGIN);
	password = string(AMQPPSWD);
}

AMQP:: AMQP() {
//	cout<<  "constructor \n";
	AMQP::init();
	AMQP::initDefault();
	AMQP::connect();
	
	};

AMQP::AMQP(const char * cnnStr) {
	AMQP::init();	
	AMQP::parseCnnString(string(cnnStr));
	connect();
	};


AMQP::AMQP(string cnnStr) {
 	 AMQP::init();
	 AMQP::parseCnnString(cnnStr);
	 AMQP::connect();
	};

AMQP::~AMQP() { 
	if (channels.size()) {
		vector<AMQPBase*>::iterator i;
		for (i=channels.begin(); i!=channels.end(); i++) {
//				cout<<  "close channel\n";		
			delete *i;
		}
	}
	
	amqp_destroy_connection(cnn);
	close(sockfd);
	
};

void AMQP::init() {
	exchange=NULL;
	channelNumber=0;
}

void AMQP::parseCnnString( string cnnString ) {
	 if (!cnnString.size()) {
		AMQP::initDefault();
		return;
	 }
		
	// find '@' if Ok -> right part is host:port else all host:port
	string hostPortStr, userPswString;
	int pos = cnnString.find('@');
	
	switch (pos) {
		case 0 :   {
			hostPortStr.assign(cnnString, 1, cnnString.size()-1);
			AMQP::parseHostPort(hostPortStr);
			user = string(AMQPLOGIN);
			password = string(AMQPPSWD);			
			break;
		} 
		case -1 : 
			AMQP::parseHostPort(cnnString);
			user = string(AMQPLOGIN);
			password = string(AMQPPSWD);			
			break;
		default :
			hostPortStr.assign(cnnString, pos+1, cnnString.size()-pos+1);
			userPswString.assign(cnnString, 0, pos);
			AMQP::parseHostPort(hostPortStr);
			AMQP:: parseUserStr(userPswString );
	}		
		
}


void AMQP:: parseUserStr(string userString ) {
	int pos = userString.find(':');
	if (pos==-1) {
		user=userString;
		password=AMQPPSWD;
	} else if (pos==0) {
		user.assign(userString, 1, userString.size()-1);
		password=AMQPPSWD;
	} else {

		user.assign(userString, pos+1, userString.size()+1-pos);
		password.assign(userString, 0, pos);	
	}

}

void AMQP::parseHostPort(string hostPortString ) {
	int pos = hostPortString.find(':');
	string hostString;

	int pos2 = hostPortString.find('/');
	
	switch (pos) {
		case 0 : {
			host = AMQPHOST;
			vhost = AMQPVHOST;	
			
			string portString; 
			string vhostString;
			
			if (pos2 == -1) {
				portString.assign(hostPortString, 1, hostPortString.size()-1);	
				port = atoi( portString.c_str());
			} else {
				
				portString.assign(hostPortString, 1, pos2-1);	
				port = atoi( portString.c_str());
				vhost.assign(hostPortString, pos2+1, hostPortString.size()-pos2);	
			}
			
			break;
		}
		case -1 : {
			port = AMQPPORT;
			if ( pos2 == -1 ) {
				vhost = AMQPVHOST;
				host = hostPortString;
			} else if (pos2 == 0 ) {
				vhost = hostPortString.assign(hostPortString, 1, hostPortString.size()-1);	
				host = AMQPHOST;
			} else {
				vhost.assign(hostPortString, pos2+1, hostPortString.size()-pos2-1);	
				host.assign(hostPortString, 0, pos2);					
			}
			break;
		}
		default : {
		
			string portString;

			if ( pos2 == -1 ) {				
				
				vhost = AMQPVHOST;	
				host = hostString.assign(hostPortString, 0, pos);
				portString.assign(hostPortString, pos+1, hostPortString.size()-pos+1);					
			} else {
				vhost.assign(hostPortString, pos2+1, hostPortString.size()-pos2-1);	
				host.assign(hostPortString, 0, pos);	
				portString.assign(hostPortString, pos+1, pos2-pos-1);				
			}
			port = atoi( portString.c_str());

		}
	}
	
}



void AMQP::connect() {
	
	 AMQP::sockConnect();
	 AMQP::login();
	}

void AMQP::printConnect() {

	 cout<<  "connect \n";
	
	 cout<<  "port ="<< port<<" \n";
	 cout<<  "host ="<< host<<" \n";
	 cout<<  "vhost="<< vhost<<" \n";
	 cout<<  "user ="<< user<<" \n";
	 cout<<  "passw="<< password<<" \n";

}


void AMQP::sockConnect() {

	cnn = amqp_new_connection();
	sockfd = amqp_open_socket(host.c_str(), port);
	
	if (sockfd<0)
		throw AMQPException::AMQPException("the can't create socket descriptor " );

//	cout << "sockfd="<< sockfd  << "  pid=" <<  getpid() <<endl;
	amqp_set_sockfd(cnn, sockfd);
	
}

void AMQP::login() {
	amqp_rpc_reply_t res = amqp_login(cnn, vhost.c_str(), 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, user.c_str(), password.c_str());
	if ( res.reply_type == AMQP_RESPONSE_NORMAL) 
		return;
	
	throw AMQPException::AMQPException(&res);
}

AMQPExchange * AMQP::createExchange() {
	channelNumber++;
	AMQPExchange * exchange = new AMQPExchange(&cnn,channelNumber);
	channels.push_back( reinterpret_cast<AMQPBase*>(exchange) );
	return exchange;
}

AMQPExchange * AMQP::createExchange(string name) {
	channelNumber++;
	AMQPExchange * exchange = new AMQPExchange(&cnn,channelNumber,name);
	channels.push_back( reinterpret_cast<AMQPBase*>(exchange) );
	return exchange;
}

AMQPExchange * AMQP::createExchange(const char * name) {
	channelNumber++;
	AMQPExchange * exchange = new AMQPExchange(&cnn,channelNumber,string(name));
	channels.push_back( reinterpret_cast<AMQPBase*>(exchange) );

	return exchange;
}


AMQPQueue * AMQP::createQueue() {
	channelNumber++;
	AMQPQueue * queue = new AMQPQueue(&cnn,channelNumber);
	channels.push_back( reinterpret_cast<AMQPBase*>(queue) );
	return queue;
}

AMQPQueue * AMQP::createQueue(const char * name) {
	channelNumber++;
	AMQPQueue * queue = new AMQPQueue(&cnn,channelNumber, string(name));
	channels.push_back( reinterpret_cast<AMQPBase*>(queue) );
	return queue;

}

AMQPQueue * AMQP::createQueue(string name) {
	AMQPQueue * queue = new AMQPQueue(&cnn,channelNumber++,name);
	channels.push_back( reinterpret_cast<AMQPBase*>(queue) );
	return queue;

}

void AMQP::closeChannel() {
	channelNumber--;
	AMQPBase * cnn = channels.back();	 
	if (cnn) {
		delete cnn;
		channels.pop_back();
	}
//	cout << "vector="<<channels.size() <<" capacity="<<channels.capacity()<<"channel="<< channelNumber<<   endl;
	
}

