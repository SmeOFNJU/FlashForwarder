#ifndef __UMDGW_TEMPLATES_HPP_INCLUDED__
#define __UMDGW_TEMPLATES_HPP_INCLUDED__


#include<stdint.h>
#include<map>
#include<vector>
#include<string>
#include"Template.hpp"
#include"boost/shared_ptr.hpp"

namespace umdgw {
  namespace compressor {

    class Templates
    {
    public:
      Templates() {
        index_ = 0;
      }
      ~Templates() {

      }
      boost::shared_ptr<Template> getTemplate(uint32_t id) {
        auto it = templatesMap_.find(id);
        if (it != templatesMap_.end()) {
          return it->second;
        }
        else {
          return nullptr;
        }
      }

      void addTemplate(boost::shared_ptr<Template> tmp) {
        templatesMap_[tmp->getId()] = tmp;
        tmp->setIndex(index_++);
      }
      uint8_t getTemplatesCount() {
        return index_;
      }
    private:
      std::map<uint32_t, boost::shared_ptr<Template>> templatesMap_;
      uint8_t index_;
    };

  }//!namespce compressor
}//!namespace umdgw

#endif // !__UMDGW_TEMPLATES_HPP_INCLUDED__