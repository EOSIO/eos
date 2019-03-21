# BOSCore - 更可用的链，为DApp而生。

## BOSCore Version: v2.0.2
### Basic EOSIO Version: v1.6.2

# 背景
EOS的出现给区块链带来了新的想象力，主网启动短短几个月以来，版本经历了几十次升级，不仅稳定性得到了很大提高，并且新功能也逐步实现，各个节点团队也积极参与建设EOSIO生态。让人更加兴奋的是，EOS已经吸引了越来越多的开发团队，当前已经有数百个DApp在EOS主网上面运行，其交易量和流通市值远超以太坊，可发展的空间愈来愈广阔。
在EOS主网逐渐发展的过程中，我们发现了一些偏离期望的地方。作为最有竞争力的第三代公链，大家希望看到的是能够有更多、更丰富的应用能够在EOS上面运行，开发者会将EOS作为自己应用开发的首选平台，但是由于目前EOS的资源模型的限制，导致了很高的使用成本，包括为用户创建更多的账户，以及部署运营DApp需要的较高成本。针对白皮书中要实现的上百万TPS需要的关键技术IBC，一直没有进行推进，主网多次出现CPU计算资源不足的情况，更是加剧了对跨链通讯需求的迫切性。此外，由于EOSIO采用的Pipeline-DPOS共识机制，一个交易需要近三分钟才能保证不可更改，虽然相较比特币、以太坊是有很大的进步，但是这也给EOS的应用场景带来很大限制，快速支付只能聚焦于小额转账，大额转账必须要等待足够长的时间才能保证不可更改，这就限制了链上、链下用户支付体验。
除了上面提到的情况，还有很多其他改进想法一直在我们社区进行活跃的讨论，由此，我们觉得应该基于EOS进行更多的尝试，让更多的开发者或者团队来参与到EOSIO生态的建设中来，一起为区块链在不同行业不同场景中的落地做出一份努力。BOS作为一条完全由社区维护的EOS侧链，在继承其良好功能的基础上，会进行更多的尝试，并且会将经过验证的新特性、新功能反哺给EOSIO生态。

# 概述
BOS致力于为用户提供方便进入并易于使用的区块链服务，为DApp运营提供更友好的基础设施，为支持更丰富的应用场景努力，为DApp大繁荣进行积极尝试。除了技术改进以外，BOS也会进行其他方面的尝试。比如，为了提高用户投票参与度，可以通过预言机技术来针对符合明确规则的账户进行激励；BOS上面的BP的奖励会根据链上DApp的数量、TPS、市值、流通量等指标进行调整，鼓励每个BP为生态提供更多资源；一项社区公投达成的决议将会尽量被代码化，减少人为的因素在里面，流程上链，保持公正透明。
BOS链的代码完全由社区贡献并维护，每个生态参与者都可以提交代码或者建议，相关的流程会参考已有开源软件来进行，比如PEP(Python Enhancement Proposals)。
为鼓励DApp在BOS的发展，BOS基金会将会为其上的DApp提供Token置换的低成本的资源抵押服务，降低DApp前期的运营成本；此外还会定期对做出贡献的开发者提供BOS激励，以便建立起一个相互促进的社区发展趋势。

# 开发者激励 
每年增发 0.8% 面向BOS生态贡献代码的开发者，由社区提出50名奖励名单，由前50名BP投票选出40名的获奖者获取对应奖励：前10名获取40%，11到20名获取30%，最后20名均分30%，奖励周期3个月一次，每次奖励名额都会进行为期一周的公示，如果有合理异议，将会重新评审，每次奖励名单都会上链记录。  
随着BOS的不断发展，开发者奖励会适当调整，让社区为BOS的进化提供更多动力。 


## 资源
1. [官网](https://boscore.io)
2. [Developer Telegram Group](https://t.me/BOSDevelopers)
3. [Community Telegram Group](https://t.me/boscorecommunity)
4. [WhitePaper](https://github.com/boscore/Documentation/blob/master/BOSCoreTechnicalWhitePaper.md)
5. [白皮书](https://github.com/boscore/Documentation/blob/master/zh-CN/BOSCoreTechnicalWhitePaper.md)

## 开始
1. 源码直接编译: `bash ./eosio_build.sh -s BOS`
2. Docker方式部署，参看 [Docker](./Docker/README.md)
3. Mac OS X Brew 安装 和 卸载 
#### Mac OS X Brew 安装
```sh
$ brew tap boscore/bos
$ brew install bos
```
#### Mac OS X Brew 卸载
```sh
$ brew remove bos
```

## BOSCore 开发流程 
BOSCore 鼓励社区开发者参与代码贡献，社区成员应当遵循以下工作流：
![BOSCore Workflow](./images/bos-workflow.png)

注意:
1. 只有待发布的 Feature Branch 或者Bug修复才应该向 Develop Branch 提交
2. 向 Develop Branch 提交 PR 之前需要现在本地执行 rebase 操作
3. EOSIO 主网版本作为一个 Feature Branch 来对待
4. 紧急问题修复采用 hotfixes 模式 

BOSCore是基于EOSIO技术的扩展，所以EOSIO的相关资料也可以参考： 

[EOSIO 开始](https://developers.eos.io/eosio-nodeos/docs/overview-1)  

[EOSIO 开发者门户](https://developers.eos.io).

