#ifndef __UMDGW_CTX_HPP_INCLUDED__
#define __UMDGW_CTX_HPP_INCLUDED__

#include"io_service_pool.hpp"
#include<boost/shared_ptr.hpp>
#include<boost/scoped_ptr.hpp>
#include<boost/enable_shared_from_this.hpp>
#include<boost/noncopyable.hpp>
#include<unordered_map>
#include<map>
#include<set>
#include<string>
#include<vector>
#include<stdint.h>
#include"service.hpp"
#include"pipe.hpp"
#include"socket.hpp"
#include"config.hpp"
#include"boost/shared_ptr.hpp"

namespace umdgw {
  class socket_t;
  class service_t;
  class i_service_t;
  class action_t;

  class ctx_t
  :public boost::enable_shared_from_this<ctx_t>,
    private boost::noncopyable
  {
  public:
    ctx_t();
    ~ctx_t();
    //report an action to the context
   // bool report(boost::shared_ptr<i_service_t>serviceInstance, action_t action);

    //initialize the ctx
    bool initialize(const config_ctx_t& config);

    ////get the service with a specific name
    //boost::shared_ptr<i_service_t> getService(const std::string& serviceName);

    //add the service
    bool addService(const std::string& name,const std::string& config);
    //start service
    bool startService(const std::string& name);
    
    //stop service
    bool stopService(const std::string& name);

    //get the monitor info
    std::string monitor();

    //assigan a socket to the service object.
    boost::shared_ptr<socket_t> createSocket(boost::shared_ptr<service_t> service);

    ////assign a unique output_service_pool
    //boost::shared_ptr<io_service_pool_t> createOutPutServicePool(int ioThreads);
    
    //publish the message_ts to the pipes which sub the service's outputing message_t.
    void pubmessage(boost::shared_ptr<i_service_t> serviceInstance, uint8_t* content, int size);
     
    //allocate the io_service_poll to the service
    boost::shared_ptr<io_service_pool_t> allocateWorkers(int wokerSize);

    //allocate the pipepair to the service
    void allocatePipePair(boost::shared_ptr<pipe_t> pipes[2]);

    //subcribe the inputservice 
    void subMssage(const std::string inputServiceName, boost::shared_ptr<pipe_t> pubPipe);

    ////the global service pool belongs to the ctx object
    //boost::scoped_ptr<io_service_pool_t> input_service_pool_ptr_;
    //boost::scoped_ptr<io_service_pool_t> output_service_pool_ptr_;

  protected:

    //service publish pipe set
    std::map<std::string,std::set<boost::shared_ptr<pipe_t> > >   pub_pipe_set_;
    std::vector<std::set<boost::shared_ptr<pipe_t> > > pub_pipe_array_;

    //input service set
    std::map<std::string,boost::shared_ptr<i_service_t> > inputServices_;
    //ooutput service set
    std::map<std::string,boost::shared_ptr<i_service_t> > outputServices_;


  };

}

#endif // !__UMDGW_CTX_HPP_INCLUDED__
