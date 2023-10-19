#include <cstring>
#include <iostream>
#include <functional>
#include <algorithm>
#include "ntmounteddevices.h"

#include "utils.h"

#include "mac_utils.h"


template<typename K,typename V>
inline static void map_update(std::map<K,V>& d,const std::map<K,V>& s)
{
    typename std::map<K,V>::const_iterator iter;
    for(iter  = s.begin();iter != s.end();iter ++)
    {
        d[iter->first] = iter->second;
    }
}

typedef  std::map<std::string,std::string> PartInfo;

/**
 * key = device path
 * value = mount point
 * @brief MountPoints
 */
typedef  std::map<std::string,std::string> MountPoints;

/*
inline static std::string map_get(const std::map<std::string,std::string> &p, const std::string &key)
{
    if(p.count(key)>0)return p.at(key);
    return std::string();
}
*/

inline static std::string part_info_get(const std::map<std::string,std::string> &p, const std::string &key)
{
    return map_get(p,key);
}

inline static std::string mount_point_get(const std::map<std::string,std::string> &p, const std::string &key)
{
    return map_get(p,key);
}


inline static std::string drive_letter(const std::string &s){
    size_t idx = s.find_last_of("\\");
    if(idx == std::string::npos)return s;
    return s.substr(idx+1);
}

inline static bool is_windows_fs_type(const std::string& _fs_type){
    static const char *FS_TYPES[] = {
        "ntfs", "fat32", "fat", "fat16", "exfat", "vfat", "msdos",NULL
    };
    for(int i=0;FS_TYPES[i] != NULL;i++){
        if(strcasecmp(FS_TYPES[i],_fs_type.data()) == 0){
            return true;
        }
    }
    return false;
}

inline static bool cmp_drive_letter(const std::map<std::string,std::string>& m1, const std::map<std::string,std::string> &m2)
{
    const std::string l1 = map_get(m1,std::string("windows_drive"));
    const std::string l2 = map_get(m2,std::string("windows_drive"));
    return strcasecmp(l1.data(),l2.data())<0;
}

inline static void part_info_linux(const std::string &dev,PartInfo &info)
{
    std::vector<std::string>lines;
    std::string cmd = "udevadm info -q property "+dev+"|sed 's|=|\\n|'";
    command_read(cmd.data(),lines);
    size_t n_lines = lines.size()/2;
    for(size_t i=0;i<n_lines;i++){
        info[lines[2*i]]=lines[2*i+1];
    }
}


inline static void dos_part_uuid(PartInfo &info)
{

    const std::string &pt = part_info_get(info,"ID_PART_ENTRY_SCHEME");
    if(pt != "dos"){
        return;
    }
    const std::string p1 = part_info_get(info,"ID_PART_TABLE_UUID");
    uint64_t p2 = strtoll(part_info_get(info,"ID_PART_ENTRY_OFFSET").data(),NULL,10);
#ifndef __APPLE__
    p2 = p2*512;
#endif
    std::string p2_s = hex((uint8_t*) & p2,sizeof(p2));
    info["DOS_PART_ENTRY_UUID"] = p1 + "-"+p2_s;

}

inline static void get_partitions_linux(std::vector<PartInfo> &parts)
{
    std::vector<std::string>lines;
    command_read("grep '^[ \t]*[0-9]' /proc/partitions|awk '{print $4}'",lines);
    std::string dev;
    std::string fs_type;
    for(size_t i=0;i<lines.size();i++){
        dev = "/dev/"+lines[i];
        PartInfo info;
        part_info_linux(dev,info);
        if(info.count("ID_FS_TYPE") ==0)continue;
        fs_type = info["ID_FS_TYPE"];
        if(!is_windows_fs_type(fs_type)){
            continue;
        }
        const std::string &pn = part_info_get(info,"PARTNAME");
        const std::string &ig = part_info_get(info,"UDISKS_IGNORE");
        if(pn == "EFI System Partition" or ig == "1"){
            continue;
        }
        const std::string &dt = part_info_get(info,"DEVTYPE");
        if(dt == "partition"){
            dos_part_uuid(info);
            parts.push_back(info);
        }
    }
}

