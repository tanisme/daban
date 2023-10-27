#ifndef BALIBALI_SINGLETON_H
#define BALIBALI_SINGLETON_H

#include <mutex>

template<typename T>
class Singleton
{
public:
    Singleton() {}
    virtual ~Singleton() {}

    static T* Instance() {
        if (m_instance == nullptr) {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (m_instance == nullptr) {
                m_instance = new T();
            }
        }
        return m_instance;
    }

    static T& GetMe() {
        return *Instance();
    }

private:
    Singleton(const Singleton&);
    Singleton& operator=(const Singleton&);

private:
    static std::mutex m_mutex;
    static T* m_instance;
};

template<typename T>
T* Singleton<T>::m_instance = nullptr;
template<typename T>
std::mutex Singleton<T>::m_mutex;

#endif //BALIBALI_SINGLETON_H
