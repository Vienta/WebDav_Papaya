//
//  Papaya.cpp
//  neonios
//
//  Created by Vienta on 15/10/10.
//  Copyright © 2015年 www.vienta.me. All rights reserved.
//

#include "Papaya.h"

#include <iostream>
#include <fstream>

#include <unistd.h> /* close */
#include <fcntl.h> /* open */
#include <cstring> /* strncpy */

#include "ne_props.h" /* ne_simple_propfind, ne_prop_result_set */
#include "ne_uri.h" /* uri */
#include "ne_auth.h" /* ne_set_server_auth, ne_ssl_trust_default_ca */
#include "ne_basic.h" /* ne_session, ne_put, ne_get, ne_delete, ne_mkcol */
#include "ne_request.h"
#include "ne_utils.h"
//#include "CTBUtil.hpp"
#include <time.h>
#include "log.h"

using namespace std;


unsigned char FromHex(unsigned char x)
{
    unsigned char y;
    if (x >= 'A' && x <= 'Z') {
        y = x - 'A' + 10;
    } else if (x >= 'a' && x <= 'z') {
        y = x - 'a' + 10;
    } else if (x >= '0' && x <= '9') {
        y = x - '0';
    } else {
        y = '0';
    }
    return y;
}

string urlUTF8Decode(const string& str)
{
    std::string strTemp = "";
    size_t length = str.length();
    for (size_t i = 0; i < length; i++)
    {
        if (str[i] == '+') strTemp += ' ';
        else if (str[i] == '%')
        {
            //            assert(i + 2 < length);
            unsigned char high = FromHex((unsigned char)str[++i]);
            unsigned char low = FromHex((unsigned char)str[++i]);
            strTemp += high*16 + low;
        }
        else strTemp += str[i];
    }
    return strTemp;
}

bool isFileExist(string filePath){
    ifstream fin(filePath);
    if (fin)
    {
        return true;
    }else{
        return false;
    }
}



static const ne_propname fetchProps[] = {
    {"DAV:", "resourcetype"},
    {"DAV:", "getlastmodified"},
    {"DAV:", "getcontenttype"},
    {NULL}
};

int Papaya::setLogin(void *userdata, const char *realm, int attempts, char *username, char *password)
{
    std::vector<std::string> *login = (std::vector<std::string> *)userdata;
    strncpy(username, login->at(0).c_str(), NE_ABUFSIZ);
    strncpy(password, login->at(1).c_str(), NE_ABUFSIZ);
    return attempts;
}

void Papaya::getProps(void *userdata, const ne_uri *uri, const ne_prop_result_set *set)
{
    std::vector<PapayaPath> *paths = (std::vector<PapayaPath> *)userdata;
    
    ne_propname props[] = {
        { "DAV:", "resourcetype" },
        { "DAV:", "getlastmodified" },
        { "DAV:", "getcontenttype"}
    };
    
    std::string resourceType = ne_propset_value(set, &props[0]) ? ne_propset_value(set, &props[0]) : std::string();
    std::string lastModified = ne_propset_value(set, &props[1]) ? ne_propset_value(set, &props[1]) : std::string();
    std::string contentType = ne_propset_value(set, &props[2]) ? ne_propset_value(set, &props[2]) : std::string();
    
    PapayaPath path(uri->host,
                    uri->path,
                    resourceType,
                    lastModified,
                    contentType);
    paths->push_back(path);
}

Papaya::Papaya(const std::string url, const unsigned port, const std::string user, const std::string pwd) {
    ne_sock_init();
    mSession = ne_session_create("http", url.c_str(), 80);
    std::vector<std::string> *login = new std::vector<std::string>();
    login->push_back(user);
    login->push_back(pwd);
    ne_set_server_auth(mSession, Papaya::setLogin, login);
    
    
    std::string errorMessage = ne_get_error(mSession);
    
}

Papaya::~Papaya()
{
    std::cout << "Papaya 解析函数" << std::endl;
    ne_forget_auth(mSession);
    ne_session_destroy(mSession);
    ne_sock_exit();
}

std::string Papaya::getLastError()
{
    return mError;
}

void Papaya::cancelSession()
{
    ne_close_connection(mSession);
}

