# Kafka Plugin 说明

### 一、Topic

默认开启以下4个topic

    1. blocks   // 其中block字段是由完整区块数据持久化的json结构，是一份全量数据。
    2. transaction
    3. transaction_trace
    4. action

    transaction、transaction_trace、action为nodeos中数据解析所得，提取了主要的可能使用的字段（相当于推荐配置），业务使用者可根据需要适当增减字段。 另，也可以删除这三个topic，仅依赖blocks中的全量数据。

    详见：`plugins/kafka_plugin/types.hpp `


### 二、常见问题

####  bos在Mac上编译常见报错
```
Could not find a package configuration file provided by "RdKafka" with any of the following names:
    RdKafkaConfig.cmake
    rdkafka-config.cmake
```

原因：系统安装的kafka版本太低

解决方法：

    删除`/usr/local/include/cppkafka` , `/usr/local/include/librdkafka`两个目录
    重新开始bos编译（会自动下载安装适配的kafka版本）