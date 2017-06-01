#ifndef __UMDGW_FIELD_HPP_INCLUDED__
#define __UMDGW_FIELD_HPP_INCLUDED__

#include"boost/shared_ptr.hpp"
#include<stdint.h>
#include<map>
#include<vector>
#include<string>


namespace umdgw {
  namespace compressor {
    enum FieldType
    {
      INT32 = 1,
      UINT32,
      INT64,
      UINT64,
      STRING,
      RAWDATA,
      DECIMAL
    };

    class Field
    {
    public:
    
      Field(FieldType type):
        sintValue_(0),
        uintValue_(0),
        type_(type)
      {

      }
      ~Field() {

      }
      void reset() {
        sintValue_ = 0;
        uintValue_ = 0;
        stringValue_.clear();
      }

      void getValue(int32_t& val) {
        if(type_ == INT32)
          val = static_cast<int32_t> (sintValue_);
      }

      void getValue(uint32_t& val) {
        if(type_ == UINT32)
          val = static_cast<uint32_t> (uintValue_);
      }

      void getValue(int64_t& val) {
        if (type_ == INT64)
          val = static_cast<int64_t> (sintValue_);
      }

      void getValue(uint64_t& val) {
        if (type_ == UINT64)
          val = static_cast<uint64_t> (uintValue_);
      }


      void getValue(std::string& val) {
        if (type_ == STRING)
          val = stringValue_;
      }


      void setValue(int32_t value) {
        if (type_ == INT32)
          sintValue_ = value;
      }
      void setValue(uint32_t value) {
        if (type_ == UINT32)
          uintValue_ = value;
      }
      void setValue(int64_t value) {
        if (type_ == INT64)
          sintValue_ = value;
      }
      void setValue(uint64_t value) {
        if (type_ == UINT64)
          uintValue_ = value;
      }
      void setValue(std::string value) {
        if (type_ == STRING)
          stringValue_ = value;
      }

    private:

      std::string stringValue_;
      int64_t sintValue_;
      uint64_t uintValue_;
      FieldType type_;
    };

  }//!namespce compressor
}//!namespace umdgw

#endif // !__UMDGW_FIELD_HPP_INCLUDED__
