#ifndef __UMDGW_FIELDINSTRUCTIONINTEGER_INCLUDED__
#define __UMDGW_FIELDINSTRUCTIONINTEGER_INCLUDED__

#include"FieldInstruction.hpp"
#include"Message.h"
#include"Field.hpp"
#include"wire_format-inl.h"

namespace umdgw {
  namespace compressor {

    template<typename intType,bool s,bool b>
    class FieldInstructionInteger:
      public FieldInstruction
    {
    public:

      FieldInstructionInteger(InstrcutionType type) :
        type_(type),
        signed_(s),
        shortBits_(!b)
      {
       
      }

      virtual ~FieldInstructionInteger() {

      }
      void setSigned(bool sign) {
        signed_ = s;
      }
      void setShotBits(bool shortBits) {
        shortBits_ = !b;
      }

      virtual void encode(const Message& message, Message& cacheMessage,
        std::vector<uint8_t>& buffer, boost::shared_ptr<Template> templatePtr) {
        uint8_t content[10];
        Field* field = const_cast<Message&>(message).getField(index_);
        Field* cacheField = cacheMessage.getField(index_);
        intType newValue, cacheValue;
        field->getValue(newValue);
        cacheField->getValue(cacheValue);
        switch (type_)
        {
        case COPY:
        {
          if (newValue != cacheValue) {
            int n = 0;
            if (signed_) {
              if (shortBits_) {
                n = vss::WireFormat::WriteSignedVarint32(newValue, content);
              }
              else {
                n = vss::WireFormat::WriteSignedVarint64(newValue, content);
              }

            }
            else {
              if (shortBits_) {
                n = vss::WireFormat::WriteVarint32(newValue, content);
              }
              else {
                n = vss::WireFormat::WriteVarint64(newValue, content);
              }
            }

            for (int j = 0; j < n; j++) {
              buffer.push_back(content[j]);
            }
            cacheField->setValue(newValue);
            templatePtr->setpMap(index_);
          }
          else {
 
          }
          break;
        }
        case DELTA:
        {
          int n = 0;

          int64_t delta = static_cast<int64_t>(newValue) - cacheValue;

          n = vss::WireFormat::WriteSignedVarint64(delta, content);

          for (int i = 0; i < n; i++) {
            buffer.push_back(content[i]);
          }
          cacheField->setValue(newValue);
          templatePtr->setpMap(index_);
          break;
        }
        default:
          break;
        }
      }

      virtual void encode(const Message& message, std::vector<uint8_t>& buffer) {
        Field* field = const_cast<Message&>(message).getField(index_);
        intType value;
        field->getValue(value);
        int size = sizeof(intType);
        for (int i = 0; i < size; i++) {
          buffer.push_back(*(reinterpret_cast<uint8_t*>(&value) + i));
        }
      }


      virtual int decode(uint8_t* buffer,uint64_t size, Message& message, Message& cacheMessage,boost::shared_ptr<Template> tmpPtr) {
        Field* outField = message.getField(index_);
        Field* cacheField = cacheMessage.getField(index_);
        const uint32_t pmap = tmpPtr->getpMap();
        int consume = 0;
        if (pmap &(uint32_t(1) << 31)) {
          if (size < sizeof(intType)) {
            return -1;
          }
          intType value = *(reinterpret_cast<intType*>(buffer));
          consume += sizeof(intType);
          outField->setValue(value);
          cacheField->setValue(value);
          return consume;
        }

        switch (type_)
        {
        case COPY:
        {
          int n = 0;
          intType value = 0;
          if (pmap&(uint32_t(1) << index_)) {
            if (signed_) {
              if (shortBits_) {
                n = vss::WireFormat::ReadSignedVarint32(buffer, reinterpret_cast<int32_t*>(&value));
                if (n == -1 || n > size) {
                  return -1;
                }
                consume += n;
                cacheField->setValue(value);
                outField->setValue(value);
              }
              else {
                n = vss::WireFormat::ReadSignedVarint64(buffer, reinterpret_cast<int64_t*>(&value));
                if (n == -1 || n > size) {
                  return -1;
                }
                consume += n;
                cacheField->setValue(value);
                outField->setValue(value);
              }
            }
            else {
              if (shortBits_) {
                n = vss::WireFormat::ReadVarint32(buffer, reinterpret_cast<uint32_t*>(&value));
                if (n == -1 || n > size) {
                  return -1;
                }
                consume += n;
                cacheField->setValue(value);
                outField->setValue(value);
              }
              else {
                n = vss::WireFormat::ReadVarint64(buffer, reinterpret_cast<uint64_t*>(&value));
                if (n == -1 || n > size) {
                  return -1;
                }
                consume += n;
                cacheField->setValue(value);
                outField->setValue(value);
              }
            }
          }
          return consume;
          break;
        }
        case DELTA:
        {
          int64_t delta = 0;
          int n = 0;
          n = vss::WireFormat::ReadSignedVarint64(buffer, &delta);
          if (n == -1 || n > size) {
            return -1;
          }

          intType newValue, cacheValue;
          cacheField->getValue(cacheValue);
          newValue = cacheValue + delta;
          cacheField->setValue(newValue);
          outField->setValue(newValue);
          consume += n;
          return consume;
          break;
        }
        default:
          return -1;
          break;
        }

        return 0;
      }
     
      virtual void setIndex( uint16_t index) {
        index_ = index;
      }

    private:
      bool shortBits_, signed_;
      InstrcutionType type_;
      uint16_t index_;

    };


  }//!namespace compressor
}//!namespace umdgw



#endif // !__UMDGW_FIELDINSTRUCTIONINTEGER_INCLUDED__

