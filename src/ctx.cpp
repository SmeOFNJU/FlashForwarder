#include"ctx.hpp"
namespace umdgw 
{
  ctx_t::ctx_t()
  {
    
  }
  ctx_t::~ctx_t() {


  }

  bool ctx_t::initialize(const config_ctx_t & config) {
    return false;
  }

  bool ctx_t::startService(boost::shared_ptr<i_service_t> service) {
    return false;
  }

  bool ctx_t::stopService(boost::shared_ptr<i_service_t> service) {
    return false;
  }

  std::string ctx_t::monitor() {
    return std::string();
  }

  bool ctx_t::start(boost::shared_ptr<i_service_t> service)
  {
    return false;
  }

  bool ctx_t::stop(boost::shared_ptr<i_service_t> service)
  {
    return false;
  }


  boost::shared_ptr<socket_t> ctx_t::createSocket(boost::shared_ptr<service_t> service) {

    boost::shared_ptr<socket_t> sock(new socket_t(shared_from_this(),service));

    bool rt = sock->init(service->getDataType());
    if (!rt)
      sock.reset();

    return sock;
  }
  void ctx_t::pubmessage(boost::shared_ptr<i_service_t> serviceInstance, uint8_t * content, int size) {
  }
  boost::shared_ptr<io_service_pool_t> ctx_t::allocateWorkers(int wokerSize) {
    return boost::shared_ptr<io_service_pool_t>();
  }
  boost::shared_ptr<io_service_pool_t> ctx_t::createOutPutServicePool(int ioThreads)
  {
    if (ioThreads <= 0) {
      return nullptr;
    }
    boost::shared_ptr<io_service_pool_t> ptr(new io_service_pool_t(ioThreads));
    return ptr;
  }

  void ctx_t::pubmessage_t(std::string serviceName, uint8_t* content, int size) {


  }

}
