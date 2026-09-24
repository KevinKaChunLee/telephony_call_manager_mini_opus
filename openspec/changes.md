一、建议让AI结合业务规格先分析一版涉及哪些接口
蓝区引用不到接口先要识别出来，并决策代替方案，必要接口要在蓝区实现一套跟黄区签名一致的桩代码，确保编译通过

二、spec需要补充:
1、外部依赖
目前已知有如下（待步骤一完成后补充）：
1)、mini 不支持 ffrt：异步任务用 radio/base/schedule 下的 EventHandler 替代；不要引入 ffrt 头文件或 API
2)、mini 是单进程多线程环境，目前只有一个线程：不要使用锁，尽量不新建线程；只有自己新建了线程时才需要加锁
3)、mini 不支持 IPC 接口：跨模块调用用 telephony_mini/common/base 封装的 SA/Feature 接口替代；不要使用 HMOS 的 IPC stub/proxy 写法。
4)、调用CoreService、CellularCall模块的接口时，使用CellularCallClient::GetInstance().Xxx()的形式，接口先打桩实现
5)、日志打印接口

2、业务规格
除之前大特性排除和特性宏之外，再补充：
1)、不支持 DSDA（双卡双待双活）：mini平台 DSDS 模式恒为 0，仅 0 值（单卡单待）流程可达，语义上等价于 HMOS CallManagerUtils::GetDsdsMode() 恒返回 0
2)、mini只支持单卡，可预留双卡扩展能力

3、架构要求
1)、目录结构尽量保持一致
2)、模块架构，如AI有识别不合理地方可以讨论调整
3)、外部模块的桩代码统一到一个目录下，与业务代码区分开

4、编码规范 -- 优先级低
可先参考HMOS的代码，进黄区后再调整（主要文件头注释）

5、C++语法限制 -- 优先级低
mini对高版本C++的支持不完整：写代码时优先用 C++14 兼容写法
目前发现<regex>在mini上用不了
