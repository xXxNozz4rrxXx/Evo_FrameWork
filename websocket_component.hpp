#pragma once
//#include <easywsclient.hpp>
#include <nlohmann/json.hpp>
#include <memory>
#include <easywsclient.hpp>

using json = nlohmann::json;
using easywsclient::WebSocket;

class websocket_component {
	WebSocket* ws;
	std::string url;
	//std::unique_ptr<WebSocket> ws;
public:
	websocket_component(const std::string& url) {
		this->url = url;
		ws = WebSocket::from_url(url);
		
	}
	~websocket_component() { 
		delete ws;
	}
	void make_request(json& j) {
		//if(ws)
			ws->send(j.dump());
	}
	void ping() {
		//if (ws)
			ws->sendPing();
	}
	void poll() {
		//if (ws)
			ws->poll();
	}
	void check_available() {
		/*if (ws && ws->getReadyState() == WebSocket::CLOSED) {
			ws->close();
			delete ws;
		}
		else */
			if(ws)return;
			ws = WebSocket::from_url(url);
	}
	template<typename Callable>
	void dispatch(Callable t) {
	/*	if (ws)*/
			ws->dispatch(t);
	}
};