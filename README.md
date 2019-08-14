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