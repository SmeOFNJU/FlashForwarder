
#ifndef __UMDGW_FIELDINSTRUCTIONSTRING_INCLUDED__
#define __UMDGW_FIELDINSTRUCTIONSTRING_INCLUDED__

#include"FieldInstruction.hpp"
#include"Message.h"
#include"Field.hpp"
#include"wire_format-inl.h"
#include<string>

namespace umdgw {
  namespace compressor {

    class FieldInstructionString :
      public FieldInstruction
    {
    public:

      FieldInstructionString(InstrcutionType type) :
        type_(type) {

      }

      virtual ~FieldInstructionString() {

      }
      virtual void encode(const Message& message, Message& cacheMessage, 
        std::vector<uint8_t>& buffer, boost::shared_ptr<Template> templatePtr) {
        uint8_t content[10];
        Field* cacheField = cacheMessage.getField(index_);
        Field* field = const_cast<Message&>(message).getField(index_);
        std::string newValue, cacheValue;
        field->getValue(newValue);
        cacheField->getValue(cacheValue);
        switch (type_)
        {
        case COPY:
        {
          if (newValue != cacheValue) {
            uint32_t size = newValue.size();
            int n = vss::WireFormat::WriteVarint32(size, content);
            for (int i = 0; i < n; i++) {
              buffer.push_back(content[i]);
            }
            for (int i = 0; i < size; i++) {
              buffer.push_back(*(newValue.data() + i));
            }
            cacheField->setValue(newValue);
            templatePtr->setpMap(index_);
          }
          else {

          }

          break;
        }
        default:
          break;
        }
      }

      virtual void encode(const Message& message, std::vector<uint8_t>& buffer) {
        Field* field = const_cast<Message&>(message).getField(index_);
        std::string value;
        field->getValue(value);
        uint32_t size = value.size();
        uint8_t content[10];
        int n = vss::WireFormat::WriteVarint32(size, content);
        for (int i = 0; i < n; i++) {
          buffer.push_back(content[i]);
        }
        for (int i = 0; i < size; i++) {
          buffer.push_back(*(value.data() + i));
        }
      }

      virtual int decode(uint8_t* buffer, uint64_t size, Message& message, Message& cacheMessage,boost::shared_ptr<Template> tmpPtr) {
        //this is the full encoded data, need not any operation on it.
        Field* cacheField = cacheMessage.getField(index_);
        Field* outField = message.getField(index_);
        const uint32_t pmap = tmpPtr->getpMap();
        int consume = 0;
        if (pmap == (uint32_t(1)<<31)) {
          int n = 0;
          uint32_t len = 0;
          n = vss::WireFormat::ReadVarint32(buffer, &len);
          if (n == -1 || n > size || len > size) {
            return -1;
          }
          buffer += n;
          consume += n;
          consume += len;

          std::string tmp;
          tmp.assign(reinterpret_cast<char*>(buffer), len);
          cacheField->setValue(tmp);
          outField->setValue(tmp);
          return consume;
        }
        switch (type_)
        {
        case COPY:
        {
          if ((pmap &(uint32_t(1) << index_))) {
            int n = 0;
            uint32_t len = 0;
            n = vss::WireFormat::ReadVarint32(buffer, &len);
            if (n == -1 || n > size || len > size) {
              return -1;
            }
            buffer += n;
            consume += n;
            consume += len;

            std::string tmp;
            tmp.assign(reinterpret_cast<char*>(buffer), len);
            cacheField->setValue(tmp);
            outField->setValue(tmp);
            return consume;
          }
          else {
            std::string tmp;
            cacheField->getValue(tmp);
            outField->setValue(tmp);
          }

          break;
        }
        default:
          return -1;
          break;
        }

        return 0;
      }

      virtual void setIndex(uint16_t index) {
        index_ = index;
      }

    private:
      InstrcutionType type_;
      uint16_t index_;

    };


  }//!namespace compressor
}//!namespace umdgw



#endif // !__UMDGW_FIELDINSTRUCTIONSTRING_INCLUDED__

