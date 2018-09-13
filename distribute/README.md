1. Overview 

				Overall architecture

layer				client
-------------------------------------------> bind9 DNS poll
LBlayer				LVS-dr/haproxy node 
*******************************************> direct route
ReverseProxylayer	ATS 
-------------------------------------------> nginx L7 LB
Weblayer			NGX web	front
-------------------------------------------> link pools
Servicelayer		Terndo 
-------------------------------------------> data balance,request balance
Datalayer		    mysql/mongodb cluster
						|
						V
					 ceph/hdfs cluster

2.Content Delivery Network (CDN)

			DNS <GSLB>
			LVS <SLB>
			ATS cluster<cache>
			
3.Load Banlance(LB)
 <1>轮循算法，就是将来自网络的请求依次分配给集群中的节点进行处理。
 <2>最小连接数算法，就是为集群中的每台服务器设置一个记数器，记录每个服务器当前的连接数，负载均衡系统总是选择当前连接数最少的服务器分配任务。 这要比"轮循算法"好很多，因为在有些场合中，简单的轮循不能判断哪个节点的负载更低，也许新的工作又被分配给了一个已经很忙的服务器了。
 <3>快速响应优先算法，是根据群集中的节点的状态（CPU、内存等主要处理部分）来分配任务。 这一点很难做到，事实上到目前为止，采用这个算法的负载均衡系统还很少。尤其对于硬件负载均衡设备来说，只能在TCP/IP协议方面做工作，几乎不可能深入到服务器的处理系统中进行监测。但是它是未来发展的方向。
5.Distribute content Cache
常用的分布式存储技术包括分布式文件系统和分布式数据库，
6.Distribute data storate

