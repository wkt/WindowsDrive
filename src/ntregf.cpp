#include <cstring>
#include <iostream>
#include <cstdlib>

#include "ntregf.h"
#include "utils.h"

static inline std::string
to_gpt_part_uuid(uint8_t *d,size_t size)
{
    uint8_t p1[4] = {0};
    uint8_t p2[2] = {0};
    uint8_t p3[2] = {0};
    uint8_t p4[2] = {0};
    uint8_t p5[6] = {0};

    memcpy(p1,d+8,sizeof(p1));
    memcpy(p2,d+12,sizeof(p2));
    memcpy(p3,d+14,sizeof(p3));
    memcpy(p4,d+16,sizeof(p4));
    memcpy(p5,d+18,sizeof(p5));
    return hex(p1,sizeof(p1),true) +"-"+hex(p2,sizeof(p2),true)+"-"+hex(p3,sizeof(p3),true)+"-" + hex(p4,sizeof(p4))+"-"+ hex(p5,sizeof(p5));
}

static inline std::string
to_mbr_part_uuid(uint8_t *d,size_t size)
{
    uint8_t p1[4] = {0};
    uint8_t p2[8] = {0};

    memcpy(p1,d,sizeof(p1));
    memcpy(p2,d+sizeof(p1),sizeof(p2));

    return hex(p1,sizeof(p1),true)+"-"+hex(p2,sizeof(p2));
}

static inline std::string
to_gpt_part_uuid(const std::string& d)
{
    std::vector<std::string>v = split(d);
    std::string ret;
    ret = v.at(3) + v.at(2) + v.at(1) + v.at(0);
    ret+= "-"+v.at(5) + v.at(4);
    ret+= "-"+v.at(7) + v.at(6);
    ret+= "-"+v.at(8) + v.at(9);
    ret+= "-";
    for(size_t i=10;i<v.size();i++){
        ret+=v.at(i);
    }
    return ret;
}

inline static std::string
to_mbr_part_uuid(const std::string& d)
{
    std::vector<std::string>v = split(d);
    std::string ret;
    ret = v.at(3) + v.at(2) + v.at(1) + v.at(0);
    ret+= "-";
    for(size_t i=4;i<v.size();i++){
        ret+=v.at(i);
    }
    return ret;
}

static const char *env_name = "_NT_REGF";

class NTRegfImpl{

protected:
    std::string rgf;
    std::map<std::string,std::string> mounted_devs;
    std::string win_ver;
    std::string dev_path;


public:
    NTRegfImpl():rgf(""),mounted_devs(),win_ver(),dev_path(){
        
    }

    ~NTRegfImpl(){
    }

    friend class NTRegf;
};

NTRegf::NTRegf(const std::string&dev,const std::string&rgf):impl(new NTRegfImpl())
{
    impl->rgf = rgf;
    impl->dev_path = dev;
}

NTRegf::~NTRegf()
{
}

const std::map<std::string,std::string>& NTRegf::mounted_devices() const
{
    return impl->mounted_devs;
}

const std::string& NTRegf::nt_version() const
{
    return impl->win_ver;
}

const std::string &NTRegf::dev_path() const
{
    return impl->dev_path;
}

bool NTRegf::read_mounted_devices() const
{
    impl->mounted_devs.clear();

    setenv(env_name,impl->rgf.data(),1);
    std::vector<std::string> lines;
    command_read("wd_regf",lines);
    unsetenv(env_name);
    for(size_t i=0;i<lines.size();i++){
        const std::string &l = lines.at(i);
        size_t ei = l.find_first_of('=');
        if(ei == std::string::npos)continue;
        if(startswith(l,"os=")){
            impl->win_ver = l.substr(ei+1);
        }else if(startswith(l,"gpt:")){
            impl->mounted_devs[to_gpt_part_uuid(l.substr(4,ei-4))] = l.substr(ei+1);
        }else if(startswith(l,"mbr:")){
            impl->mounted_devs[to_mbr_part_uuid(l.substr(4,ei-4))] = l.substr(ei+1);
        }
    }
    //std::cerr<<"mounted_devs:"<<impl->mounted_devs.size()<<","<<to_string(impl->mounted_devs)<<std::endl;
    return impl->mounted_devs.size()>0;
}
