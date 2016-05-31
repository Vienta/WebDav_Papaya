//
//  Papaya.hpp
//  neonios
//
//  Created by Vienta on 15/10/10.
//  Copyright © 2015年 www.vienta.me. All rights reserved.
//

// **这个是webdav的封装库 papaya是名称** //

#ifndef Papaya_H
#define Papaya_H

#include <stdio.h>
#include <string>
#include <vector>
#include "ne_props.h"
#include <functional>
#include <map>
#include <iostream>



#define PAPAYA_SUCCESS "200"

struct PapayaPath {
    
    PapayaPath(std::string host, std::string path, std::string resourceType, std::string lastModified, std::string contentType): host(host), path(path), resourceType(resourceType),lastModified(lastModified), contentType(contentType) {
    
    }
    
    bool isFile() {
        if (!resourceType.compare("")) {
            return true;
        } else {
            return false;
        }
    }
    
    bool isDirectory() {
        return !isFile();
    }
    
    std::string host;
    std::string path;
    std::string resourceType;
    std::string lastModified;
    std::string contentType;
};



class Papaya {
public:
    Papaya(const std::string url, const unsigned port, const std::string user, const std::string password);
    ~Papaya();
    
private:
    static int setLogin(void *userdata, const char *realm, int attempts, char *username, char *password);
    static void getProps(void *userdata, const ne_uri *uri, const ne_prop_result_set *set);
    ne_session *mSession;
    std::string mError;
    
public:
    std::string getLastError();
    
    void cancelSession();
    
    /*带有callback的获取当前路径同层级的所有其他目录*/
    void ls(std::string uri, const std::function<void (std::vector<PapayaPath> files, std::string result)>& callback);
    
    /*带有callback的拉取路径下所有的目录 方法暂时在车听宝上不可用*/
    void tree(std::string uri,const std::function<void (std::vector<PapayaPath> allFiles, std::string result)>& callback);
    
    /*判断某路径下文件是否存在*/
    int exist(std::string uri);
    
    /*下载webdav server上某路径的文件到本地*/
    bool get(std::string uri,const std::string localDestination, const std::function<void (std::string result)>& callback);
    
    /*上传本地路径文件到webdav server上, 使用http的 PUT协议*/ /*http://www.oschina.net/translate/put-or-post PUT和POST区别*/
    bool put(std::string uri, std::string localSource, const std::function<void (std::string result, int code)>& callback);
    
    /*在webdav server上创建文件夹*/
    int mkdir(std::string uri, const std::function<void (std::string result)>& callback);
    
    /*删除webdav server上的文件或者文件夹*/
    bool rm(std::string uri, const std::function<void (std::string result)>& callback);
    
    //src' to 'dest'
    bool mv(std::string srcUri, std::string destUri, const std::function<void (std::string result)>& callback);
    
    /*读取 remote data */
    bool read(std::string destUri, const std::function<void (std::string result, std::string response)>& callback);
    
};



#endif /* Papaya_H */
