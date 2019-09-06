环境
1.WINDOWS:
  a.VS2015
  b.MySQL Connector C, 安装目录设置环境变量MYSQL = C:\Program Files (x86)\MySQL\MySQL Connector C 6.1，%MYSQL%\lib加入path
  c.安装MySQL服务器及运行
  编译运行
2.linux Ubuntu
  sudo apt-get install mysql-server
  sudo apt-get install libmysqlclient-dev
  sudo apt-get install cmake
  GServer 目录下
  cd ..
  mkdir GServerBuild
  cd GServerBuild
  cmake -DCMAKE_BUILD_TYPE=Release ../GServer
  make 
  cp ../GServer/gsock/*.xml .
  sudo bin/gsocket
  
  代码说明: 优化socket生存周期处理，连接对象数据处理与socket线程隔离，线程个数可以自己修改，GSManager/UavManager，通过xml解析实现
  1.gshare 业务逻辑的底层抽象
    GSocketManager: tcp数据处理，做为客户端，构造对象nThread设为0
    GSocket: 通用tcp连接类, 作为服务器不需要再重载了
    IObjectManager: a.连接对象管理抽象类，不同种类的连接需要重载GetObjectType(),ProcessReceive(); PrcsRemainMsg根据需要重载。
        b.ProcessMassage()处理初始数据，判断连接是否属于自己,参考GSManager/UavManager的重载
        c.GetObjectType()不同的抽象类返回值不能一样
    IObjectManager: 连接对象抽象类，参考ObjectGS/ObjectUav重载，GetObjectType()与连接对象管理类的GetObjectType()返回值必须一样
    Thread: 通用线程抽象类.
    Utility: 通用函数.
    GLibrary: 动态库加载类.
	ObjectManagers: a.连接对象管理器集合，发送消息的中间件，接收socket初始数据；
	    b.宏DECLARE_MANAGER_ITEM(xxx)，需要定义在连接对象管理类cpp文件中，xxx为连接对象管理类的类名
  2.tinyxml 小型xml解析器(GPL)，根据需要自行下载
  3.vgmysql mysql数据库操作通用库，通过xml解析获取建表、数据库、触发器、操作对象...
  4.protobuf Google的c++ protobuf，根据需要自行下载，可以自行构建需要的protobuf
  5.uavandqgc 飞机及地面站及管理对象GSManager/UavManager；动态库，动态加载
  
  使用代码请帮忙点赞，您的支持是我前进的动力；E-mail：hsj8262@163.com
