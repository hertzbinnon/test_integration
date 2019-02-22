一、架构总览

				<overall architecture>

layer				client
-------------------------------------------> bind9 DNS poll
LBlayer				LVS-dr/haproxy node 
+++++++++++++++++++++++++++++++++++++++++++> direct route
ReverseProxylayer	ATS 
-------------------------------------------> nginx L7 LB
Weblayer			NGX web	front
-------------------------------------------> link pools
Servicelayer		Tornado 
-------------------------------------------> data balance,request balance
Datalayer		    mysql/mongodb cluster
						|
						V
					 ceph/hdfs cluster

2.Content Delivery Network (CDN)

			DNS <GSLB>
			LVS <SLB>
			ATS cluster<cache>
缓存算法[Squid]: 缓存算法决定命中率、源服务器压力、POP节点存储能力
分发能力:		 取决于IDC能力和IDC策略性分布
负载均衡[Nginx]: 负载均衡（智能调度）决定最佳路由、响应时间、可用性、服务质量。当用户请求内容时,该内容能够由以最快速度交付的Cache来向用户提供，这个挑选“最优”的过程就叫做负载均衡
基于DNS[BIND]:	 以CNAME实现[to cluster]，智取最优节点服务 缓存点分为客户端浏览器缓存、本地DNS服务器缓存。缓存内容有DNS地址缓存、客户请求内容缓存、动态内容缓存。
支持协议:		 静动态加速（图片加速、https带证书加速）、下载加速、流媒体加速、企业应用加速、手机应用加速。
			
3.Load Banlance(LB)

 <1>轮循算法，就是将来自网络的请求依次分配给集群中的节点进行处理。

 <2>最小连接数算法，就是为集群中的每台服务器设置一个记数器，记录每个服务器当前的连接数，负载均衡系统总是选择当前连接数最少的服务器分配任务。 这要比"轮循算法"好很多，因为在有些场合中，简单的轮循不能判断哪个节点的负载更低，也许新的工作又被分配给了一个已经很忙的服务器了。

	 LVS作为四层负载均衡软件常见于大规模网络应用场景中，其中常见的负载策略为wlc。该负载策略基于最少连接数进行调度（即依据backend server的连接数）把新近的连接调度到连接数最小的backend server。在正常情况下，该种策略可以有效的均匀分散（从大量连接处理的总体角度看）负载至各个backend server。但是如果其中backend server有状态切换的时候（主要为由Down状态切换至Up状态），会极大的引起系统的颠簸。即在此时刻，系统会把整个集群的新进请求全部调度到新Up的backend server。因此，这样的调度方式会引起系统颠簸，在极端的情况下，会引起整个集群的雪崩效应。因为如果单个server不能处理全部的新近请求时，会产生上述问题。因此，LVS在采用wlc调度策略时需要非常注意这个问题。由这个问题引起的思考，我们可以看出在做集群系统时，要考虑到backend server状态发生切换时的颠簸问题。其实该问题是由负载的切换策略引起，即我们需要平滑的调度负载。因此，更加合理的调度策略为，按照检测周期，分步、平滑的把整个集群的新进负载逐步的调度到新加入集群的server。因此，我们可以对LVS的wlc的调度策略进行改进，让新进的请求逐步的调度到新的server上。

<3>快速响应优先算法，是根据群集中的节点的状态（CPU、内存等主要处理部分）来分配任务。这一点很难做到，事实上到目前为止，采用这个算法的负载均衡系统还很少。尤其对于硬件负载均衡设备来说，只能在TCP/IP协议方面做工作，几乎不可能深入到服务器的处理系统中进行监测。但是它是未来发展的方向。


常用的分布式存储技术包括分布式文件系统和分布式数据库,

5.Distribute content Cache
    ats

6.Distribute data storage
    ceph


二、开源软件

<1> Apache Traffic Server优点

·高可用性

—多种管理界面，可管理程度高
—容灾设计，可消化常见硬件故障
—快速启动，快速恢复
—配置可定制程度高，可控制核心参数500+条目
—Cache控制能力强
—可定制化的日志采集汇总汇报系统
—可定制化的数据统计系统
—集群化管理能力

·高度可扩展性

—模块化程度高：核心http引擎只是TS的2个引擎之一（另一个是流媒体引擎）
—高度可编程核心插件设计，可以完成各式各样业务，如已有巨型插件：
·ESI，Edge Side Include，雅虎贡献的代码
·Metalink
·Lua remap插件，支持Lua语言的script
·Gzip插件，可以对html等文本文件进行深度压缩
—API扩展支持完善，插件开发介入门槛低

·完善的日志
—支持binary日志，高效
—支持squid日志格式，兼容
—可根据源站分离存储
—可将日志发送至日志服务器
—内建高性能、可定制化日志聚合功能，自动生成流量/性能报告
 
·自带高性能日志收集功能，也有独立服务器

结构与模型：

--事件驱动，异步处理，多线程，异步非阻塞AIO, 异步阻塞IO
--Event System: To schedule work to be done on threads.
++Continuation: Passive, event-driven state machine.
++Processors: Talk to the event system and schedule work on threads.
              An executing thread calls back a continuation by sending it an event.
              When a continuation receives an event, it wakes up,does some work, and either destroys itself or goes back to sleep & waits for the next event.
