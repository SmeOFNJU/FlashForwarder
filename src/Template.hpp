#ifndef __UMDGW_TEMPLATE_HPP_INCLUDED__
#define __UMDGW_TEMPLATE_HPP_INCLUDED__

#include<stdint.h>
#include<map>
#include<vector>
#include<string>
#include"FieldInstruction.hpp"
#include"Message.h"
#include"wire_format.h"
#include"boost/shared_ptr.hpp"
#include"boost/enable_shared_from_this.hpp"

namespace umdgw {
  namespace compressor {
    using namespace boost;
    class Template:
      public boost::enable_shared_from_this<Template>
    {
    public:
      Template(uint32_t id):
        id_(id),
        inputMessage_(nullptr)
      {

      }
      ~Template() {

      }

      boost::shared_ptr<Message> getMessage() {
        return inputMessage_;
      }

      void setMessage(boost::shared_ptr<Message> message) {
        inputMessage_.reset();
        inputMessage_ = message;
      }

      uint8_t getIndex() {
        return index_;
      }

      void setIndex(const uint8_t& index) {
        index_ = index;
      }

      uint32_t getId() {
        return id_;
      }

      void setpMap(uint16_t bit) {
        pMap_ |= ((uint32_t)1 << bit);
      }

      const uint32_t& getpMap() {
        return pMap_;
      }

      void encode(const Message& message,Message& cacheMessage, std::vector<uint8_t>& buffer) {
        pMap_ = 0;
        int n = 0;
        uint8_t content[10];
        n = vss::WireFormat::WriteVarint32(id_,content);
        //encode the template id
        for (int i = 0; i < n; i++) {
          buffer.push_back(content[i]);
        }
        //encode the message index
        n = vss::WireFormat::WriteVarint64(cacheMessage.getIndex(),content);
        for (int i = 0; i < n; i++) {
          buffer.push_back(content[i]);
        }

        //reserve for the pmap
        int pmapSize = buffer.size();
        for (int i = 0; i < 4; i++) {
          buffer.push_back(0);
        }

        for (int i = 0; i < instrcutionArray_.size(); i++) {
          instrcutionArray_[i]->encode(message, cacheMessage, buffer,shared_from_this());
        }

        *(reinterpret_cast<uint32_t*>(buffer.data() + pmapSize)) = pMap_;
      }

      //encode the message with out field operation
      // in the case that encoding a new type of message
      void encode(const Message& message, std::vector<uint8_t>& buffer) {
        pMap_ = 0;
        int n = 0;
        uint8_t content[10];
        n = vss::WireFormat::WriteVarint32(id_, content);
        //encode the template id
        for (int i = 0; i < n; i++) {
          buffer.push_back(content[i]);
        }
        //encode the message index
        n = vss::WireFormat::WriteVarint64(const_cast<Message&>(message).getIndex(), content);
        for (int i = 0; i < n; i++) {
          buffer.push_back(content[i]);
        }

        //reserve for the pmap,if pmap = 0x80000000,means this is the new type of message
        //which was encoded without field operation.
        int pmapSize = buffer.size();
        for (int i = 0; i < 4; i++) {
          buffer.push_back(0);
        }
        for (int i = 0; i < instrcutionArray_.size(); i++) {
          instrcutionArray_[i]->encode(message, buffer);
        }
        //the magic number indicating full message
        pMap_ = uint32_t(1) << 31;
        *(reinterpret_cast<uint32_t*>(buffer.data() + pmapSize)) = pMap_;
      }

      int decode(uint8_t* buffer, uint64_t size, std::vector<Message>& message, Message& cacheMessage){
        if (size <= 0) {
          return - 1;
        }
        uint8_t* begin = buffer;
        int bufferSize = size;
        int n = 0;
        pMap_ = *(reinterpret_cast<uint32_t*>(begin));
        begin += 4;
        bufferSize -= 4;
        int consume = 4;
        message.push_back(Message(*inputMessage_.get()));
        for (int i = 0; i < instrcutionArray_.size(); i++) {
          if (bufferSize < 0) {
            break;
          }
          n = instrcutionArray_[i]->decode(begin, bufferSize, message.back(), cacheMessage,shared_from_this());
          if (n == -1)
            return -1;
          begin += n;
          bufferSize -= n;
          consume += n;
        }
        return consume;
      }

      void addInstruction(boost::shared_ptr<FieldInstruction> ins) {
        instrcutionArray_.push_back(ins);
        ins->setIndex(instrcutionArray_.size() - 1);
      }

    private:
      uint32_t pMap_;
      uint32_t id_;
      uint8_t index_;
      std::vector<boost::shared_ptr<FieldInstruction>> instrcutionArray_;
      boost::shared_ptr<Message> inputMessage_;
    };

  }//!namespce compressor
}//!namespace umdgw

#endif // !__UMDGW_TEMPLATE_HPP_INCLUDED__
