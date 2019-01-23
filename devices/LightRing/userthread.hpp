#ifndef AMIRO_USERTHREAD_H_
#define AMIRO_USERTHREAD_H_

#include <ch.hpp>
#include <amiroosconf.h>
#include <vector>
#include <complex>

namespace amiro {

class UserThread : public chibios_rt::BaseStaticThread<USER_THREAD_STACK_SIZE>
{
public:
    explicit UserThread();
    virtual ~UserThread();

    virtual msg_t main();

private:
    int cycleNumber;
    void microphoneInput();
    void sleepForSec(int inSeconds);
    std::vector<std::complex<float> > computeDft(const std::vector<std::complex<float> > &input);
};

} // end of namespace amiro

#endif // AMIRO_USERTHREAD_H_