static std::map<std::string,std::string>* DISK_ID_CACHE = NULL;
inline static const std::string dos_disk_id_mac(const std::string &dev_path)
{
    std::string ddid;

    if(DISK_ID_CACHE == NULL){
        DISK_ID_CACHE = new std::map<std::string,std::string>;
    }

    ddid = map_get(*DISK_ID_CACHE,dev_path);
    if(ddid.length()>1){
        return ddid;
    }

    const std::vector<std::string>& exec_dirs = exec_search_paths();
    std::string info;
    for(size_t i=0;i<exec_dirs.size();i++){
        std::string exec_path =  exec_dirs[i] + "/" + WD_HELPER;
        if(is_setuid_enabled(exec_path)){
            command_read(exec_path+" disk_id "+dev_path,info);
        }
    }

    if(info.length()>0){
        size_t pos = info.find(":");
        if(pos != std::string::npos){
            ddid = info.substr(pos+1);
            ddid = strip(ddid);
            (*DISK_ID_CACHE)[dev_path] = ddid;
        }
    }
    return ddid;
}

inline static void part_info_mac(const std::string &dev,PartInfo &info,const bool& verbose=false)
{
    std::vector<std::string>lines;
    std::string cmd = "diskutil info "+dev+"|sed '/^$/d;s|[[:space:]]||g'|tr ':' '\\n'";
    command_read(cmd,lines);
    
    if(verbose)std::cerr<<__FUNCTION__<<"lines: "<<to_string(lines)<<std::endl;

    size_t n_lines = lines.size()/2;
    for(size_t i=0;i<n_lines;i++){
        info[lines[2*i]]=lines[2*i+1];
    }

    if(verbose)std::cerr<<__FUNCTION__<<", info: "<<to_string(info)<<std::endl;

    info["DEVNAME"]=dev;
    info["ID_FS_TYPE"] = part_info_get(info,"Type(Bundle)");
    info["ID_PART_ENTRY_UUID"] = to_lower(part_info_get(info,"Disk/PartitionUUID"));
    info["ID_FS_LABEL"] = part_info_get(info,"VolumeName");
    if(info.count("PartitionType")>0){
        info["DEVTYPE"]="partition";
    }
}

inline static const std::map<std::string,PartInfo> ioreg_info()
{
    std::string xml;
    command_read("ioreg -c IOMedia -r -a",xml);
    const std::vector< std::map<std::string,std::string> >info = disk_info_from_ioreg_xml(xml);

    std::map<std::string,PartInfo> ret;

    for(size_t i=0;i<info.size();i++){
        const std::map<std::string,std::string>& m = info.at(i);
        const std::string &dk = m.at("DISK");
        const std::string &pn = m.at("PARTITION");
        const std::string &pt = m.at("ID_PART_LABEL_TYPE");
        PartInfo ptf;
        ptf["ID_PART_ENTRY_SCHEME"] = pt;
        if(pt == "dos"){
            ptf["ID_PART_ENTRY_OFFSET"] = m.at("ID_PART_ENTRY_OFFSET");
            ptf["ID_PART_TABLE_UUID"] = dos_disk_id_mac(dk);
        }
        ret[pn] = ptf;
        //std::cerr<<__PRETTY_FUNCTION__<<",ptf: "<<to_string(ptf)<<std::endl;
    }
    return ret;
}

