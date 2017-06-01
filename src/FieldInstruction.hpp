#ifndef __UMDGW_FIELDINSTRUCTION_HPP_INCLUDED__
#define __UMDGW_FIELDINSTRUCTION_HPP_INCLUDED__

#include<stdint.h>
#include<map>
#include<vector>
#include<string>
#include"Message.h"
#include"Template.hpp"

namespace umdgw {
  namespace compressor {
   
    class Template;


    enum InstrcutionType
    {
      COPY = 0,
      DELTA,
    };

    class FieldInstruction
    {
    public:
 
      virtual ~FieldInstruction() {

      }
      virtual void encode(const Message& message, std::vector<uint8_t>& buffer) = 0;

      virtual void encode(const Message& message, Message& cacheMessage,
        std::vector<uint8_t>& buffer, boost::shared_ptr<Template> templatePtr) = 0;

      virtual int decode(uint8_t* buffer,uint64_t size, Message& message,Message& cacheMessage,boost::shared_ptr<Template> tmpPtr) = 0;

      virtual void setIndex(uint16_t index) = 0;
    };

  }//!namespce compressor
}//!namespace umdgw

#endif // !__UMDGW_FIELDINSTRUCTION_HPP_INCLUDED__
