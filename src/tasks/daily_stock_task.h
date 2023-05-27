#pragma once

#include "../util.h"
#include "task_base.h"
#include "utf8cpp/utf8.h"
#include <codecvt>
#include <locale>
#include <regex>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

const std::string add_stock_cmd = u8"绑定股票";
const std::string remove_stock_cmd = u8"移除股票";
const std::string query_stock_cmd = u8"查询股票";

static std::string gb2312_to_utf8(std::string const &strGb2312) {
	std::vector<wchar_t> buff(strGb2312.size());
#ifdef _MSC_VER
	std::locale loc("zh-CN");
#else
	std::locale loc("C.utf8");
#endif
	wchar_t *pwszNext = nullptr;
	const char *pszNext = nullptr;
	mbstate_t state = {};
	int res = std::use_facet<std::codecvt<wchar_t, char, mbstate_t>>(loc).in(
	    state, strGb2312.data(), strGb2312.data() + strGb2312.size(), pszNext, buff.data(),
	    buff.data() + buff.size(), pwszNext);

	if (std::codecvt_base::ok == res) {
		std::wstring_convert<std::codecvt_utf8<wchar_t>> cutf8;
		return cutf8.to_bytes(std::wstring(buff.data(), pwszNext));
	}

	return "";
}

struct daily_stock_task_setting {
	daily_stock_task_setting() = default;
	bool enable = true;
	mn::time time = {0, 0, 9};
	std::map<std::uint64_t, std::vector<std::string>> subscribed_stocks;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(daily_stock_task_setting, enable, time, subscribed_stocks)

class daily_stock_task : public task_base {
private:
	std::vector<std::string> call_cmds = {add_stock_cmd, remove_stock_cmd, query_stock_cmd};
	daily_stock_task_setting setting;
	std::uint64_t user;

public:
	daily_stock_task() { task_name = "daily_stock_task"; }

	virtual bool shoule_call(mn::meesage whose) override {
		for (const std::string &cmd : call_cmds) {
			if (handle_message(whose.message, cmd)) {
				user = whose.to;
				return true;
			}
		}
		return false;
	}

	virtual void begin() override {
		load_config<daily_stock_task_setting>(setting);
		enable = setting.enable;
	}

	void setting_save_to_file() {
		nlohmann::json root = setting;
		std::string config_path = get_config_path();
		if (std::filesystem::exists(config_path)) {
			std::ofstream out(config_path.c_str());
			if (out.is_open()) {
				out << root.dump(4);
			} else {
				spdlog::error("Failed to generate the configuration file. Procedure {}",
				              config_path.c_str());
			}
			out.close();
		}
	}

	virtual void run(std::string &response) override {
		if (request_cmd == add_stock_cmd) {
			setting.subscribed_stocks[user].push_back(request_body);
			setting_save_to_file();

		} else if (request_cmd == remove_stock_cmd) {
			setting.subscribed_stocks[user].erase(std::remove(setting.subscribed_stocks[user].begin(),
			                                                  setting.subscribed_stocks[user].end(),
			                                                  request_body),
			                                      setting.subscribed_stocks[user].end());
			setting_save_to_file();
		} else if (request_cmd == query_stock_cmd) {
			std::string stock_ids;
			for (const std::string &id : setting.subscribed_stocks[user]) {
				std::string stock_id = "s_sh" + id;
				stock_ids += "," + stock_id;
			}

			if (!stock_ids.empty()) {
				std::string request_url = "https://qt.gtimg.cn/q=" + stock_ids;
				std::string stock_data;
				if (http_get(request_url, false, "", stock_data) == CURLE_OK) {

					std::string temp;
					utf8::replace_invalid(stock_data.begin(), stock_data.end(),
					                      back_inserter(temp));

					std::stringstream ss(temp);
					std::string field;
					std::regex reg("\"([^\"]*)\"");
					while (std::getline(ss, field, ';')) {
						std::smatch match;
						std::regex_search(field, match, reg);
						if (match.size() > 1) {
							response += match[1].str();
							response += "\n";
						}
					}
				}
			}
		}
	}

	virtual void tick(mn::time time) override {}
};