/*
 *  AMQP.cpp
 *  librabbitmq++
 *
 *  Created by Alexandre Kalendarev on 01.03.10.
 *
 */

#include "AMQPcpp.h"

using namespace std;

AMQP::AMQP() {
	use_ssl = false;
	proto = SET_AMQP_PROTO_BY_SSL_USAGE(use_ssl);
	AMQP::init(proto);
	AMQP::initDefault(proto);
	AMQP::connect();
};

AMQP::AMQP(string cnnStr, bool use_ssl_,
		string cacert_path_, string client_cert_path_, string client_key_path_,
		bool verify_peer_, bool verify_hostname_) {
	use_ssl = use_ssl_;
	proto = SET_AMQP_PROTO_BY_SSL_USAGE(use_ssl);
	cacert_path = cacert_path_;
	client_cert_path = client_cert_path_;
	client_key_path = client_key_path_;
	verify_peer = verify_peer_;
	verify_hostname = verify_hostname_;

	AMQP::init(proto);
	AMQP::parseCnnString(cnnStr);
	AMQP::connect();
};

AMQP::~AMQP() {
	if (channels.size()) {
		vector<AMQPBase*>::iterator i;
		for (i=channels.begin(); i!=channels.end(); i++) {
			delete *i;
		}
	}

	amqp_connection_close(cnn, AMQP_REPLY_SUCCESS);
	amqp_destroy_connection(cnn);
};

void AMQP::init(enum AMQPProto_e proto) {
	switch(proto) {
		case AMQPS_proto:

		case AMQP_proto:
		default:
			exchange=NULL;
			channelNumber=0;
			break;
	}
}

void AMQP::initDefault(enum AMQPProto_e proto) {

	host = AMQPHOST;
	vhost = AMQPVHOST;
	user = AMQPLOGIN;
	password = AMQPPSWD;

	switch(proto) {
		case AMQPS_proto:
			port = AMQPSPORT;
			break;
		case AMQP_proto:
		default:
			port = AMQPPORT;
			break;
	}
}

void AMQP::parseCnnString( string cnnString ) {
	 if (!cnnString.size()) {
		AMQP::initDefault(proto);
		return;
	 }

	// find '@' if Ok -> right part is host:port else all host:port
	string hostPortStr, userPswString;
	string::size_type pos = cnnString.find('@');

	if (0 == pos) {
		hostPortStr.assign(cnnString, 1, string::npos);
		AMQP::parseHostPort(hostPortStr);
		user = AMQPLOGIN;
		password = AMQPPSWD;
	} else if (string::npos == pos) {
		AMQP::parseHostPort(cnnString);
		user = AMQPLOGIN;
		password = AMQPPSWD;
	} else {
		hostPortStr.assign(cnnString, pos+1, string::npos);
		userPswString.assign(cnnString, 0, pos);
		AMQP::parseHostPort(hostPortStr);
		AMQP::parseUserStr(userPswString );
	}
}

void AMQP::parseUserStr(string userString) {
	string::size_type pos = userString.find(':');
	if (0 == pos) {
		user = AMQPLOGIN;
		password.assign(userString, 1, string::npos);
	} else if (string::npos == pos) {
		user = userString;
		password = AMQPPSWD;
	} else {
		user.assign(userString, 0, pos);
		password.assign(userString, pos+1, string::npos);
	}
}

void AMQP::parseHostPort(string hostPortString ) {
	string hostString;
	string portString;

	string::size_type pos = hostPortString.find(':');
	string::size_type pos2 = hostPortString.find('/');

	host = AMQPHOST;
	vhost = AMQPVHOST;
	port = AMQPPORT;

	if (pos == string::npos) {
		if ( pos2 == string::npos) {
			host = hostPortString;
		} else {
			vhost.assign(hostPortString, pos2+1, string::npos);
			if (pos2 > 0) {
				host.assign(hostPortString, 0, pos2);
			}
		}
	} else if (pos == 0) {
		if (pos2 == string::npos) {
			portString.assign(hostPortString, 1, string::npos);
		} else {
			portString.assign(hostPortString, 1, pos2-1);
			vhost.assign(hostPortString, pos2+1, string::npos);
		}
		port = atoi(portString.c_str());
	} else {
		if ( pos2 == string::npos ) {
			host.assign(hostPortString, 0, pos);
			portString.assign(hostPortString, pos+1, string::npos);
		} else {
			vhost.assign(hostPortString, pos2+1, string::npos);
			host.assign(hostPortString, 0, pos);
			portString.assign(hostPortString, pos+1, pos2-pos-1);
		}
		port = atoi(portString.c_str());
	}
}

