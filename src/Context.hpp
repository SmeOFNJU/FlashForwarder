#ifndef __UMDGW_CONTEXT_HPP_INCLUDED__
#define __UMDGW_CONTEXT_HPP_INCLUDED__

#include"boost/shared_ptr.hpp"
#include<stdint.h>
#include<map>
#include<vector>
#include<string>
#include"Message.h"
#include"Template.hpp"
#include"Templates.hpp"

namespace umdgw {
  namespace compressor {


    class Context 
    {
    public:

      Context(boost::shared_ptr<Templates> temps):
      templates_(temps)
      {
        templateCacheMaps_.resize(templates_->getTemplatesCount());
        templateCacheArrays_.resize(templates_->getTemplatesCount());

      }
      ~Context(){

      }
      boost::shared_ptr<Message> getTemplateMessage(uint32_t templateId) {
        auto tmp = templates_->getTemplate(templateId);
        if (tmp == nullptr) {
          return nullptr;
        }
        return tmp->getMessage();
      }

      void encode(const Message& message, uint32_t templateID,const std::string& token, std::vector<uint8_t>& buffer) {
        boost::shared_ptr<Template> tmp = templates_->getTemplate(templateID);
        if (tmp == nullptr) {
          return;
        }
        uint8_t index = tmp->getIndex();
        snapShotCacheMap_t& cacheMap = templateCacheMaps_[index];
        snapShotCacheArray_t& cacheArray = templateCacheArrays_[index];

        auto it = cacheMap.find(token);
        if (it != cacheMap.end()) {
          tmp->encode(message, *(it->second).get(), buffer);
        }
        else {
          boost::shared_ptr<Message> cacheMessage(new Message(message));
          cacheArray.push_back(cacheMessage);
          cacheMessage->setIndex(cacheArray.size()-1);
          cacheMap[token] = cacheMessage;
        //  cacheMessage->reset();
          tmp->encode(*cacheMessage.get(),buffer);
        }
      }

      void decode(uint8_t* buffer, uint64_t size, std::vector<Message>& messages) {
        if (size <= 0) {
          return;
        }
        uint8_t* begin = buffer;
        uint8_t* end = buffer + size-1;
        while (begin <= end) {
          uint32_t templateId = 0;
          int n = vss::WireFormat::ReadVarint32(begin, &templateId);
          if (size < n || n == -1) {
            return;
          }
          begin += n;
          size -= n;
          uint64_t cacheIndex = 0;
          n = vss::WireFormat::ReadVarint64(begin, &cacheIndex);
          if (size < n || n == -1) {
            return;
          }
          begin += n;
          size -= n;
          auto tmp = templates_->getTemplate(templateId);
          if (tmp == nullptr) {
            return;
          }
          snapShotCacheArray_t& snapShotArray = templateCacheArrays_[tmp->getIndex()];
          int consume = 0;
          if (cacheIndex >= snapShotArray.size()) {
            snapShotArray.resize(cacheIndex + 1);
            //the cache messsage must conform to the same message structure.
            boost::shared_ptr<Message> cacheMessage(new Message(*(tmp->getMessage().get())));
            snapShotArray[cacheIndex] = cacheMessage;
            cacheMessage->setIndex(cacheIndex);
            cacheMessage->reset();
            consume = tmp->decode(begin, size, messages, *cacheMessage.get());
          }
          else {
            consume = tmp->decode(begin, size, messages, *snapShotArray[cacheIndex].get());
          }
          if (consume == -1) {
            messages.clear();
            return;
          }
          begin += consume;
          size -= consume;
        }


      }
    private:
      boost::shared_ptr<Templates> templates_;

      typedef std::map<std::string, boost::shared_ptr<Message>> snapShotCacheMap_t;
      typedef std::vector<boost::shared_ptr<Message>> snapShotCacheArray_t;
      std::vector<snapShotCacheMap_t> templateCacheMaps_;
      std::vector<snapShotCacheArray_t> templateCacheArrays_;

    };

  }//!namespce compressor
}//!namespace umdgw

#endif // !__UMDGW_CONTEXT_HPP_INCLUDED__
