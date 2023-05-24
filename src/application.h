#pragma once

#include "mono.h"
#include "processes.h"
#include "type.h"

typedef websocketpp::client<websocketpp::config::asio_client> client;

class application {
public:
	application();
	virtual ~application();
	void run();
	void send_message(std::string message);

protected:
	bool connect_server();
	void on_server_message(websocketpp::connection_hdl hdl, client::message_ptr msg);
	void on_server_open(websocketpp::connection_hdl hdl);

	void tick(mn::time time, float delta_time);

	uint64_t get_online_time();
	uint64_t get_local_time();

	bool load_app_config();

public:
	static bool should_exit_app;

private:
	mn::setting setting;
	processes proc;
	websocketpp::connection_hdl websocket_hdl;
	client websocket_endpoint;
	websocketpp::lib::shared_ptr<websocketpp::lib::thread> websocket_thread;
	std::string app_config_path;
};