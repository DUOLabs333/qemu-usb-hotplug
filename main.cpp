#include <shared_mutex>
#include <string>
#include <vector>
#include <utility>
#include <fstream>
#include <iostream>
#include <set>
#include <fcntl.h>

#include <format>

#include "usb.ids.h"

#include <boost/json/src.hpp>
#include <asio.hpp>
#include <libusb/libusb.h>


std::string getEnv(const char* _key, std::string _default){
	auto result=std::getenv(_key);
	if (result == NULL){
		return _default;
	}else{
		return result;
	}
}

auto SOCKET=getEnv("QEMU_SOCKET", "./qemu-monitor.sock");
auto CONFIG=getEnv("HOTPLUG_CONF_FILE", "./qemu-usb-hotplug.json");

typedef std::pair<int, int> usb_pair;
std::set<usb_pair> products;
std::set<int> vendors;

std::string mode; //"allow_list", only allowing what's on the list, and blocking everything else; "block_list", only blocking what's on the list, and allowing everything else

boost::json::object config;

asio::io_context context;
asio::local::stream_protocol::socket qemu_socket(context);
std::mutex socket_mutex;

void connectToQemu(){
	asio::error_code ec;
	while(true){
		qemu_socket.connect(asio::local::stream_protocol::endpoint(SOCKET), ec);
		if(!ec){
			break;
		}
		std::cerr << std::format("Error! Can't connect to socket {} due to: {}", SOCKET, ec.message()) << std::endl;
	}
}

void sendRequest(bool add, uint16_t vendor_id, uint16_t product_id){
	std::unique_lock<std::mutex> lk(socket_mutex);
	asio::error_code ec;
	while(true){
		asio::write(qemu_socket, asio::buffer(std::format("device_{} usb-host,vendorid={},productid={}\n", add ? "add" : "del", vendor_id, product_id)), ec);
		if (!ec){
			break;
		}
		connectToQemu();
	}
}
void HandleEvent(libusb_hotplug_event event, uint16_t vendor_id, uint16_t product_id){
	if (event==LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED){
		bool match_action;
		if(mode=="BLOCK"){
			match_action=false;
		}else{
			match_action=true;
		}

		if(vendors.contains(vendor_id) || products.contains({vendor_id, product_id})){
				sendRequest(match_action, vendor_id, product_id);
		}else{
				sendRequest(!match_action, vendor_id, product_id);
		}
			
	}else{
		sendRequest(false, vendor_id, product_id);
	}

}

int USBCallBack(libusb_context *, libusb_device* device, libusb_hotplug_event event, void*){
	struct libusb_device_descriptor descriptor;
	libusb_get_device_descriptor(device, &descriptor);

	std::thread(HandleEvent, event, descriptor.idVendor, descriptor.idProduct).detach();

	return 0;
}
int main(int argc, char** argv){
	auto config_ifstream=std::ifstream(CONFIG.c_str());

	if (config_ifstream.fail()){
		std::cerr << std::format("Warning! Can not open config file {} due to: {}",CONFIG, strerror(errno)) << std::endl;
	}else{
		std::error_code ec;
		auto parsed=boost::json::parse(config_ifstream, ec);
		if(ec){
			std::cerr << std::format("Warning! Can not read config file {} due to: {}", CONFIG, ec.message()) << std::endl;
		}else{
			config=parsed.as_object();
		}
	}
	if (!config.contains("mode")){
		config["mode"]="BLOCK";
	}
	if (!config.contains("list")){
		config["list"]=boost::json::array();
	}
	
	std::istringstream usb_ids_stream(std::move(std::string(usb_ids_json,usb_ids_json+usb_ids_json_len)));
	
	std::error_code ec;
	auto usb_ids=boost::json::value_to<std::vector<std::pair<std::string,usb_pair>>>(boost::json::parse(usb_ids_stream, ec));

	mode=boost::json::value_to<std::string>(config.at("mode"));

	for (auto& val: config["list"].as_array()){
		if(val.is_string()){ //Substring of the form "Sandisk ..."
			auto val_str=val.as_string();
			for(auto& usb_id: usb_ids){
				if((usb_id.first.find(val_str.c_str()) != std::string::npos) && !vendors.contains(usb_id.second.first)){
					if (usb_id.second.second==-1){
						vendors.insert(usb_id.second.first);
					}else{
						products.insert(usb_id.second);
					}
				}
			}
		}else {
			auto val_arr=val.as_array(); //[...,...]
			std::array<int, 2> usb_id;
			for(int i=0; i< 2; i++){
				if(val_arr[i].is_string()){ //Hex string of the form "0xaaa"
					usb_id[i]=std::strtoul(val_arr[i].as_string().c_str(), NULL, 0);
					
				}else if(val_arr[i].is_uint64()){
					usb_id[i]=val_arr[i].as_uint64();
				}else{ // [..., -1]
					usb_id[1]=-1;
					
				}
					
			}
			if (usb_id[1]==-1){
				vendors.insert(usb_id[0]);
			}else{
				products.insert({usb_id[0], usb_id[1]});
			}

		}
	}
	
	connectToQemu();
	libusb_init_context(NULL, NULL, 0);
	libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, USBCallBack, NULL, NULL);
	
	while(true){
		libusb_handle_events_completed(NULL, NULL);
	}	


}
