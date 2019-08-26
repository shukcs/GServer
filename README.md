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
  cmake _DCMAKE_BUILD_TYPE=Release ../GServer
  make 
  cp ../GServer/gsock/*.xml .
  sudo bin/gsocket
  
  代码说明
  1.gshare 业务逻辑的底层抽象
    GSocketManager: tcp数据处理, 作为服务器不需要再重载了
    GSocket: 通用tcp连接类
    IObjectManager: a.连接对象管理抽象类，不同种类的连接需要重载GetObjectType(),ProcessReceive(); PrcsRemainMsg根据需要重载。
        b.ProcessMassage()处理初始数据，判断连接是否属于自己,参考GSManager/UavManager的重载
        c.GetObjectType()不同的抽象类返回值不能一样
    IObjectManager: 连接对象抽象类，参考ObjectGS/ObjectUav重载，GetObjectType()与连接对象管理类的GetObjectType()返回值必须一样
    Thread: 通用线程抽象类.
    Utility: 通用函数.
    GLibrary: 动态库加载类.
  2.vgmysql mysql数据库操作通用库，通过xml解析获取建表、数据库、触发器、操作对象...
