--参考网址:
https://github.com/topics/c
https://github.com/topics/cpp
https://github.com/topics/java
https://github.com/topics/python
https://github.com/topics/javascript

--技术摘要

1.IO
	同步阻塞，
	同步非阻塞(O_NONBLOCK)，
	异步阻塞(select/poll/epoll,O_NONBLOCK)，这也称异步同步化处理。
	异步非阻塞(AIO)。

2.并发/并行,多线程,多进程。
	同步与异步显著差别：是否可以在多个间“任务”交替执行,非串行。

3.TCP/IP 
	L4	udp:
		tcp:
	L3	ip:
	L2	mac:

4.多路复用
	select: 描述符集不能重复使用，须要迭代，限制1024个,不能动态修改集合，不能动态并发操作socket，不能改变事件。时间精度1ns.不能监控close
  	poll:   能复用，              须要迭代, 无限制，   不能动态修改,                                           时间精度1ms,返回close状态。用户态添加fd位图，一次小于1000,大于1000时保证短连接。只能同时有一个线程处理。
	epoll:  能复用, 只返回一个有事件的子集，能够设置数据，动态多线程可改，边缘触发， 更改事件/增删fd,epoll_ctl会发生系统调用进入内核态，
