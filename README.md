### 1 MyKV简介

MyKV是一个针对DRAM-PM异构存储的持久性键值存储系统。MyKV利用PM的可字节寻址性、接近DRAM的读写延迟以及数据持久性，通过在PM中建立高效的日志机制，保证键值存储系统的数据一致性，通过在DRAM中缓存热点数据，提升键值存储系统的读写性能。

MyKV对外提供服务如下表所示，提供查询键值对、插入/更新键值对和删除键值对服务。

| 名称 | 输入      | 输出  | 注释                                                         |
| :--: | --------- | ----- | ------------------------------------------------------------ |
| get  | key       | value | 查找并返回与key关联的value                                   |
| set  | key,value | /     | 将value与key关联并插入到键值存储中。如果key已存在，则会覆盖旧的value |
| del  | key       | /     | 从键值存储中移除key对应的键值对                              |

### 2 使用MyKV

#### 2.1 准备PM

1. Create namespace:

   ```shell
   ndctl create-namespace
   ```
	这时候，你会在`/dev` 目录下看到`/dev/pmem<id>`
	


2. Format the pmem device:

   ```shell
   mkfs.ext4 /dev/pmem<id>
   ```
   
3. Mount as dax:

   ```shell
   mkdir /mnt/pmem<id>
   mount -o dax /dev/pmem<id> /mnt/pmem
   ```

#### 2.2 编译MyKV

1. 下载源码

   ```shell
   git clone https://github.com/mryvae/MyKV.git
   ```

2. 编译

   ```shell
   cd MyKV
   make
   ```

#### 2.3 运行mykv-server

1. 修改mykv.conf

   必须将`pmemdir`设置为PM的挂载点`/mnt/pmem`

2. 运行mykv-server

   ```shell
   ./mykv-server mykv.conf
   ```

#### 2.4 使用mykv-cli

可以使用mykv-cli连接MyKV Server，进行查询键值对、插入/更新键值对和删除键值对操作。

示例如下：

```
./mykv-cli -h 127.0.0.1 -p 6379 set key value
./mykv-cli -h 127.0.0.1 -p 6379 get key
./mykv-cli -h 127.0.0.1 -p 6379 del key
```

### 3 MyKV设计与实现

