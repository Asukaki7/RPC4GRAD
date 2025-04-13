C++11实现多线程条件下的单例模式使用静态局部变量只初始化一次且线程安全的语法特性就行了，不用加锁
```c++
class Singleton
{
public:
    static Singleton& getInstance()
    {
        static Singleton value;
        return value;
    }
private:
    Singleton() = default;
    Singleton(const Singleton &rhs) = delete;
    Singleton &operator=(const Singleton &rhs) = delete;
};
```

另一种double check的实现
```c++
class Lock  
    {  
    private:         
        CCriticalSection m_cs;  
    public:  
        Lock(CCriticalSection  cs) : m_cs(cs)  
        {  
            m_cs.Lock();  
        }  
        ~Lock()  
        {  
            m_cs.Unlock();  
        }  
    };  
      
    class Singleton  
    {  
    private:  
        Singleton();  
        Singleton(const Singleton &);  
        Singleton& operator = (const Singleton &);  
      
    public:  
        static Singleton *Instantialize();  
        static Singleton *pInstance;  
        static CCriticalSection cs;  
    };  
      
    Singleton* Singleton::pInstance = 0;  
      
    Singleton* Singleton::Instantialize()  
    {  
        if(pInstance == NULL)  
        {   //double check  
            Lock lock(cs);           //用lock实现线程安全，用资源管理类，实现异常安全  
            //使用资源管理类，在抛出异常的时候，资源管理类对象会被析构，析构总是发生的无论是因为异常抛出还是语句块结束。  
            if(pInstance == NULL)  
            {  
                pInstance = new Singleton();  
            }  
        }  
        return pInstance;  
    } 
```