inline static void get_partitions_mac(std::vector<PartInfo> &parts,const std::map<std::string,PartInfo>& disk_id_info)
{
    std::vector<std::string>lines;
    command_read("diskutil list|grep 'disk[0-9]*s[0-9]*'|sed 's|.*[[:space:]]disk|disk|g'",lines);
    std::string dev;
    std::string fs_type;
    for(size_t i=0;i<lines.size();i++){
        dev = "/dev/"+lines[i];
        PartInfo info;
        part_info_mac(dev,info);
        if(info.count("ID_FS_TYPE") ==0)continue;
        fs_type = info["ID_FS_TYPE"];
        if(!is_windows_fs_type(fs_type)){
            continue;
        }
        const std::string &pn = part_info_get(info,"PARTNAME");
        const std::string &ig = part_info_get(info,"UDISKS_IGNORE");
        const std::string &pt = part_info_get(info,"PartitionType");
        if(pn == "EFI System Partition" || ig == "1" || pt == "EFI"){
            continue;
        }
        const std::string &dt = part_info_get(info,"DEVTYPE");
        if(dt == "partition"){
            map_update(info,map_get(disk_id_info,dev));
            dos_part_uuid(info);
            parts.push_back(info);
            //std::cerr<<__FUNCTION__<<",info: "<<to_string(info)<<std::endl;
        }
    }
}


inline static void get_mount_points(MountPoints &mpts)
{
    std::vector<std::string>lines;
#if defined (__APPLE__) && __APPLE__
    command_read("df|grep /disk |sed 's|[[:space:]].*%[[:space:]]*|$|g'|tr '$' '\n'",lines);
#else
    command_read("df|grep ^/dev|sed 's|[[:space:]].*%[[:space:]]*|$|g'|tr '$' '\n'",lines);
#endif

    size_t n_lines = lines.size()/2;
    for(size_t i=0;i<n_lines;i++){
        std::string &k = lines[2*i];
        const std::string &v = lines[2*i+1];
        if(k.find("ntfs://")!=std::string::npos){
            k = path_dirname("/dev/"+k.substr(7));
        }
        //std::cerr<<__FUNCTION__<<",k=`"<<k<<"`,v=`"<<v<<"`"<<std::endl;
        mpts[k] = v;
    }
}

inline static void mount_part(const std::string &dev_path)
{
    std::string out;
    command_read("df | grep '"+dev_path+"[ \t][ \t]*' |sed 's|.*[ \t]/|/|g'",out);
    if(out.length()>1){
        //mounted
        return;
    }
#if defined (__APPLE__) && __APPLE__
    command_read("diskutil mount '"+dev_path+"'",out);
#else
    const std::string& ukc="udisksctl";
    if(has_executable(ukc)){
        command_read(ukc + " mount -b '"+dev_path+"'",out);
    }else{
        const std::string& cmd = WD_HELPER  " mount '"+dev_path+"'";
        command_read(cmd,out);
    }
#endif

}


class NTMountedDevicesIMPL
{
protected:
    std::vector<PartInfo>parts;
    std::vector<NTRegf>regfs;
    int nt_idx;
    std::map<std::string,PartInfo> disk_id_info;

public:
    NTMountedDevicesIMPL():parts(),regfs(),nt_idx(-1),disk_id_info(){

    }

    inline const NTRegf* current_nt()
    {
        if(nt_idx<0 || nt_idx >= static_cast<int>(regfs.size()))return NULL;
        return &regfs[nt_idx];
    }

    friend class NTMountedDevices;
};

NTMountedDevices::NTMountedDevices():impl(new NTMountedDevicesIMPL())
{

}

NTMountedDevices::~NTMountedDevices()
{
    delete impl;
}


bool NTMountedDevices::scan_paritions()
{
    impl->parts.clear();
    impl->regfs.clear();

#ifdef __APPLE__
    read_dos_disk_id();
    get_partitions_mac(impl->parts,impl->disk_id_info);
#else
    get_partitions_linux(impl->parts);
#endif
    return false;
}

//mount all partitions
bool NTMountedDevices::mount_paritions()
{
    for(size_t i=0;i<impl->parts.size();i++){
        const PartInfo &p = impl->parts[i];
        mount_part(p.at("DEVNAME"));
    }
    return false;
}

