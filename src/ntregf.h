#ifndef NTREGF_H
#define NTREGF_H

#include<string>
#include<map>

class NTRegfImpl;

class NTRegf
{
private:
    class NTRegfImpl *impl;
public:
    NTRegf(const std::string &dev_path,const std::string &rgf);
    ~NTRegf();

    bool read_mounted_devices() const;
    const std::map<std::string,std::string>& mounted_devices() const;
    const std::string& nt_version() const;
    const std::string& dev_path() const;
};

#endif // NTREGF_H
