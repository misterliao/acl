#include "stdafx.h"
#include "sysload.h"
#include "server/ServerConnection.h"
#include "server/ServerManager.h"

ServerConnection* ServerManager::min()
{
	if (conns_.empty())
		return NULL;

	ServerConnection* conn = NULL;

	std::vector<ServerConnection*>::iterator it = conns_.begin();
	for (; it != conns_.end(); ++it)
	{
		if (conn == NULL || (*it)->get_conns() < conn->get_conns())
			conn = *it;
	}

	return conn;
}

void ServerManager::set(ServerConnection* conn)
{
	std::vector<ServerConnection*>::iterator it = conns_.begin();
	for (; it != conns_.end(); ++it)
	{
		if ((*it) == conn)
			return;
	}
	conns_.push_back(conn);
}

void ServerManager::del(ServerConnection* conn)
{
	std::vector<ServerConnection*>::iterator it = conns_.begin();
	for (; it != conns_.end(); ++it)
	{
		if ((*it) == conn)
		{
			conns_.erase(it);
			break;
		}
	}
}

// 璇ュ嚱鏁扮敱涓荤嚎绋嬩腑鐨勫畾鏃跺櫒璋冪敤
void ServerManager::buildStatus()
{
	// 鍥犱负鍦ㄥ瓙绾跨▼涓篃浼氳鍙� json_ 瀵硅薄锛屾墍浠ュ鍗曚緥鎴愬憳鍙橀噺
	// json_ 杩涜鍔犻攣淇濇姢锛屼负浜嗛槻姝富绾跨▼琚暱鏈熼樆濉炲湪閿佷笂锛屾墍浠�
	// 閲囩敤浜嗗皾璇曞姞閿佹柟寮�
	if (lock_.try_lock() == false)
		return;

	json_.reset();
	xml_.reset();

	acl::json_node& json_servers = json_.create_array();
	json_.get_root().add_child("server", json_servers);

	acl::xml_node& xml_servers = xml_.create_node("server");
	xml_.get_root().add_child(xml_servers);

	long long total_conns = 0, total_used = 0, total_qlen = 0;
	long long total_max_threads = 0, total_curr_threads = 0;
	long long total_busy_threads = 0;
	acl::string load_s;
	(void) sysload::get_load(&load_s);  // 鑾峰緱褰撳墠绯荤粺鐨勮礋杞�

	std::vector<ServerConnection*>::const_iterator cit = conns_.begin();
	for (; cit != conns_.end(); ++cit)
	{
		total_conns += (*cit)->get_conns();
		total_used += (*cit)->get_used();
		total_qlen += (*cit)->get_qlen();
		total_max_threads += (*cit)->get_max_threads();
		total_curr_threads += (*cit)->get_curr_threads();
		total_busy_threads += (*cit)->get_busy_threads();

		acl::json_node& json_server = json_.create_node();
		json_server.add_number("conns", (*cit)->get_conns())
			.add_number("used", (*cit)->get_used())
			.add_number("pid", (*cit)->get_pid())
			.add_number("max_threads", (*cit)->get_max_threads())
			.add_number("curr_threads", (*cit)->get_curr_threads())
			.add_number("busy_threads", (*cit)->get_busy_threads())
			.add_number("qlen", (*cit)->get_qlen())
			.add_text("type", (*cit)->get_type());
		json_servers.add_child(json_server);

		xml_servers.add_child("proc", true)
			.add_child("conns", (long long int)
					(*cit)->get_conns())
			.add_child("used", (long long int) (*cit)->get_used())
			.add_child("pid", (long long int) (*cit)->get_pid())
			.add_child("max_threads", (long long int)
					(*cit)->get_max_threads())
			.add_child("curr_threads", (long long int)
					(*cit)->get_curr_threads())
			.add_child("busy_threads", (long long int)
					(*cit)->get_busy_threads())
			.add_child("qlen", (long long int) (*cit)->get_qlen())
			.add_child("type", (*cit)->get_type().c_str());
	}

	json_.get_root().add_number("conns", total_conns)
		.add_number("used", total_used)
		.add_number("qlen", total_qlen)
		.add_number("max_threads", total_max_threads)
		.add_number("curr_threads", total_curr_threads)
		.add_number("busy_threads", total_busy_threads)
		.add_text("addr", var_cfg_local_addr.c_str())
		.add_text("load", load_s.c_str());

	xml_servers.add_attr("conns", total_conns)
		.add_attr("used", total_used)
		.add_attr("qlen", total_qlen)
		.add_attr("max_threads", total_max_threads)
		.add_attr("curr_threads", total_curr_threads)
		.add_attr("busy_threads", total_busy_threads)
		.add_attr("addr", var_cfg_local_addr.c_str())
		.add_attr("load", load_s.c_str());

#if 0
	acl::json_node& n = json_.create_node();
	n.add_number("conns", 1).add_number("used", 2)
		.add_number("pid", 1).add_number("max_threads", 10)
		.add_number("curr_threads", 1).add_number("busy_threads", 2)
		.add_number("qlen", 0).add_text("type", "default");
	servers.add_child(n);
	acl::json_node& m = json_.create_node();
	m.add_number("conns", 2).add_number("used", 3)
		.add_number("pid", 2).add_number("max_threads", 10)
		.add_number("curr_threads", 2).add_number("busy_threads", 1)
		.add_number("qlen", 0).add_text("type", "default");
	servers.add_child(m);

	acl::string buf;
	statusToString(buf);
	printf(">>>buf: %s\r\n", buf.c_str());
#endif

	lock_.unlock();
}

void ServerManager::statusToJson(acl::string& buf)
{
	// 鍥犱负璇ユ柟娉曞皢鐢卞瓙绾跨▼璋冪敤锛屾墍浠ュ鍗曚緥鎴愬憳鍙橀噺 json_ 杩涜鍔犻攣淇濇姢
	lock_.lock();
	json_.build_json(buf);
	lock_.unlock();
}

void ServerManager::statusToXml(acl::string& buf)
{
	// 鍥犱负璇ユ柟娉曞皢鐢卞瓙绾跨▼璋冪敤锛屾墍浠ュ鍗曚緥鎴愬憳鍙橀噺 xml_ 杩涜鍔犻攣淇濇姢
	lock_.lock();
	xml_.build_xml(buf);
	lock_.unlock();
}