void Papaya::ls(std::string uri, const std::function<void (std::vector<PapayaPath>, std::string)> &callback)
{
    const int depth = NE_DEPTH_ONE;
    std::vector<PapayaPath> *progs = new std::vector<PapayaPath>;
    
    int res = ne_simple_propfind(mSession, uri.c_str(), depth, fetchProps, Papaya::getProps, progs);
    
    
    if (res != NE_OK) {
        mError = ne_get_error(mSession);
        FILE_LOG(logINFO) << "[Syncing ls]...获取webdav中的文件列表:" << "失败" << " 原因: " << mError;
        std::cout << "[Syncing ls]...获取webdav中的文件列表:" << "失败" << " 原因: " << mError << std::endl;
        
        callback(*progs, mError);
    } else {
        progs->erase(progs->begin());
        if (progs->empty()) {
            FILE_LOG(logINFO) << "[Syncing ls]...获取webdav中的文件列表:" << "失败" << " 原因: empty";
            std::cout << "[Syncing ls]...获取webdav中的文件列表:" << "失败" << " 原因: empty" << std::endl;
            
            callback(*progs, "empty");
        } else {
            FILE_LOG(logINFO) << "[Syncing ls]...获取webdav中的文件列表:" << "成功";
            std::cout << "[Syncing ls]...获取webdav中的文件列表:" << "成功" << std::endl;
            
            callback(*progs, PAPAYA_SUCCESS);
        }
    }
}

void Papaya::tree(std::string uri, const std::function<void (std::vector<PapayaPath>, std::string)> &callback)
{
    std::vector<PapayaPath> *progs = new std::vector<PapayaPath>;
    const int depth = NE_DEPTH_INFINITE;
    
    int res = ne_simple_propfind(mSession, uri.c_str(), depth, fetchProps, Papaya::getProps, progs);

    if (res != NE_OK) {
        mError = ne_get_error(mSession);
        callback(*progs, mError);
    } else {
        callback(*progs, PAPAYA_SUCCESS);
    }
}

int Papaya::exist(std::string uri)
{
    const int depth = NE_DEPTH_ZERO;
    std::vector<PapayaPath> *progs = new std::vector<PapayaPath>;
    
    int res = ne_simple_propfind(mSession, uri.c_str(), depth, fetchProps, Papaya::getProps, progs);
    
    std::cout << "syncing 判定文件或文件夹是否存在:" <<" result:" << ((res == 0) ? "成功": "失败") << " 完成时间:" << time(NULL);
    FILE_LOG(logINFO) << "syncing 判定文件或文件夹是否存在:" <<" result:" << ((res == 0) ? "成功": "失败") ;
    
    if (res != NE_OK) {
        mError = ne_get_error(mSession);
        std::cout << " 原因: " << mError << std::endl;
        if (mError.find("404") != std::string::npos) {
            return 404;
        }
        return res;
    }
    std::cout << std::endl;
    progs->erase(progs->begin());
    return res;
}

bool Papaya::get(std::string uri, const std::string localDestination, const std::function<void (std::string)> &callback)
{
    int fd = open(localDestination.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRWXO | S_IRWXG | S_IRWXU);//一定要加上这些参数，否则在iOS上会产生无法读取的问题

    FILE_LOG(logINFO) << "[Syncing GET Begin]...开始获取文件:" << uri;
    std::cout <<  "[Syncing GET Begin]...开始获取文件:" << uri << std::endl;

    int res = ne_get(mSession, uri.c_str(), fd);
    

    close(fd);
    
    if (res != NE_OK) {
        mError = ne_get_error(mSession);
        
        std::cout << " 原因: " << mError << std::endl;
        FILE_LOG(logINFO) << "[Syncing GET End]..获取文件:" << urlUTF8Decode(uri.c_str()) << " 到:" << localDestination << " 结果:" << ((res == 0) ? "成功": "失败") << " 原因:" << mError;
        std::cout << "[Syncing GET End]..获取文件:" << urlUTF8Decode(uri.c_str()) << " 到:" << localDestination << " 结果:" << ((res == 0) ? "成功": "失败") << " 原因:" << mError << std::endl;
        
        if (mError == "404 Not Found") {
            remove(localDestination.c_str());
        }

        if (callback) {
            callback(mError);
        }
        return  false;
    }
    FILE_LOG(logINFO) << "[Syncing GET End]..获取文件:" << urlUTF8Decode(uri.c_str()) << " 到:" << localDestination << " 结果:" << ((res == 0) ? "成功": "失败");
    std::cout <<  "[Syncing GET End]..获取文件:" << urlUTF8Decode(uri.c_str()) << " 到:" << localDestination << " 结果:" << ((res == 0) ? "成功": "失败") << std::endl;
    
    if (callback) {
        callback(PAPAYA_SUCCESS);
    }
    
    return true;
}

