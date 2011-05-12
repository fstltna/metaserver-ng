#include "MetaServer.hpp"
#include "MetaServerHandlerTCP.hpp"
#include "MetaServerHandlerUDP.hpp"

MetaServer::MetaServer(boost::asio::io_service& ios)
	: expiry_timer_delay_milliseconds_(1500),
	  update_timer_delay_milliseconds_(4000)
{
	ms_data_.clear();
	expiry_timer_ = new boost::asio::deadline_timer(ios, boost::posix_time::seconds(1));
	expiry_timer_->async_wait(boost::bind(&MetaServer::expiry_timer, this, boost::asio::placeholders::error));
	update_timer_ = new boost::asio::deadline_timer(ios, boost::posix_time::seconds(1));
	update_timer_->async_wait(boost::bind(&MetaServer::update_timer, this, boost::asio::placeholders::error));

	srand( (unsigned)time(0));

}

MetaServer::~MetaServer()
{
	std::cout << "dtor" << std::endl;
}

void
MetaServer::expiry_timer(const boost::system::error_code& error)
{
	boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();
	//std::cout << "expiry_timer_(" << now << ")" << std::endl;
	/**
	 * do expiry tasks
	 * * possibly add a time of last update and do a delta and sleep less if we're running behind
	 */

	std::cout << "EXPIRY DUMP:" << std::endl;
	// this doesn't work !!!!!!!!
	dumpHandshake();
	std::cout << "EXPIRY DUMP:" << std::endl;


    expiry_timer_->expires_from_now(boost::posix_time::milliseconds(expiry_timer_delay_milliseconds_));
    expiry_timer_->async_wait(boost::bind(&MetaServer::expiry_timer, this, boost::asio::placeholders::error));
}

void
MetaServer::update_timer(const boost::system::error_code& error)
{
	boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();
	//std::cout << "update_timer_(" << now << ")" << std::endl;
	/**
	 * do update tasks
	 * possibly add a time of last update and do a delta and sleep less if we're running behind
	 */
    update_timer_->expires_from_now(boost::posix_time::milliseconds(update_timer_delay_milliseconds_));
    update_timer_->async_wait(boost::bind(&MetaServer::update_timer, this, boost::asio::placeholders::error));
}


/**
 * Convenience method that evaluates what type of packet and call appropriate handle method
 * @param msp incoming metaserver packet
 * @param rsp outgoing metaserver packet to be filled
 */
void
MetaServer::processMetaserverPacket(MetaServerPacket& msp, MetaServerPacket& rsp)
{
	switch(msp.getPacketType())
	{
	case NMT_SERVERKEEPALIVE:
		processSERVERKEEPALIVE(msp,rsp);
		break;
	default:
		std::cout << "Packet type [" << msp.getPacketType() << "] not supported" << std::endl;
	}
}

/**
 * @param in incoming metaserver packet
 * @param out outgoing metaserver packet
 *
 * 	NMT_SERVERKEEPALIVE		indicates a keep alive for a server, also serves as a "registration"
 * 		Type Size: 4 bytes
 * 		Value: 1 ( 00 00 00 01 )
 * 		Payload Size: 0
 * 	Response Packet Type: NMT_HANDSHAKE
 */
void
MetaServer::processSERVERKEEPALIVE(MetaServerPacket& in, MetaServerPacket& out)
{

	unsigned int handshake = rand();
	std::string a = in.getAddress();
	boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();

	std::map<std::string,std::string> tmp_;

	tmp_["ip"] = in.getAddress();
	tmp_["expiry"] = boost::posix_time::to_simple_string(now);

	int i = addHandshake(handshake,tmp_);

	std::cout << "handshake value: " << i << std::endl;

	// construct response
	out.setPacketType(NMT_HANDSHAKE);
	out.addPacketData(handshake);

}

int
MetaServer::addHandshake(unsigned int hs, std::map<std::string,std::string> attr )
{
	handshake_queue_[hs] = attr;
	std::cout << "HANDSHAKE DUMP:" << std::endl;
	dumpHandshake();
	std::cout << "HANDSHAKE DUMP:" << std::endl;
	return handshake_queue_.size();
}

void
MetaServer::dumpHandshake()
{
    std::map<unsigned int, std::map<std::string,std::string> >::iterator itr;
    std::map<std::string,std::string>::iterator iitr;

    std::cout << "handshake_queue_.size() : " << handshake_queue_.size() << std::endl;

    for( itr = handshake_queue_.begin(); itr != handshake_queue_.end(); itr++)
    {

        std::cout << itr->first << std::endl;
        for ( iitr = itr->second.begin() ; iitr != itr->second.end(); iitr++)
        {
           std::cout << "\t" << iitr->first << "\t\t\t" << iitr->second << std::endl;
        }

    }
}

/*
	Entry point
*/
int main(int argc, char** argv)
{
	std::cout << "Start" << std::endl;
	boost::asio::io_service io_service;

	MetaServer ms(io_service);

	MetaServerHandlerTCP tcp(ms, io_service, "192.168.1.200", 8453);
	MetaServerHandlerUDP udp(ms, io_service, "192.168.1.200", 8453);


	try
	{
		std::cout << "do stuff" << std::endl;
		io_service.run();
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << std::endl;
	}
	std::cout << "End" << std::endl;
	return 0;
}