bool NTMountedDevices::load_windows()
{
    impl->regfs.clear();
    MountPoints mpts;
    get_mount_points(mpts);
    for(size_t i=0;i<impl->parts.size();i++){
        PartInfo &p = impl->parts[i];
        const std::string &devp = p.at("DEVNAME");
        const std::string &mp = mount_point_get(mpts,devp);
        p["MOUNT_POINT"] = mp;
        const std::string &rgf = find_path_by_names(mp,"Windows", "System32", "config", "SYSTEM",NULL);
        if(rgf.length()>0){
            const NTRegf &ntr = NTRegf(devp,rgf);
            if(ntr.read_mounted_devices()){
                impl->regfs.push_back(ntr);
                p["REGISTRY_SYSTEM"] = rgf;
            }
            //std::cerr<<__PRETTY_FUNCTION__<<": "<<devp<<" => "<<mp<<std::endl;
            //std::cerr<<__PRETTY_FUNCTION__<<": "<<to_string(ntr.mounted_devices())<<std::endl;
        }
    }
    return impl->regfs.size()>0;
}

//scan regf on this computer
const std::vector<NTRegf>& NTMountedDevices::get_windows()
{
    return impl->regfs;
}


void NTMountedDevices::set_ntregf(int index)
{
    if(index < 0 || impl->regfs.size() == 0){
        impl->nt_idx = -1;
        return;
    }
    if(index >= static_cast<int>(impl->regfs.size())){
        index = 0;
    }
    impl->nt_idx = index;
}

void NTMountedDevices::set_ntregf(const std::string& devpath)
{
    int index = -1;
    for(size_t i=0;i< impl->regfs.size();i++){
        if(index == -1)index = 0;
        const std::string& dvp = impl->regfs.at(i).dev_path();
        //std::cerr<<"dvp:"<<dvp<<", devpath:"<<devpath<<std::endl;
        if(dvp == devpath){
            index = i;
            break;
        }
    }
    set_ntregf(index);
    //std::cerr<<__FUNCTION__<<": "<<devpath<<",ntregf_index:"<<ntregf_index()<<std::endl;

}

std::vector< std::map<std::string,std::string> > NTMountedDevices::windows_drives()
{
    std::vector< std::map<std::string,std::string> > ret;
    const NTRegf *nt = impl->current_nt();
    if(nt == NULL)return ret;
    const std::map<std::string,std::string>& devices = nt->mounted_devices();
    //std::cerr<<__FUNCTION__<<",devices:"<<to_string(devices)<<std::endl;
    for(size_t i=0;i<impl->parts.size();i++){
        const PartInfo &p = impl->parts[i];
        const std::string &pt = part_info_get(p,"ID_PART_ENTRY_SCHEME");
        const std::string &mp = part_info_get(p,"MOUNT_POINT");
        if(mp.length() == 0 || mp.at(0) != '/'){
            continue;
        }
        std::string u = "__";
        if(pt == "gpt"){
            u = part_info_get(p,"ID_PART_ENTRY_UUID");
        }else if(pt == "dos"){
            u = part_info_get(p,"DOS_PART_ENTRY_UUID");
        }

        const std::string &dn = map_get(devices,u);
        //std::cerr<<"mp: "<<mp<<",u: "<<u<<",dn: "<<dn<<std::endl;
        if(dn.length()>0){
            std::map<std::string,std::string>drive;
            drive["windows_drive"] = drive_letter(dn);
            drive["partition"] = part_info_get(p,"DEVNAME");
            drive["fs_label"] = part_info_get(p,"ID_FS_LABEL");
            drive["mount_point"] = mp;
            ret.push_back(drive);
        }
    }
    std::sort(ret.begin(),ret.end(),cmp_drive_letter);
    return ret;
}

int NTMountedDevices::ntregf_index()
{
    return impl->nt_idx;
}

#ifdef __APPLE__
void NTMountedDevices::read_dos_disk_id()
{
    impl->disk_id_info = ioreg_info();
}
#endif
