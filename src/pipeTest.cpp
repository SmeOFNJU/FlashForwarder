#include<iostream>
#include<boost/thread.hpp>
#include<boost/bind.hpp>
#include"pipe.hpp"
#include"message_allocator.hpp"
#include<boost/shared_ptr.hpp>
#include<boost/thread/mutex.hpp>
#include<chrono>
#include<boost/chrono.hpp>
#include<queue>

using namespace umdgw;
const int capacity = 8*1024;
const unsigned long int  count = 1000*1000;
const unsigned int limit = 0;
boost::mutex mtx;
boost::mutex queueMtx;
boost::condition_variable cv;

void producer(boost::shared_ptr<pipe_t> pipe) {
  message_allocator_t* alloc = pipe->allocator();
  message_t* inputMessage;
  auto b = std::chrono::steady_clock::now();
  auto begin = std::chrono::steady_clock::now();
  for (unsigned long int i = 0; i < count; i++) {
    long long timeStamp = std::chrono::steady_clock::now().time_since_epoch().count();
    alloc->Allocate(capacity, &inputMessage);
 
    inputMessage->set_timestamp(timeStamp);
    pipe->Write(inputMessage);
    if (i % 500 == 0) {
      while (true) {
        auto testPoint = std::chrono::steady_clock::now();
        if ((testPoint - begin) >= std::chrono::milliseconds(limit)) {
          begin = std::chrono::steady_clock::now();
          break;
        }
      }

    }
  }
  pipe->Write(nullptr);
  auto duration = std::chrono::duration_cast< std::chrono::milliseconds >(std::chrono::steady_clock::now() - b);

 long long ticks = duration.count();
 
  boost::mutex::scoped_lock lock(mtx);
  std::cout<< "throughPut(*/s):"<< double(count) / ticks *1000 << std::endl;
}


void consumer(boost::shared_ptr<pipe_t> pipe) {
  message_t* readMessage;
  unsigned long int readCount = 0;
  long long delay = 0;

  while (pipe->Read(0, &readMessage) == 0) {
    readCount++;
   
    auto tick = readMessage->timestamp();

    pipe->Recyclemessage_t(readMessage);
    auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    delay += (now - tick);
  }

  delay /= (double)readCount;
  
  boost::mutex::scoped_lock lock(mtx);
  std::cout <<"delays(us):"<< delay /1000<< "  readCount: "<< readCount<<std::endl;
}

void producerSTD(boost::shared_ptr<std::queue<message_t*>> inputQueue) {

  auto b = std::chrono::steady_clock::now();
  auto begin = std::chrono::steady_clock::now();
  for (unsigned long int  i = 0; i < count; i++) {
    auto timeStamp = std::chrono::steady_clock::now().time_since_epoch().count();
    message_t* msg = new message_t();
    msg->Init(1, capacity);
    msg->set_timestamp(timeStamp);
    {
      boost::mutex::scoped_lock lock(queueMtx);
      inputQueue->push(msg);
    }
    cv.notify_all();
    if (i % 500 == 0) {
      while (true) {
        auto testPoint = std::chrono::steady_clock::now();
        if ((testPoint - begin) >= std::chrono::milliseconds(limit)) {
          begin = std::chrono::steady_clock::now();
          break;
        }
      }
    }
  }
  {
    boost::mutex::scoped_lock lock(queueMtx);
    inputQueue->push(nullptr);
  }
  auto e = std::chrono::steady_clock::now();
  auto ticks = std::chrono::duration_cast< std::chrono::milliseconds >(e - b).count();
  boost::mutex::scoped_lock lock(mtx);
  std::cout << "throughPut(*/s):" << double(count) / ticks * 1000 << std::endl;
}

void consumerSTD(boost::shared_ptr<std::queue<message_t*>> outQueue) {
  unsigned long int readCount = 0;
  message_t * msg = nullptr;
  long long delay = 0;
  while(true) {
    {
      boost::mutex::scoped_lock lock(queueMtx);
      while (outQueue->empty()) {
        cv.wait(lock);
      }
      msg = outQueue->front();
      outQueue->pop();
    }

    if (msg == nullptr) {
      break;
    }

    readCount++;
    auto tick = msg->timestamp();

    delete msg;
    msg = nullptr;
    delay += (std::chrono::steady_clock::now().time_since_epoch().count() - tick );

  }

  delay /= (double)readCount;

  boost::mutex::scoped_lock lock(mtx);
  std::cout << "delays(us):" << delay / 1000 << "  readCount: " << readCount << std::endl;
}

int main() {
  boost::shared_ptr<pipe_t> pipes[2];
  CreatePipePair(pipes, 1);

  auto b1 = std::chrono::steady_clock::now();
  boost::thread t1(boost::bind(producer, pipes[0]));
  boost::thread t2(boost::bind(consumer, pipes[1]));

  t1.join();
  t2.join();
  auto e1 = std::chrono::steady_clock::now();

  boost::shared_ptr<std::queue<message_t*>> queue_;
  queue_.reset(new std::queue<message_t*>);

  auto b2 = std::chrono::steady_clock::now();
  boost::thread t3(boost::bind(producerSTD, queue_));
  boost::thread t4(boost::bind(consumerSTD, queue_));
  t3.join();
  t4.join();
  auto e2 = std::chrono::steady_clock::now();

  std::cout << double((e1-b1).count())/double(1000)<<" us"<< std::endl;
  std::cout << (e2 - b2).count() / double(1000) << " us" << std::endl;
  getchar();
  return 0;
}