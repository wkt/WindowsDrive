#ifndef NTMOUNTEDDEVICES_H
#define NTMOUNTEDDEVICES_H

#include <map>
#include <string>
#include <vector>

#include "ntregf.h"

class NTMountedDevicesIMPL;

class NTMountedDevices
{
private:
    NTMountedDevicesIMPL *impl;

public:
    NTMountedDevices();
    ~NTMountedDevices();

    //get all partitions
    virtual bool scan_paritions();

    //mount all partitions
    virtual bool mount_paritions();

    //scan regf on this computer
    virtual bool load_windows();
    virtual const std::vector<NTRegf>& get_windows();

#ifdef __APPLE__
    // call it before scan_paritions
    virtual void read_dos_disk_id();
#endif

    virtual void set_ntregf(int index);
    virtual void set_ntregf(const std::string& devpath);
    virtual int ntregf_index();
    virtual std::vector< std::map<std::string,std::string> > windows_drives();
};

#endif // NTMOUNTEDDEVICES_H
