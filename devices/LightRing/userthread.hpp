#ifndef AMIRO_USERTHREAD_H_
#define AMIRO_USERTHREAD_H_

#include <ch.hpp>
#include <amiroosconf.h>

namespace amiro {

class UserThread : public chibios_rt::BaseStaticThread<USER_THREAD_STACK_SIZE>
{
public:
    explicit UserThread();
    virtual ~UserThread();

    virtual msg_t main();

private:
    void input();
};

} // end of namespace amiro

#endif // AMIRO_USERTHREAD_H_
