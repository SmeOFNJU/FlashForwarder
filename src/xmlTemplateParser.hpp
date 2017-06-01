#ifndef __UMDGW_XMLTEMPLATEPARSER_HPP_INCLUDED__
#define __UMDGW_XMLTEMPLATEPARSER_HPP_INCLUDED__


#include<stdint.h>
#include<map>
#include<vector>
#include<string>
#include"Template.hpp"
#include"Templates.hpp"
#include"boost/shared_ptr.hpp"
#include"tinyxml2.h"
#include"FieldInstructionString.hpp"
#include"FieldInstructionInteger.hpp"

namespace umdgw {
  namespace compressor {
    enum configStringType
    {
      FILE = 0x10,
      RAWSTRING
    };
    using namespace tinyxml2;
    class XmlTemplateParser
    {

    public:
      XmlTemplateParser() {

      }
      ~XmlTemplateParser() {

      }

      boost::shared_ptr<Templates> parse(const std::string& config, configStringType type) {

        XMLDocument doc;
        XMLError error;

        switch (type)
        {
        case umdgw::compressor::configStringType::FILE:
        {
          error = doc.LoadFile(config.c_str());
        }
          break;
        case umdgw::compressor::configStringType::RAWSTRING:
        {
          error = doc.Parse(config.c_str());
        }
          break;
        default:
          return nullptr;
          break;
        }
        if (error != XML_SUCCESS) {
          return nullptr;
        }
        XMLHandle handle(&doc);
        XMLHandle root = handle.FirstChildElement("templates");
        XMLElement* it = root.FirstChildElement("template").ToElement();
        boost::shared_ptr<Templates> tmps(new Templates());
        XMLElement* fieldins;
        while (it) {
          int templateId = 0;
          error = it->QueryIntAttribute("id", &templateId);
          if (error != XML_SUCCESS) {
            return nullptr;
          }
          boost::shared_ptr<Template> tmp(new Template(templateId));
          boost::shared_ptr<Message> message(new Message());

          fieldins = it->FirstChildElement();
          if (fieldins == nullptr) {
            break;
          }
          while (fieldins) {
            std::string fieldType = fieldins->Name();
            std::string insType = fieldins->Attribute("action");
            if (fieldType.empty() || insType.empty()) {
              break;
            }
            boost::shared_ptr<FieldInstruction> ins;
            if (fieldType == "int64") {
              Field fd(FieldType::INT64);
              if (insType == "delta") {
                ins.reset(new FieldInstructionInteger<int64_t,true,true>(InstrcutionType::DELTA));
              }
              else if(insType == "copy"){
                ins.reset(new FieldInstructionInteger<int64_t,true,true>(InstrcutionType::COPY));
              }
              message->addField(fd);
              tmp->addInstruction(ins);
            }
            else if (fieldType == "uint64") {
              Field fd(FieldType::UINT64);
              if (insType == "delta") {
                ins.reset(new FieldInstructionInteger<uint64_t,false,true>(InstrcutionType::DELTA));
              }
              else if (insType == "copy") {
                ins.reset(new FieldInstructionInteger<uint64_t,false,true>(InstrcutionType::COPY));
              }
              message->addField(fd);
              tmp->addInstruction(ins);

            }
            else if (fieldType == "int32") {
              Field fd(FieldType::INT32);
              if (insType == "delta") {
                ins.reset(new FieldInstructionInteger<uint64_t, true, false>(InstrcutionType::DELTA));
              }
              else if (insType == "copy") {
                ins.reset(new FieldInstructionInteger<uint64_t, true, false>(InstrcutionType::COPY));
              }
              message->addField(fd);
              tmp->addInstruction(ins);

            }
            else if (fieldType == "uint32") {
              Field fd(FieldType::UINT32);
              if (insType == "delta") {
                ins.reset(new FieldInstructionInteger<uint32_t, false, false>(InstrcutionType::DELTA));
              }
              else if (insType == "copy") {
                ins.reset(new FieldInstructionInteger<uint32_t, false, false>(InstrcutionType::COPY));
              }
              message->addField(fd);
              tmp->addInstruction(ins);
            }
            else if (fieldType == "string") {
              Field fd(FieldType::STRING);
              if (insType == "delta") {
                ins.reset(new FieldInstructionString(InstrcutionType::DELTA));
              }
              else if (insType == "copy") {
                ins.reset(new FieldInstructionString(InstrcutionType::COPY));
              }
              message->addField(fd);
              tmp->addInstruction(ins);
            }
            fieldins = fieldins->NextSiblingElement();
          }

          tmp->setMessage(message);
          tmps->addTemplate(tmp);
          it = it->NextSiblingElement();
        }

        return tmps;
      }
     
    private:
    };

  }//!namespce compressor
}//!namespace umdgw

#endif // !__UMDGW_XMLTEMPLATEPARSER_HPP_INCLUDED__