void AMQP::connect() {
	AMQP::sockConnect();
	AMQP::login();
}

void AMQP::printConnect() {
	 cout<<  "AMQP connection: \n";

	 cout<<  "port  = " << port << endl;
	 cout<<  "host  = " << host << endl;
	 cout<<  "vhost = " << vhost << endl;
	 cout<<  "user  = " << user << endl;
	 cout<<  "passw = " << password << endl;
}

void AMQP::sockConnect() {
	int status;
	cnn = amqp_new_connection();

	switch(proto) {
		case AMQPS_proto: {
			sockfd = amqp_ssl_socket_new(cnn);

			status = amqp_ssl_socket_set_cacert(sockfd, cacert_path.c_str());
			if (status) {
				throw AMQPException("AMQP cannot set CA certificate");
			}

			status = amqp_ssl_socket_set_key(sockfd, client_cert_path.c_str(), client_key_path.c_str());
			if (status) {
				throw AMQPException("AMQP cannot set client certificate or key");
			}

#if AMQP_VERSION_MINOR <= 7
			amqp_ssl_socket_set_verify(sockfd, verify_peer ? 1 : 0);
#else
			amqp_ssl_socket_set_verify_peer(sockfd, verify_peer ? 1 : 0);
			amqp_ssl_socket_set_verify_hostname(sockfd, verify_hostname ? 1 : 0);
#endif
		}
		break;

		case AMQP_proto:
		default:
			sockfd = amqp_tcp_socket_new(cnn);
			break;
	}


	status = amqp_socket_open(sockfd, host.c_str(), port);

	if (status){
		amqp_destroy_connection(cnn);
		throw AMQPException("AMQP cannot create socket");
	}
}

void AMQP::login() {
	amqp_rpc_reply_t res = amqp_login(cnn, vhost.c_str(), 0, FRAME_MAX, 0, AMQP_SASL_METHOD_PLAIN, user.c_str(), password.c_str());
	if ( res.reply_type != AMQP_RESPONSE_NORMAL) {
		const AMQPException exception(&res);
		amqp_destroy_connection(cnn);
		throw exception;
	}
}

AMQPExchange * AMQP::createExchange() {
	channelNumber++;
	AMQPExchange * exchange = new AMQPExchange(&cnn,channelNumber);
	channels.push_back( dynamic_cast<AMQPBase*>(exchange) );
	return exchange;
}

AMQPExchange * AMQP::createExchange(string name) {
	channelNumber++;
	AMQPExchange * exchange = new AMQPExchange(&cnn,channelNumber,name);
	channels.push_back( dynamic_cast<AMQPBase*>(exchange) );
	return exchange;
}

AMQPQueue * AMQP::createQueue() {
	channelNumber++;
	AMQPQueue * queue = new AMQPQueue(&cnn,channelNumber);
	channels.push_back( dynamic_cast<AMQPBase*>(queue) );
	return queue;
}

AMQPQueue * AMQP::createQueue(string name) {
	channelNumber++;
	AMQPQueue * queue = new AMQPQueue(&cnn,channelNumber,name);
	channels.push_back( dynamic_cast<AMQPBase*>(queue) );
	return queue;
}

void AMQP::closeChannel() {
	channelNumber--;
	AMQPBase * cnn = channels.back();
	if (cnn) {
		delete cnn;
		channels.pop_back();
	}
}

