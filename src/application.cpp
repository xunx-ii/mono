#include "application.h"
#include "spdlog/spdlog.h"
#include "type.h"
#include "util.h"
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <string>

bool application::should_exit_app = false;

application::application() {

	if (!load_app_config()) {
		application::should_exit_app = true;
		return;
	}

	proc.app = this;

	websocket_endpoint.set_access_channels(websocketpp::log::alevel::all);
	websocket_endpoint.clear_access_channels(websocketpp::log::alevel::frame_payload);
	websocket_endpoint.set_error_channels(websocketpp::log::elevel::all);
	websocket_endpoint.clear_error_channels(websocketpp::log::elevel::all);

	websocket_endpoint.init_asio();
	websocket_endpoint.start_perpetual();

	websocket_thread = websocketpp::lib::make_shared<websocketpp::lib::thread>(&client::run, &websocket_endpoint);
}

bool application::load_app_config() {
	std::filesystem::path current_path = std::filesystem::current_path() / "config.json";
	app_config_path = current_path.string();

	if (!std::filesystem::exists(app_config_path)) {
		make_app_config(app_config_path);
		spdlog::warn("No configuration file detected, try to generate automatically: {}", app_config_path);
	}

	std::ifstream ifs(app_config_path);
	if (!ifs.is_open()) {
		spdlog::error("Failed to open config file: {}", app_config_path);
		return false;
	}

	try {
		nlohmann::json root = nlohmann::json::parse(ifs);
		setting = root.get<mn::setting>();
	} catch (nlohmann::json::parse_error &e) {
		spdlog::error("Failed to parse config file: {} {}", app_config_path, e.what());
		ifs.close();
		return false;
	}

	ifs.close();
	return true;
}

application::~application() {
	websocket_endpoint.stop_perpetual();
	websocketpp::lib::error_code ec;
	websocket_endpoint.close(websocket_hdl, websocketpp::close::status::going_away, "", ec);
	if (ec) {
		spdlog::error("Error closing connection : {}", ec.message());
	}
}

void application::run() {
	if (!connect_server()) {
		spdlog::error("connection faild!");
		return;
	}

	uint64_t now_online_time = get_online_time();

	if (now_online_time == 0) {
		spdlog::error("get_online_time faild!");
		return;
	}

	uint64_t now_local_time = get_local_time();
	int64_t deviation_time = now_online_time - now_local_time;
	uint64_t last_time = now_local_time + deviation_time;

	if (last_time != now_online_time) {
		spdlog::error("set time error");
	}

	proc.begin();

	// 使用中国的时区
	int offset = 8 * 3600;

	{
		uint64_t now_time = get_local_time() + deviation_time;
		time_t tt = static_cast<time_t>((now_time / 1000) + offset);
		struct tm *time_info = gmtime(&tt);

		char buffer[80];
		std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", time_info);
		spdlog::info("当前服务器时间 {}", buffer);
	}

	while (!should_exit_app) {

		// 使用本地时区+网络时间校准偏移
		uint64_t now_time = get_local_time() + deviation_time;

		time_t tt = static_cast<time_t>((now_time / 1000) + offset);

		struct tm *time_info = gmtime(&tt);

		float delta_time = (now_time - last_time) / 1000.f;

		tick({(uint8_t)time_info->tm_sec, (uint8_t)time_info->tm_min, (uint8_t)time_info->tm_hour}, delta_time);

		last_time = now_time;
	}
}

bool application::connect_server() {
	websocketpp::lib::error_code ec;

	client::connection_ptr connection = websocket_endpoint.get_connection(setting.server_uri, ec);
	if (ec) {
		spdlog::error("Connect initialization error:  {}", ec.message());
		return false;
	}

	if (!setting.access_token.empty()) {
		std::string access_token = "access_token " + setting.access_token;
		connection->append_header("Authorization", access_token);
	}

	connection->append_header("Sec-WebSocket-Protocol", "application/json; charset=utf-8");

	connection->set_message_handler(websocketpp::lib::bind(&application::on_server_message, this,
	                                                       websocketpp::lib::placeholders::_1,
	                                                       websocketpp::lib::placeholders::_2));

	connection->set_open_handler(
	    websocketpp::lib::bind(&application::on_server_open, this, websocketpp::lib::placeholders::_1));

	websocket_endpoint.connect(connection);

	return true;
}

void application::on_server_message(websocketpp::connection_hdl hdl, client::message_ptr msg) {
	switch (msg->get_opcode()) {
	case websocketpp::frame::opcode::text: {
		proc.processing(recv_message_type::text, msg->get_payload());
		break;
	}
	case websocketpp::frame::opcode::binary: {
		proc.processing(recv_message_type::binary, websocketpp::utility::to_hex(msg->get_payload()));
		break;
	}
	default: {
		spdlog::warn("on_server_message : Unsupported message format");
		break;
	}
	}
}

void application::on_server_open(websocketpp::connection_hdl hdl) { websocket_hdl = hdl; }

void application::send_message(std::string message) {
	websocketpp::lib::error_code ec;
	websocket_endpoint.send(websocket_hdl, message, websocketpp::frame::opcode::text, ec);
	if (ec) {
		spdlog::error("send daily news error:  {}", ec.message());
	}
}

void application::tick(mn::time time, float delta_time) { proc.tick(time, delta_time); }

uint64_t application::get_online_time() {
	std::string url = "http://api.m.taobao.com/rest/api3.do?api=mtop.common.getTimestamp";
	std::string response;
	uint64_t time_now = 0;
	if (http_get(url, false, "", response) == CURLE_OK) {
		try {
			nlohmann::json root = nlohmann::json::parse(response);
			time_now = std::stoull(root["data"]["t"].get<std::string>());
		} catch (nlohmann::json::parse_error &e) {
			spdlog::error("failed to parse message: {}", e.what());
		}
	}

	return time_now;
}

uint64_t application::get_local_time() {
	return std::chrono::duration_cast<std::chrono::milliseconds>(
	           std::chrono::system_clock::now().time_since_epoch())
	    .count();
}
