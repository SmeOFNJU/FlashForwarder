#ifndef __UMDGW_MESSAGE_HPP_INCLUDED__
#define __UMDGW_MESSAGE_HPP_INCLUDED__

#include"boost/shared_ptr.hpp"
#include<stdint.h>
#include<map>
#include<vector>
#include<string>
#include"Field.hpp"

namespace umdgw {
  namespace compressor {

    class Message 
    {
    public:
      Message() :
        index_(0),
        templateId_(0),
        fieldCount_(0)
      {

      }
      ~Message() {

      }
      void addField(Field& field) {
        fields_.push_back(field);
        fieldCount_++;
      }

      void reset() {
        for (int i = 0; i < fieldCount_; i++) {
          fields_[i].reset();
        }
        index_ = 0;
        templateId_ = 0;
        fieldCount_ = fields_.size();
      }

      template<typename valueType>
      bool setField(uint16_t index, valueType value) {
        if (index < 0 || index >= fieldCount_) {
          return false;
        }
        fields_[index].setValue(value);
        return true;
      }

      Field* getField(uint16_t index) {
        if (index < 0 || index >= fieldCount_) {
          return nullptr;
        }
        return fields_.data() + index;
      }

      void setIndex(const uint64_t& index) {
        index_ = index;
      }
      uint64_t getIndex() {
        return index_;
      }
    private:
      //the index in the templateCacheMessage array.
      //for the decoder,can just use the index to find the cache message.
      //so we need not tranfer the key except the first full message.
      uint64_t index_;

      uint16_t templateId_;
      uint16_t fieldCount_;
      std::vector<Field> fields_;
    };

  }//!namespce compressor
}//!namespace umdgw

#endif // !__UMDGW_MESSAGE_HPP_INCLUDED__
