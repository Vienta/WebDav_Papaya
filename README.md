# Papaya(木瓜)

一个操作WebDav的库。

## About Papaya

Papaya是我在实际项目中为了解决WebDav服务器的读写操作而封装的一个库，它是基于[neon库](http://www.webdav.org/neon/)开发而来。库非常的轻量，使用起来也非常简单。

## Usage

Papaya的使用非常简单：

### 实例化
  Papaya *papaya = new Papaya(webdavHost, port, user ,pwd); //实例化对象，这里分别是服务器的ip，端口号，用户名和密码。

### 具体方法
  提供了文件操作方法，主要如下：

  papaya->ls();   //获取文件or文件夹列表
  papaya->put();  //上传文件
  papaya->get();  //获取文件
  papaya->read(); //读取文件
  papaya->rm();   //删除文件
  papaya->mkdir();//新建文件夹
  papaya->mv();   //移动文件
  
具体细节可以参见源码

## Supported platforms
理论上支持iOS和Android设备