bool Papaya::put(std::string uri, std::string localSource, const std::function<void (std::string, int code)> &callback)
{
    int fd = open(localSource.c_str(), O_RDONLY);
    
    FILE_LOG(logINFO) << "[Syncing PUT Begin]...开始上传文件:" << localSource;
    std::cout << "[Syncing PUT Begin]...开始上传文件:" << localSource << std::endl;
    if (isFileExist(localSource) == false) {
        FILE_LOG(logINFO) << "[Syncing PUT]...上传是发现本地文件不存在，安卓会crash，做防御";
        if (callback) {
            callback("404 file not exist", 1);
        }
        return false;
    }
    
    try {
        int res = ne_put(mSession, uri.c_str(), fd);
        close(fd);
        
        if (res != NE_OK) {
            mError = ne_get_error(mSession);
            
            FILE_LOG(logINFO) << "[Syncing PUT End]...上传文件：" << localSource << " 到:" << uri << " 失败" << " 原因:" << mError;
            std::cout <<  "[Syncing PUT End]...上传文件：" << localSource << " 到:" << uri << " 失败" << " 原因:" << mError << std::endl;
            
            if (callback) {
                callback(mError, res);
            }
            return false;
            
        }
        
        FILE_LOG(logINFO) << "[Syncing PUT End]...上传文件：" << localSource << " 到:" << uri << "成功";
        std::cout << "[Syncing PUT End]...上传文件：" << localSource << " 到:" << uri << "成功" << std::endl;
        if (callback) {
            callback(PAPAYA_SUCCESS, res);
        }
        
        return true;

    } catch (std::exception& e) {
        FILE_LOG(logINFO) << "PUT 异常:" << std::endl;
        
        if (callback) {
            callback("network error", 1);
        }

        return false;
    }
}

int Papaya::mkdir(std::string uri, const std::function<void (std::string)> &callback)
{
    int res = ne_mkcol(mSession, uri.c_str());
    
    std::cout << "syncing 新建文件夹" <<" result:" << ((res == 0) ? "成功": "失败") << " 完成时间:" << time(NULL);

    if (res != NE_OK) {
        mError = ne_get_error(mSession);
        
        std::cout << " 原因: " << mError << std::endl;
        
        if (callback) {
            callback(mError);
        }
        return res;
    }
    std::cout << std::endl;
    
    if (callback) {
        callback(PAPAYA_SUCCESS);
    }
    return res;
}

bool Papaya::mv(std::string srcUri, std::string destUri, const std::function<void (std::string)> &callback)
{
    int res = ne_move(mSession, 0, srcUri.c_str(), destUri.c_str());
    if (res != NE_OK) {
        mError = ne_get_error(mSession);
        if (callback) {
            callback(mError);
        }
        return false;
    }
    if (callback) {
        callback(PAPAYA_SUCCESS);
    }
    return true;
}

bool Papaya::rm(std::string uri, const std::function<void (std::string)> &callback)
{
    int res = ne_delete(mSession, uri.c_str());
    
    std::cout << "syncing 删除文件: " << uri << " result:" << ((res == 0) ? "成功":"失败");
    
    if (res != NE_OK) {
        mError = ne_get_error(mSession);
        
        FILE_LOG(logINFO) << "[Syncing Remove]...删除文件：" << uri << " 结果:" << ((res == 0) ? "成功":"失败") << " 原因:"<< mError;
        std::cout << "[Syncing Remove]...删除文件：" << uri << " 结果:" << ((res == 0) ? "成功":"失败") << " 原因:"<< mError << std::endl;
        
        if (callback) {
            callback(mError);
        }
        return false;
    }

    FILE_LOG(logINFO) << "[Syncing Remove]...删除文件：" << uri << " 结果:" << "成功";
    std::cout << "[Syncing Remove]...删除文件：" << uri << " 结果:" << "成功" << std::endl;
    
    if (callback) {
        callback(PAPAYA_SUCCESS);
    }
    return true;
}

int httpResponseReader(void *userdata, const char *buf, size_t len)
{
    string *str = (string *)userdata;
    str->append(buf, len);
    
    return 0;
}

bool Papaya::read(std::string destUri, const std::function<void (std::string, std::string response)> &callback)
{
    ne_set_useragent(mSession, "MyAgent/1.0");
    
    ne_request *req;
    string response;
    
    const char *dest = destUri.c_str();
    
    req = ne_request_create(mSession, "GET", dest);
    
    ne_add_response_body_reader(req, ne_accept_always, httpResponseReader, &response);
    
    int res = ne_request_dispatch(req);
    int status = ne_get_status(req)->code;
    
    ne_request_destroy(req);
    
    if (res != NE_OK) {
        mError = ne_get_error(mSession);
        if (callback) {
            callback(mError, "");
        }
        return false;
    }
    if (callback) {
        callback(PAPAYA_SUCCESS, response);
    }
    return true;
}





