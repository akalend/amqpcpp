#include <deque>
#include <boost/thread.hpp>


template <class T>
class ISynchronizedQueue
{
public:
    virtual bool add(T pkt) = 0;
    virtual bool get(T& pkt) = 0;
};


template <class T>
class SynchronizedQueue: public ISynchronizedQueue<T>
{
    boost::mutex m_mutex;
    std::deque<T> m_queue;
 
public:
    bool add(T pkt)
    {
        boost::lock_guard<boost::mutex> lock(m_mutex);
        m_queue.push_back(pkt);
        return true;
    }

    bool get(T& pkt)
    {
        boost::lock_guard<boost::mutex> lock(m_mutex);
        if (!m_queue.size())
        {
            return false;
        }
        pkt = m_queue.front();
        m_queue.pop_front();
        return true;
    }
};


