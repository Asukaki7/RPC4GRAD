# Reactor
  **事件处理模型**，又叫做反应堆模型  
  核心是一个loop循环，在循环里面不断调用epoll_wait监听套接字IO事件，一旦发生io事件，epoll_wait返回，线程转而去处理这些IO事件（一般是执行绑定在其上边的call back函数），处理完事件之后，又重新陷入到epoo_wait中，不断循环这个过程
```c++ {.line-number}
void loop() {
    // 1. 取得下次定时任务的时间，与设定的timeout去比较最大值 
    int time_out = MAX(1000, getNextTimerCallBack());
    // 2. 调用Epoll等待事件发生，超时时间为上述的time_out 
    int rt = epoll_wiat(epfd, fds, ..., time_out);

    if (rt < 0) {
        // epoll call failed
    } else {
        if (rt > 0) {
            foreach (fd in fds) {
                tasks.push(fd);
            }
        }
    }

    foreach (task in tasks) {
        task();
    }
}

```
